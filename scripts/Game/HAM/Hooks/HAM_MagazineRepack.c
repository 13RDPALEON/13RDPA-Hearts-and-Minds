//------------------------------------------------------------------------------------------------
// Chargeur de mod : temps par balle à 0. Le transfert part alors à chaque image
// et sature le canal fiable, jusqu'à la déconnexion de ce joueur.
//------------------------------------------------------------------------------------------------
modded class SCR_MagazineRepackingSystem
{
	protected static const float HAM_MIN_ROUND_TIME = 0.05;
	protected static ref array<SCR_ChimeraCharacter> s_aHAM_FloorLogged;

	//------------------------------------------------------------------------------------------------
	override protected void OnUpdatePoint(WorldUpdatePointArgs args)
	{
		if (m_mRepackingData)
		{
			foreach (SCR_ChimeraCharacter character, SCR_MagRepackingData data : m_mRepackingData)
			{
				if (!character || !data)
					continue;

				if (data.GetTimePerRound() >= HAM_MIN_ROUND_TIME)
					continue;

				HAM_LogFloor(character, data.GetTimePerRound());
				data.SetTimePerRound(HAM_MIN_ROUND_TIME);
			}
		}

		super.OnUpdatePoint(args);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_LogFloor(SCR_ChimeraCharacter character, float before)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		int playerId = 0;
		if (players)
			playerId = players.GetPlayerIdFromControlledEntity(character);

		if (playerId <= 0)
			return;

		if (!s_aHAM_FloorLogged)
			s_aHAM_FloorLogged = {};

		if (s_aHAM_FloorLogged.Find(character) >= 0)
			return;

		s_aHAM_FloorLogged.Insert(character);
		Print(string.Format("[13RDPA H&M] Repack : temps par balle relevé, joueur %1, %2 -> %3 s", HAM_Util.PlayerTag(playerId), before, HAM_MIN_ROUND_TIME), LogLevel.WARNING);
	}
}
