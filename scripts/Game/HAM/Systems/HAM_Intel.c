//------------------------------------------------------------------------------------------------
// Renseignement et réputation : intel dans les bâtiments des cellules du front, fouille des corps
// ennemis, civils des villes révélées qui parlent si la réputation globale le permet.
// Un intel ne révèle qu'une cellule cachée de la ligne de front.
//------------------------------------------------------------------------------------------------

class HAM_MineFavor
{
	IEntity m_Civilian;
	ref array<IEntity> m_aMines = {};
	ref array<SCR_Task> m_aTasks = {};
	SCR_Task m_ReportTask;
	bool m_bReady;
}

class HAM_IntelCellState
{
	string m_sCell;
	bool m_bIntelPlaced;
	ref array<IEntity> m_aIntel = {};
	bool m_bCiviliansActive;
	ref array<string> m_aCivilianPoints = {};
	ref array<IEntity> m_aCivilians = {};
	ref array<IEntity> m_aTalkActions = {};
}

class HAM_RoadCar
{
	Vehicle m_Vehicle;
	bool m_bRolled;
	IEntity m_Talker;
	SCR_Task m_Task;
	bool m_bTaskDone;
	bool m_bOnFoot;
	float m_fSince;
}

class HAM_CivilianWave
{
	HAM_Point m_Point;
	HAM_IntelCellState m_State;
	SCR_AIGroup m_Group;
	int m_iLeft;
	int m_iIndex;
	int m_iSpawned;
	int m_iTries;
	string m_sKey;
	vector m_vCenter;
	float m_fRadius;
	float m_fSpin;
	bool m_bReleased;
	ref array<SCR_AIGroup> m_aGroups = {};
	ref array<IEntity> m_aBodies = {};
	float m_fOrderAt;
	vector m_vAnchor;
	bool m_bAnchor;
}

class HAM_CivilianNeed
{
	IEntity m_Civilian;
	HAM_EResource m_eKind;
	float m_fAmount;
	SCR_Task m_Task;
}

class HAM_Intel
{
	static const int CIVILIANS_PER_GROUP = 1;
	static const float BODY_ACTION_LIFETIME = 1200;
	static const int BODY_ACTION_DELAY_MS = 3000;
	static const float BODY_ACTION_SIDE_OFFSET = 0.6;

