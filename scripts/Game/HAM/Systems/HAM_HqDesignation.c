//------------------------------------------------------------------------------------------------
// « Définir comme QG » : une base Conflit du camp joueur est posée sur le maître désigné et devient
// la base principale de la faction (spawn, fournitures, construction du Conflit).
// L'ancien QG garde sa base et son stock ; seul son point d'apparition est coupé.
// Rien n'est transféré tout seul. La ligne de front ne bouge pas.
//------------------------------------------------------------------------------------------------

class HAM_HqDesignation
{
	static const ResourceName HQ_BASE_PREFAB = "{7E42BC76C7DD77DA}Prefabs/HAM/Bases/HAM_HQBase.et";
	static const float BASE_SEARCH_RADIUS = 150;

	//------------------------------------------------------------------------------------------------
	static SCR_CampaignMilitaryBaseComponent CurrentBase()
	{
		return ConflictBaseOf(HAM_Mission.s_sDesignatedHq);
	}

	//------------------------------------------------------------------------------------------------
	//! Base Conflit d'une base alliée : le QG du monde, ou la base posée sur un maître désigné.
	static SCR_CampaignMilitaryBaseComponent ConflictBaseOf(string baseName)
	{
		if (baseName == HAM_Mission.HQ_NAME)
			return HAM_Mission.s_HqBase;

		HAM_Cell cell = HAM_Mission.FindCell(baseName);
		if (!cell)
			return null;

		SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), BASE_SEARCH_RADIUS);
		if (!base || base == HAM_Mission.s_EnemyHqBase)
			return null;

		Faction faction = base.GetFaction();
		if (!faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
			return null;

		return base;
	}

	//------------------------------------------------------------------------------------------------
	static bool CanDesignate(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || playerId <= 0)
			return false;

		if (players.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER))
			return true;

		if (players.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR) || players.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR))
			return true;

		return HAM_GroupAccessHelper.IsPlayerWhitelisted(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static bool Designate(string baseName, int playerId)
	{
		if (!HAM_Mission.IsRunning())
			return false;

		if (!CanDesignate(playerId))
		{
			HAM_Net.NotifyPlayer(playerId, "QG", "Action réservée au Game Master et aux identités du bloc groupAccess.", 6);
			return false;
		}

		if (!HAM_Mission.IsAlliedBase(baseName))
		{
			HAM_Net.NotifyPlayer(playerId, "QG", "Le tableau doit être dans une base tenue pour la désigner comme QG.", 6);
			return false;
		}

		if (baseName == HAM_Mission.s_sDesignatedHq)
		{
			HAM_Net.NotifyPlayer(playerId, "QG", baseName + " est déjà le QG.", 6);
			return false;
		}

		SCR_CampaignMilitaryBaseComponent newBase = EnsureBase(baseName);
		if (!newBase)
		{
			HAM_Net.NotifyPlayer(playerId, "QG", "La base Conflit n'a pas pu être posée ici.", 6);
			return false;
		}

		SCR_CampaignMilitaryBaseComponent oldBase = CurrentBase();
		Activate(newBase, oldBase);
		HAM_Mission.s_sDesignatedHq = baseName;
		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();
		HAM_Mission.Notify("Nouveau QG", string.Format("%1 est désormais le QG : apparition, construction et versements s'y font.", baseName), 12);
		Print(string.Format("[13RDPA H&M] QG désigné : %1 (joueur %2)", baseName, playerId), LogLevel.NORMAL);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_CampaignMilitaryBaseComponent EnsureBase(string baseName)
	{
		if (baseName == HAM_Mission.HQ_NAME)
			return HAM_Mission.s_HqBase;

		SCR_CampaignMilitaryBaseComponent existing = ConflictBaseOf(baseName);
		if (existing)
			return existing;

		HAM_Cell cell = HAM_Mission.FindCell(baseName);
		if (!cell)
			return null;

		Resource resource = Resource.Load(HQ_BASE_PREFAB);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = HAM_Util.FreeSpot(cell.Pos() + Vector(30, 0, 0), 40);
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!entity)
			return null;

		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(entity.FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (!base)
		{
			HAM_Util.Delete(entity);
			return null;
		}

		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(HAM_Util.FactionByKey(HAM_Units.PLAYER_FACTION));
		if (faction)
			base.SetFaction(faction);

		base.Initialize();
		HAM_Mission.DisableBasePatrols(base);
		return base;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Activate(SCR_CampaignMilitaryBaseComponent newBase, SCR_CampaignMilitaryBaseComponent oldBase)
	{
		if (!newBase)
			return;

		newBase.SetAsHQ(true);
		SetSpawnEnabled(newBase, true);
		if (oldBase && oldBase != newBase)
		{
			oldBase.SetAsHQ(false);
			SetSpawnEnabled(oldBase, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetSpawnEnabled(SCR_CampaignMilitaryBaseComponent base, bool enabled)
	{
		if (!base)
			return;

		SCR_SpawnPoint spawnPoint = base.GetSpawnPoint();
		if (spawnPoint)
			spawnPoint.SetSpawnPointEnabled_S(enabled);
	}

	//------------------------------------------------------------------------------------------------
	//! Au boot : réapplique le QG sauvegardé.
	static void Restore()
	{
		string designated = HAM_Mission.s_sDesignatedHq;
		if (designated.IsEmpty() || designated == HAM_Mission.HQ_NAME || !HAM_Mission.IsAlliedBase(designated))
		{
			HAM_Mission.s_sDesignatedHq = HAM_Mission.HQ_NAME;
			SetSpawnEnabled(HAM_Mission.s_HqBase, true);
			return;
		}

		SCR_CampaignMilitaryBaseComponent base = EnsureBase(designated);
		if (!base)
		{
			HAM_Mission.s_sDesignatedHq = HAM_Mission.HQ_NAME;
			return;
		}

		GetGame().GetCallqueue().CallLater(DelayedActivate, 5000, false, designated);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DelayedActivate(string designated)
	{
		if (HAM_Mission.s_sDesignatedHq != designated)
			return;

		Activate(ConflictBaseOf(designated), HAM_Mission.s_HqBase);
	}

	//------------------------------------------------------------------------------------------------
	//! Le maître désigné tombe : le QG repart au QG de départ, la base posée sur la zone disparaît avec elle.
	static void OnCellLost(HAM_Cell cell)
	{
		SCR_CampaignMilitaryBaseComponent base = ConflictBaseOf(cell.m_sName);
		if (HAM_Mission.s_sDesignatedHq == cell.m_sName)
		{
			Activate(HAM_Mission.s_HqBase, base);
			HAM_Mission.s_sDesignatedHq = HAM_Mission.HQ_NAME;
			HAM_Mission.Notify("QG", string.Format("%1 est perdu : le QG revient au QG de départ.", cell.m_sName), 10);
		}

		if (base && base.GetOwner())
			HAM_Util.Delete(base.GetOwner());
	}
}
