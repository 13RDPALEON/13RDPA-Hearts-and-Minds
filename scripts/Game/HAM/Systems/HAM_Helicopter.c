//------------------------------------------------------------------------------------------------
// Une patrouille AH-1S par zone ennemie du front, quel que soit son type.
// Le waypoint de patrouille du jeu est créé en l'air. Personne ne le pose dans le monde.
//------------------------------------------------------------------------------------------------

class HAM_Helicopter
{
	protected static ref map<string, Vehicle> s_mSpawned;

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
			s_mSpawned = new map<string, Vehicle>();

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (!Wants(point))
				continue;

			string name = point.m_Def.m_sName;
			float nearest = HAM_Util.NearestDistance(bodies, point.Pos());
			float leave = settings.DespawnOf(point.m_Def);
			float activate = settings.ActivationOf(point.m_Def);
			if (leave < activate)
				leave = activate;

			if (s_mSpawned.Contains(name))
			{
				Vehicle live = s_mSpawned.Get(name);
				if (live && nearest <= leave)
					continue;

				Retire(live);
				s_mSpawned.Remove(name);
			}

			if (nearest > activate)
				continue;

			Vehicle helicopter = Launch(point, settings);
			if (helicopter)
				s_mSpawned.Set(name, helicopter);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Wants(HAM_Point point)
	{
		if (!point || !point.m_Def || point.m_bPlayer)
			return false;

		if (!point.m_Cell || point.m_Cell.IsComplete() || !HAM_Mission.IsFront(point.m_Cell))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Retire(Vehicle helicopter)
	{
		if (!helicopter)
			return;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(helicopter.FindComponent(SCR_BaseCompartmentManagerComponent));
		SCR_AIGroup crew;
		if (compartments)
			crew = HAM_Garrison.CrewGroup(compartments);

		HAM_Util.Delete(helicopter);
		if (crew)
			HAM_Util.DeleteGroup(crew);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyAltitude(Vehicle helicopter, HAM_Settings settings)
	{
		AERO_HeliAutopilotComponent autopilot = AERO_HeliAutopilotComponent.Cast(helicopter.FindComponent(AERO_HeliAutopilotComponent));
		if (!autopilot)
			return;

		float low = settings.m_fHeliAltitudeMin;
		float high = settings.m_fHeliAltitudeMax;
		if (high < low)
			high = low;

		autopilot.SetAlturaCrucero(Math.RandomFloat(low, high));
	}

	//------------------------------------------------------------------------------------------------
	protected static Vehicle Launch(HAM_Point point, HAM_Settings settings)
	{
		vector pad;
		if (!HAM_Util.FindHeliPad(point.Pos(), pad))
			return null;

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
			return null;

		ApplyAltitude(helicopter, settings);
		HAM_Util.Info(string.Format("Patrouille hélicoptère : %1", point.m_Def.m_sName));
		return helicopter;
	}
}