	protected static ref array<ref HAM_IntelCellState> s_aStates;
	protected static ref array<IEntity> s_aBodyActions;
	protected static ref array<float> s_aBodyActionTimes;
	protected static ref array<IEntity> s_aAsked;
	protected static ref array<IEntity> s_aPay;
	protected static ref array<IEntity> s_aStay;
	protected static ref array<ref HAM_CivilianNeed> s_aNeeds;
	protected static ref array<ref HAM_MineFavor> s_aMineFavors;
	protected static int s_iIedTask;
	protected static ref array<ref HAM_CivilianWave> s_aWaves;
	protected static int s_iTrafficLeft;
	protected static int s_iTrafficFails;
	protected static ref array<vector> s_aTrafficSpots;
	protected static ref array<vector> s_aTrafficRoads;
	protected static ref array<ref HAM_RoadCar> s_aRoadCars;
	protected static int s_iHailTask;
	protected static int s_iNeedTask;
	protected static ref array<IEntity> s_aSpotBodies;
	protected static ref array<vector> s_aSpotPos;
	protected static ref array<IEntity> s_aBanana;
	protected static ref array<IEntity> s_aInfo;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aStates = null;
		s_aBodyActions = null;
		s_aBodyActionTimes = null;
		s_aAsked = null;
		s_aPay = null;
		s_aStay = null;
		s_aNeeds = null;
		s_aMineFavors = null;
		s_iIedTask = 0;
		s_aWaves = null;
		s_iTrafficLeft = 0;
		s_iTrafficFails = 0;
		s_aTrafficSpots = null;
		s_aTrafficRoads = null;
		s_aRoadCars = null;
		s_iHailTask = 0;
		s_iNeedTask = 0;
		s_aSpotBodies = null;
		s_aSpotPos = null;
		s_aBanana = null;
		s_aInfo = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aStates)
			s_aStates = {};

		if (!s_aBodyActions)
			s_aBodyActions = {};

		if (!s_aBodyActionTimes)
			s_aBodyActionTimes = {};
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_IntelCellState StateOf(HAM_Cell cell)
	{
		foreach (HAM_IntelCellState state : s_aStates)
		{
			if (state.m_sCell == cell.m_sName)
				return state;
		}

		HAM_IntelCellState created = new HAM_IntelCellState();
		created.m_sCell = cell.m_sName;
		s_aStates.Insert(created);
		return created;
	}

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		Ensure();
		HAM_Settings settings = HAM_Settings.Get();
		array<IEntity> bodies = HAM_Mission.s_aPlayerBodies;
		TickTraffic(bodies, settings);
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			HAM_IntelCellState state = StateOf(cell);
			float nearest = HAM_Util.NearestDistance(bodies, cell.Pos());

			if (!state.m_bIntelPlaced && HAM_Mission.IsFront(cell) && nearest <= settings.m_fActivationDistance)
			{
				state.m_bIntelPlaced = true;
				PlaceBuildingIntel(cell, state, settings);
			}

			TickCivilians(cell, state, bodies, settings);
		}

		TickCivilianLines(bodies, settings);
		RefreshCivilianWaves();

		float now = HAM_Util.Now();
		for (int i = s_aBodyActions.Count() - 1; i >= 0; i--)
		{
			IEntity action = s_aBodyActions[i];
			if (action && now - s_aBodyActionTimes[i] < BODY_ACTION_LIFETIME)
				continue;

			HAM_Util.Delete(action);
			s_aBodyActions.Remove(i);
			s_aBodyActionTimes.Remove(i);
		}

		TickRoadHails(bodies);
		SweepMineFavors();
	}

	//------------------------------------------------------------------------------------------------
	static void SlowTick(float dt)
	{
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceBuildingIntel(HAM_Cell cell, HAM_IntelCellState state, HAM_Settings settings)
	{
		array<SCR_DestructibleBuildingEntity> buildings = {};
		HAM_Interior.CollectBuildings(cell.Pos(), settings.m_fIntelBuildingRadius, buildings);
		if (buildings.IsEmpty())
			return;

		array<vector> placed = {};
		int guard = 0;
		while (placed.Count() < settings.m_iIntelPerCell && guard < settings.m_iIntelPerCell * 6)
		{
			guard++;
			vector spot;
			if (!HAM_Interior.FindSpot(buildings.GetRandomElement(), placed, spot))
				continue;

			IEntity intel = HAM_Util.Spawn(HAM_Units.INTEL_ITEM, spot + Vector(0, 0.05, 0), Math.RandomFloat(0, 360));
			if (!intel)
				continue;

			placed.Insert(spot);
			state.m_aIntel.Insert(intel);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Maître, esclave et QG ennemi. Le nombre vient de zoneTypes, y compris pour un camp.
	//! Les bases et les deux dépôts de ressources restent sans civils.
	protected static bool CiviliansAllowed(HAM_MapPoint def)
	{
		if (!def)
			return false;

		if (def.m_eRole != HAM_ERole.MASTER && def.m_eRole != HAM_ERole.SLAVE && def.m_eRole != HAM_ERole.ENEMY_HQ)
			return false;

		if (def.m_eType == HAM_EMasterType.BASE)
			return false;

		if (def.m_eType == HAM_EMasterType.SUPPLY_VANILLA || def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickCivilians(HAM_Cell cell, HAM_IntelCellState state, array<IEntity> bodies, HAM_Settings settings)
	{
		HAM_Units.Init();
		TickCivilianPoint(cell.m_Master, state, bodies, settings);
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			TickCivilianPoint(slave, state, bodies, settings);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Chaque point a sa propre vague. On ne les recrée pas tant que le joueur n'est pas parti puis revenu.
	//! La prise ne les retire pas : ville, planque et tout point où CiviliansAllowed est vrai restent habités.
	protected static void TickCivilianPoint(HAM_Point point, HAM_IntelCellState state, array<IEntity> bodies, HAM_Settings settings)
	{
		if (!point || !CiviliansAllowed(point.m_Def))
			return;

		float reach = point.Radius();
		if (reach < 30)
			reach = 30;

		TickCivilianAnchor(state, point, point.m_Def.m_sName, point.Pos(), reach, bodies, settings, point.m_Def);
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickCivilianLines(array<IEntity> bodies, HAM_Settings settings)
	{
		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || !line.m_Def || line.m_Def.m_eRole != HAM_ERole.CIVILIANS || !line.m_Cell)
				continue;

			float reach = line.m_Def.m_fRadius;
			if (reach < 20)
				reach = 20;

			TickCivilianAnchor(StateOf(line.m_Cell), null, line.m_Def.m_sName, line.Pos(), reach, bodies, settings, line.m_Def);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickCivilianAnchor(HAM_IntelCellState state, HAM_Point point, string name, vector center, float reach, array<IEntity> bodies, HAM_Settings settings, HAM_MapPoint zoneDef)
	{
		if (!state || name.IsEmpty())
			return;

		float activate = settings.ActivationOf(zoneDef);
		float leave = settings.DespawnOf(zoneDef);
		if (leave < activate)
			leave = activate;

		float nearest = float.MAX;
		if (bodies && !bodies.IsEmpty())
			nearest = HAM_Util.NearestDistance(bodies, center);

		bool listed = state.m_aCivilianPoints.Contains(name);
		if (listed)
		{
			if (!bodies || bodies.IsEmpty() || nearest > leave)
			{
				ClearCivilianAnchor(state, name);
				return;
			}

			if (AnchorHasLiving(name))
				return;

			DropEmptyGroups(name);
			state.m_aCivilianPoints.RemoveItem(name);
		}

		if (nearest > activate)
			return;

		state.m_aCivilianPoints.Insert(name);
		state.m_bCiviliansActive = true;
		int count = settings.CiviliansOf(zoneDef);
		if (count < 1)
			count = settings.m_iCivilianGroups * CIVILIANS_PER_GROUP;

		if (count < 1)
			return;

		HAM_CivilianWave wave = new HAM_CivilianWave();
		wave.m_Point = point;
		wave.m_State = state;
		wave.m_sKey = name;
		wave.m_vCenter = center;
		wave.m_fRadius = reach;
		wave.m_fSpin = Math.RandomFloat(0, Math.PI2);
		wave.m_iLeft = 0;
		wave.m_aGroups = {};
		wave.m_aBodies = {};
		for (int civilianIndex = 0; civilianIndex < count; civilianIndex++)
		{
			float floor = 8;
			if (reach > 80)
				floor = 22;

			vector spot = HAM_Util.CivilianSpot(center, floor, reach);
			if (HAM_Util.IsNearWater(spot) || !HAM_Util.OutOfBuildings(spot))
				continue;

			SCR_AIGroup solo = CivilianGroup(spot, 1);
			if (!solo)
				continue;

			wave.m_aGroups.Insert(solo);
			if (!wave.m_Group)
				wave.m_Group = solo;
		}
		if (!s_aWaves)
			s_aWaves = {};

		s_aWaves.Insert(wave);
		GetGame().GetCallqueue().CallLater(AdoptSpawnedCivilians, 800, false, wave);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearCivilianPoint(HAM_IntelCellState state, HAM_Point point)
	{
		if (!point)
			return;

		ClearCivilianAnchor(state, point.m_Def.m_sName);
	}

	//------------------------------------------------------------------------------------------------
	//! Les groupes vides partent. Les corps suivis restent jusqu'au départ du joueur.
	protected static void DropEmptyGroups(string name)
	{
		if (!s_aWaves || name.IsEmpty())
			return;

		foreach (HAM_CivilianWave wave : s_aWaves)
		{
			if (!wave || wave.m_sKey != name || WaveHasLiving(wave))
				continue;

			array<SCR_AIGroup> groups = {};
			if (wave.m_aGroups)
			{
				foreach (SCR_AIGroup solo : wave.m_aGroups)
				{
					if (solo && groups.Find(solo) < 0)
						groups.Insert(solo);
				}

				wave.m_aGroups.Clear();
			}

			if (wave.m_Group && groups.Find(wave.m_Group) < 0)
				groups.Insert(wave.m_Group);

			wave.m_Group = null;
			HAM_Util.DeleteGroups(groups);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Vivants de cette ancre, où qu'ils aient marché. La petite sphère du centre ne compte pas.
	protected static bool AnchorHasLiving(string name)
	{
		if (!s_aWaves || name.IsEmpty())
			return false;

		foreach (HAM_CivilianWave wave : s_aWaves)
		{
			if (wave && wave.m_sKey == name && WaveHasLiving(wave))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool WaveHasLiving(HAM_CivilianWave wave)
	{
		if (!wave)
			return false;

		if (wave.m_aBodies)
		{
			foreach (IEntity body : wave.m_aBodies)
			{
				if (body && !HAM_Util.IsDead(body))
					return true;
			}
		}

		if (HAM_Util.AliveCount(wave.m_Group) > 0)
			return true;

		if (!wave.m_aGroups)
			return false;

		foreach (SCR_AIGroup solo : wave.m_aGroups)
		{
			if (HAM_Util.AliveCount(solo) > 0)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool WaveStillTracked(HAM_CivilianWave wave)
	{
		if (!wave)
			return false;

		if (wave.m_Group)
			return true;

		if (wave.m_aGroups)
		{
			foreach (SCR_AIGroup solo : wave.m_aGroups)
			{
				if (solo)
					return true;
			}
		}

		if (!wave.m_aBodies)
			return false;

		foreach (IEntity body : wave.m_aBodies)
		{
			if (body)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberAgent(SCR_AIGroup group, notnull array<IEntity> known, notnull array<IEntity> livingAgents)
	{
		if (!group)
			return;

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body)
				continue;

			if (known.Find(body) < 0)
				known.Insert(body);

			if (!HAM_Util.IsDead(body) && livingAgents.Find(body) < 0)
				livingAgents.Insert(body);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Groupes, vivants et corps suivis de cette ancre. La distance au centre ne les épargne pas.
	protected static void ClearCivilianAnchor(HAM_IntelCellState state, string name)
	{
		if (!state)
			return;

		array<IEntity> known = {};
		array<IEntity> livingAgents = {};
		array<SCR_AIGroup> groups = {};
		if (s_aWaves)
		{
			foreach (HAM_CivilianWave wave : s_aWaves)
			{
				if (!wave || wave.m_sKey != name)
					continue;

				if (wave.m_aBodies)
				{
					foreach (IEntity body : wave.m_aBodies)
					{
						if (body && known.Find(body) < 0)
							known.Insert(body);
					}
				}

				if (wave.m_aGroups)
				{
					foreach (SCR_AIGroup solo : wave.m_aGroups)
					{
						if (!solo || groups.Find(solo) >= 0)
							continue;

						groups.Insert(solo);
						RememberAgent(solo, known, livingAgents);
					}
				}

				if (wave.m_Group && groups.Find(wave.m_Group) < 0)
				{
					groups.Insert(wave.m_Group);
					RememberAgent(wave.m_Group, known, livingAgents);
				}
			}
		}

		HAM_Util.DeleteGroups(groups);
		foreach (IEntity knownBody : known)
		{
			if (!knownBody || livingAgents.Find(knownBody) >= 0)
				continue;

			HAM_Util.Delete(knownBody);
		}

		for (int i = state.m_aCivilians.Count() - 1; i >= 0; i--)
		{
			IEntity civilian = state.m_aCivilians[i];
			if (!civilian || known.Find(civilian) >= 0)
				state.m_aCivilians.Remove(i);
		}

		state.m_aCivilianPoints.RemoveItem(name);
		state.m_bCiviliansActive = !state.m_aCivilianPoints.IsEmpty();
		DropWaves(name);
	}

	//------------------------------------------------------------------------------------------------
	//! Le groupe a fait naître ses membres. On leur pose l'action de dialogue, puis la patrouille.
	protected static void AdoptSpawnedCivilians(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_Group || !wave.m_State)
			return;

		if (!wave.m_State.m_bCiviliansActive || !wave.m_State.m_aCivilianPoints.Contains(wave.m_sKey))
			return;

		array<AIAgent> agents = {};
		if (wave.m_aGroups)
		{
			foreach (SCR_AIGroup solo : wave.m_aGroups)
			{
				if (!solo)
					continue;

				array<AIAgent> soloAgents = {};
				solo.GetAgents(soloAgents);
				foreach (AIAgent soloAgent : soloAgents)
					agents.Insert(soloAgent);
			}
		}

		if (agents.IsEmpty() && wave.m_Group)
			wave.m_Group.GetAgents(agents);

		if (agents.IsEmpty())
		{
			GetGame().GetCallqueue().CallLater(SpawnNextCivilian, 400, false, wave);
			return;
		}

		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body || wave.m_State.m_aCivilians.Contains(body))
				continue;

			wave.m_State.m_aCivilians.Insert(body);
			if (wave.m_aBodies && wave.m_aBodies.Find(body) < 0)
				wave.m_aBodies.Insert(body);

			wave.m_iSpawned++;
		}

		wave.m_iLeft = 0;
		WakeCivilianGroup(wave.m_Group);
		wave.m_bReleased = true;
		OrderCivilianPatrol(wave);
		HAM_Util.Info(string.Format("Civils : %1, %2 posés", wave.m_sKey, wave.m_iSpawned));
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnNextCivilian(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_State || wave.m_iLeft <= 0 || wave.m_sKey.IsEmpty())
			return;

		if (!wave.m_State.m_bCiviliansActive || !wave.m_State.m_aCivilianPoints.Contains(wave.m_sKey))
			return;

		wave.m_iLeft--;
		float reach = wave.m_fRadius * 0.85;
		if (reach < 30)
			reach = 30;

		float floor = 8;
		if (wave.m_fRadius > 80)
			floor = 22;

		if (floor > reach)
			floor = reach * 0.5;

		if (floor < 6)
			floor = 6;

		vector spot = HAM_Util.CivilianSpot(wave.m_vCenter, floor, reach);
		if (!HAM_Util.FootSpot(spot))
			spot = HAM_Util.CivilianSpot(wave.m_vCenter, reach, reach + 25);

		if (!HAM_Util.FootSpot(spot))
		{
			if (wave.m_iLeft > 0)
				GetGame().GetCallqueue().CallLater(SpawnNextCivilian, 400, false, wave);

			return;
		}

		wave.m_iIndex++;
		IEntity civilian = HAM_Util.Spawn(HAM_Units.Civilian(), spot, Math.RandomFloat(0, 360));
		if (civilian)
		{
			HAM_Util.PlaceUpright(civilian, spot);
			wave.m_State.m_aCivilians.Insert(civilian);
			if (!wave.m_aBodies)
				wave.m_aBodies = {};

			if (wave.m_aBodies.Find(civilian) < 0)
				wave.m_aBodies.Insert(civilian);

			wave.m_iSpawned++;
			JoinCivilianGroup(civilian, wave.m_Group);
			HAM_Util.PlaceUpright(civilian, spot);
			AllowPatrolMove(civilian);
			HAM_Util.KeepAwake(wave.m_Group);
		}

		if (wave.m_iLeft > 0)
		{
			GetGame().GetCallqueue().CallLater(SpawnNextCivilian, 400, false, wave);
			return;
		}

		GetGame().GetCallqueue().CallLater(ReleaseCivilianWave, 1200, false, wave);
		HAM_Util.Info(string.Format("Civils : %1, %2 posés", wave.m_sKey, wave.m_iSpawned));
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReleaseCivilianWave(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_Group || !wave.m_State)
			return;

		if (wave.m_aBodies)
		{
			foreach (IEntity civilian : wave.m_aBodies)
			{
				if (!civilian || HAM_Util.IsDead(civilian))
					continue;

				JoinCivilianGroup(civilian, wave.m_Group);
				AllowPatrolMove(civilian);
			}
		}

		UnstackWave(wave);
		WakeCivilianGroup(wave.m_Group);
		array<AIAgent> agents = {};
		wave.m_Group.GetAgents(agents);
		if (agents.IsEmpty())
			return;

		wave.m_bReleased = true;
		OrderCivilianPatrol(wave);
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul point de patrouille est consommé dès qu'il est atteint. On en redonne un tant que la vague vit.
	protected static void OrderCivilianPatrol(HAM_CivilianWave wave)
	{
		if (!wave)
			return;

		if (wave.m_aGroups && !wave.m_aGroups.IsEmpty())
		{
			foreach (SCR_AIGroup solo : wave.m_aGroups)
				OrderCivilianGroup(wave, solo);

			return;
		}

		OrderCivilianGroup(wave, wave.m_Group);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OrderCivilianGroup(HAM_CivilianWave wave, SCR_AIGroup group)
	{
		if (!wave || !group)
			return;

		WakeCivilianGroup(group);
		WalkOnly(group);
		float reach = wave.m_fRadius * 0.65;
		if (reach < 25)
			reach = 25;
		if (reach > 90)
			reach = 90;

		HAM_Util.ClearWaypoints(group);
		vector origin = group.GetOrigin();
		int given = 0;
		for (int step = 0; step < 8 && given < 3; step++)
		{
			vector spot = HAM_Util.CivilianSpot(wave.m_vCenter, 30, reach);
			if (!HAM_Util.FootSpot(spot))
				continue;

			HAM_Util.SnapToNavmesh(group, spot);
			if (vector.DistanceXZ(spot, origin) < 20)
				continue;

			HAM_Util.Order(group, HAM_Units.WP_MOVE, spot, 10);
			given++;
		}

		if (given < 1)
		{
			vector spot = HAM_Util.DryAround(origin, 40, reach);
			if (HAM_Util.FootSpot(spot))
			{
				HAM_Util.SnapToNavmesh(group, spot);
				HAM_Util.Order(group, HAM_Units.WP_MOVE, spot, 10);
			}
		}

		wave.m_vAnchor = origin;
		wave.m_bAnchor = true;
		wave.m_fOrderAt = HAM_Util.Now();
	}

	//------------------------------------------------------------------------------------------------
	//! L'ordre est posé, mais tout le monde est encore sur place : on le redonne.
	protected static bool CivilianWaveStuck(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_bAnchor || !wave.m_bReleased)
			return false;

		if (HAM_Util.Now() - wave.m_fOrderAt < 8)
			return false;

		if (!wave.m_State)
			return false;

		int seen = 0;
		foreach (IEntity civilian : wave.m_State.m_aCivilians)
		{
			if (!civilian || HAM_Util.IsDead(civilian))
				continue;

			if (vector.DistanceXZ(civilian.GetOrigin(), wave.m_vCenter) > wave.m_fRadius + 80)
				continue;

			seen++;
			if (vector.DistanceXZ(civilian.GetOrigin(), wave.m_vAnchor) > 8)
				return false;
		}

		return seen > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberCivilianSpot(IEntity civilian, vector spot)
	{
		if (!civilian)
			return;

		if (!s_aSpotBodies)
		{
			s_aSpotBodies = {};
			s_aSpotPos = {};
		}

		int index = s_aSpotBodies.Find(civilian);
		if (index < 0)
		{
			s_aSpotBodies.Insert(civilian);
			s_aSpotPos.Insert(spot);
		}
		else
		{
			s_aSpotPos[index] = spot;
		}

		GetGame().GetCallqueue().CallLater(KeepCivilianSpot, 500, false, civilian);
	}

	//------------------------------------------------------------------------------------------------
	//! Le groupe ramène parfois tout le monde sur son origine juste après l'ajout. On repose le civil s'il a été collé.
	protected static void KeepCivilianSpot(IEntity civilian)
	{
		if (!civilian || HAM_Util.IsDead(civilian) || !s_aSpotBodies)
			return;

		int index = s_aSpotBodies.Find(civilian);
		if (index < 0)
			return;

		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(civilian.FindComponent(CompartmentAccessComponent));
		if (access && access.IsInCompartment())
			return;

		vector spot = s_aSpotPos[index];
		if (vector.DistanceXZ(civilian.GetOrigin(), spot) < 1.2)
			return;

		HAM_Util.PlaceUpright(civilian, spot);
	}

	//------------------------------------------------------------------------------------------------
	protected static void UnstackWave(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_State)
			return;

		float keep = wave.m_fRadius + 80;
		array<IEntity> bodies = {};
		foreach (IEntity civilian : wave.m_State.m_aCivilians)
		{
			if (!civilian || HAM_Util.IsDead(civilian))
				continue;

			if (vector.DistanceXZ(civilian.GetOrigin(), wave.m_vCenter) > keep)
				continue;

			bodies.Insert(civilian);
		}

		float floor = 8;
		if (wave.m_fRadius > 80)
			floor = 22;

		for (int i = 0; i < bodies.Count(); i++)
		{
			IEntity body = bodies[i];
			bool stacked = vector.DistanceXZ(body.GetOrigin(), wave.m_vCenter) < 2;
			for (int j = 0; j < i; j++)
			{
				if (vector.DistanceXZ(body.GetOrigin(), bodies[j].GetOrigin()) < 2)
					stacked = true;
			}

			if (!stacked)
				continue;

			vector spot = HAM_Util.CivilianSpot(wave.m_vCenter, floor, wave.m_fRadius * 0.7);
			if (HAM_Util.IsNearWater(spot))
				continue;

			HAM_Util.PlaceUpright(body, spot);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpreadRoadCrew(Vehicle vehicle)
	{
		if (!vehicle || HAM_Util.IsDead(vehicle))
			return;

		HAM_Util.SeparateNearOrigin(vehicle.GetOrigin(), 4.5, 2.2);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RefreshCivilianWaves()
	{
		if (!s_aWaves)
			return;

		for (int i = s_aWaves.Count() - 1; i >= 0; i--)
		{
			HAM_CivilianWave wave = s_aWaves[i];
			if (!wave || !WaveStillTracked(wave))
			{
				s_aWaves.Remove(i);
				continue;
			}

			// Les corps restent suivis jusqu'au départ du joueur, même hors de la zone.
			if (!WaveHasLiving(wave) || !wave.m_Group)
				continue;

			if (!wave.m_bReleased)
			{
				ReleaseCivilianWave(wave);
				continue;
			}

			RejoinWaveBodies(wave);
			HAM_Util.KeepAwake(wave.m_Group);
			array<AIWaypoint> waypoints = {};
			int pending = wave.m_Group.GetWaypoints(waypoints);
			if (pending > 0 && !CivilianWaveStuck(wave))
				continue;

			WakeCivilianGroup(wave.m_Group);
			OrderCivilianPatrol(wave);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Raccroche un civil de cette vague qui a quitté son groupe. Pas ceux des autres ancres.
	protected static void RejoinWaveBodies(HAM_CivilianWave wave)
	{
		if (!wave || !wave.m_aBodies || !wave.m_Group)
			return;

		foreach (IEntity body : wave.m_aBodies)
		{
			if (!body || HAM_Util.IsDead(body))
				continue;

			AIControlComponent control = AIControlComponent.Cast(body.FindComponent(AIControlComponent));
			if (control)
			{
				AIAgent agent = control.GetControlAIAgent();
				if (!agent)
					agent = control.GetAIAgent();

				if (agent && agent.GetParentGroup())
					continue;
			}

			JoinCivilianGroup(body, wave.m_Group);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropWaves(string name)
	{
		if (!s_aWaves || name.IsEmpty())
			return;

		for (int i = s_aWaves.Count() - 1; i >= 0; i--)
		{
			HAM_CivilianWave wave = s_aWaves[i];
			if (wave && wave.m_sKey == name)
				s_aWaves.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void WalkOnly(SCR_AIGroup group)
	{
		if (!group)
			return;

		AIGroupMovementComponent movement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (movement)
			movement.SetGroupCharactersWantedMovementType(EMovementType.WALK);

		SCR_AIGroupSettingsComponent settings = SCR_AIGroupSettingsComponent.Cast(group.FindComponent(SCR_AIGroupSettingsComponent));
		if (!settings)
			return;

		settings.AddSetting(SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.SCENARIO, EMovementType.WALK), false, true);
		settings.AddSetting(SCR_AIGroupCombatModeSetting.Create(SCR_EAISettingOrigin.SCENARIO, EAIGroupCombatMode.HOLD_FIRE), false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetCivilianCalm(IEntity civilian)
	{
		if (!civilian)
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(civilian.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetStanceChange(ECharacterStanceChange.STANCECHANGE_TOERECTED);

		SCR_AICharacterSettingsComponent settings = SCR_AICharacterSettingsComponent.Cast(civilian.FindComponent(SCR_AICharacterSettingsComponent));
		if (settings)
			settings.AddCharacterSetting(SCR_AICharacterStanceSetting.Create(SCR_EAISettingOrigin.SCENARIO, SCR_EAIBehaviorCause.ALWAYS, ECharacterStance.STAND), false, true);

		SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(civilian.FindComponent(SCR_AICombatComponent));
		if (combat)
			combat.SetPerceptionFactor(0);
	}

	//------------------------------------------------------------------------------------------------
	static SCR_AIGroup CivilianGroup(vector pos, int count)
	{
		Resource prefab = Resource.Load(HAM_Units.CQB_SOLO_GROUP);
		if (!prefab || !prefab.IsValid())
			return null;

		if (count < 1)
			count = 1;
		if (count > 12)
			count = 12;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = HAM_Util.Ground(pos);

		SCR_AIGroup.IgnoreSpawning(true);
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);
		if (!group)
			return null;

		array<ResourceName> slots = {};
		for (int i = 0; i < count; i++)
			slots.Insert(HAM_Units.Civilian());

		group.m_aUnitPrefabSlots = slots;
		group.SetMaxUnitsToSpawn(count);
		group.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		group.SetImportance(SCR_EAISpawnImportance.CRITICAL);
		Faction civilians = GetGame().GetFactionManager().GetFactionByKey(HAM_Units.CIVILIAN_FACTION);
		if (civilians)
			group.SetFaction(civilians);

		group.SpawnUnits();
		HAM_Util.Forget(group);
		WakeCivilianGroup(group);
		return group;
	}

	//------------------------------------------------------------------------------------------------
	//! Un groupe rempli à la main ne démarre pas sa patrouille tant que chaque agent n'est pas activé.
	protected static void WakeCivilianGroup(SCR_AIGroup group)
	{
		if (!group)
			return;

		group.PreventMaxLOD();
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			agent.PreventMaxLOD();
			if (!agent.IsAIActivated())
				agent.ActivateAI();

			IEntity body = agent.GetControlledEntity();
			if (!body || CivilianStays(body))
				continue;

			CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
			if (!controller)
				continue;

			controller.SetDisableMovementControls(false);
			controller.SetCanMove(true);
		}

		HAM_Util.KeepAwake(group);
	}

	//------------------------------------------------------------------------------------------------
	protected static void JoinCivilianGroup(IEntity civilian, SCR_AIGroup group)
	{
		if (!civilian || !group)
			return;

		AIControlComponent control = AIControlComponent.Cast(civilian.FindComponent(AIControlComponent));
		if (!control)
			return;

		if (!control.IsAIActivated())
			control.ActivateAI();

		AIAgent waking = control.GetControlAIAgent();
		if (!waking)
			waking = control.GetAIAgent();
		if (waking && !waking.IsAIActivated())
			waking.ActivateAI();

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			agent = control.GetAIAgent();

		if (!agent)
			return;

		AIGroup previous = agent.GetParentGroup();
		if (previous && previous != group)
			previous.RemoveAgent(agent);

		if (agent.GetParentGroup() != group)
			group.AddAgent(agent);
	}

	//------------------------------------------------------------------------------------------------
	protected static float s_fCivilReach = -1;
	protected static float s_fCivilLeave = -1;
	protected static float s_fNextTraffic;

	//------------------------------------------------------------------------------------------------
	protected static void CivilDistances(HAM_Settings settings, out float reach, out float leave)
	{
		if (s_fCivilReach < 0)
		{
			HAM_MapPoint probe = new HAM_MapPoint();
			probe.m_eRole = HAM_ERole.CIVILIANS;
			s_fCivilReach = settings.ActivationOf(probe);
			s_fCivilLeave = settings.DespawnOf(probe);
			if (s_fCivilLeave < s_fCivilReach + 100)
				s_fCivilLeave = s_fCivilReach + 100;
		}

		reach = s_fCivilReach;
		leave = s_fCivilLeave;
	}

	//------------------------------------------------------------------------------------------------
	//! Les voitures civiles naissent dans le rayon d'activation et partent au-delà du rayon de disparition.
	protected static void TickTraffic(array<IEntity> bodies, HAM_Settings settings)
	{
		float reach;
		float leave;
		CivilDistances(settings, reach, leave);
		DropFarTraffic(bodies, leave);
		if (!bodies || bodies.IsEmpty() || settings.m_iCivilianVehicles < 1)
			return;

		if (HAM_Util.Now() < s_fNextTraffic)
			return;

		if (TrafficNear(bodies, leave) >= settings.m_iCivilianVehicles)
			return;

		IEntity body = bodies.GetRandomElement();
		if (!body)
			return;

		s_fNextTraffic = HAM_Util.Now() + 6;
		SpawnTrafficNear(body.GetOrigin(), reach);
	}

	//------------------------------------------------------------------------------------------------
	protected static int TrafficNear(array<IEntity> bodies, float leave)
	{
		if (!s_aRoadCars)
			return 0;

		int count = 0;
		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (!car || !car.m_Vehicle || HAM_Util.IsDead(car.m_Vehicle))
				continue;

			if (HAM_Util.NearestDistance(bodies, car.m_Vehicle.GetOrigin()) <= leave)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropFarTraffic(array<IEntity> bodies, float leave)
	{
		if (!s_aRoadCars)
			return;

		for (int i = s_aRoadCars.Count() - 1; i >= 0; i--)
		{
			HAM_RoadCar car = s_aRoadCars[i];
			if (!car || !car.m_Vehicle || HAM_Util.IsDead(car.m_Vehicle))
			{
				s_aRoadCars.Remove(i);
				continue;
			}

			if (TrafficHasPlayer(car.m_Vehicle))
				continue;

			if (bodies && !bodies.IsEmpty() && HAM_Util.NearestDistance(bodies, car.m_Vehicle.GetOrigin()) <= leave)
				continue;

			ReleaseTraffic(car);
			s_aRoadCars.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TrafficHasPlayer(Vehicle vehicle)
	{
		if (!vehicle)
			return false;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
			return false;

		PlayerManager players = GetGame().GetPlayerManager();
		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			if (occupant && players && players.GetPlayerIdFromControlledEntity(occupant) > 0)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReleaseTraffic(HAM_RoadCar car)
	{
		if (!car || !car.m_Vehicle)
			return;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(car.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (compartments)
		{
			SCR_AIGroup crew = HAM_Garrison.CrewGroup(compartments);
			if (crew)
				HAM_Util.DeleteGroup(crew);
		}

		HAM_Util.Delete(car.m_Vehicle);
		car.m_Vehicle = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnTrafficNear(vector origin, float reach)
	{
		vector road;
		if (!FindTrafficRoad(origin, reach, road))
			return;

		array<vector> route = {};
		int stops = Math.RandomInt(2, 4);
		for (int step = 0; step < stops; step++)
		{
			vector sample = HAM_Util.RandomAround(origin, 40, reach);
			vector stop;
			if (!HAM_Util.RoadPoint(sample, 80, stop) || !HAM_Util.VehicleSpot(stop))
				continue;

			if (vector.DistanceXZ(stop, road) < 40)
				continue;

			route.Insert(stop);
		}

		if (route.IsEmpty())
			route.Insert(road);

		ResourceName prefab = TrafficPrefab(Math.RandomInt(0, 4));
		Vehicle vehicle = HAM_Garrison.SpawnCrewedVehicle(prefab, road, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_MOVE, 30, null, false, ResourceName.Empty);
		if (!vehicle)
			return;

		if (!s_aRoadCars)
			s_aRoadCars = {};

		HAM_RoadCar car = new HAM_RoadCar();
		car.m_Vehicle = vehicle;
		s_aRoadCars.Insert(car);
		HAM_Util.Info(string.Format("Circulation civile : %1", HAM_Util.PrefabFileName(prefab)));
	}

	//------------------------------------------------------------------------------------------------
	//! Un groupe civil entre deux zones : une voiture déjà utilisée par la circulation, sinon des piétons.
	static bool SpawnPassCivilians(vector pos, notnull array<IEntity> into)
	{
		if (Math.RandomFloat01() < 0.5)
		{
			vector road;
			if (HAM_Util.RoadPoint(pos, 150, road) && HAM_Util.VehicleSpot(road))
			{
				array<vector> route = {};
				route.Insert(HAM_Util.RandomAround(road, 400, 900));
				Vehicle car = HAM_Garrison.SpawnCrewedVehicle(TrafficPrefab(Math.RandomInt(0, 4)), road, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_MOVE, 30, null, false, ResourceName.Empty);
				if (car)
				{
					into.Insert(car);
					return true;
				}
			}
		}

		HAM_MapPoint probe = new HAM_MapPoint();
		probe.m_eRole = HAM_ERole.CIVILIANS;
		HAM_ZoneTune tune = HAM_Settings.Get().TuneFor(probe);
		int count = 1;
		if (tune && tune.m_iCivilians > 0)
			count = tune.m_iCivilians;

		int spawned = 0;
		for (int i = 0; i < count; i++)
		{
			vector spot = HAM_Util.CivilianSpot(pos, 8, 40);
			if (!HAM_Util.FieldSpot(spot))
				continue;

			SCR_AIGroup solo = CivilianGroup(spot, 1);
			if (!solo)
				continue;

			array<AIAgent> agents = {};
			solo.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!agent || !agent.GetControlledEntity())
					continue;

				into.Insert(agent.GetControlledEntity());
				spawned++;
			}
		}

		return spawned > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindTrafficRoad(vector origin, float reach, out vector road)
	{
		road = "0 0 0";
		float outer = reach;
		if (outer < 80)
			outer = 80;

		for (int attempt = 0; attempt < 8; attempt++)
		{
			vector sample = HAM_Util.RandomAround(origin, 40, outer);
			if (!HAM_Util.RoadPoint(sample, 80, road) || !HAM_Util.VehicleSpot(road))
				continue;

			if (vector.DistanceXZ(road, origin) > outer)
				continue;

			if (TrafficRoadTaken(road))
				continue;

			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TrafficRoadTaken(vector road)
	{
		if (s_aTrafficRoads)
		{
			foreach (vector used : s_aTrafficRoads)
			{
				if (vector.DistanceXZ(used, road) < 140)
					return true;
			}
		}

		if (!s_aRoadCars)
			return false;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (car && car.m_Vehicle && vector.DistanceXZ(car.m_Vehicle.GetOrigin(), road) < 40)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickRoadHails(array<IEntity> bodies)
	{
		if (!s_aRoadCars || !bodies)
			return;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (car && car.m_bOnFoot && car.m_Talker && !car.m_bTaskDone && car.m_fSince > 0 && HAM_Util.Now() - car.m_fSince > 90)
			{
				ReleaseCivilian(car.m_Talker);
				continue;
			}

			if (!car || car.m_bRolled || !car.m_Vehicle || HAM_Util.IsDead(car.m_Vehicle))
				continue;

			if (HAM_Garrison.IsCrewPending(car.m_Vehicle))
				continue;

			if (HAM_Util.NearestDistance(bodies, car.m_Vehicle.GetOrigin()) > 50)
				continue;

			car.m_bRolled = true;
			if (Math.RandomInt(0, 3) != 0)
				continue;

			StartHail(car);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void StartHail(HAM_RoadCar car)
	{
		if (!car || !car.m_Vehicle)
			return;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(car.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
		{
			car.m_bRolled = false;
			return;
		}

		HAM_Util.ClearWaypoints(HAM_Garrison.CrewGroup(compartments));
		Physics physics = car.m_Vehicle.GetPhysics();
		if (physics)
			physics.SetVelocity("0 0 0");

		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		IEntity talker;
		foreach (IEntity occupant : occupants)
		{
			if (!occupant || HAM_Util.IsDead(occupant))
				continue;

			if (!talker)
				talker = occupant;

			LeaveVehicle(occupant, car.m_Vehicle);
		}

		if (!talker)
		{
			car.m_bRolled = false;
			return;
		}

		CompartmentAccessComponent stillIn = CompartmentAccessComponent.Cast(talker.FindComponent(CompartmentAccessComponent));
		if (stillIn && stillIn.IsInCompartment())
			return;

		car.m_Talker = talker;
		car.m_bOnFoot = true;
		car.m_fSince = HAM_Util.Now();
		HAM_Util.SeparateNearOrigin(car.m_Vehicle.GetOrigin(), 4.5, 2.2);
		GetGame().GetCallqueue().CallLater(FinishHail, 700, false, car);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool LeaveVehicle(IEntity occupant, Vehicle vehicle)
	{
		if (!occupant)
			return false;

		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(occupant.FindComponent(CompartmentAccessComponent));
		if (!access || !access.IsInCompartment())
			return true;

		access.GetOutVehicle(EGetOutType.TELEPORT, 0, ECloseDoorAfterActions.INVALID, true);
		if (!access.IsInCompartment())
			return true;

		if (!vehicle)
			return false;

		vector mat[4];
		vehicle.GetWorldTransform(mat);
		vector spot = vehicle.GetOrigin() + mat[0] * 3;
		spot[1] = HAM_Util.SurfaceY(spot);
		mat[3] = spot;
		access.GetOutVehicle_NoDoor(mat, false, true);
		return !access.IsInCompartment();
	}

	//------------------------------------------------------------------------------------------------
	protected static void FinishHail(HAM_RoadCar car)
	{
		if (!car || !car.m_Talker || HAM_Util.IsDead(car.m_Talker) || !HAM_Mission.IsRunning())
			return;

		HoldCivilian(car.m_Talker);

		if (car.m_Vehicle)
		{
			HAM_Util.SeparateNearOrigin(car.m_Vehicle.GetOrigin(), 4.5, 2.2);
			GetGame().GetCallqueue().CallLater(SpreadRoadCrew, 1200, false, car.m_Vehicle);
		}

		SetCivilianCalm(car.m_Talker);
		if (car.m_Talker.GetPrefabData())
		{
			string prefab = car.m_Talker.GetPrefabData().GetPrefabName();
			if (!prefab.Contains("HAM_CIV"))
				AttachAction(HAM_Units.CIVILIAN_TALK_ACTION, car.m_Talker, Vector(0, 1.2, 0));
		}

		car.m_Task = CreateHailTask(car.m_Talker);
		HAM_Mission.Notify("Civil", "Un civil s'est arrêté sur la route. Il veut vous parler.", 8);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_Task CreateHailTask(IEntity civilian)
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem || !civilian)
			return null;

		s_iHailTask++;
		string taskId = "HAM_HAIL_" + s_iHailTask.ToString();
		SCR_Task task = taskSystem.CreateTask("{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et", taskId, "Civil", "Il veut vous parler.", civilian.GetOrigin());
		if (!task)
			return null;

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		return task;
	}

	//------------------------------------------------------------------------------------------------
	static void CompleteHail(IEntity civilian)
	{
		if (!s_aRoadCars || !civilian)
			return;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (!car || car.m_bTaskDone || car.m_Talker != civilian)
				continue;

			car.m_bTaskDone = true;
			if (car.m_Task)
				car.m_Task.SetTaskState(SCR_ETaskState.COMPLETED);

			car.m_Task = null;
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static ResourceName TrafficPrefab(int index)
	{
		int pick = index;
		while (pick >= 4)
			pick = pick - 4;

		if (pick == 0)
			return "{128253A267BE9424}Prefabs/Vehicles/Wheeled/S105/S105_randomized.et";

		if (pick == 1)
			return "{57A441224AC02CF3}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_covered_CIV_Randomized.et";

		if (pick == 2)
			return "{1B0D46B41C50873D}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_uncovered_CIV_Randomized.et";

		return "{C7CE8044B5E301F5}Prefabs/Vehicles/Wheeled/UAZ452/UAZ452_transport_CIV_randomized.et";
	}

	//------------------------------------------------------------------------------------------------
	//! Entité d'action posée sur un personnage et accrochée à lui.
	//! Les autres actions du civil occupent le même point : on les retire le temps du rapport.
	protected static void ClearTalkActions(IEntity civilian)
	{
		if (!civilian)
			return;

		array<IEntity> drop = {};
		IEntity child = civilian.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			if (child.GetPrefabData())
			{
				string name = child.GetPrefabData().GetPrefabName();
				if (name.Contains("HAM_CivilianTalkAction") || name.Contains("HAM_CivilianAskAction"))
					drop.Insert(child);
			}

			child = next;
		}

		foreach (IEntity action : drop)
			HAM_Util.Delete(action);
	}

	//------------------------------------------------------------------------------------------------
	static IEntity AttachAction(ResourceName prefab, IEntity body, vector localOffset)
	{
		if (!body || prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.Parent = body;
		params.TransformMode = ETransformMode.LOCAL;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = localOffset;
		IEntity action = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!action)
			return null;

		action.SetFlags(EntityFlags.ACTIVE, true);
		HAM_Util.Forget(action);
		return action;
	}

	//------------------------------------------------------------------------------------------------
	static void OnCellRevealed(HAM_Cell cell)
	{
		HAM_MasterBases.ShowDiscovered(cell);
	}

	//------------------------------------------------------------------------------------------------
	static void OnCellComplete(HAM_Cell cell)
	{
		Ensure();
		HAM_IntelCellState state = StateOf(cell);
		foreach (IEntity intel : state.m_aIntel)
		{
			HAM_Util.Delete(intel);
		}

		state.m_aIntel.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aRepVictims;

	//------------------------------------------------------------------------------------------------
	protected static bool RepCounted(IEntity victim)
	{
		if (!victim)
			return true;

		if (!s_aRepVictims)
			s_aRepVictims = {};

		if (s_aRepVictims.Contains(victim))
			return true;

		s_aRepVictims.Insert(victim);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Mort réelle par un joueur. Une suppression de script (DELETED) ou une IA ne compte pas.
	protected static bool PlayerKill(SCR_InstigatorContextData data)
	{
		if (!data)
			return false;

		if (data.HasAnyVictimKillerRelation(SCR_ECharacterDeathStatusRelations.DELETED))
			return false;

		if (data.HasAnyVictimKillerRelation(SCR_ECharacterDeathStatusRelations.DELETED_BY_EDITOR))
			return false;

		if (data.GetKillerPlayerID() <= 0)
			return false;

		return data.HasAnyKillerCharacterControlType(SCR_ECharacterControlType.PLAYER);
	}

	//------------------------------------------------------------------------------------------------
	static void OnUnitKilled(IEntity victim, SCR_InstigatorContextData data)
	{
		Ensure();
		string factionKey = HAM_Util.FactionKey(victim);
		HAM_Settings settings = HAM_Settings.Get();
		bool playerKill = PlayerKill(data);
		if (factionKey == HAM_Units.CIVILIAN_FACTION)
		{
			if (playerKill && !RepCounted(victim))
			{
				float loss = settings.m_iRepCivilianKillPenalty;
				HAM_Mission.AddGlobalRep(-loss, "civil tué");
				int killerId = data.GetKillerPlayerID();
				string name = GetGame().GetPlayerManager().GetPlayerName(killerId);
				HAM_Mission.Notify("Réputation", name + " a tué un civil : -" + HAM_Mission.FormatRep(loss) + ".", 8);
			}

			return;
		}

		if (!HAM_Units.IsEnemyFaction(factionKey))
			return;

		if (playerKill && !RepCounted(victim))
			HAM_Mission.AddGlobalRep(settings.m_fRepPerEnemyKill, "ennemi tué");

		// Chaque ennemi laisse un dossier. intel.bodyChance décide si ce dossier localise une zone.
		GetGame().GetCallqueue().CallLater(PlaceBodyAction, BODY_ACTION_DELAY_MS, false, victim);
	}

	//------------------------------------------------------------------------------------------------
	//! Le corps est retombé : le dossier va au sol, à côté du bassin. Sur le corps, les zones d'action
	//! du personnage (inventaire, soins) passaient devant la fouille.
	protected static void PlaceBodyAction(IEntity body)
	{
		if (!body || !HAM_Mission.IsRunning())
			return;

		vector hips = body.GetOrigin();
		Animation anim = body.GetAnimation();
		if (anim)
		{
			TNodeId bone = anim.GetBoneIndex("Hips");
			if (bone != -1)
			{
				vector boneMat[4];
				anim.GetBoneMatrix(bone, boneMat);
				hips = body.CoordToParent(boneMat[3]);
			}
		}

		float angle = Math.RandomFloat(0, Math.PI2);
		vector spot = HAM_Util.Ground(hips + Vector(Math.Cos(angle) * BODY_ACTION_SIDE_OFFSET, 0, Math.Sin(angle) * BODY_ACTION_SIDE_OFFSET));
		IEntity action = HAM_Util.Spawn(HAM_Units.BODY_SEARCH_ACTION, spot + Vector(0, 0.03, 0), Math.RandomFloat(0, 360));
		if (!action)
			return;

		Ensure();
		s_aBodyActions.Insert(action);
		s_aBodyActionTimes.Insert(HAM_Util.Now());
	}

	//------------------------------------------------------------------------------------------------
	static void SearchBody(IEntity action, int playerId)
	{
		if (!Replication.IsServer() || !action || !HAM_Mission.IsRunning())
			return;

		vector pos = action.GetOrigin();
		int index = s_aBodyActions.Find(action);
		if (index >= 0)
		{
			s_aBodyActions.Remove(index);
			s_aBodyActionTimes.Remove(index);
		}

		HAM_Util.Delete(action);
		GiveIntel(pos, playerId, "corps fouillé", true);
	}

	//------------------------------------------------------------------------------------------------
	static void OnIntelPicked(IEntity intel, int playerId)
	{
		if (!Replication.IsServer() || !intel || !HAM_Mission.IsRunning())
			return;

		vector pos = intel.GetOrigin();
		foreach (HAM_IntelCellState state : s_aStates)
		{
			state.m_aIntel.RemoveItem(intel);
		}

		HAM_Util.Delete(intel);
		GiveIntel(pos, playerId, "renseignement trouvé", false);
	}

	//------------------------------------------------------------------------------------------------
	//! Révèle parfois une cellule du front. Chaque document crédite la réputation réglée.
	//! Un dossier de corps utilise intel.bodyChance. Un document de bâtiment utilise intelRevealOneIn.
	protected static void GiveIntel(vector pos, int playerId, string reason, bool bodyDocument)
	{
		HAM_Settings settings = HAM_Settings.Get();
		float bonus = settings.m_fRepIntel;
		HAM_Mission.AddGlobalRep(bonus, "renseignement ramassé");
		string gain = "Réputation +" + HAM_Mission.FormatRep(bonus) + ".";
		bool useful = false;
		if (bodyDocument)
		{
			float chance = settings.m_fIntelBodyChance;
			if (chance < 0)
				chance = 0;
			if (chance > 1)
				chance = 1;

			useful = Math.RandomFloat01() < chance;
		}
		else
		{
			int oneIn = settings.m_iIntelRevealOneIn;
			if (oneIn < 1)
				oneIn = 1;

			useful = Math.RandomInt(0, oneIn) == 0;
		}

		if (!useful)
		{
			HAM_Net.NotifyPlayer(playerId, "Renseignement", "Rien qui désigne une zone. " + gain, 6);
			return;
		}

		HAM_Cell revealed = HAM_Front.RevealFrontCell(reason, pos, true);
		if (revealed)
		{
			HAM_Net.NotifyPlayer(playerId, "Renseignement", string.Format("Le document localise une zone ennemie : %1. %2", revealed.m_sName, gain), 8);
			return;
		}

		HAM_Net.NotifyPlayer(playerId, "Renseignement", "Rien de nouveau sur la ligne de front. " + gain, 8);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_Point HideoutAt(vector pos)
	{
		HAM_Point best;
		float bestSq = -1;
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			ConsiderHideout(cell.m_Master, pos, best, bestSq);
			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				ConsiderHideout(slave, pos, best, bestSq);
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ConsiderHideout(HAM_Point point, vector pos, out HAM_Point best, out float bestSq)
	{
		if (!point || point.m_bPlayer || point.m_Def.m_eType != HAM_EMasterType.HIDEOUT || !point.m_Cache)
			return;

		float distSq = vector.DistanceSqXZ(pos, point.Pos());
		float radius = point.Radius();
		if (distSq > radius * radius)
			return;

		if (bestSq >= 0 && distSq >= bestSq)
			return;

		bestSq = distSq;
		best = point;
	}

	//------------------------------------------------------------------------------------------------
	static bool HadInfo(IEntity civilian)
	{
		return civilian && s_aInfo && s_aInfo.Contains(civilian);
	}

	//------------------------------------------------------------------------------------------------
	//! Une fois par civil. Trois chances sur dix de révéler un maître déjà sur la ligne de front.
	static void AskForInfo(IEntity civilian, int playerId)
	{
		if (!Replication.IsServer() || !civilian || !HAM_Mission.IsRunning())
			return;

		if (HadInfo(civilian))
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", "Il n'a plus rien à signaler.", 5);
			return;
		}

		if (!s_aInfo)
			s_aInfo = {};

		s_aInfo.Insert(civilian);
		vector pos = civilian.GetOrigin();
		if (Math.RandomInt(0, 100) >= 30)
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", NothingSeen(), 6);
			return;
		}

		HAM_Point hideout = HideoutAt(pos);
		HAM_Cell here = HAM_Mission.CellAt(pos);
		if (hideout && here && here.m_bRevealed && HAM_SideMissions.OpenHideout(hideout))
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", string.Format("« Il y a une cache à %1. Détruisez-la. »", hideout.m_Def.m_sName), 8);
			return;
		}

		HAM_Cell revealed = HAM_Front.RevealFrontCell("information d'un civil", pos, true);
		if (revealed)
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", string.Format("« Des soldats américains tiennent %1. »", revealed.m_sName), 8);
			return;
		}

		HAM_Net.NotifyPlayer(playerId, "Civil", NothingLeft(), 6);
	}

	//------------------------------------------------------------------------------------------------
	protected static string NothingSeen()
	{
		int roll = Math.RandomInt(0, 6);
		if (roll == 0)
			return "« J'ai rien vu, je vous jure. Je regarde mes pieds et j'avance. »";

		if (roll == 1)
			return "« Des camions, oui, mais j'ai pas levé la tête. J'aime mieux pas savoir. »";

		if (roll == 2)
			return "« Demandez à quelqu'un d'autre. Moi, on m'a déjà trop posé de questions. »";

		if (roll == 3)
			return "« La route était vide. Ou alors j'ai pas voulu voir. C'est pareil. »";

		if (roll == 4)
			return "« J'ai une famille, moi. Je ne répète pas ce que j'entends. »";

		return "« Si j'avais vu quelque chose, je ne serais plus là pour vous le dire. »";
	}

	//------------------------------------------------------------------------------------------------
	protected static string NothingLeft()
	{
		int roll = Math.RandomInt(0, 6);
		if (roll == 0)
			return "« Ceux dont je pouvais parler, vous les avez déjà trouvés. »";

		if (roll == 1)
			return "« Plus loin, je ne sais pas. Ici, il ne reste rien que je puisse vous dire. »";

		if (roll == 2)
			return "« Vous êtes au courant de tout ce que je sais. Le reste, c'est des bruits. »";

		if (roll == 3)
			return "« J'aurais voulu vous aider davantage. Je n'ai plus de nom à vous donner. »";

		if (roll == 4)
			return "« Les soldats sont passés, et puis plus rien. Je ne sais pas où ils sont allés. »";

		return "« Écoutez, je vous dis ce que je peux. Là, je suis au bout. »";
	}

	//------------------------------------------------------------------------------------------------
	static bool HadBanana(IEntity civilian)
	{
		return civilian && s_aBanana && s_aBanana.Contains(civilian);
	}

	//------------------------------------------------------------------------------------------------
	static void GiveBanana(IEntity user, IEntity civilian, int playerId)
	{
		if (!Replication.IsServer() || !user || !HAM_Mission.IsRunning())
			return;

		if (HadBanana(civilian))
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", "Ce civil a déjà reçu une banane.", 5);
			return;
		}

		if (!TakeBanana(user))
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", "Tu n'as pas de banane.", 5);
			return;
		}

		if (civilian)
		{
			if (!s_aBanana)
				s_aBanana = {};

			s_aBanana.Insert(civilian);
		}

		HAM_Mission.AddGlobalRep(HAM_Settings.Get().m_iRepBanana, "banane donnée à un civil");
		HAM_Net.NotifyPlayer(playerId, "Civil", string.Format("Le civil accepte la banane. Réputation : %1.", HAM_Mission.FormatRep(HAM_Mission.s_fGlobalRep)), 6);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TakeBanana(IEntity user)
	{
		SCR_InventoryStorageManagerComponent inventory = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!inventory)
			return false;

		array<IEntity> items = {};
		inventory.GetItems(items, EStoragePurpose.PURPOSE_ANY);
		foreach (IEntity item : items)
		{
			if (!item || !item.GetPrefabData())
				continue;

			string prefab = item.GetPrefabData().GetPrefabName();
			if (!prefab.Contains("Items/Core/ACE_Banana.et"))
				continue;

			return inventory.TryDeleteItem(item);
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void BeginInterrogate(IEntity civilian, int playerId)
	{
		if (!civilian || HAM_Util.IsDead(civilian))
			return;

		CompleteHail(civilian);
		SCR_PlayerController player = HAM_Net.ControllerOf(playerId);
		if (!player)
			return;

		RplId id = HAM_Util.IdOf(civilian);
		HAM_CivilianNeed waiting = NeedOf(civilian);
		if (waiting)
		{
			string asked = string.Format("%1 rations", HAM_Util.FormatInt(waiting.m_fAmount));
			if (waiting.m_eKind == HAM_EResource.WATER)
				asked = string.Format("%1 d'eau", HAM_Util.FormatInt(waiting.m_fAmount));
			else if (waiting.m_eKind == HAM_EResource.AMMO)
				asked = string.Format("%1 munitions", HAM_Util.FormatInt(waiting.m_fAmount));

			HoldCivilian(civilian);
			player.HAM_OpenCivilianTalk(id, true);
			player.HAM_CivilianReply(string.Format("« J'attends toujours %1. »", asked), true, "Donner " + asked);
			return;
		}

		bool payOnly = PayPending(civilian);
		if (Asked(civilian) && !payOnly)
		{
			HAM_Net.NotifyPlayer(playerId, "Civil", "Il n'a plus rien à dire.", 5);
			return;
		}

		if (payOnly)
			HoldCivilian(civilian);
		else
			FreezeForTalk(civilian);

		player.HAM_OpenCivilianTalk(id, payOnly);
		if (payOnly)
		{
			int cost = HAM_Settings.Get().m_iPaySupplies;
			player.HAM_CivilianReply(string.Format("« Je n'ai rien vu, mais je peux vous aider si vous payez. %1 fournitures sur la base la plus proche. »", cost), true, string.Format("Payer %1 fournitures", cost));
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Answer(IEntity civilian, int playerId, int question)
	{
		if (!civilian || Asked(civilian))
			return;

		MarkAsked(civilian);
		float reputation = HAM_Mission.s_fGlobalRep;
		string text = "« Je ne veux pas vous parler. »";
		bool canPay = false;
		vector pos = civilian.GetOrigin();

		bool hold = false;
		string confirm = string.Empty;
		if (question == 1)
			text = AnswerSeen(civilian, pos, reputation, canPay);
		else if (question == 2)
			text = AnswerNeed(civilian, pos, reputation, canPay, confirm);
		else if (question == 3)
			text = AnswerHelp(civilian, pos, reputation, playerId, hold);

		if (hold || canPay)
			HoldCivilian(civilian);
		else
			ReleaseCivilian(civilian);

		if (canPay && confirm.IsEmpty())
			confirm = string.Format("Payer %1 fournitures", HAM_Settings.Get().m_iPaySupplies);

		SCR_PlayerController player = HAM_Net.ControllerOf(playerId);
		if (player)
			player.HAM_CivilianReply(text, canPay, confirm);
	}

	//------------------------------------------------------------------------------------------------
	static void Pay(IEntity civilian, int playerId)
	{
		if (!civilian)
			return;

		HAM_CivilianNeed need = NeedOf(civilian);
		if (need)
		{
			PayNeed(civilian, playerId, need);
			return;
		}

		if (!PayPending(civilian))
			return;

		SCR_PlayerController player = HAM_Net.ControllerOf(playerId);
		SCR_CampaignMilitaryBaseComponent base = HAM_VehicleDepot.SupplyBaseAt(civilian.GetOrigin());
		int cost = HAM_Settings.Get().m_iPaySupplies;
		if (!base || base.GetSupplies() < cost)
		{
			if (player)
				player.HAM_CivilianReply(string.Format("« Il faut %1 fournitures sur la base la plus proche. »", cost), true, string.Format("Payer %1 fournitures", cost));

			return;
		}

		base.AddSupplies(-cost);
		ClearPay(civilian);
		ReleaseCivilian(civilian);
		HAM_Mission.AddGlobalRep(HAM_Settings.Get().m_iRepPay, "paiement d'un civil");
		if (player)
			player.HAM_CivilianReply(SeenText(civilian.GetOrigin()), false, "Fermer");

		RemoveAskAction(civilian);
	}

	//------------------------------------------------------------------------------------------------
	static void OnIedCleared(IEntity mine)
	{
		if (!s_aMineFavors || !mine)
			return;

		foreach (HAM_MineFavor favor : s_aMineFavors)
		{
			if (!favor || favor.m_bReady || !favor.m_aMines)
				continue;

			int index = favor.m_aMines.Find(mine);
			if (index < 0)
				continue;

			DropFavorMine(favor, index);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Une mine sautée disparaît sans passer par le désamorçage : elle compte quand même.
	protected static void SweepMineFavors()
	{
		if (!s_aMineFavors)
			return;

		foreach (HAM_MineFavor favor : s_aMineFavors)
		{
			if (!favor || favor.m_bReady || !favor.m_aMines)
				continue;

			for (int i = favor.m_aMines.Count() - 1; i >= 0; i--)
			{
				IEntity mine = favor.m_aMines[i];
				if (mine && !HAM_Util.IsDead(mine))
					continue;

				if (mine)
					OnIedCleared(mine);
				else
					DropFavorMine(favor, i);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropFavorMine(HAM_MineFavor favor, int index)
	{
		if (!favor || !favor.m_aMines || index < 0 || index >= favor.m_aMines.Count())
			return;

		if (index < favor.m_aTasks.Count() && favor.m_aTasks[index])
			DeleteIedTask(favor.m_aTasks[index]);

		favor.m_aMines.Remove(index);
		if (index < favor.m_aTasks.Count())
			favor.m_aTasks.Remove(index);

		if (!favor.m_aMines.IsEmpty())
			return;

		favor.m_bReady = true;
		if (!favor.m_Civilian || HAM_Util.IsDead(favor.m_Civilian))
			return;

		ClearTalkActions(favor.m_Civilian);
		AttachAction(HAM_Units.CIVILIAN_REPORT_ACTION, favor.m_Civilian, Vector(0, 1.2, 0.15));
		favor.m_ReportTask = CreateReturnTask(favor.m_Civilian);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_Task CreateReturnTask(IEntity civilian)
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem || !civilian)
			return null;

		s_iIedTask++;
		string taskId = "HAM_IED_BACK_" + s_iIedTask.ToString();
		SCR_Task task = taskSystem.CreateTask("{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et", taskId, "Retour au civil", "Les IED sont traités. Retournez parler au civil.", civilian.GetOrigin());
		if (!task)
			return null;

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		return task;
	}

	//------------------------------------------------------------------------------------------------
	static void FinishMineHelp(IEntity civilian, int playerId)
	{
		if (s_aMineFavors && civilian)
		{
			foreach (HAM_MineFavor favor : s_aMineFavors)
			{
				if (!favor || favor.m_Civilian != civilian)
					continue;

				DeleteIedTask(favor.m_ReportTask);
				favor.m_ReportTask = null;
			}
		}

		HAM_Credit.CreditMen(HAM_Mission.HQ_NAME, HAM_Settings.Get().m_iIedFavorMen, "retour des IED");
		HAM_Mission.MarkDirty();
		HAM_Net.NotifyPlayer(playerId, "Civil", "Merci de nous avoir aidez. De nouvelle recrute ce dirige vers la base.", 10);
	}

	//------------------------------------------------------------------------------------------------
	//! 0 refus, 1 parole sans service, 2 palier 1, 3 palier 2, 5 réponse du palier 3. Au plafond, un des trois est tiré.
	protected static int AnswerBand(float reputation)
	{
		HAM_Settings settings = HAM_Settings.Get();
		if (reputation < settings.m_iRepCivilianTalkThreshold)
			return 0;

		int band = 1;
		if (reputation >= settings.m_iRepTier1)
			band = 2;

		if (reputation >= settings.m_iRepTier2)
			band = 3;

		bool ceiling = reputation + 0.001 >= settings.m_iRepMax;
		if (reputation < settings.m_iRepTier3 && !ceiling)
			return band;

		int roll = Math.RandomInt(0, 3);
		if (roll == 0)
			return 2;

		if (roll == 1)
			return 3;

		return 5;
	}

	//------------------------------------------------------------------------------------------------
	protected static string AnswerSeen(IEntity civilian, vector pos, float reputation, out bool canPay)
	{
		canPay = false;
		int band = AnswerBand(reputation);
		if (band == 0)
			return "« Je ne veux pas vous parler. »";

		if (band == 1)
			return "« Je peux vous parler, mais je n'ai rien à vous apprendre. »";

		if (band == 2)
		{
			canPay = true;
			if (!s_aPay)
				s_aPay = {};

			if (!s_aPay.Contains(civilian))
				s_aPay.Insert(civilian);

			return string.Format("« Je n'ai rien vu, mais je pourrais vous aider si vous payez. %1 fournitures. »", HAM_Settings.Get().m_iPaySupplies);
		}

		if (band == 5)
			return SeenText(pos);

		return "« Des troupes ennemies sont dans le coin. »";
	}

	//------------------------------------------------------------------------------------------------
	protected static string SeenText(vector pos)
	{
		HAM_Cell cell = HAM_Mission.CellAt(pos);
		if (!cell || !cell.m_bRevealed)
			return "« Des troupes ennemies sont dans le coin. »";

		string slaveName = SlaveNear(pos);
		if (slaveName.IsEmpty())
			return "« Des troupes ennemies sont dans le coin. »";

		return string.Format("« Des troupes ennemies ont été vues près de %1. »", slaveName);
	}

	//------------------------------------------------------------------------------------------------
	protected static string AnswerNeed(IEntity civilian, vector pos, float reputation, out bool canPay, out string confirm)
	{
		canPay = false;
		confirm = string.Empty;
		int band = AnswerBand(reputation);
		if (band == 0)
			return "« Je n'ai besoin de rien. »";

		if (band == 1)
			return "« Pour l'instant, je n'ai rien à vous demander. »";

		HAM_Settings settings = HAM_Settings.Get();
		int minAmount = settings.m_iNeedMin;
		int maxAmount = settings.m_iNeedMax;
		if (maxAmount < minAmount)
			maxAmount = minAmount;

		HAM_EResource kind = HAM_EResource.FOOD;
		int amount = Math.RandomInt(minAmount, maxAmount + 1);
		string asked = string.Format("%1 rations", amount);
		if (Math.RandomInt(0, 2) == 0)
		{
			kind = HAM_EResource.WATER;
			amount = Math.RandomInt(minAmount, maxAmount + 1);
			asked = string.Format("%1 d'eau", amount);
		}

		RememberNeed(civilian, kind, amount);
		canPay = true;
		confirm = "Donner " + asked;
		if (band == 3)
			return string.Format("« Il me faut %1. Prenez-les sur le stock tenu le plus proche. »", asked);

		if (band == 5)
			return string.Format("« Donnez-moi %1. Je verrai ensuite ce que je peux vous dire. »", asked);

		return string.Format("« J'ai besoin de %1. Elles seront prises sur le stock tenu le plus proche. »", asked);
	}

	//------------------------------------------------------------------------------------------------
	protected static void PayNeed(IEntity civilian, int playerId, HAM_CivilianNeed need)
	{
		if (!need)
			return;

		string baseName;
		HAM_Stock stock = NearestHeldStock(civilian.GetOrigin(), baseName);
		SCR_PlayerController player = HAM_Net.ControllerOf(playerId);
		string asked = string.Format("%1 rations", HAM_Util.FormatInt(need.m_fAmount));
		if (need.m_eKind == HAM_EResource.WATER)
			asked = string.Format("%1 d'eau", HAM_Util.FormatInt(need.m_fAmount));
		else if (need.m_eKind == HAM_EResource.AMMO)
			asked = string.Format("%1 munitions", HAM_Util.FormatInt(need.m_fAmount));

		if (!stock || !stock.Take(need.m_eKind, need.m_fAmount))
		{
			string missing = string.Format("« Il me faut %1 sur %2. Il n'y en a pas assez. »", asked, baseName);
			if (player)
				player.HAM_CivilianReply(missing, true, "Donner " + asked);

			return;
		}

		if (baseName != HAM_Mission.HQ_NAME)
		{
			HAM_Cell paidCell = HAM_Mission.FindCell(baseName);
			if (!paidCell)
			{
				HAM_Deposit paidDeposit = HAM_Mission.FindDeposit(baseName);
				if (paidDeposit)
					paidCell = paidDeposit.m_Cell;
			}

			HAM_Mission.RefreshZoneProps(paidCell);
		}

		HAM_Mission.MarkDirty();
		ClearNeed(civilian);
		ReleaseCivilian(civilian);
		GrantNeedReward(civilian.GetOrigin());
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Mission.AddGlobalRep(settings.m_iRepNeed, "demande d'un civil");
		string extra = "« Je ne sais pas où est leur chef. »";
		int oneIn = settings.m_iNameOneIn;
		if (oneIn < 1)
			oneIn = 1;

		if (Math.RandomInt(0, oneIn) == 0)
		{
			string master = MasterName(civilian.GetOrigin());
			extra = string.Format("« Le maître ennemi, c'est %1. »", master);
		}

		string done = string.Format("%1 Pris sur %2. +%3 fournitures, +%4 hommes.", extra, baseName, settings.m_iNeedSupplies, settings.m_iNeedMen);
		if (player)
			player.HAM_CivilianReply(done, false, "Fermer");

		RemoveAskAction(civilian);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_Stock NearestHeldStock(vector pos, out string baseName)
	{
		baseName = HAM_Mission.HQ_NAME;
		HAM_Stock best = HAM_Mission.StockOf(HAM_Mission.HQ_NAME);
		vector hq = HAM_Mission.BasePos(HAM_Mission.HQ_NAME);
		float bestSq = vector.DistanceSqXZ(pos, hq);
		if (!HAM_Mission.s_aCells)
			return best;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || !cell.IsMasterPlayer() || !cell.m_Stock)
				continue;

			float distSq = vector.DistanceSqXZ(pos, cell.Pos());
			if (distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = cell.m_Stock;
			baseName = cell.m_sName;
		}

		if (HAM_Mission.s_aDeposits)
		{
			foreach (HAM_Deposit deposit : HAM_Mission.s_aDeposits)
			{
				if (!deposit || !deposit.m_Stock || !deposit.m_Def)
					continue;

				HAM_Point held = HAM_Mission.FindPoint(deposit.m_Def.m_sName);
				if (held && !held.m_bPlayer)
					continue;

				float distSq = vector.DistanceSqXZ(pos, deposit.Pos());
				if (distSq >= bestSq)
					continue;

				bestSq = distSq;
				best = deposit.m_Stock;
				baseName = deposit.m_Def.m_sName;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static string AnswerHelp(IEntity civilian, vector pos, float reputation, int playerId, out bool hold)
	{
		hold = false;
		int band = AnswerBand(reputation);
		if (band == 0)
			return "« Vous ne pouvez pas nous aider. »";

		if (band == 1)
			return "« Vous ne pouvez pas encore nous aider. »";

		if (band == 3)
			return "« Je ne peux pas encore vous lancer un service. »";

		if (band == 2)
		{
			IEntity body = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			vector from = pos;
			if (body)
				from = body.GetOrigin();

			if (HAM_SideMissions.OpenFamilyRescue(from))
				return "« Il faut sauver mon mari, ou ma femme. Ils sont retenus tout près. »";

			return "« Il n'y a personne à sauver par ici. »";
		}

		array<IEntity> mines = {};
		if (!HAM_Devices.CollectZoneIeds(pos, mines))
			return "« Je ne vois pas d'IED par ici. »";

		hold = true;
		StartMineFavor(civilian, mines);
		return "« Des IED ont été posés ici. Enlevez-les, puis revenez me parler. »";
	}

	//------------------------------------------------------------------------------------------------
	protected static void StartMineFavor(IEntity civilian, notnull array<IEntity> mines)
	{
		if (!s_aMineFavors)
			s_aMineFavors = {};

		foreach (HAM_MineFavor existing : s_aMineFavors)
		{
			if (existing && existing.m_Civilian == civilian && !existing.m_bReady)
				return;
		}

		HAM_MineFavor favor = new HAM_MineFavor();
		favor.m_Civilian = civilian;
		foreach (IEntity mine : mines)
		{
			if (!mine)
				continue;

			favor.m_aMines.Insert(mine);
			favor.m_aTasks.Insert(CreateIedMarker(mine));
		}

		if (favor.m_aMines.IsEmpty())
			return;

		s_aMineFavors.Insert(favor);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_Task CreateIedMarker(IEntity mine)
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem || !mine)
			return null;

		s_iIedTask++;
		string taskId = "HAM_IED_" + s_iIedTask.ToString();
		SCR_Task task = taskSystem.CreateTask("{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et", taskId, "IED", "Désamorcez cet IED.", mine.GetOrigin());
		if (!task)
			return null;

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		return task;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteIedTask(SCR_Task task)
	{
		if (!Replication.IsServer() || !task)
			return;

		task.SetTaskState(SCR_ETaskState.COMPLETED);
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (taskSystem)
			taskSystem.UnregisterTask(task);

		RplComponent replication = RplComponent.Cast(task.FindComponent(RplComponent));
		if (replication)
			replication.DeleteRplEntity(task, false);
		else
			SCR_EntityHelper.DeleteEntityAndChildren(task);
	}

	//------------------------------------------------------------------------------------------------
	//! Le dialogue est fermé sans demande en cours : le civil reprend sa patrouille.
	static void OnTalkClosed(IEntity civilian)
	{
		if (!civilian)
			return;

		if (RoadTalker(civilian))
		{
			if (CivilianStays(civilian))
				return;

			ReleaseCivilian(civilian);
			return;
		}

		if (CivilianStays(civilian))
			return;

		ReleaseCivilian(civilian);
	}

	//------------------------------------------------------------------------------------------------
	protected static void HoldCivilian(IEntity civilian)
	{
		if (!civilian)
			return;

		FreezeForTalk(civilian);
		if (!s_aStay)
			s_aStay = {};

		if (s_aStay.Find(civilian) < 0)
			s_aStay.Insert(civilian);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CivilianStays(IEntity civilian)
	{
		return civilian && s_aStay && s_aStay.Find(civilian) >= 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static void FreezeForTalk(IEntity civilian)
	{
		if (!civilian)
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(civilian.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetCanMove(false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AllowPatrolMove(IEntity civilian)
	{
		if (!civilian || CivilianStays(civilian))
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(civilian.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetCanMove(true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReleaseCivilian(IEntity civilian)
	{
		if (!civilian)
			return;

		if (s_aStay)
			s_aStay.RemoveItem(civilian);

		AllowPatrolMove(civilian);
		ReleaseRoadCar(civilian);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool RoadTalker(IEntity civilian)
	{
		if (!civilian || !s_aRoadCars)
			return false;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (car && car.m_bOnFoot && car.m_Talker == civilian)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! L'équipage peut remonter : le groupe reprend la route au prochain passage.
	protected static void ReleaseRoadCar(IEntity civilian)
	{
		if (!civilian || !s_aRoadCars)
			return;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (!car || !car.m_bOnFoot || car.m_Talker != civilian)
				continue;

			car.m_bOnFoot = false;
			car.m_bTaskDone = true;
			if (car.m_Task)
				car.m_Task.SetTaskState(SCR_ETaskState.COMPLETED);

			car.m_Task = null;
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Tâche routière : l'équipage est à pied. Le trafic sans tâche continue d'être rassis.
	static bool RoadCrewOnFoot(Vehicle vehicle)
	{
		if (!vehicle || !s_aRoadCars)
			return false;

		foreach (HAM_RoadCar car : s_aRoadCars)
		{
			if (car && car.m_bOnFoot && car.m_Vehicle == vehicle)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void GrantNeedReward(vector pos)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Credit.CreditConflict(HAM_Mission.DesignatedHq(), settings.m_iNeedSupplies, "demande d'un civil");
		HAM_Cell cell = HAM_Mission.CellAt(pos);
		string menBase = HAM_Mission.HQ_NAME;
		if (cell && cell.IsMasterPlayer())
			menBase = cell.m_sName;

		HAM_Credit.CreditMen(menBase, settings.m_iNeedMen, "demande d'un civil");
		if (cell)
			HAM_Mission.RefreshZoneProps(cell);
	}

	//------------------------------------------------------------------------------------------------
	protected static string SlaveNear(vector pos)
	{
		HAM_Cell cell = HAM_Mission.CellAt(pos);
		if (!cell || cell.m_aSlaves.IsEmpty())
			return string.Empty;

		HAM_Point best;
		float bestSq = -1;
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			float distSq = vector.DistanceSqXZ(pos, slave.Pos());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = slave;
		}

		if (!best)
			return string.Empty;

		return best.m_Def.m_sName;
	}

	//------------------------------------------------------------------------------------------------
	protected static string MasterName(vector pos)
	{
		HAM_Cell nearest;
		float bestSq = -1;
		foreach (HAM_Cell other : HAM_Mission.s_aCells)
		{
			if (!other || other.m_bRevealed || other.IsMasterPlayer() || !HAM_Mission.IsFront(other))
				continue;

			float distSq = vector.DistanceSqXZ(pos, other.Pos());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			nearest = other;
		}

		if (!nearest)
			return "aucune zone de la ligne de front";

		return nearest.m_sName;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Asked(IEntity civilian)
	{
		return s_aAsked && s_aAsked.Contains(civilian);
	}

	protected static void MarkAsked(IEntity civilian)
	{
		if (!s_aAsked)
			s_aAsked = {};

		if (!s_aAsked.Contains(civilian))
			s_aAsked.Insert(civilian);
	}

	protected static void RememberNeed(IEntity civilian, HAM_EResource kind, float amount)
	{
		if (!s_aNeeds)
			s_aNeeds = {};

		HAM_CivilianNeed need = NeedOf(civilian);
		if (!need)
		{
			need = new HAM_CivilianNeed();
			need.m_Civilian = civilian;
			s_aNeeds.Insert(need);
		}

		need.m_eKind = kind;
		need.m_fAmount = amount;
		if (!need.m_Task)
			need.m_Task = CreateNeedTask(civilian, kind, amount);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_Task CreateNeedTask(IEntity civilian, HAM_EResource kind, float amount)
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem || !civilian)
			return null;

		s_iNeedTask++;
		string taskId = "HAM_NEED_" + s_iNeedTask.ToString();
		string title = "Vivres pour un civil";
		string detail = string.Format("Donnez %1 rations à ce civil.", HAM_Util.FormatInt(amount));
		if (kind == HAM_EResource.WATER)
		{
			title = "Eau pour un civil";
			detail = string.Format("Donnez %1 d'eau à ce civil.", HAM_Util.FormatInt(amount));
		}
		else if (kind == HAM_EResource.AMMO)
		{
			title = "Munitions pour un civil";
			detail = string.Format("Donnez %1 munitions à ce civil.", HAM_Util.FormatInt(amount));
		}

		detail = detail + " Elles sont prises sur le QG ou la base joueur la plus proche quand vous les donnez.";
		SCR_Task task = taskSystem.CreateTask("{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et", taskId, title, detail, civilian.GetOrigin());
		if (!task)
			return null;

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		return task;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteNeedTask(SCR_Task task)
	{
		if (!Replication.IsServer() || !task)
			return;

		task.SetTaskState(SCR_ETaskState.COMPLETED);
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (taskSystem)
			taskSystem.UnregisterTask(task);

		RplComponent replication = RplComponent.Cast(task.FindComponent(RplComponent));
		if (replication)
			replication.DeleteRplEntity(task, false);
		else
			SCR_EntityHelper.DeleteEntityAndChildren(task);
	}

	protected static HAM_CivilianNeed NeedOf(IEntity civilian)
	{
		if (!s_aNeeds || !civilian)
			return null;

		foreach (HAM_CivilianNeed need : s_aNeeds)
		{
			if (need && need.m_Civilian == civilian)
				return need;
		}

		return null;
	}

	protected static void ClearNeed(IEntity civilian)
	{
		if (!s_aNeeds)
			return;

		for (int i = s_aNeeds.Count() - 1; i >= 0; i--)
		{
			if (!s_aNeeds[i] || s_aNeeds[i].m_Civilian != civilian)
				continue;

			DeleteNeedTask(s_aNeeds[i].m_Task);
			s_aNeeds[i].m_Task = null;
			s_aNeeds.Remove(i);
		}
	}

	protected static bool PayPending(IEntity civilian)
	{
		return s_aPay && s_aPay.Contains(civilian);
	}

	protected static void ClearPay(IEntity civilian)
	{
		if (s_aPay)
			s_aPay.RemoveItem(civilian);
	}

	protected static void RemoveAskAction(IEntity civilian)
	{
		if (!civilian)
			return;

		IEntity child = civilian.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			if (!child.GetPrefabData())
			{
				child = next;
				continue;
			}

			string prefab = child.GetPrefabData().GetPrefabName();
			if (prefab.Contains("HAM_CivilianAskAction"))
				HAM_Util.Delete(child);

			child = next;
		}
	}
}
