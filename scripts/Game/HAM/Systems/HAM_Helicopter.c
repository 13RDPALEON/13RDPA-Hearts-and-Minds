//------------------------------------------------------------------------------------------------
// Une patrouille UH-1H par zone camp, ville, base ou dépôt personnel.
// Le waypoint de patrouille du jeu est créé en l'air. Personne ne le pose dans le monde.
//------------------------------------------------------------------------------------------------

class HAM_Helicopter
{
	protected static ref map<string, bool> s_mSpawned;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_mSpawned = null;
	}

	//------------------------------------------------------------------------------------------------
	static void Tick(array<IEntity> bodies)
	{
		HAM_Settings settings = HAM_Settings.Get();
		if (!settings.m_bHelicoptersEnabled || !HAM_Mission.s_aPoints)
			return;

		if (!s_mSpawned)
			s_mSpawned = new map<string, bool>();

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (!Wants(point))
				continue;

			if (s_mSpawned.Contains(point.m_Def.m_sName))
				continue;

			if (HAM_Util.NearestDistance(bodies, point.Pos()) > settings.m_fActivationDistance)
				continue;

			if (Launch(point, settings))
				s_mSpawned.Set(point.m_Def.m_sName, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Wants(HAM_Point point)
	{
		if (!point || !point.m_Def || point.m_bPlayer)
			return false;

		if (!point.m_Cell || point.m_Cell.IsComplete() || !HAM_Mission.IsFront(point.m_Cell))
			return false;

		HAM_EMasterType type = point.m_Def.m_eType;
		if (type == HAM_EMasterType.CAMP)
			return true;

		if (type == HAM_EMasterType.CITY)
			return true;

		if (type == HAM_EMasterType.BASE)
			return true;

		if (type == HAM_EMasterType.SUPPLY_PERSONAL)
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Launch(HAM_Point point, HAM_Settings settings)
	{
		vector pad;
		if (!HAM_Util.FindHeliPad(point.Pos(), pad))
			return false;

		vector origin = point.Pos();
		array<vector> route = {};
		float radius = settings.m_fHeliRadius;
		if (radius < 50)
			radius = 50;

		// Un seul Defend : le pilote automatique tourne dans ce rayon.
		// Plusieurs Move sont pris d'un coup et il s'arrête au dernier.
		route.Insert(origin);
		Vehicle helicopter = HAM_Garrison.SpawnAirPatrol(pad, route, radius);
		if (!helicopter)
			return false;

		HAM_Util.Info(string.Format("Patrouille hélicoptère : %1", point.m_Def.m_sName));
		return true;
	}
}
