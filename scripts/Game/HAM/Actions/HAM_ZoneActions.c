//------------------------------------------------------------------------------------------------
// Actions des prefabs H&M. Celles qui changent l'état s'exécutent sur le serveur seul ;
// celles qui affichent une information restent locales.
//------------------------------------------------------------------------------------------------

class HAM_ActionUtil
{
	//------------------------------------------------------------------------------------------------
	static HAM_ZoneComponent Zone(IEntity owner)
	{
		if (!owner)
			return null;

		return HAM_ZoneComponent.Cast(owner.FindComponent(HAM_ZoneComponent));
	}

	//------------------------------------------------------------------------------------------------
	static int PlayerId(IEntity user)
	{
		return HAM_Util.PlayerIdOf(user);
	}

	//------------------------------------------------------------------------------------------------
	//! Nom de base alliée du prop : « QG » pour la logistique du QG, sinon le nom de la cellule.
	static string BaseName(HAM_ZoneComponent zone)
	{
		if (!zone)
			return string.Empty;

		if (zone.GetRole() == HAM_ERole.HQ)
			return HAM_Mission.HQ_NAME;

		return zone.GetZoneName();
	}

	//------------------------------------------------------------------------------------------------
	static string BaseLabel(UIInfo info)
	{
		if (!info)
			return string.Empty;

		return info.GetName();
	}

	//------------------------------------------------------------------------------------------------
	//! Le personnage auquel l'action est accrochée.
	static IEntity CivilianBody(IEntity owner)
	{
		if (!owner)
			return null;

		IEntity parent = owner.GetParent();
		if (parent && ChimeraCharacter.Cast(parent))
			return parent;

		if (ChimeraCharacter.Cast(owner))
			return owner;

		return parent;
	}
}

//------------------------------------------------------------------------------------------------
class HAM_UnloadCargoUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(GetOwner());
		return user && zone && zone.IsPlayer();
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(pOwnerEntity);
		if (zone)
			HAM_Logistics.Unload(HAM_ActionUtil.BaseName(zone), HAM_ActionUtil.PlayerId(pUserEntity), pOwnerEntity.GetOrigin());
	}
}

//------------------------------------------------------------------------------------------------
class HAM_BoardUnloadUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		// La liste des bases tenues n'existe que sur le serveur : un test ici cachait
		// l'action à tous les clients. Le serveur refuse quand même hors base alliée.
		return user != null;
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		string baseName = HAM_BoardServer.BaseOfBoard(pOwnerEntity);
		HAM_Logistics.Unload(baseName, HAM_ActionUtil.PlayerId(pUserEntity), pOwnerEntity.GetOrigin());
	}
}

//------------------------------------------------------------------------------------------------
class HAM_LoadCargoUserAction : ScriptedUserAction
{
	[Attribute("3", desc: "0 eau, 1 nourriture, 2 munitions, 3 les trois")]
	protected int m_iKind;

	override bool CanBeShownScript(IEntity user)
	{
		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(GetOwner());
		return user && zone && zone.GetRole() == HAM_ERole.DEPOSIT;
	}

	override bool GetActionNameScript(out string outName)
	{
		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(GetOwner());
		if (!zone)
			return false;

		int amount = zone.GetWater() + zone.GetFood() + zone.GetAmmo();
		if (m_iKind == 0)
			amount = zone.GetWater();
		else if (m_iKind == 1)
			amount = zone.GetFood();
		else if (m_iKind == 2)
			amount = zone.GetAmmo();

		outName = string.Format("%1 (%2)", HAM_ActionUtil.BaseLabel(GetUIInfo()), amount);
		return true;
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(pOwnerEntity);
		if (!zone)
			return;

		HAM_EResource kind = HAM_EResource.MEN;
		if (m_iKind == 0)
			kind = HAM_EResource.WATER;
		else if (m_iKind == 1)
			kind = HAM_EResource.FOOD;
		else if (m_iKind == 2)
			kind = HAM_EResource.AMMO;

		HAM_Logistics.LoadFromDeposit(zone.GetZoneName(), HAM_ActionUtil.PlayerId(pUserEntity), kind);
	}
}

