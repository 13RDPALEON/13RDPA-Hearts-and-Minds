//------------------------------------------------------------------------------------------------
// Battement client toutes les 30 s, pour dater un gel quand le journal se tait.
// Pas de boucle par image. Le serveur dédié (application console) ne l'arme pas.
//------------------------------------------------------------------------------------------------

class HAM_ClientWatch
{
	protected static bool s_bStarted;

	static void Start()
	{
		if (s_bStarted || !GetGame() || System.IsConsoleApp())
			return;

		s_bStarted = true;
		GetGame().GetCallqueue().CallLater(Pulse, 30000, true);
	}

	static void Stop()
	{
		if (!s_bStarted || !GetGame())
			return;

		GetGame().GetCallqueue().Remove(Pulse);
		s_bStarted = false;
	}

	static void Pulse()
	{
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[13RDPA H&M] client vivant %1", HAM_Util.PlayerTag(playerId)), LogLevel.NORMAL);
	}
}

//------------------------------------------------------------------------------------------------
// Quitter vers le menu principal. OnExitConfirm est privé : CloseToMainMenu est le point protégé.
//------------------------------------------------------------------------------------------------
modded class PauseMenuUI
{
	override protected void CloseToMainMenu()
	{
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[13RDPA H&M] scenario_exit, joueur %1", HAM_Util.PlayerTag(playerId)), LogLevel.NORMAL);
		super.CloseToMainMenu();
	}
}
