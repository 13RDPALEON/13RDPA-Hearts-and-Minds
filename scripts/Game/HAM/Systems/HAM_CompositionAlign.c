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
