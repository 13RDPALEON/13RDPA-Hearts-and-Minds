//------------------------------------------------------------------------------------------------
// Toggles téléport (JSON Conflict → sync clients).
// playerToPlayer : drapeau → liste joueurs
// betweenBases   : drapeau → liste bases capturées amies
//------------------------------------------------------------------------------------------------

class HAM_TeleportSettings
{
	protected static bool s_bPlayerToPlayer = true;
	protected static bool s_bBetweenBases = true;
	protected static bool s_bClientSynced;

	static bool PlayerToPlayerEnabled()
	{
		return s_bPlayerToPlayer;
	}

	static bool BetweenBasesEnabled()
	{
		return s_bBetweenBases;
	}

	static void Apply(bool playerToPlayer, bool betweenBases)
	{
		s_bPlayerToPlayer = playerToPlayer;
		s_bBetweenBases = betweenBases;
		s_bClientSynced = true;

		if (!Replication.IsServer())
			return;

		Print(string.Format("[13RDPA H&M Teleport] réglages — joueurs=%1 bases=%2", playerToPlayer, betweenBases), LogLevel.NORMAL);
		Broadcast();
	}

	static void ApplyClient(bool playerToPlayer, bool betweenBases)
	{
		s_bPlayerToPlayer = playerToPlayer;
		s_bBetweenBases = betweenBases;
		s_bClientSynced = true;
	}

	static void Broadcast()
	{
		if (!Replication.IsServer())
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			PlayerController controller = players.GetPlayerController(playerId);
			SCR_PlayerController player = SCR_PlayerController.Cast(controller);
			if (player)
				player.HAM_SyncTeleportSettings(s_bPlayerToPlayer, s_bBetweenBases);
		}
	}

	static void SyncToPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId < 1)
			return;

		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (player)
			player.HAM_SyncTeleportSettings(s_bPlayerToPlayer, s_bBetweenBases);
	}

	// Depuis le GM (client) : force le passage serveur + broadcast.
	static void RequestApply(bool playerToPlayer, bool betweenBases)
	{
		if (Replication.IsServer())
		{
			Apply(playerToPlayer, betweenBases);
			return;
		}

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_RequestSetTeleportSettings(playerToPlayer, betweenBases);
	}
}
