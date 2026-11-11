//------------------------------------------------------------------------------------------------
// Indicatif sous le nom d'une zone, comme sous les bases du Conflit : tiré de la liste d'indicatifs
// de base de la faction joueur, toujours le même pour une zone (calculé à partir de son nom),
// sans doublon entre zones. Calculé sur chaque client, rien à envoyer.
//------------------------------------------------------------------------------------------------

class HAM_Callsigns
{
	protected static ref map<string, string> s_mByZone;
	protected static ref set<string> s_aUsed;

	//------------------------------------------------------------------------------------------------
	//! Clé de texte de l'indicatif (traduite par le widget), vide si la faction n'en a pas.
	static string Of(string zoneName)
	{
		if (zoneName.IsEmpty())
			return string.Empty;

		if (!s_mByZone)
		{
			s_mByZone = new map<string, string>();
			s_aUsed = new set<string>();
		}

		string known;
		if (s_mByZone.Find(zoneName, known))
			return known;

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return string.Empty;

		SCR_Faction faction = SCR_Faction.Cast(factionManager.GetFactionByKey(HAM_Units.PLAYER_FACTION));
		if (!faction)
			return string.Empty;

		int start = Math.AbsInt(zoneName.Hash());
		string callsign;
		for (int probe = 0; probe < 64; probe++)
		{
			SCR_MilitaryBaseCallsign info = faction.GetBaseCallsignByIndex(start + probe);
			if (!info)
				break;

			callsign = info.GetCallsign();
			if (!s_aUsed.Contains(callsign))
				break;
		}

		if (callsign.IsEmpty())
			return string.Empty;

		s_aUsed.Insert(callsign);
		s_mByZone.Set(zoneName, callsign);
		return callsign;
	}

	//------------------------------------------------------------------------------------------------
	static void Release()
	{
		s_mByZone = null;
		s_aUsed = null;
	}
}
