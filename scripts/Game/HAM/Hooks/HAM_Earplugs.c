//------------------------------------------------------------------------------------------------
// WCS_Earplugs baisse le volume SFX et retient bMuted. Le menu de déploiement vanilla
// remet le volume à 1 en se fermant. On remet le volume des bouchons après ça.
// bMuted et EarplugsVolume sont des champs sans mot-clé d'accès de WCS_EarplugsSystem.
//------------------------------------------------------------------------------------------------

class HAM_EarplugsGuard
{
	protected static int s_iLastLog;

	static void Restore(string reason)
	{
		WCS_EarplugsSystem plugs = WCS_EarplugsSystem.Instance;
		if (!plugs || !plugs.bMuted)
			return;

		float wanted = plugs.EarplugsVolume;
		float now = AudioSystem.GetMasterVolume(AudioSystem.SFX);
		if (Math.AbsFloat(now - wanted) < 0.01)
			return;

		AudioSystem.SetMasterVolume(AudioSystem.SFX, wanted);

		int tick = System.GetTickCount();
		if (s_iLastLog != 0)
		{
			int since = tick - s_iLastLog;
			if (since >= 0 && since < 5000)
				return;
		}

		s_iLastLog = tick;
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[13RDPA H&M] Bouchons réappliqués, joueur %1, SFX %2 -> %3, raison: %4", HAM_Util.PlayerTag(playerId), now, wanted, reason), LogLevel.WARNING);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_DeployMenuBase
{
	override void OnMenuOpen()
	{
		super.OnMenuOpen();
		int playerId = SCR_PlayerController.GetLocalPlayerId();
		Print(string.Format("[13RDPA H&M] Menu déploiement ouvert (%1), joueur %2", Type().ToString(), HAM_Util.PlayerTag(playerId)), LogLevel.NORMAL);
	}

	override void MuteSounds(bool mute = true)
	{
		super.MuteSounds(mute);
		if (mute)
			return;

		HAM_EarplugsGuard.Restore("menu déploiement");
	}
}
