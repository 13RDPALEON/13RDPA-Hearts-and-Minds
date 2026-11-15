//------------------------------------------------------------------------------------------------
// Tant que Begin est actif, les pièces d'une composition ne se collent pas à la pente.
//------------------------------------------------------------------------------------------------

class HAM_CompositionAlign
{
	protected static int s_iHold;

	static void Begin()
	{
		s_iHold++;
	}

	static void End()
	{
		if (s_iHold > 0)
			s_iHold--;
	}

	static bool Hold()
	{
		return s_iHold > 0;
	}
}

modded class SCR_SlotCompositionComponent
{
	override void EOnInit(IEntity owner)
	{
		if (HAM_CompositionAlign.Hold())
			return;

		super.EOnInit(owner);
	}
}

//------------------------------------------------------------------------------------------------
// Un groupe déjà retiré n'a plus de SCR_AIGroup : la liste Zeus ne doit pas planter dessus.
modded class SCR_EditableGroupComponent
{
	override Faction GetFaction()
	{
		if (m_Leader)
		{
			Faction leaderFaction = m_Leader.GetFaction();
			if (leaderFaction)
				return leaderFaction;
		}

		if (!m_Group)
			return null;

		return m_Group.GetFaction();
	}
}

//------------------------------------------------------------------------------------------------
// Le poste de commandement contient une logique de base. Si on la retire tout de suite,
// le gestionnaire de bases rappelle quand même la création : sans propriétaire, ça plante.
modded class SCR_CampaignMilitaryBaseManager
{
	override void CreateCampaignMilitaryBase(notnull SCR_MilitaryBaseComponent base)
	{
		if (!base.GetOwner())
			return;

		super.CreateCampaignMilitaryBase(base);
	}

	override void OnBaseInitialized(notnull SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base.GetOwner() || !base.GetFaction())
			return;

		super.OnBaseInitialized(base);
	}

	//------------------------------------------------------------------------------------------------
	//! Les chiffres du HUD sont les maîtres tenus, pas les points de contrôle vanilla.
	override void EvaluateControlPoints()
	{
		if (!HAM_Mission.IsRunning() || !HAM_Mission.s_aCells)
		{
			super.EvaluateControlPoints();
			return;
		}

		int friendly = 0;
		int hostile = 0;
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || !cell.m_Master)
				continue;

			if (cell.IsMasterPlayer())
				friendly++;
			else
				hostile++;
		}

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign)
			return;

		FactionManager factions = GetGame().GetFactionManager();
		if (!factions)
			return;

		SCR_CampaignFaction player = SCR_CampaignFaction.Cast(factions.GetFactionByKey(HAM_Units.PLAYER_FACTION));
		SCR_CampaignFaction enemy = SCR_CampaignFaction.Cast(factions.GetFactionByKey(HAM_Units.ENEMY_FACTION));
		if (player)
			campaign.SetControlPointsHeld(player, friendly);

		if (enemy)
			campaign.SetControlPointsHeld(enemy, hostile);
	}

	//------------------------------------------------------------------------------------------------
	//! « Hors de portée du signal » à côté du micro. Le client dédié n'a souvent
	//! ni la faction de la base, ni la liste du gestionnaire. On lit les bases du monde.
	override bool IsEntityInFactionRadioSignal(notnull IEntity entity, notnull Faction faction)
	{
		if (!HAM_Mission.IsActive() || !faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
			return super.IsEntityInFactionRadioSignal(entity, faction);

		if (HAM_HearsFriendlyRadio(entity.GetOrigin(), faction.GetFactionKey()))
			return true;

		return super.IsEntityInFactionRadioSignal(entity, faction);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HAM_HearsFriendlyRadio(vector pos, string factionKey)
	{
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (system)
		{
			array<SCR_MilitaryBaseComponent> bases = {};
			system.GetBases(bases);
			foreach (SCR_MilitaryBaseComponent listed : bases)
			{
				SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(listed);
				if (!base || !base.GetOwner())
					continue;

				Faction owner = base.GetFaction();
				string ownerKey = "";
				if (owner)
					ownerKey = owner.GetFactionKey();

				// Sur le client, la faction de la base arrive souvent vide. L'ennemi, lui, est déjà US.
				if (ownerKey == HAM_Units.ENEMY_FACTION)
					continue;

				float range = base.GetRadioRange();
				if (range < 1000)
					range = 1000;

				if (vector.DistanceXZ(pos, base.GetOwner().GetOrigin()) <= range)
					return true;
			}
		}

		array<SCR_SpawnPoint> points = SCR_SpawnPoint.GetSpawnPoints();
		if (!points)
			return false;

		foreach (SCR_SpawnPoint point : points)
		{
			if (!point)
				continue;

			string pointKey = point.GetFactionKey();
			if (pointKey == HAM_Units.ENEMY_FACTION)
				continue;

			if (vector.DistanceXZ(pos, point.GetOrigin()) <= 500)
				return true;
		}

		return false;
	}
}

modded class SCR_CampaignMilitaryBaseComponent
{
	override void EvaluateDefenders()
	{
		if (!GetCampaignFaction())
			return;

		super.EvaluateDefenders();
	}
}
