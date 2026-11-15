//------------------------------------------------------------------------------------------------
// Menu de déploiement des modes hors Conflit. En H&M (Conflit), le menu vanilla du Conflit reste.
//------------------------------------------------------------------------------------------------

modded class SCR_RespawnSystemComponent
{
	override void OnInit(IEntity owner)
	{
		// Le Workbench charge parfois le mode de jeu avant les attributs script :
		// m_SpawnLogic reste vide, OnInit vanilla déréférence null et le lobby ne s'ouvre pas.
		if (!m_SpawnLogic)
			m_SpawnLogic = new SCR_MenuSpawnLogic();

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
