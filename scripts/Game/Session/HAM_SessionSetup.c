//------------------------------------------------------------------------------------------------
// Menu de déploiement des modes hors Conflit. En H&M (Conflit), le menu vanilla du Conflit reste.
//------------------------------------------------------------------------------------------------

modded class SCR_RespawnSystemComponent
{
	override void OnInit(IEntity owner)
	{
		HAM_ApplyDeployMenu(owner);
		super.OnInit(owner);
	}

	protected void HAM_ApplyDeployMenu(IEntity owner)
	{
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(owner);
		if (!mode)
			mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		if (!mode)
			return;

		if (SCR_GameModeCampaign.Cast(mode))
			return;

		if (SCR_GameModeEditor.Cast(mode))
			return;

		SCR_MenuSpawnLogic menu = SCR_MenuSpawnLogic.Cast(m_SpawnLogic);
		if (!menu)
		{
			menu = new SCR_MenuSpawnLogic();
			m_SpawnLogic = menu;
		}
	}
}