//------------------------------------------------------------------------------------------------
//! Affiche localement les stocks et les seuils de la zone.
class HAM_ZoneStatusUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user && HAM_ActionUtil.Zone(GetOwner());
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ZoneComponent zone = HAM_ActionUtil.Zone(pOwnerEntity);
		if (!zone)
			return;

		string title = zone.GetZoneName();
		string text;
		if (zone.GetRole() == HAM_ERole.DEPOSIT)
		{
			text = string.Format("Eau %1/%2 · Nourriture %3/%4 · Munitions %5/%6.\nUne ressource vidée se remplit doucement.", zone.GetWater(), Math.Round(HAM_ClientState.s_fDepositWaterMax), zone.GetFood(), Math.Round(HAM_ClientState.s_fDepositFoodMax), zone.GetAmmo(), Math.Round(HAM_ClientState.s_fDepositAmmoMax));
		}
		else if (zone.GetRole() == HAM_ERole.SLAVE)
		{
			text = "Point de combat. Il se prend, avant ou après le maître, une fois la cellule découverte et la base maître reliée par radio.";
		}
		else
		{
			string camp = "ennemi";
			if (zone.IsPlayer())
				camp = "tenu";

			text = string.Format("Zone %1.\nEau %2/%3 · Nourriture %4/%5 · Munitions %6/%7 · Hommes %8/%9", camp, zone.GetWater(), HAM_Util.FormatInt(HAM_ClientState.s_fWaterThreshold), zone.GetFood(), HAM_Util.FormatInt(HAM_ClientState.s_fFoodThreshold), zone.GetAmmo(), HAM_Util.FormatInt(HAM_ClientState.s_fAmmoThreshold), zone.GetMen(), HAM_ClientState.s_iMenThreshold);
			text = text + string.Format(" · max %1", HAM_ClientState.s_iMenMax);
		}

		SCR_HintManagerComponent.ShowCustomHint(text, title, 10);
	}
}

//------------------------------------------------------------------------------------------------
class HAM_IntelPickupUserAction : ScriptedUserAction
{
	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (Replication.IsServer())
			HAM_Intel.OnIntelPicked(pOwnerEntity, HAM_ActionUtil.PlayerId(pUserEntity));
	}
}

//------------------------------------------------------------------------------------------------
class HAM_HostageFreeUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = HAM_ActionUtil.CivilianBody(GetOwner());
		return user && body && !HAM_Util.IsDead(body);
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (Replication.IsServer())
			HAM_SideMissions.FreeHostage(pOwnerEntity, HAM_ActionUtil.PlayerId(pUserEntity));
	}
}

//------------------------------------------------------------------------------------------------
class HAM_SearchBodyUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = GetOwner().GetParent();
		return user && (!body || HAM_Util.IsDead(body));
	}

	override bool CanBroadcastScript()
	{
		return true;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (Replication.IsServer())
			HAM_Intel.SearchBody(pOwnerEntity, HAM_ActionUtil.PlayerId(pUserEntity));
	}
}

//------------------------------------------------------------------------------------------------
//! Une fois par civil. Une chance sur cinq de révéler un maître de la ligne de front.
class HAM_CivilianInfoUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = HAM_ActionUtil.CivilianBody(GetOwner());
		return user && body && !HAM_Util.IsDead(body) && !HAM_Intel.HadInfo(body);
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity civilian = HAM_ActionUtil.CivilianBody(pOwnerEntity);
		RplId id = HAM_Util.IdOf(civilian);
		if (Replication.IsServer())
			HAM_Intel.AskForInfo(civilian, HAM_ActionUtil.PlayerId(pUserEntity));
		else
		{
			SCR_PlayerController local = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (local)
				local.HAM_RequestCivilianInfo(id);
		}
	}
}

//------------------------------------------------------------------------------------------------
class HAM_CivilianInterrogateUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = HAM_ActionUtil.CivilianBody(GetOwner());
		return user && body && !HAM_Util.IsDead(body);
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		IEntity civilian = HAM_ActionUtil.CivilianBody(pOwnerEntity);

		RplId id = HAM_Util.IdOf(civilian);
		SCR_PlayerController local = SCR_PlayerController.Cast(GetGame().GetPlayerController());

		if (Replication.IsServer())
			HAM_Intel.BeginInterrogate(civilian, HAM_ActionUtil.PlayerId(pUserEntity));
		else if (local)
			local.HAM_RequestCivilianTalk(id);

		HAM_Util.Info("Civil : interrogation demandée");
	}
}

//------------------------------------------------------------------------------------------------
class HAM_CivilianReportUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = HAM_ActionUtil.CivilianBody(GetOwner());
		return user && body && !HAM_Util.IsDead(body);
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!Replication.IsServer())
			return;

		HAM_Intel.FinishMineHelp(HAM_ActionUtil.CivilianBody(pOwnerEntity), HAM_ActionUtil.PlayerId(pUserEntity));
		HAM_Util.Delete(pOwnerEntity);
	}
}

//------------------------------------------------------------------------------------------------
class HAM_CivilianBananaUserAction : ScriptedUserAction
{
	override bool CanBeShownScript(IEntity user)
	{
		IEntity body = HAM_ActionUtil.CivilianBody(GetOwner());
		return user && body && !HAM_Util.IsDead(body) && !HAM_Intel.HadBanana(body);
	}

	override bool CanBroadcastScript()
	{
		return false;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (Replication.IsServer())
			HAM_Intel.GiveBanana(pUserEntity, HAM_ActionUtil.CivilianBody(pOwnerEntity), HAM_ActionUtil.PlayerId(pUserEntity));
	}
}
