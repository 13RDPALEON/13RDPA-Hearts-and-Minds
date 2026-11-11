//------------------------------------------------------------------------------------------------
// Construction libre en H&M : partout sur l'île, sur tout terrain, sans distance minimale ni maximale.
// Garages, hélipads, QG et toutes les compositions passent par les mêmes contrôles vanilla.
//------------------------------------------------------------------------------------------------

modded class SCR_GameModeCampaign
{
	//! Le Conflit n'autorise l'établissement de bases qu'avec le rôle de commandant, coupé en H&M.
	override bool GetEstablishingBasesEnabled()
	{
		if (HAM_Mission.IsActive())
			return true;

		return super.GetEstablishingBasesEnabled();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingProviderComponent
{
	//! Couvre toute l'île depuis n'importe quel point.
	static const float HAM_FREE_BUILDING_RADIUS = 20000;

	//------------------------------------------------------------------------------------------------
	//! Les fournisseurs non rattachés à une base (camions de construction) gardent leur rayon :
	//! le Conflit balaie toutes les entités de ce rayon à chaque ouverture.
	override float GetBuildingRadius()
	{
		if (HAM_Mission.IsActive() && GetMilitaryBaseComponent())
			return HAM_FREE_BUILDING_RADIUS;

		return super.GetBuildingRadius();
	}

	//------------------------------------------------------------------------------------------------
	//! Avec un rayon couvrant l'île, n'importe quel ennemi bloquerait la construction.
	override bool ObstrucViewWhenEnemyInRange()
	{
		if (HAM_Mission.IsActive())
			return false;

		return super.ObstrucViewWhenEnemyInRange();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingPlacingObstructionEditorComponent
{
	//! Ni obstacle, ni pente, ni eau : la composition se pose où le joueur la met.
	override bool CanCreate(out ENotification outNotification = -1, out SCR_EPreviewState previewStateToShow = SCR_EPreviewState.PLACEABLE)
	{
		if (!HAM_Mission.IsActive() || m_bSuperiorCanBeCreated)
			return super.CanCreate(outNotification, previewStateToShow);

		m_bCanBeCreated = true;
		previewStateToShow = SCR_EPreviewState.PLACEABLE;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingPlacingHQBaseEditorComponent
{
	//! Un QG se pose sans tâche d'établissement et sans distance minimale avec les autres bases.
	override bool IsNearAnyHQ(out ENotification outNotification = -1)
	{
		if (HAM_Mission.IsActive())
			return false;

		return super.IsNearAnyHQ(outNotification);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingBudgetEditorComponent
{
	//! Sans commandant, aucune tâche d'établissement n'existe : seules la faction et la limite de bases comptent.
	override protected bool CanEstablishBase()
	{
		if (!HAM_Mission.IsActive())
			return super.CanEstablishBase();

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign || !campaign.GetEstablishingBasesEnabled() || !campaign.GetBaseManager())
			return false;

		SCR_ChimeraCharacter player = SCR_ChimeraCharacter.Cast(SCR_PlayerController.GetLocalControlledEntity());
		if (!player || !player.GetFaction())
			return false;

		return campaign.GetBaseManager().CanFactionBuildNewBase(player.GetFaction());
	}
}
