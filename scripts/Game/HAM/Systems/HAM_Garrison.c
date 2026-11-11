//------------------------------------------------------------------------------------------------
// IA du front : garnisons ennemies des points et dépôts, patrouilles, CQB, renforts,
// garnisons amies des maîtres tenus (plafonnées par le stock d'hommes).
// Tout naît à l'approche d'un joueur et disparaît quand plus personne n'est autour.
//------------------------------------------------------------------------------------------------

//! Véhicule dont l'équipage vanilla est en cours de création ; ses ordres sont donnés quand il est prêt.
class HAM_PendingCrew
{
	Vehicle m_Vehicle;
	ResourceName m_sWaypoint;
	ref array<vector> m_aRoute = {};
	float m_fRadius;
	array<SCR_AIGroup> m_aInto;
	bool m_bEnemy;
	float m_fSince;
	float m_fSeatedSince;
	int m_iSpawnTries;
	int m_iCrewSeats;
	SCR_AIGroup m_Crew;
	SCR_AIGroup m_CrewExtra;
	bool m_bAir;
}

//! Équage déjà créé : on le garde éveillé, et une patrouille dont le waypoint est fini en reprend un.
class HAM_LiveCrew
{
	SCR_AIGroup m_Group;
	Vehicle m_Vehicle;
	bool m_bPatrol;
	bool m_bEnemy;
	bool m_bAir;
	bool m_bFlightCut;
	bool m_bCivilRoad;
	bool m_bHavePos;
	vector m_vLastPos;
	float m_fRadius;
	float m_fOutSince;
	ResourceName m_sWaypoint;
	ref array<vector> m_aRoute = {};
}

//! Un homme de CQB : sa maison et sa place à l'intérieur.
class HAM_CqbActor
{
	SCR_AIGroup m_Group;
	SCR_DestructibleBuildingEntity m_House;
	vector m_vPos;
}

//! Groupe CQB d'origine d'un bâtiment, en attente d'être scindé en hommes seuls.
class HAM_CqbSeed
{
	SCR_AIGroup m_Group;
	SCR_DestructibleBuildingEntity m_House;
	ref array<vector> m_aSpots;
	ref array<SCR_AIGroup> m_aInto;
	int m_iRetries = 20;
}

//! Une apparition différée : groupe, maison CQB, patrouille ou véhicule. 450 ms entre chaque.
class HAM_SpawnStep
{
	int m_iKind;
	ResourceName m_Prefab;
	vector m_vCenter;
	float m_fRadius;
	int m_iSize;
	int m_iWanted;
	ref array<SCR_AIGroup> m_aGroups;
	HAM_Point m_Point;
	HAM_Line m_Line;
	SCR_DestructibleBuildingEntity m_House;
	string m_sName;
}

class HAM_Garrison
{
	//! Seuils de CPR_BuildingSize du Conflit : surface au sol × étages, en m².
	static const float CQB_MEDIUM_SPACE = 18;
	static const float CQB_LARGE_SPACE = 40;
	//! Délai pour monter dans un véhicule ennemi avant d'abandonner l'équipage.
	static const float CREW_BOARD_SECONDS = 45;
	//! Rayon de recherche des bâtiments CQB autour d'un point, comme autour d'une base du Conflit.
	static const float ZONE_CQB_RADIUS = 220;
	static const float ZONE_PATROL_RADIUS = 250;

	protected static ref array<SCR_AIGroup> s_aEnemyGroups;
	protected static ref array<ref HAM_PendingCrew> s_aPendingCrews;
	protected static ref array<ref HAM_LiveCrew> s_aLiveCrews;
	protected static ref map<IEntity, string> s_mFriendlyBodies;
	protected static int s_iActivePatrols;
	protected static int s_iVehiclePatrols;
	protected static ref array<ref HAM_CqbActor> s_aCqbActors;
	protected static ref array<ref HAM_CqbSeed> s_aCqbSeeds;
	protected static ref array<ref HAM_SpawnStep> s_aSpawnSteps;
	protected static bool s_bSpawnDrain;
	protected static ref map<IEntity, bool> s_mScriptVehicles;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_mScriptVehicles = null;
		s_aEnemyGroups = null;
		s_aPendingCrews = null;
		s_aLiveCrews = null;
		s_mFriendlyBodies = null;
		s_iActivePatrols = 0;
		s_iVehiclePatrols = 0;
		s_aCqbActors = null;
		s_aCqbSeeds = null;
		s_aSpawnSteps = null;
		s_bSpawnDrain = false;
		HAM_Helicopter.ResetSession();
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aEnemyGroups)
			s_aEnemyGroups = {};

		if (!s_aCqbActors)
			s_aCqbActors = {};

		if (!s_aCqbSeeds)
			s_aCqbSeeds = {};

		if (!s_aPendingCrews)
			s_aPendingCrews = {};

		if (!s_aLiveCrews)
			s_aLiveCrews = {};

		if (!s_mFriendlyBodies)
			s_mFriendlyBodies = new map<IEntity, string>();

		if (!s_mScriptVehicles)
			s_mScriptVehicles = new map<IEntity, bool>();
	}

	//------------------------------------------------------------------------------------------------
	//! Véhicule posé par H&M (patrouille, contre-attaque, convoi) : l'atelier n'y met pas de cargaison.
	static bool IsScriptVehicle(IEntity vehicle)
	{
		return vehicle && s_mScriptVehicles && s_mScriptVehicles.Contains(vehicle);
	}

	//------------------------------------------------------------------------------------------------
	static void RegisterEnemy(SCR_AIGroup group)
	{
		Ensure();
		if (group && s_aEnemyGroups.Find(group) < 0)
			s_aEnemyGroups.Insert(group);
	}

	//------------------------------------------------------------------------------------------------
	//! Un ennemi vivant de nos groupes (garnisons, patrouilles, contre-attaques, missions) dans le rayon.
	static bool EnemyAliveNear(vector pos, float radius)
	{
		Ensure();
		return HAM_Util.AnyAliveNear(s_aEnemyGroups, pos, radius);
	}

	//------------------------------------------------------------------------------------------------
	static bool FriendlyAliveNear(HAM_Cell cell, vector pos, float radius)
	{
		if (!cell)
			return false;

		return HAM_Util.AnyAliveNear(cell.m_aFriendly, pos, radius);
	}

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		Ensure();
		HAM_Settings settings = HAM_Settings.Get();
		array<IEntity> bodies = HAM_Mission.s_aPlayerBodies;

		for (int i = s_aEnemyGroups.Count() - 1; i >= 0; i--)
		{
			if (!s_aEnemyGroups[i])
				s_aEnemyGroups.Remove(i);
		}

		s_iActivePatrols = 0;
		s_iVehiclePatrols = 0;
		foreach (HAM_Line countLine : HAM_Mission.s_aLines)
		{
			if (countLine.m_Def.m_eRole == HAM_ERole.PATROL)
			{
				HAM_Util.PruneGroups(countLine.m_aGroups);
				s_iActivePatrols += countLine.m_aGroups.Count();
			}
		}

		foreach (HAM_Point countPoint : HAM_Mission.s_aPoints)
		{
			HAM_Util.PruneGroups(countPoint.m_aPatrols);
			s_iActivePatrols += countPoint.m_aPatrols.Count();
			s_iVehiclePatrols += ActiveVehicles(countPoint);
		}

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			TickPoint(point, bodies, settings);
			TickZone(point, bodies, settings);
		}

		foreach (HAM_Deposit deposit : HAM_Mission.s_aDeposits)
		{
			TickDeposit(deposit, bodies, settings);
		}

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (line.m_Def.m_eRole == HAM_ERole.PATROL)
				TickPatrol(line, bodies, settings);
			else if (line.m_Def.m_eRole == HAM_ERole.CQB)
				TickCqb(line, bodies, settings);
		}

		s_mFriendlyBodies.Clear();
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			TickFriendly(cell, bodies, settings);
		}

		KeepCqbInside();
		TickPendingCrews();
		TickLiveCrews();
		HAM_Helicopter.Tick(bodies);
	}

	//------------------------------------------------------------------------------------------------
	//! Une cellule fait naître des ennemis tant qu'elle est sur la ligne de front et pas entière.
	protected static bool CellSpawnsEnemies(HAM_Cell cell)
	{
		return cell && !cell.IsComplete() && HAM_Mission.IsFront(cell);
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickPoint(HAM_Point point, array<IEntity> bodies, HAM_Settings settings)
	{
		float nearest = HAM_Util.NearestDistance(bodies, point.Pos());
		float activation = settings.ActivationOf(point.m_Def);
		float despawn = settings.DespawnOf(point.m_Def);

		if (point.m_bPlayer || !CellSpawnsEnemies(point.m_Cell))
		{
			if (!point.m_aGroups.IsEmpty() && nearest > despawn)
			{
				HAM_Util.DeleteGroups(point.m_aGroups);
				point.m_bActive = false;
			}

			return;
		}

		// Une seule vague. Un groupe tué ou un compteur incomplet ne relance rien.
		if (!point.m_bGarrisonSpawned && nearest <= activation)
		{
			point.m_bGarrisonSpawned = true;
			point.m_bActive = true;
			int groups = settings.PatrolUnitsOf(point.m_Def);
			int spawned = SpawnEnemyUnits(point.m_Def, point.Pos(), point.Radius(), groups, point.m_aGroups);
			HAM_Util.Info(string.Format("Garnison activée : %1, %2 hommes en %3 groupes", point.m_Def.m_sName, spawned, point.m_aGroups.Count()));
			return;
		}

		if (point.m_bActive && nearest > despawn)
		{
			int alive = HAM_Util.AliveCountAll(point.m_aGroups);
			HAM_Util.DeleteGroups(point.m_aGroups);
			point.m_bActive = false;
			if (alive < 1)
			{
				point.m_iEnemyLeft = 0;
			}
			else
			{
				point.m_bGarrisonSpawned = false;
				point.m_iEnemyLeft = -1;
			}

			HAM_Util.Debug(string.Format("Garnison en sommeil : %1", point.m_Def.m_sName));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickDeposit(HAM_Deposit deposit, array<IEntity> bodies, HAM_Settings settings)
	{
		if (deposit.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
			return;

		float nearest = HAM_Util.NearestDistance(bodies, deposit.Pos());
		float despawn = settings.DespawnOf(deposit.m_Def);
		if (!CellSpawnsEnemies(deposit.m_Cell))
		{
			if (!deposit.m_aGroups.IsEmpty() && nearest > despawn)
			{
				HAM_Util.DeleteGroups(deposit.m_aGroups);
				deposit.m_bActive = false;
			}

			return;
		}

		if (!deposit.m_bSpawnedOnce && nearest <= settings.ActivationOf(deposit.m_Def))
		{
			deposit.m_bSpawnedOnce = true;
			deposit.m_bActive = true;
			int guards = settings.PatrolUnitsOf(deposit.m_Def);
			int guardMen = SpawnEnemyUnits(deposit.m_Def, deposit.Pos(), 40, guards, deposit.m_aGroups);
			HAM_Util.Debug(string.Format("Garde du dépôt activée : %1, %2 hommes en %3 groupes", deposit.m_Def.m_sName, guardMen, guards));
			return;
		}

		if (deposit.m_bActive && nearest > despawn)
		{
			int alive = HAM_Util.AliveCountAll(deposit.m_aGroups);
			HAM_Util.DeleteGroups(deposit.m_aGroups);
			deposit.m_bActive = false;
			if (alive < 1)
			{
				deposit.m_iEnemyLeft = 0;
			}
			else
			{
				deposit.m_bSpawnedOnce = false;
				deposit.m_iEnemyLeft = -1;
			}

			HAM_Util.Debug(string.Format("Garde du dépôt en sommeil : %1", deposit.m_Def.m_sName));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Autant de groupes entiers que patrolUnits. Pas de plafond d'hommes.
	static int SpawnEnemyUnits(HAM_MapPoint def, vector center, float radius, int groups, notnull array<SCR_AIGroup> into)
	{
		if (!def || groups < 1)
			return 0;

		int spawned = 0;
		for (int i = 0; i < groups; i++)
		{
			ResourceName prefab = HAM_Units.ZoneFootGroup(def);
			if (prefab.IsEmpty())
				break;

			vector pos = HAM_Util.DryAround(center, 5, Math.Max(15, radius * 0.5));
			SCR_AIGroup group = HAM_Util.SpawnFullGroup(prefab, pos, 0);
			if (!group)
				continue;

			int size = group.GetNumberOfMembersToSpawn();
			if (size < 1)
				size = 1;

			spawned += size;
			HAM_Util.Order(group, HAM_Units.WP_DEFEND, center, Math.Max(25, radius * 0.6));
			into.Insert(group);
			RegisterEnemy(group);
		}

		return spawned;
	}

	//------------------------------------------------------------------------------------------------
	protected static void EnqueueSpawn(HAM_SpawnStep step)
	{
		if (!s_aSpawnSteps)
			s_aSpawnSteps = {};

		s_aSpawnSteps.Insert(step);
		if (s_bSpawnDrain)
			return;

		s_bSpawnDrain = true;
		GetGame().GetCallqueue().CallLater(DrainSpawn, 450, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DrainSpawn()
	{
		if (!s_aSpawnSteps || s_aSpawnSteps.IsEmpty() || !HAM_Mission.IsRunning())
		{
			s_bSpawnDrain = false;
			s_aSpawnSteps = null;
			return;
		}

		HAM_SpawnStep step = s_aSpawnSteps[0];
		s_aSpawnSteps.Remove(0);
		RunSpawnStep(step);
		if (!s_aSpawnSteps.IsEmpty())
			GetGame().GetCallqueue().CallLater(DrainSpawn, 450, false);
		else
			s_bSpawnDrain = false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RunSpawnStep(HAM_SpawnStep step)
	{
		if (!step)
			return;

		if (step.m_iKind == 1)
		{
			vector pos = HAM_Util.DryAround(step.m_vCenter, 5, Math.Max(15, step.m_fRadius * 0.5));
			SCR_AIGroup group = HAM_Util.SpawnFullGroup(step.m_Prefab, pos, 0);
			if (!group || !step.m_aGroups)
				return;

			HAM_Util.Order(group, HAM_Units.WP_DEFEND, step.m_vCenter, Math.Max(25, step.m_fRadius * 0.6));
			step.m_aGroups.Insert(group);
			RegisterEnemy(group);
			return;
		}

		if (step.m_iKind == 2)
		{
			SpawnQueuedHouse(step);
			return;
		}

		if (step.m_iKind == 3 && step.m_Line)
		{
			vector start = HAM_Util.DryAround(step.m_Line.Pos(), 0, step.m_Line.m_Def.m_fRadius);
			if (!ClearOfHeldZones(start))
				return;

			SCR_AIGroup patrol = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_RIFLE, start, 0);
			if (!patrol)
				return;

			step.m_Line.m_aGroups.Insert(patrol);
			RegisterEnemy(patrol);
			GivePatrolRoute(patrol, step.m_Line);
			s_iActivePatrols++;
			return;
		}

		if (step.m_iKind == 4 && step.m_Point)
		{
			HAM_Settings settings = HAM_Settings.Get();
			if (s_iVehiclePatrols >= settings.m_iVehiclePatrolMax || ActiveVehicles(step.m_Point) >= settings.VehiclesOf(step.m_Point.m_Def))
				return;

			if (SpawnZoneVehicle(step.m_Point, step.m_fRadius, settings))
				s_iVehiclePatrols++;

			return;
		}

		if (step.m_iKind == 6 && step.m_Point)
		{
			SpawnMasterArmor(step.m_Point);
			return;
		}

		if (step.m_iKind == 5 && step.m_Point)
		{
			HAM_Settings settings = HAM_Settings.Get();
			if (s_iActivePatrols >= settings.m_iPatrolMaxActive || step.m_Point.m_aPatrols.Count() >= settings.PatrolUnitsOf(step.m_Point.m_Def))
				return;

			vector start = HAM_Util.DryAround(step.m_Point.Pos(), step.m_Point.Radius() * 0.5, step.m_fRadius);
			if (!ClearOfHeldZones(start))
				return;

			SCR_AIGroup patrol = HAM_Util.SpawnFullGroup(HAM_Units.ZoneFootGroup(step.m_Point.m_Def), start, 0);
			if (!patrol)
				return;

			step.m_Point.m_aPatrols.Insert(patrol);
			RegisterEnemy(patrol);
			GiveRoute(patrol, step.m_Point.Pos(), step.m_fRadius);
			s_iActivePatrols++;
			HAM_Util.Info(string.Format("Patrouille à pied : %1, %2 hommes (%3 sur la carte)", step.m_Point.m_Def.m_sName, patrol.GetNumberOfMembersToSpawn(), s_iActivePatrols));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickPatrol(HAM_Line line, array<IEntity> bodies, HAM_Settings settings)
	{
		float nearest = HAM_Util.NearestDistance(bodies, line.Pos());
		if (HAM_Mission.SlaveHolds(line.m_Cell, line.Pos()))
		{
			if (!line.m_aGroups.IsEmpty())
				HAM_Util.DeleteGroups(line.m_aGroups);

			ReleaseLineVehicles(line);
			line.m_bActive = false;
			line.m_bSpawnedOnce = true;
			return;
		}

		if (!ClearOfHeldZones(line.Pos()))
		{
			if (!line.m_aGroups.IsEmpty())
				HAM_Util.DeleteGroups(line.m_aGroups);

			ReleaseLineVehicles(line);
			line.m_bActive = false;
			line.m_bSpawnedOnce = false;
			return;
		}

		bool spawns = CellSpawnsEnemies(line.m_Cell);
		if (!spawns || nearest > settings.DespawnOf(line.m_Def))
		{
			bool cleaned = LineZoneCleaned(line);
			if (!line.m_aGroups.IsEmpty())
			{
				int alive = HAM_Util.AliveCountAll(line.m_aGroups);
				HAM_Util.DeleteGroups(line.m_aGroups);
				if (!cleaned && alive > 0)
					line.m_bSpawnedOnce = false;
			}

			ReleaseLineVehicles(line);

			if (cleaned)
				line.m_bSpawnedOnce = true;

			line.m_bActive = false;
			return;
		}

		KeepPatrolsMoving(line);

		if (nearest > settings.ActivationOf(line.m_Def))
			return;

		if (line.m_bSpawnedOnce || !settings.m_bPatrolsEnabled)
			return;

		line.m_bSpawnedOnce = true;
		line.m_bActive = true;
		vector start = HAM_Util.DryAround(line.Pos(), 0, line.m_Def.m_fRadius);
		if (Math.RandomFloat01() < settings.m_fPatrolVehicleShare)
		{
			ResourceName vehiclePrefab = HAM_Units.EnemyPatrolVehicle();
			bool vehiclePlaced = false;
			if (!vehiclePrefab.IsEmpty())
			{
				for (int attempt = 0; attempt < 12 && !vehiclePlaced; attempt++)
				{
					vector roadStart;
					vector sample = HAM_Util.RandomAround(line.Pos(), 0, line.m_Def.m_fRadius);
					if (!HAM_Util.RoadPoint(sample, 150, roadStart) || HAM_Util.IsNearWater(roadStart))
						continue;

					if (!VehicleClearOfPlayers(roadStart) || !ClearOfHeldZones(roadStart))
						continue;

					array<vector> route = {};
					for (int r = 0; r < 4; r++)
					{
						vector roadPoint;
						if (HAM_Util.RoadPoint(HAM_Util.RandomAround(line.Pos(), 0, line.m_Def.m_fRadius), 200, roadPoint) && !HAM_Util.IsWater(roadPoint))
							route.Insert(roadPoint);
					}

					Vehicle patrolVehicle = SpawnCrewedVehicle(vehiclePrefab, roadStart, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_PATROL, 30, line.m_aGroups, true, ResourceName.Empty);
					if (!patrolVehicle)
						continue;

					if (!line.m_aVehicles)
						line.m_aVehicles = {};

					line.m_aVehicles.Insert(patrolVehicle);
					s_iActivePatrols++;
					vehiclePlaced = true;
					HAM_Util.Debug(string.Format("Patrouille motorisée : %1 (%2 actives)", line.m_Def.m_sName, s_iActivePatrols));
				}
			}

			if (vehiclePlaced)
				return;
		}

		int groups = settings.PatrolUnitsOf(line.m_Def);
		for (int i = 0; i < groups; i++)
		{
			if (s_iActivePatrols >= settings.m_iPatrolMaxActive)
				break;

			vector spot = HAM_Util.DryAround(line.Pos(), 0, line.m_Def.m_fRadius);
			if (!ClearOfHeldZones(spot))
				continue;

			SCR_AIGroup group = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_RIFLE, spot, 0);
			if (!group)
				continue;

			line.m_aGroups.Insert(group);
			RegisterEnemy(group);
			GivePatrolRoute(group, line);
			s_iActivePatrols++;
			HAM_Util.Debug(string.Format("Patrouille à pied : %1, %2 hommes (%3 actives)", line.m_Def.m_sName, group.GetNumberOfMembersToSpawn(), s_iActivePatrols));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void GivePatrolRoute(SCR_AIGroup group, HAM_Line line)
	{
		if (!group || !line)
			return;

		HAM_Util.KeepAwake(group);
		if (IsLiveCrew(group))
			return;

		for (int i = 0; i < 4; i++)
		{
			vector spot = HAM_Util.DryAround(line.Pos(), 0, line.m_Def.m_fRadius);
			HAM_Util.SnapToNavmesh(group, spot);
			HAM_Util.Order(group, HAM_Units.WP_PATROL, spot, 15);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Les groupes déjà dehors continuent, même si le joueur n'est plus dans le rayon d'activation.
	protected static void KeepPatrolsMoving(HAM_Line line)
	{
		if (!line)
			return;

		foreach (SCR_AIGroup patrol : line.m_aGroups)
		{
			if (!patrol)
				continue;

			HAM_Util.KeepAwake(patrol);
			if (GroupThreatened(patrol))
			{
				HAM_Util.ClearWaypoints(patrol);
				continue;
			}

			array<AIWaypoint> waypoints = {};
			if (patrol.GetWaypoints(waypoints) == 0)
				GivePatrolRoute(patrol, line);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickCqb(HAM_Line line, array<IEntity> bodies, HAM_Settings settings)
	{
		float nearest = HAM_Util.NearestDistance(bodies, line.Pos());
		float activation = Math.Min(settings.ActivationOf(line.m_Def), line.m_Def.m_fRadius + 350);
		if (HAM_Mission.SlaveHolds(line.m_Cell, line.Pos()))
		{
			HAM_Util.DeleteGroups(line.m_aGroups);
			line.m_bActive = false;
			line.m_bSpawnedOnce = true;
			return;
		}

		if (line.m_bActive && nearest > activation + 300)
		{
			if (!line.m_aGroups.IsEmpty())
			{
				int alive = HAM_Util.AliveCountAll(line.m_aGroups);
				HAM_Util.DeleteGroups(line.m_aGroups);
				if (alive > 0)
					line.m_bSpawnedOnce = false;
			}

			line.m_bActive = false;
			return;
		}

		if (line.m_bSpawnedOnce || line.m_bActive || !settings.m_bCqbEnabled || !CellSpawnsEnemies(line.m_Cell) || nearest > activation)
			return;

		line.m_bSpawnedOnce = true;
		line.m_bActive = true;
		int houses;
		int men = PlaceCqb(line.Pos(), line.m_Def.m_fRadius, settings.BuildingsOf(line.m_Def), line.m_aGroups, houses);
		HAM_Util.Info(string.Format("CQB : ligne %1, %2 hommes dans %3 bâtiments", line.m_Def.m_sName, men, houses));
	}

	//------------------------------------------------------------------------------------------------
	//! Comme le Conflit : jusqu'à maxBuildings bâtiments du rayon, 1–2, 2–4 ou 4–6 hommes selon la taille,
	//! un groupe d'un homme par place à l'intérieur, sur l'étage trouvé. Rend le nombre d'hommes.
	protected static int PlaceCqb(vector center, float radius, int maxBuildings, notnull array<SCR_AIGroup> into, out int houses)
	{
		houses = 0;
		array<SCR_DestructibleBuildingEntity> buildings = {};
		HAM_Interior.CollectBuildings(center, radius, buildings);
		int men = 0;
		while (houses < maxBuildings && !buildings.IsEmpty())
		{
			int index = buildings.GetRandomIndex();
			SCR_DestructibleBuildingEntity house = buildings[index];
			buildings.Remove(index);
			if (!CqbHouseFits(house))
				continue;

			vector mins;
			vector maxs;
			house.GetBounds(mins, maxs);
			if (HAM_CQBInterior.IsTooSmall(house, mins, maxs))
				continue;

			int wanted = CqbRollUnits(CqbBuildingSize(mins, maxs));
			HAM_SpawnStep step = new HAM_SpawnStep();
			step.m_iKind = 2;
			step.m_House = house;
			step.m_iWanted = wanted;
			step.m_aGroups = into;
			EnqueueSpawn(step);
			houses++;
			men += wanted;
		}

		return men;
	}

	//------------------------------------------------------------------------------------------------
	//! CPR_BuildingSize du Conflit : 1 petit, 2 moyen, 3 grand.
	protected static int CqbBuildingSize(vector mins, vector maxs)
	{
		float area = Math.AbsFloat(maxs[0] - mins[0]) * Math.AbsFloat(maxs[2] - mins[2]);
		float floors = Math.Max(1, Math.Floor((Math.AbsFloat(maxs[1] - mins[1]) - 1.5) / 3.5));
		float space = area * floors;
		if (space >= CQB_LARGE_SPACE)
			return 3;

		if (space >= CQB_MEDIUM_SPACE)
			return 2;

		return 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Petit bâtiment : 1 homme. Les autres : 3.
	protected static int CqbRollUnits(int size)
	{
		if (size <= 1)
			return 1;

		return 3;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SpawnQueuedHouse(HAM_SpawnStep step)
	{
		if (!step || !step.m_House || !step.m_aGroups)
			return;

		vector mins;
		vector maxs;
		step.m_House.GetBounds(mins, maxs);
		array<vector> spots = {};
		if (!CollectCqbSpots(step.m_House, mins, maxs, step.m_iWanted, spots))
			return;

		SCR_AIGroup seed = SpawnCqbSeed(spots.Count(), spots[0]);
		if (!seed)
			return;

		step.m_aGroups.Insert(seed);
		if (!s_aCqbSeeds)
			s_aCqbSeeds = {};

		HAM_CqbSeed pending = new HAM_CqbSeed();
		pending.m_Group = seed;
		pending.m_House = step.m_House;
		pending.m_aSpots = spots;
		pending.m_aInto = step.m_aGroups;
		s_aCqbSeeds.Insert(pending);
		GetGame().GetCallqueue().CallLater(SplitCqbSeed, 2000, false, pending);
	}

	//------------------------------------------------------------------------------------------------
	//! Points intérieurs distincts du bâtiment, étage par étage, comme CPR_CollectSpreadSpots du Conflit.
	protected static bool CollectCqbSpots(SCR_DestructibleBuildingEntity house, vector mins, vector maxs, int wanted, notnull array<vector> spots)
	{
		array<float> floors = {};
		if (HAM_CQBInterior.FillCoreSpots(house, mins, maxs, wanted, spots) && !spots.IsEmpty())
			return true;

		HAM_CQBInterior.DetectFloorWorldYs(house, mins, maxs, floors);
		int floorCount = Math.Max(1, floors.Count());
		int maxTries = wanted * 4 + 8;
		for (int i = 0; i < maxTries && spots.Count() < wanted; i++)
		{
			vector spot;
			int floorSlot = i - (i / floorCount) * floorCount;
			if (!HAM_CQBInterior.FindInteriorSpot(house, mins, maxs, floors, floorSlot, spots, true, spot))
				continue;

			spots.Insert(spot);
		}

		return !spots.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Un homme par place, tiré parmi le mitrailleur, le fusilier et le sniper. Scindé 2 s plus tard.
	protected static SCR_AIGroup SpawnCqbSeed(int wanted, vector pos)
	{
		if (wanted < 1)
			return null;

		Resource prefab = Resource.Load(HAM_Units.CQB_SOLO_GROUP);
		if (!prefab || !prefab.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = pos;

		SCR_AIGroup.IgnoreSpawning(true);
		SCR_AIGroup seed = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);
		if (!seed)
			return null;

		array<ResourceName> slots = {};
		for (int i = 0; i < wanted; i++)
		{
			ResourceName man = HAM_Units.CqbMan();
			if (!man.IsEmpty())
				slots.Insert(man);
		}

		if (slots.IsEmpty())
		{
			SCR_EntityHelper.DeleteEntityAndChildren(seed);
			return null;
		}

		seed.m_aUnitPrefabSlots = slots;
		seed.SetMaxUnitsToSpawn(slots.Count());
		Faction enemyFaction = GetGame().GetFactionManager().GetFactionByKey(HAM_Units.ENEMY_FACTION);
		if (enemyFaction)
			seed.SetFaction(enemyFaction);
		seed.SetImportance(SCR_EAISpawnImportance.CRITICAL);
		seed.PreventMaxLOD();
		seed.SpawnUnits();
		TrimGroup(seed, slots.Count());

		HAM_Util.Forget(seed);
		GetGame().GetCallqueue().CallLater(HAM_Util.ForgetGroup, 1500, false, seed);
		RegisterEnemy(seed);
		return seed;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TrimGroup(SCR_AIGroup group, int maxCount)
	{
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		for (int i = agents.Count() - 1; i >= maxCount; i--)
			DropAgent(group, agents[i]);

		group.SetMaxUnitsToSpawn(maxCount);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropAgent(SCR_AIGroup group, AIAgent agent)
	{
		if (!agent)
			return;

		IEntity body = agent.GetControlledEntity();
		if (group)
			group.RemoveAgent(agent);

		if (body)
			SCR_EntityHelper.DeleteEntityAndChildren(body);
	}

	//------------------------------------------------------------------------------------------------
	//! Comme le Conflit : on attend que le groupe CQB soit au complet, puis chaque homme passe dans son propre groupe, à sa place.
	protected static void SplitCqbSeed(HAM_CqbSeed pending)
	{
		if (!s_aCqbSeeds || s_aCqbSeeds.Find(pending) < 0)
			return;

		SCR_AIGroup seed = pending.m_Group;
		if (!seed || !pending.m_House)
		{
			s_aCqbSeeds.RemoveItem(pending);
			if (seed)
				SCR_EntityHelper.DeleteEntityAndChildren(seed);

			return;
		}

		array<AIAgent> agents = {};
		seed.GetAgents(agents);
		int ready = 0;
		foreach (AIAgent listed : agents)
		{
			if (listed && listed.GetControlledEntity())
				ready++;
		}

		int need = pending.m_aSpots.Count();
		if (ready < need && pending.m_iRetries > 0)
		{
			pending.m_iRetries--;
			seed.PreventMaxLOD();
			seed.SpawnUnits();
			GetGame().GetCallqueue().CallLater(SplitCqbSeed, 1000, false, pending);
			return;
		}

		s_aCqbSeeds.RemoveItem(pending);
		int seated = 0;
		array<vector> taken = {};
		for (int i = 0; i < agents.Count(); i++)
		{
			AIAgent agent = agents[i];
			if (!agent || !agent.GetControlledEntity())
				continue;

			if (seated >= need)
			{
				DropAgent(seed, agent);
				continue;
			}

			vector spot;
			if (i < pending.m_aSpots.Count())
				spot = pending.m_aSpots[i];
			else
			{
				DropAgent(seed, agent);
				continue;
			}

			bool seatedSpot = HoldableCqbSpot(seed, pending.m_House, spot) && HAM_CQBInterior.IsFarEnough(spot, taken, 2.0);
			if (!seatedSpot)
				seatedSpot = ReplacementCqbSpot(seed, pending.m_House, taken, spot) && HAM_CQBInterior.IsFarEnough(spot, taken, 2.0);

			if (!seatedSpot)
			{
				DropAgent(seed, agent);
				continue;
			}

			taken.Insert(spot);
			SCR_AIGroup solo = SeatCqbMan(seed, agent, pending.m_House, spot);
			if (!solo)
				continue;

			if (pending.m_aInto && pending.m_aInto.Find(solo) < 0)
				pending.m_aInto.Insert(solo);

			seated++;
		}

		if (pending.m_aInto)
			pending.m_aInto.RemoveItem(seed);

		if (seed.GetAgentsCount() == 0)
			SCR_EntityHelper.DeleteEntityAndChildren(seed);
		else
			seed.SetMaxUnitsToSpawn(0);

		HAM_Util.Info(string.Format("CQB : intérieur, %1 homme seul sur %2 places", seated, need));
	}

	//------------------------------------------------------------------------------------------------
	//! Autre place tenable du même bâtiment, quand celle prévue est hors navmesh, ouverte ou au grenier.
	protected static bool ReplacementCqbSpot(SCR_AIGroup group, SCR_DestructibleBuildingEntity house, notnull array<vector> taken, out vector spot)
	{
		vector mins;
		vector maxs;
		house.GetBounds(mins, maxs);
		array<float> floors = {};
		HAM_CQBInterior.DetectFloorWorldYs(house, mins, maxs, floors);
		int floorCount = Math.Max(1, floors.Count());
		for (int attempt = 0; attempt < 6; attempt++)
		{
			vector candidate;
			if (!HAM_CQBInterior.FindInteriorSpot(house, mins, maxs, floors, Math.RandomInt(0, floorCount), taken, true, candidate))
				continue;

			if (!HoldableCqbSpot(group, house, candidate))
				continue;

			spot = candidate;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Comme CPR_HoldableSpot du Conflit : dans la pièce, sur la navmesh à 2,5 m au plus, pas au grenier.
	protected static bool HoldableCqbSpot(SCR_AIGroup group, SCR_DestructibleBuildingEntity house, inout vector spot)
	{
		if (!HAM_CQBInterior.IsEnclosed(house, spot))
			return false;

		vector nav = spot;
		if (!HAM_Util.SnapToNavmesh(group, nav, 2.5, 2.0))
			return false;

		float dx = nav[0] - spot[0];
		float dz = nav[2] - spot[2];
		if (dx * dx + dz * dz > 6.25)
			return false;

		if (!HAM_CQBInterior.IsEnclosed(house, nav) || HAM_CQBInterior.IsUnderRoof(house, nav))
			return false;

		spot = nav;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Groupe vide (Group_Base, comme le Conflit), l'homme y passe seul, posé à sa place, en défense.
	protected static SCR_AIGroup SeatCqbMan(SCR_AIGroup seed, AIAgent agent, SCR_DestructibleBuildingEntity house, vector spot)
	{
		IEntity body = agent.GetControlledEntity();
		Resource prefab = Resource.Load(HAM_Units.CQB_SOLO_GROUP);
		if (!prefab || !prefab.IsValid())
		{
			DropAgent(seed, agent);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = spot;

		SCR_AIGroup.IgnoreSpawning(true);
		SCR_AIGroup solo = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);
		if (!solo)
		{
			DropAgent(seed, agent);
			return null;
		}

		Faction faction = seed.GetFaction();
		if (faction)
			solo.SetFaction(faction);

		solo.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		solo.SetImportance(SCR_EAISpawnImportance.CRITICAL);
		seed.RemoveAgent(agent);
		solo.AddAgent(agent);
		solo.SetMaxUnitsToSpawn(1);
		HAM_Util.ApplySkillValue(solo, 1);
		if (agent.GetParentGroup() != solo || solo.GetAgentsCount() != 1)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(solo);
			if (body)
				SCR_EntityHelper.DeleteEntityAndChildren(body);

			return null;
		}

		vector mat[4];
		body.GetWorldTransform(mat);
		mat[3] = spot;
		body.SetWorldTransform(mat);
		solo.SetOrigin(spot);
		solo.PreventMaxLOD();
		HAM_Util.Order(solo, HAM_Units.WP_DEFEND, spot, 3);

		HAM_Util.Forget(body);
		HAM_Util.Forget(solo);
		RegisterEnemy(solo);

		HAM_CqbActor actor = new HAM_CqbActor();
		actor.m_Group = solo;
		actor.m_House = house;
		actor.m_vPos = spot;
		s_aCqbActors.Insert(actor);
		return solo;
	}

	//------------------------------------------------------------------------------------------------
	//! Maison assez grande pour un intérieur, comme le filtre du Conflit.
	protected static bool CqbHouseFits(SCR_DestructibleBuildingEntity house)
	{
		vector mins;
		vector maxs;
		house.GetBounds(mins, maxs);
		float spanX = Math.AbsFloat(maxs[0] - mins[0]);
		float spanZ = Math.AbsFloat(maxs[2] - mins[2]);
		return spanX >= 4.5 && spanZ >= 4.5 && spanX * spanZ >= 20.0;
	}

	//------------------------------------------------------------------------------------------------
	//! Comme le Conflit : un homme de CQB sorti de sa maison y est remis à sa place.
	protected static void KeepCqbInside()
	{
		for (int i = s_aCqbActors.Count() - 1; i >= 0; i--)
		{
			HAM_CqbActor actor = s_aCqbActors[i];
			if (!actor.m_Group || !actor.m_House || HAM_Util.AliveCount(actor.m_Group) < 1)
			{
				s_aCqbActors.Remove(i);
				continue;
			}

			array<AIAgent> agents = {};
			actor.m_Group.GetAgents(agents);
			foreach (AIAgent agent : agents)
			{
				IEntity body;
				if (agent)
					body = agent.GetControlledEntity();

				if (!body || HAM_Util.IsDead(body))
					continue;

				vector mat[4];
				body.GetWorldTransform(mat);
				if (HAM_CQBInterior.IsEnclosed(actor.m_House, mat[3]))
					continue;

				mat[3] = actor.m_vPos;
				body.SetWorldTransform(mat);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Autour de chaque point ennemi du front, comme autour d'une base du Conflit : CQB dans les
	//! bâtiments, patrouilles à pied et véhicules sur les routes. Tout part quand les joueurs s'éloignent.
	protected static void TickZone(HAM_Point point, array<IEntity> bodies, HAM_Settings settings)
	{
		float nearest = HAM_Util.NearestDistance(bodies, point.Pos());
		if (nearest > settings.m_fDespawnDistance)
		{
			ClearZone(point);
			return;
		}

		KeepGroupsMoving(point.m_aPatrols, point.Pos(), Math.Max(point.Radius(), ZONE_PATROL_RADIUS));

		if (point.m_bPlayer || !CellSpawnsEnemies(point.m_Cell) || nearest > settings.m_fActivationDistance)
			return;

		if (!point.m_bCqbPlaced && settings.m_bCqbEnabled)
		{
			point.m_bCqbPlaced = true;
			int houses;
			int men = PlaceCqb(point.Pos(), Math.Max(point.Radius(), ZONE_CQB_RADIUS), settings.BuildingsOf(point.m_Def), point.m_aCqb, houses);
			HAM_Util.Info(string.Format("CQB : %1, %2 hommes dans %3 bâtiments", point.m_Def.m_sName, men, houses));
		}

		if (point.IsMaster())
			PullArmorFromHeld(point.m_Cell);

		if (!ClearOfHeldZones(point.Pos()))
		{
			DropZoneRoaming(point);
			point.m_bZoneSpawned = false;
			return;
		}

		if (point.m_Cell && !point.m_Cell.IsMasterPlayer() && !point.m_Cell.m_bArmorPatrol && point.m_Cell.m_Master)
		{
			point.m_Cell.m_bArmorPatrol = true;
			for (int armor = 0; armor < 2; armor++)
			{
				HAM_SpawnStep armorStep = new HAM_SpawnStep();
				armorStep.m_iKind = 6;
				armorStep.m_Point = point.m_Cell.m_Master;
				EnqueueSpawn(armorStep);
			}
		}

		if (!settings.m_bPatrolsEnabled || point.m_bZoneSpawned)
			return;

		point.m_bZoneSpawned = true;
		float reach = Math.Max(point.Radius(), ZONE_PATROL_RADIUS);
		int foot = settings.PatrolUnitsOf(point.m_Def);
		if (foot > settings.m_iPatrolMaxActive)
			foot = settings.m_iPatrolMaxActive;

		for (int i = 0; i < foot; i++)
		{
			HAM_SpawnStep step = new HAM_SpawnStep();
			step.m_iKind = 5;
			step.m_Point = point;
			step.m_fRadius = reach;
			EnqueueSpawn(step);
		}

		int cars = settings.VehiclesOf(point.m_Def);
		if (cars > settings.m_iVehiclePatrolMax)
			cars = settings.m_iVehiclePatrolMax;

		for (int v = 0; v < cars; v++)
		{
			HAM_SpawnStep step = new HAM_SpawnStep();
			step.m_iKind = 4;
			step.m_Point = point;
			step.m_fRadius = reach;
			EnqueueSpawn(step);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteArmor(HAM_Cell cell)
	{
		if (!cell || !cell.m_aArmor)
			return;

		foreach (IEntity vehicle : cell.m_aArmor)
			HAM_Util.Delete(vehicle);

		cell.m_aArmor.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool LineZoneCleaned(HAM_Line line)
	{
		if (!line || !line.m_Cell)
			return false;

		HAM_Point point = HAM_Mission.NearestSlave(line.m_Cell, line.Pos());
		if (!point)
			point = line.m_Cell.m_Master;

		return point && point.m_bGarrisonSpawned && point.m_iEnemyLeft == 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Deux véhicules armés par maître, sur les routes entre ses zones. Une zone nettoyée ne les revoit plus.
	protected static void SpawnMasterArmor(HAM_Point master)
	{
		if (!master || !master.m_Cell || master.m_bPlayer)
			return;

		if (master.m_bGarrisonSpawned && master.m_iEnemyLeft == 0)
			return;

		if (!master.m_Cell.m_aArmor)
			master.m_Cell.m_aArmor = {};

		ResourceName prefab = HAM_Units.EnemyPatrolVehicle();
		if (prefab.IsEmpty())
			return;

		array<vector> places = {};
		places.Insert(master.Pos());
		foreach (HAM_Point slave : master.m_Cell.m_aSlaves)
		{
			if (slave && !slave.m_bPlayer)
				places.Insert(slave.Pos());
		}

		foreach (string neighborName : master.m_Cell.m_aNeighbors)
		{
			HAM_Cell neighbor = HAM_Mission.FindCell(neighborName);
			if (neighbor)
				places.Insert(neighbor.Pos());
		}

		vector road;
		bool found = false;
		for (int attempt = 0; attempt < 8; attempt++)
		{
			vector sample = HAM_Util.RandomAround(places.GetRandomElement(), 40, 500);
			if (!HAM_Util.RoadPoint(sample, 280, road) || HAM_Util.IsWater(road) || HAM_Util.IsNearWater(road))
				continue;

			if (!VehicleClearOfPlayers(road) || !ClearOfHeldZones(road))
				continue;

			bool close = false;
			foreach (IEntity other : master.m_Cell.m_aArmor)
			{
				if (other && vector.DistanceXZ(other.GetOrigin(), road) < 160)
					close = true;
			}

			if (close)
				continue;

			found = true;
			break;
		}

		if (!found)
			return;

		array<vector> route = {};
		for (int stop = 0; stop < places.Count(); stop++)
		{
			vector roadStop;
			vector sample = HAM_Util.RandomAround(places.GetRandomElement(), 30, 450);
			if (!HAM_Util.RoadPoint(sample, 280, roadStop) || HAM_Util.IsWater(roadStop))
				continue;

			if (vector.DistanceXZ(roadStop, road) < 80)
				continue;

			route.Insert(roadStop);
		}

		Vehicle vehicle = SpawnCrewedVehicle(prefab, road, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_PATROL, 45, null, true, ResourceName.Empty);
		if (!vehicle)
			return;

		master.m_Cell.m_aArmor.Insert(vehicle);
		HAM_Util.Info(string.Format("Patrouille armée entre les zones : %1, %2", master.m_Cell.m_sName, HAM_Util.PrefabFileName(prefab)));
	}

	//------------------------------------------------------------------------------------------------
	protected static const float VEHICLE_PLAYER_CLEAR = 400;
	protected static const float HELD_PATROL_CLEAR = 2000;

	//------------------------------------------------------------------------------------------------
	//! Le point d'apparition d'une patrouille de véhicules normale est à au moins 400 m de chaque joueur.
	protected static bool VehicleClearOfPlayers(vector pos)
	{
		if (!Replication.IsServer())
			return false;

		array<int> ids = {};
		array<IEntity> bodies = {};
		HAM_Util.AlivePlayers(ids, bodies);
		if (bodies.IsEmpty())
			return true;

		return !HAM_Util.AnyNear(bodies, pos, VEHICLE_PLAYER_CLEAR);
	}

	//------------------------------------------------------------------------------------------------
	//! Même écart que les patrouilles interzone : pas d'apparition à moins de 2 km d'une zone tenue, QG compris.
	protected static bool ClearOfHeldZones(vector pos)
	{
		if (HAM_Mission.s_HqPoint && vector.DistanceXZ(pos, HAM_Mission.s_HqPoint.m_vPos) < HELD_PATROL_CLEAR)
			return false;

		if (!HAM_Mission.s_aPoints)
			return true;

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (!point || !point.m_bPlayer || !point.m_Def)
				continue;

			HAM_ERole role = point.m_Def.m_eRole;
			if (role != HAM_ERole.MASTER && role != HAM_ERole.SLAVE && role != HAM_ERole.HQ && role != HAM_ERole.ENEMY_HQ)
				continue;

			if (vector.DistanceXZ(pos, point.Pos()) < HELD_PATROL_CLEAR)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropZoneRoaming(HAM_Point point)
	{
		if (!point)
			return;

		HAM_Util.DeleteGroups(point.m_aPatrols);
		HAM_Util.DeleteGroups(point.m_aVehicleCrews);
		if (!point.m_aVehicles)
			return;

		foreach (IEntity vehicle : point.m_aVehicles)
		{
			if (!vehicle || HasPlayerOccupant(vehicle))
				continue;

			HAM_Util.Delete(vehicle);
		}

		point.m_aVehicles.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static void PullArmorFromHeld(HAM_Cell cell)
	{
		if (!cell || !cell.m_aArmor)
			return;

		for (int i = cell.m_aArmor.Count() - 1; i >= 0; i--)
		{
			IEntity vehicle = cell.m_aArmor[i];
			if (!vehicle)
			{
				cell.m_aArmor.Remove(i);
				continue;
			}

			if (HasPlayerOccupant(vehicle) || ClearOfHeldZones(vehicle.GetOrigin()))
				continue;

			ForgetLiveCrew(vehicle);
			HAM_Util.Delete(vehicle);
			cell.m_aArmor.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ForgetLiveCrew(IEntity vehicle)
	{
		if (!vehicle || !s_aLiveCrews)
			return;

		for (int i = s_aLiveCrews.Count() - 1; i >= 0; i--)
		{
			HAM_LiveCrew live = s_aLiveCrews[i];
			if (!live || live.m_Vehicle != vehicle)
				continue;

			HAM_Util.DeleteGroup(live.m_Group);
			s_aLiveCrews.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpawnZoneVehicle(HAM_Point point, float reach, HAM_Settings settings)
	{
		HAM_ZoneTune tune = settings.TuneFor(point.m_Def);
		ResourceName prefab = HAM_Units.EnemyZoneVehicle(tune.m_iArmor, tune.m_iCar);
		if (prefab.IsEmpty())
			return false;

		vector roadStart;
		bool found = false;
		for (int attempt = 0; attempt < 12 && !found; attempt++)
		{
			vector sample = HAM_Util.RandomAround(point.Pos(), point.Radius() * 0.5, reach);
			if (!HAM_Util.RoadPoint(sample, 250, roadStart) || HAM_Util.IsNearWater(roadStart))
				continue;

			if (!VehicleClearOfPlayers(roadStart) || !ClearOfHeldZones(roadStart))
				continue;

			found = true;
		}

		if (!found)
			return false;

		array<vector> route = {};
		for (int r = 0; r < 4; r++)
		{
			vector roadPoint;
			if (HAM_Util.RoadPoint(HAM_Util.RandomAround(point.Pos(), 0, reach + 200), 250, roadPoint) && !HAM_Util.IsWater(roadPoint))
				route.Insert(roadPoint);
		}

		Vehicle vehicle = SpawnCrewedVehicle(prefab, roadStart, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_PATROL, 30, point.m_aVehicleCrews, true, ResourceName.Empty);
		if (!vehicle)
			return false;

		point.m_aVehicles.Insert(vehicle);
		HAM_Util.Info(string.Format("Véhicule en patrouille : %1, %2 (%3 sur la carte)", point.m_Def.m_sName, HAM_Util.PrefabFileName(prefab), s_iVehiclePatrols + 1));
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Véhicules de la zone encore en état, avec un équipage vivant ou en cours de création.
	protected static int ActiveVehicles(HAM_Point point)
	{
		int count = 0;
		for (int i = point.m_aVehicles.Count() - 1; i >= 0; i--)
		{
			Vehicle vehicle = Vehicle.Cast(point.m_aVehicles[i]);
			if (!vehicle)
			{
				point.m_aVehicles.Remove(i);
				continue;
			}

			if (HAM_Util.IsDead(vehicle))
				continue;

			if (IsCrewPending(vehicle))
			{
				count++;
				continue;
			}

			SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compartments)
				continue;

			SCR_AIGroup crew = CrewGroup(compartments);
			if (crew && HAM_Util.AliveCount(crew) > 0)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearZone(HAM_Point point)
	{
		if (point.m_aGroups.IsEmpty() && point.m_aPatrols.IsEmpty() && point.m_aVehicleCrews.IsEmpty() && point.m_aCqb.IsEmpty() && point.m_aVehicles.IsEmpty())
			return;

		int garrisonAlive = HAM_Util.AliveCountAll(point.m_aGroups);
		int patrolAlive = HAM_Util.AliveCountAll(point.m_aPatrols);
		int cqbAlive = HAM_Util.AliveCountAll(point.m_aCqb);
		int vehiclesAlive = ActiveVehicles(point);
		bool hadPatrols = !point.m_aPatrols.IsEmpty();
		bool hadVehicles = !point.m_aVehicles.IsEmpty();
		bool hadCqb = !point.m_aCqb.IsEmpty();
		bool garrisonSpawned = point.m_bGarrisonSpawned;
		HAM_Util.DeleteGroups(point.m_aGroups);
		HAM_Util.DeleteGroups(point.m_aPatrols);
		HAM_Util.DeleteGroups(point.m_aVehicleCrews);
		HAM_Util.DeleteGroups(point.m_aCqb);
		point.m_bActive = false;
		bool zoneWiped = point.m_bZoneSpawned && patrolAlive < 1 && vehiclesAlive < 1 && (hadPatrols || hadVehicles);
		if (!zoneWiped)
			point.m_bZoneSpawned = false;

		bool cqbWiped = point.m_bCqbPlaced && hadCqb && cqbAlive < 1;
		if (!cqbWiped)
			point.m_bCqbPlaced = false;
		if (garrisonSpawned && garrisonAlive < 1)
		{
			point.m_bGarrisonSpawned = true;
			point.m_iEnemyLeft = 0;
			point.m_bZoneSpawned = true;
			point.m_bCqbPlaced = true;
			if (point.IsMaster() && point.m_Cell)
			{
				point.m_Cell.m_bArmorPatrol = true;
				DeleteArmor(point.m_Cell);
			}
		}
		else if (garrisonSpawned)
		{
			point.m_bGarrisonSpawned = false;
			point.m_iEnemyLeft = -1;
		}

		float keepSq = (ZONE_PATROL_RADIUS + 500) * (ZONE_PATROL_RADIUS + 500);
		foreach (IEntity vehicle : point.m_aVehicles)
		{
			// Un véhicule pris ou emmené loin par des joueurs reste dans le monde.
			if (!vehicle || HAM_Util.IsDead(vehicle) || HasPlayerOccupant(vehicle))
				continue;

			if (vector.DistanceSqXZ(vehicle.GetOrigin(), point.Pos()) > keepSq)
				continue;

			HAM_Util.Delete(vehicle);
		}

		point.m_aVehicles.Clear();
		point.m_fNextPatrol = 0;
		point.m_fNextVehicle = 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Le véhicule de la patrouille part avec son équipage. Celui qu'un joueur occupe reste dans le monde.
	protected static void ReleaseLineVehicles(HAM_Line line)
	{
		if (!line || !line.m_aVehicles)
			return;

		for (int i = line.m_aVehicles.Count() - 1; i >= 0; i--)
		{
			Vehicle vehicle = Vehicle.Cast(line.m_aVehicles[i]);
			if (!vehicle || HasPlayerOccupant(vehicle))
				continue;

			if (s_mScriptVehicles)
				s_mScriptVehicles.Remove(vehicle);

			HAM_Util.Delete(vehicle);
		}

		line.m_aVehicles.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasPlayerOccupant(IEntity vehicle)
	{
		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
			return false;

		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		PlayerManager players = GetGame().GetPlayerManager();
		foreach (IEntity occupant : occupants)
		{
			if (occupant && players.GetPlayerIdFromControlledEntity(occupant) > 0)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static float RouteRadius(float radius)
	{
		if (radius < 15)
			return 30;

		return radius;
	}

	//------------------------------------------------------------------------------------------------
	//! Civil à pied après un embarquement raté : trajet et réveil, le véhicule vide part avec AbandonCrew.
	protected static bool ReleaseUnseatedCivilians(HAM_PendingCrew pending)
	{
		if (!pending || !pending.m_Vehicle)
			return false;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(pending.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!pending.m_Crew)
			pending.m_Crew = GroupNear(pending.m_Vehicle.GetOrigin(), 14);

		if (compartments && pending.m_Crew)
			SeatAgents(pending.m_Vehicle, compartments, pending.m_Crew, 0, true);

		if (compartments && pending.m_Crew && CrewSeated(compartments))
		{
			FinishCrew(pending, true);
			pending.m_Crew = null;
			return true;
		}

		if (!pending.m_Crew)
			return false;

		HAM_Util.KeepAwake(pending.m_Crew);
		GiveRoute(pending.m_Crew, pending.m_Vehicle.GetOrigin(), RouteRadius(pending.m_fRadius));
		pending.m_Crew = null;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void RecoverDismountedCrew(Vehicle vehicle)
	{
		if (!vehicle || HAM_Util.IsDead(vehicle))
			return;

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		SCR_AIGroup group = null;
		if (compartments)
			group = CrewGroup(compartments);

		if (!group)
			group = GroupNear(vehicle.GetOrigin(), 14);

		if (!group)
			return;

		if (compartments)
			SeatAgents(vehicle, compartments, group, 0, true);

		HAM_Util.KeepAwake(group);
		HAM_Util.ClearWaypoints(group);
		if (compartments && CrewSeated(compartments))
		{
			vector next = HAM_Util.DryAround(vehicle.GetOrigin(), 80, 250);
			HAM_Util.SnapToNavmesh(group, next);
			HAM_Util.Order(group, HAM_Units.WP_MOVE, next, 30);
			return;
		}

		GiveRoute(group, vehicle.GetOrigin(), 40);
	}

	//------------------------------------------------------------------------------------------------
	//! Le GM a déplacé la voiture : le groupe est resté sur l'ancienne position, ou l'ordre y ramène.
	protected static bool CivilVehicleMoved(HAM_LiveCrew live)
	{
		vector here = live.m_Vehicle.GetOrigin();
		bool moved = false;
		if (live.m_bHavePos)
		{
			float step = vector.DistanceXZ(here, live.m_vLastPos);
			float groupToOld = 0;
			float groupToNew = 0;
			if (live.m_Group)
			{
				groupToOld = vector.DistanceXZ(live.m_Group.GetOrigin(), live.m_vLastPos);
				groupToNew = vector.DistanceXZ(live.m_Group.GetOrigin(), here);
			}

			if (step > 120)
				moved = true;

			if (step > 25 && groupToOld < 12 && groupToNew > 20)
				moved = true;
		}

		live.m_vLastPos = here;
		live.m_bHavePos = true;
		return moved;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RebuildCivilRoute(HAM_LiveCrew live)
	{
		if (!live || !live.m_Group || !live.m_Vehicle)
			return;

		vector here = live.m_Vehicle.GetOrigin();
		live.m_aRoute.Clear();
		live.m_Group.SetOrigin(here);
		HAM_Util.ClearWaypoints(live.m_Group);
		HAM_Util.KeepAwake(live.m_Group);
		for (int step = 0; step < 3; step++)
		{
			vector sample = HAM_Util.RandomAround(here, 80, 450);
			vector stop;
			if (!HAM_Util.RoadPoint(sample, 200, stop) || HAM_Util.IsWater(stop))
				continue;

			live.m_aRoute.Insert(stop);
		}

		ReissueCivilRoute(live);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReissueCivilRoute(HAM_LiveCrew live)
	{
		if (!live || !live.m_Group || !live.m_Vehicle)
			return;

		ResourceName waypoint = live.m_sWaypoint;
		if (waypoint.IsEmpty() || waypoint == HAM_Units.WP_PATROL)
			waypoint = HAM_Units.WP_MOVE;

		float radius = RouteRadius(live.m_fRadius);
		if (live.m_aRoute && !live.m_aRoute.IsEmpty())
		{
			foreach (vector routePoint : live.m_aRoute)
			{
				vector spot = routePoint;
				HAM_Util.SnapToNavmesh(live.m_Group, spot);
				HAM_Util.Order(live.m_Group, waypoint, spot, radius);
			}

			return;
		}

		vector next = HAM_Util.DryAround(live.m_Vehicle.GetOrigin(), 80, 250);
		HAM_Util.SnapToNavmesh(live.m_Group, next);
		HAM_Util.Order(live.m_Group, waypoint, next, radius);
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<SCR_AIGroup> s_aNearGroups;

	protected static SCR_AIGroup GroupNear(vector pos, float radius)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		s_aNearGroups = {};
		world.QueryEntitiesBySphere(pos, radius, SeeNearAgent);
		if (!s_aNearGroups || s_aNearGroups.IsEmpty())
			return null;

		return s_aNearGroups[0];
	}

	protected static bool SeeNearAgent(IEntity ent)
	{
		ChimeraCharacter character = ChimeraCharacter.Cast(ent);
		if (!character)
			return true;

		AIControlComponent control = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
		if (!control)
			return true;

		AIAgent agent = control.GetControlAIAgent();
		if (!agent)
			agent = control.GetAIAgent();

		if (!agent)
			return true;

		SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
		if (!group)
			return true;

		if (!s_aNearGroups)
			s_aNearGroups = {};

		if (s_aNearGroups.Find(group) < 0)
			s_aNearGroups.Insert(group);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool GroupThreatened(SCR_AIGroup group)
	{
		if (!group)
			return false;

		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body)
				continue;

			SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(body.FindComponent(SCR_AICombatComponent));
			if (combat && combat.HAM_HasWeaponAlert())
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! 30 % après une vraie explosion d'IED : un groupe part d'une zone encore ennemie, à 600 m de chaque joueur.
	static void IedAttract(vector explosion)
	{
		if (!Replication.IsServer() || !HAM_Mission.s_aPoints)
			return;

		array<int> ids = {};
		array<IEntity> bodies = {};
		HAM_Util.AlivePlayers(ids, bodies);
		HAM_Point origin;
		vector spawn;
		bool found = false;
		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (!point || point.m_bPlayer || !point.m_Def)
				continue;

			if (point.m_Def.m_eRole == HAM_ERole.HQ)
				continue;

			for (int attempt = 0; attempt < 8; attempt++)
			{
				vector candidate = HAM_Util.DryAround(point.Pos(), point.Radius() * 0.3, point.Radius());
				if (HAM_Util.AnyNear(bodies, candidate, 600))
					continue;

				origin = point;
				spawn = candidate;
				found = true;
				break;
			}

			if (found)
				break;
		}

		if (!found)
		{
			Print("[13RDPA H&M] Patrouille IED non créée : aucun point ennemi à 600 m de tous les joueurs.", LogLevel.NORMAL);
			return;
		}

		SCR_AIGroup group = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_LIGHT, spawn, 0);
		if (!group)
			return;

		HAM_Util.Order(group, HAM_Units.WP_MOVE, explosion, 25);
		HAM_Util.Info(string.Format("Patrouille attirée par un IED depuis %1", origin.m_Def.m_sName));
	}

	//------------------------------------------------------------------------------------------------
	protected static void GiveRoute(SCR_AIGroup group, vector center, float radius)
	{
		if (!group)
			return;

		HAM_Util.KeepAwake(group);
		for (int i = 0; i < 4; i++)
		{
			vector spot = HAM_Util.DryAround(center, 0, radius);
			HAM_Util.SnapToNavmesh(group, spot);
			HAM_Util.Order(group, HAM_Units.WP_PATROL, spot, 15);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void KeepGroupsMoving(array<SCR_AIGroup> groups, vector center, float radius)
	{
		if (!groups)
			return;

		foreach (SCR_AIGroup group : groups)
		{
			if (!group || IsLiveCrew(group))
				continue;

			HAM_Util.KeepAwake(group);
			if (GroupThreatened(group))
			{
				HAM_Util.ClearWaypoints(group);
				continue;
			}

			array<AIWaypoint> waypoints = {};
			if (group.GetWaypoints(waypoints) == 0)
				GiveRoute(group, center, radius);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static int LivingGroups(array<SCR_AIGroup> groups)
	{
		int count = 0;
		foreach (SCR_AIGroup group : groups)
		{
			if (group && HAM_Util.AliveCount(group) > 0)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Garnison amie d'un maître tenu : posée quand un joueur ou une contre-attaque approche.
	protected static void TickFriendly(HAM_Cell cell, array<IEntity> bodies, HAM_Settings settings)
	{
		if (!cell.IsMasterPlayer())
		{
			if (!cell.m_aFriendly.IsEmpty())
				HAM_Util.DeleteGroups(cell.m_aFriendly);

			cell.m_bFriendlyActive = false;
			return;
		}

		vector pos = cell.Pos();
		float nearest = HAM_Util.NearestDistance(bodies, pos);
		bool threatened = HAM_CounterAttack.EnemyNear(pos, settings.m_fActivationDistance);
		if (!cell.m_bFriendlyActive && (nearest <= settings.m_fActivationDistance || threatened))
		{
			cell.m_bFriendlyActive = true;
			int units = Math.Min(cell.m_Stock.m_iMen, settings.m_iFriendlyMaxUnits);
			int guard = 0;
			while (units > 0 && guard < 8)
			{
				guard++;
				int size = Math.Min(units, settings.m_iMaxGroupSize);
				units -= size;
				SCR_AIGroup group = HAM_Util.SpawnGroup(HAM_Units.PlayerGarrisonGroup(), HAM_Util.DryAround(pos, 5, 30), size);
				if (!group)
					continue;

				HAM_Util.Order(group, HAM_Units.WP_DEFEND, pos, Math.Max(30, cell.m_Master.Radius() * 0.5));
				cell.m_aFriendly.Insert(group);
			}

			return;
		}

		if (cell.m_bFriendlyActive && nearest > settings.m_fDespawnDistance && !HAM_CounterAttack.EnemyNear(pos, settings.m_fDespawnDistance))
		{
			HAM_Util.DeleteGroups(cell.m_aFriendly);
			cell.m_bFriendlyActive = false;
			return;
		}

		array<IEntity> friends = {};
		foreach (SCR_AIGroup friendly : cell.m_aFriendly)
		{
			HAM_Util.AliveBodies(friendly, friends);
		}

		foreach (IEntity mate : friends)
		{
			s_mFriendlyBodies.Set(mate, cell.m_sName);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un défenseur ami tué : un homme de moins dans le stock de sa cellule.
	static void OnUnitKilled(IEntity victim)
	{
		Ensure();
		string cellName;
		if (!s_mFriendlyBodies.Find(victim, cellName))
			return;

		s_mFriendlyBodies.Remove(victim);
		HAM_Cell cell = HAM_Mission.FindCell(cellName);
		if (!cell)
			return;

		cell.m_Stock.Add(HAM_EResource.MEN, -1);
		HAM_Mission.RefreshZoneProps(cell);
		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	//! Renforts : une cellule ennemie du front reçoit des troupes à intervalle, selon la chance réglée.
	static void SlowTick(float dt)
	{
		HAM_Settings settings = HAM_Settings.Get();
		if (!settings.m_bReinforcementsEnabled)
			return;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (cell.IsMasterPlayer() || !CellSpawnsEnemies(cell))
				continue;

			cell.m_fReinforceLeft -= dt;
			if (cell.m_fReinforceLeft > 0)
				continue;

			cell.m_fReinforceLeft = settings.m_fReinforcementMinutes * 60;
			if (Math.RandomFloat01() > settings.m_fReinforcementChance)
				continue;

			Reinforce(cell);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un groupe entier rejoint le maître ennemi dont la garnison est encore en vie.
	//! Une zone déjà vidée n'est pas reremplie.
	protected static void Reinforce(HAM_Cell cell)
	{
		HAM_Point master = cell.m_Master;
		if (!master || master.m_bPlayer || !master.m_bActive)
			return;

		if (HAM_Util.AliveCountAll(master.m_aGroups) < 1)
			return;

		int spawned = SpawnEnemyUnits(master.m_Def, master.Pos(), master.Radius(), 1, master.m_aGroups);
		if (spawned < 1)
			return;

		HAM_Util.Info(string.Format("Renfort : %1, %2 hommes", master.m_Def.m_sName, spawned));
	}

	//------------------------------------------------------------------------------------------------
	//! Point à distance du centre, du côté opposé aux joueurs les plus proches.
	static vector FarSide(vector center, float distance, array<IEntity> bodies)
	{
		vector away = vector.Zero;
		foreach (IEntity body : bodies)
		{
			if (!body)
				continue;

			vector delta = center - body.GetOrigin();
			delta[1] = 0;
			if (delta.LengthSq() < 4000000)
				away = away + delta;
		}

		if (away.LengthSq() < 1)
			return HAM_Util.DryAround(center, distance * 0.8, distance);

		away.Normalize();
		vector pos = HAM_Util.Ground(center + away * distance);
		if (HAM_Util.IsWater(pos))
			return HAM_Util.DryAround(center, distance * 0.8, distance);

		return pos;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsCivilianGroup(SCR_AIGroup group)
	{
		if (!group)
			return false;

		Faction faction = group.GetFaction();
		if (!faction)
			return false;

		return faction.GetFactionKey() == HAM_Units.CIVILIAN_FACTION;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteMilitaryGroups(array<SCR_AIGroup> groups)
	{
		if (!groups)
			return;

		array<SCR_AIGroup> military = {};
		foreach (SCR_AIGroup group : groups)
		{
			if (!group || IsCivilianGroup(group))
				continue;

			military.Insert(group);
		}

		HAM_Util.DeleteGroups(military);
	}

	//------------------------------------------------------------------------------------------------
	static void OnPointCaptured(HAM_Point point)
	{
		point.m_bActive = false;
		point.m_iEnemyLeft = 0;
		DeleteMilitaryGroups(point.m_aGroups);
		DeleteMilitaryGroups(point.m_aPatrols);
		DeleteMilitaryGroups(point.m_aVehicleCrews);
		DeleteMilitaryGroups(point.m_aCqb);
		foreach (IEntity vehicle : point.m_aVehicles)
		{
			if (!vehicle || HAM_Util.IsDead(vehicle) || HasPlayerOccupant(vehicle))
				continue;

			HAM_Util.Delete(vehicle);
		}

		point.m_aVehicles.Clear();
		if (!point.m_Cell)
			return;

		foreach (HAM_Line line : point.m_Cell.m_aLines)
		{
			HAM_Point slave = HAM_Mission.NearestSlave(point.m_Cell, line.Pos());
			if (slave != point)
				continue;

			line.m_bSpawnedOnce = true;
			line.m_bActive = false;
			HAM_Util.DeleteGroups(line.m_aGroups);
			ReleaseLineVehicles(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Une ligne vidée, ou des IED déjà posés, ne doivent pas renaître au redémarrage. Les mines AP, si.
	static bool LineStaysClear(HAM_Line line)
	{
		if (!line || !line.m_bSpawnedOnce || !line.m_Def)
			return false;

		HAM_ERole role = line.m_Def.m_eRole;
		if (role == HAM_ERole.IED)
			return true;

		if (role != HAM_ERole.PATROL && role != HAM_ERole.CQB)
			return false;

		return HAM_Util.AliveCountAll(line.m_aGroups) < 1;
	}

	//------------------------------------------------------------------------------------------------
	static void OnCellComplete(HAM_Cell cell)
	{
		foreach (HAM_Line line : cell.m_aLines)
		{
			line.m_bActive = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	static void OnMasterLost(HAM_Cell cell)
	{
		HAM_Util.DeleteGroups(cell.m_aFriendly);
		cell.m_bFriendlyActive = false;
		cell.m_Master.m_bActive = false;
		cell.m_Master.m_bGarrisonSpawned = false;
		cell.m_Master.m_bCqbPlaced = false;
		cell.m_Master.m_bZoneSpawned = false;
		cell.m_bArmorPatrol = false;
		DeleteArmor(cell);
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			slave.m_bActive = false;
			slave.m_bGarrisonSpawned = false;
			slave.m_bCqbPlaced = false;
			slave.m_bZoneSpawned = false;
		}

		foreach (HAM_Line line : cell.m_aLines)
		{
			line.m_bSpawnedOnce = false;
			line.m_bActive = false;
		}

		HAM_Devices.OnMasterLost(cell);
	}

	//------------------------------------------------------------------------------------------------
	//! Le point n'est plus tenu : sa garnison, ses patrouilles et son CQB peuvent revenir.
	static void OnSlaveLost(HAM_Point slave)
	{
		if (!slave)
			return;

		slave.m_bActive = false;
		slave.m_bGarrisonSpawned = false;
		slave.m_bCqbPlaced = false;
		slave.m_bZoneSpawned = false;
		if (!slave.m_Cell)
			return;

		foreach (HAM_Line line : slave.m_Cell.m_aLines)
		{
			if (HAM_Mission.NearestSlave(slave.m_Cell, line.Pos()) != slave)
				continue;

			line.m_bSpawnedOnce = false;
			line.m_bActive = false;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Véhicule avec son équipage vanilla ; les ordres partent quand l'équipage est assis.
	//! Patrouille entre deux zones : véhicule si la part du réglage le veut, sinon à pied.
	static bool SpawnLoosePatrol(vector pos, notnull array<SCR_AIGroup> groups)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_MapPoint probe = new HAM_MapPoint();
		probe.m_eRole = HAM_ERole.PATROL;
		if (Math.RandomFloat01() < settings.m_fPatrolVehicleShare)
		{
			vector road;
			if (HAM_Util.RoadPoint(pos, 150, road) && !HAM_Util.IsNearWater(road) && HAM_Util.OutOfBuildings(road))
			{
				array<vector> route = {};
				vector nextRoad;
				if (HAM_Util.RoadPoint(HAM_Util.RandomAround(pos, 80, 200), 200, nextRoad) && !HAM_Util.IsNearWater(nextRoad))
					route.Insert(nextRoad);
				else
					route.Insert(road);

				array<SCR_AIGroup> crews = {};
				Vehicle patrolVehicle = SpawnCrewedVehicle(HAM_Units.VEHICLE_M1025, road, Math.RandomFloat(0, 360), false, route, HAM_Units.WP_PATROL, 30, crews, true, ResourceName.Empty);
				if (patrolVehicle)
				{
					foreach (SCR_AIGroup crew : crews)
					{
						RegisterEnemy(crew);
						groups.Insert(crew);
					}

					return groups.Count() > 0;
				}
			}
		}

		vector start = HAM_Util.DryAround(pos, 0, 100);
		if (HAM_Util.IsNearWater(start) || !HAM_Util.OutOfBuildings(start))
		{
			if (!HAM_Util.FindSite(pos, 4, start) || HAM_Util.IsNearWater(start))
				return false;
		}

		SCR_AIGroup group = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_LIGHT, start, 0);
		if (!group)
			return false;

		RegisterEnemy(group);
		groups.Insert(group);
		HAM_Util.KeepAwake(group);
		for (int i = 0; i < 4; i++)
		{
			vector spot = HAM_Util.DryAround(pos, 0, 100);
			if (HAM_Util.IsNearWater(spot) || !HAM_Util.OutOfBuildings(spot))
				continue;

			HAM_Util.SnapToNavmesh(group, spot);
			HAM_Util.Order(group, HAM_Units.WP_PATROL, spot, 15);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static Vehicle SpawnCrewedVehicle(ResourceName prefab, vector pos, float yaw, bool withCargo, array<vector> route, ResourceName waypointPrefab, float radius, array<SCR_AIGroup> into, bool enemy, ResourceName crewPrefab)
	{
		Ensure();
		Vehicle vehicle = Vehicle.Cast(HAM_Util.Spawn(prefab, HAM_Util.Ground(pos) + Vector(0, 0.5, 0), yaw));
		if (!vehicle)
			return null;

		s_mScriptVehicles.Set(vehicle, true);
		vehicle.SetFlags(EntityFlags.ACTIVE, false);

		SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
		if (!compartments)
		{
			HAM_Util.Delete(vehicle);
			return null;
		}

		array<ECompartmentType> types = {ECompartmentType.PILOT, ECompartmentType.TURRET};
		if (withCargo)
			types.Insert(ECompartmentType.CARGO);

		UnlockCrewSlots(compartments);
		bool heavy = false;
		if (vehicle.GetPrefabData())
			heavy = vehicle.GetPrefabData().GetPrefabName().Contains("LAV25");

		SCR_AIGroup crew = null;
		SCR_AIGroup extra = null;
		if (enemy)
		{
			if (heavy)
				crew = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_LAT, pos, 0);
			else
			{
				crew = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_TRANSPORT, pos, 0);
				extra = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_TRANSPORT, pos, 0);
			}
		}
		else if (!crewPrefab.IsEmpty())
		{
			crew = HAM_Util.SpawnFullGroup(crewPrefab, pos, 0);
		}

		if (!crew && !enemy && crewPrefab.IsEmpty())
		{
			int seats = BoardSeatCount(compartments);
			if (seats < 1)
				seats = 1;

			crew = HAM_Intel.CivilianGroup(pos, seats);
		}

		if (!crew)
			compartments.SpawnDefaultOccupants(types);

		HAM_PendingCrew pending = new HAM_PendingCrew();
		pending.m_Vehicle = vehicle;
		pending.m_sWaypoint = waypointPrefab;
		pending.m_fRadius = radius;
		pending.m_aInto = into;
		pending.m_bEnemy = enemy;
		pending.m_fSince = HAM_Util.Now();
		pending.m_iSpawnTries = 1;
		if (crew)
		{
			pending.m_Crew = crew;
			pending.m_CrewExtra = extra;
			pending.m_iCrewSeats = BoardSeatCount(compartments);
		}
		if (route)
		{
			foreach (vector routePoint : route)
			{
				pending.m_aRoute.Insert(routePoint);
			}
		}

		s_aPendingCrews.Insert(pending);
		return vehicle;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickPendingCrews()
	{
		for (int i = s_aPendingCrews.Count() - 1; i >= 0; i--)
		{
			HAM_PendingCrew pending = s_aPendingCrews[i];
			if (!pending || !pending.m_Vehicle || HAM_Util.IsDead(pending.m_Vehicle))
			{
				AbandonCrew(pending);
				s_aPendingCrews.Remove(i);
				continue;
			}

			if (HAM_Util.Now() - pending.m_fSince > CREW_BOARD_SECONDS)
			{
				bool keepVehicle = false;
				if (!pending.m_bEnemy && !pending.m_aInto)
					keepVehicle = ReleaseUnseatedCivilians(pending);

				if (!keepVehicle)
					AbandonCrew(pending);

				s_aPendingCrews.Remove(i);
				continue;
			}

			pending.m_Vehicle.SetFlags(EntityFlags.ACTIVE, false);
			SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(pending.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compartments)
			{
				AbandonCrew(pending);
				s_aPendingCrews.Remove(i);
				continue;
			}

			if (!pending.m_Crew)
				pending.m_Crew = CrewGroup(compartments);

			if (!pending.m_Crew && !pending.m_bEnemy && !pending.m_aInto && pending.m_Vehicle)
				pending.m_Crew = GroupNear(pending.m_Vehicle.GetOrigin(), 14);

			if (pending.m_Crew)
				HAM_Util.KeepAwake(pending.m_Crew);

			if (compartments.IsSpawningDefaultOccupants())
				continue;

			if (pending.m_bAir)
			{
				if (pending.m_Crew)
					SeatAgents(pending.m_Vehicle, compartments, pending.m_Crew, 1, false);

				if (pending.m_CrewExtra)
				{
					HAM_Util.KeepAwake(pending.m_CrewExtra);
					SeatAgents(pending.m_Vehicle, compartments, pending.m_CrewExtra, 2, false);
				}
			}
			else
			{
				if (pending.m_CrewExtra)
					HAM_Util.KeepAwake(pending.m_CrewExtra);

				SeatGroups(pending.m_Vehicle, compartments, pending.m_Crew, pending.m_CrewExtra);
			}

			if (pending.m_iCrewSeats > 0)
				DropUnseated(pending.m_Crew, pending.m_CrewExtra, pending.m_iCrewSeats);

			bool seated = CrewSeated(compartments);
			if (seated && !pending.m_Crew)
				pending.m_Crew = CrewGroup(compartments);

			if (seated && pending.m_Crew)
			{
				if (pending.m_bAir)
				{
					if (pending.m_fSeatedSince <= 0)
						pending.m_fSeatedSince = HAM_Util.Now();

					if (HAM_Util.Now() - pending.m_fSeatedSince < 8)
						continue;
				}

				FinishCrew(pending, true);
				s_aPendingCrews.Remove(i);
				continue;
			}

		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickLiveCrews()
	{
		if (!s_aLiveCrews)
			return;

		for (int i = s_aLiveCrews.Count() - 1; i >= 0; i--)
		{
			HAM_LiveCrew live = s_aLiveCrews[i];
			if (!live || !live.m_Group)
			{
				s_aLiveCrews.Remove(i);
				continue;
			}

			if (HAM_Util.AliveCount(live.m_Group) < 1)
			{
				s_aLiveCrews.Remove(i);
				continue;
			}

			HAM_Util.KeepAwake(live.m_Group);
			if (!live.m_Vehicle || HAM_Util.IsDead(live.m_Vehicle))
			{
				if (live.m_bCivilRoad)
				{
					array<AIWaypoint> left = {};
					if (live.m_Group.GetWaypoints(left) == 0)
						GiveRoute(live.m_Group, live.m_Group.GetOrigin(), RouteRadius(live.m_fRadius));
				}

				continue;
			}

			live.m_Vehicle.SetFlags(EntityFlags.ACTIVE, false);
			if (live.m_bAir && !live.m_bFlightCut && HeliLiftGone(live.m_Vehicle))
			{
				CutHeliAutopilot(live);
				live.m_bFlightCut = true;
			}

			if (live.m_bCivilRoad && CivilVehicleMoved(live))
			{
				RebuildCivilRoute(live);
				continue;
			}

			SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(live.m_Vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compartments)
				continue;

			if (!CrewSeated(compartments))
			{
				if (live.m_bCivilRoad && HAM_Intel.RoadCrewOnFoot(live.m_Vehicle))
				{
					array<AIWaypoint> foot = {};
					if (live.m_Group.GetWaypoints(foot) == 0)
						GiveRoute(live.m_Group, live.m_Vehicle.GetOrigin(), RouteRadius(live.m_fRadius));

					continue;
				}

				SeatAgents(live.m_Vehicle, compartments, live.m_Group, 0, true);
				if (CrewSeated(compartments))
				{
					live.m_fOutSince = 0;
					continue;
				}

				if (!live.m_bCivilRoad)
					continue;

				if (live.m_fOutSince <= 0)
					live.m_fOutSince = HAM_Util.Now();

				if (HAM_Util.Now() - live.m_fOutSince < 12)
					continue;

				array<AIWaypoint> held = {};
				if (live.m_Group.GetWaypoints(held) == 0)
					GiveRoute(live.m_Group, live.m_Vehicle.GetOrigin(), RouteRadius(live.m_fRadius));

				continue;
			}

			live.m_fOutSince = 0;
			if (!live.m_bPatrol && !live.m_bCivilRoad)
				continue;

			array<AIWaypoint> waypoints = {};
			if (live.m_Group.GetWaypoints(waypoints) > 0)
				continue;

			if (live.m_bCivilRoad)
				ReissueCivilRoute(live);
			else if (live.m_bAir)
				ReissueAirRoute(live);
			else
				GiveRoute(live.m_Group, live.m_Vehicle.GetOrigin(), RouteRadius(live.m_fRadius));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void FinishCrew(HAM_PendingCrew pending, bool seated)
	{
		if (!pending || !pending.m_Crew)
			return;

		HAM_Util.Forget(pending.m_Vehicle);
		HAM_Util.ForgetGroup(pending.m_Crew);
		HAM_Util.KeepAwake(pending.m_Crew);
		if (seated)
		{
			if (pending.m_bAir)
			{
				DropBodiesOnFoot(pending.m_Crew);
				ClaimVehicle(pending.m_Crew, pending.m_Vehicle);
			}

			GiveCrewOrders(pending, pending.m_Crew);
		}

		RememberLiveCrew(pending, pending.m_Crew, true);
		if (pending.m_aInto && pending.m_aInto.Find(pending.m_Crew) < 0)
			pending.m_aInto.Insert(pending.m_Crew);

		if (pending.m_bEnemy)
			RegisterEnemy(pending.m_Crew);

		KeepExtraCrew(pending);
	}

	//------------------------------------------------------------------------------------------------
	protected static void KeepExtraCrew(HAM_PendingCrew pending)
	{
		if (!pending || !pending.m_CrewExtra)
			return;

		HAM_Util.ForgetGroup(pending.m_CrewExtra);
		HAM_Util.KeepAwake(pending.m_CrewExtra);
		RememberLiveCrew(pending, pending.m_CrewExtra, false);
		if (pending.m_aInto && pending.m_aInto.Find(pending.m_CrewExtra) < 0)
			pending.m_aInto.Insert(pending.m_CrewExtra);

		if (pending.m_bEnemy)
			RegisterEnemy(pending.m_CrewExtra);
	}

	//------------------------------------------------------------------------------------------------
	protected static void GiveCrewOrders(HAM_PendingCrew pending, SCR_AIGroup crew)
	{
		if (!pending || !crew || !pending.m_Vehicle)
			return;

		if (pending.m_bAir)
		{
			foreach (vector airPoint : pending.m_aRoute)
			{
				HAM_Util.OrderAt(crew, pending.m_sWaypoint, airPoint, pending.m_fRadius);
			}

			SetAirPrecision(crew);
			return;
		}

		if (pending.m_sWaypoint == HAM_Units.WP_PATROL)
		{
			SetPace(crew, EMovementType.RUN);
			SetPace(pending.m_CrewExtra, EMovementType.RUN);
			float pace = HAM_AISettingsComponent.HAM_VehicleCruiseSpeed();
			if (pace <= 0)
				pace = 30;

			pending.m_Vehicle.HAM_SetCruiseSpeedOverride(pace);
		}

		if (pending.m_aRoute.IsEmpty())
		{
			vector spot = HAM_Util.RandomAround(pending.m_Vehicle.GetOrigin(), 80, 400);
			HAM_Util.SnapToNavmesh(crew, spot);
			HAM_Util.Order(crew, pending.m_sWaypoint, spot, pending.m_fRadius);
			return;
		}

		foreach (vector routePoint : pending.m_aRoute)
		{
			vector spot = routePoint;
			HAM_Util.SnapToNavmesh(crew, spot);
			HAM_Util.Order(crew, pending.m_sWaypoint, spot, pending.m_fRadius);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberLiveCrew(HAM_PendingCrew pending, SCR_AIGroup crew, bool lead)
	{
		if (!pending || !crew)
			return;

		if (!s_aLiveCrews)
			s_aLiveCrews = {};

		HAM_LiveCrew live = new HAM_LiveCrew();
		live.m_Group = crew;
		live.m_Vehicle = pending.m_Vehicle;
		live.m_bPatrol = lead && pending.m_sWaypoint == HAM_Units.WP_PATROL;
		live.m_bAir = lead && pending.m_bAir;
		live.m_bEnemy = pending.m_bEnemy;
		live.m_bCivilRoad = !pending.m_bEnemy && !pending.m_aInto;
		live.m_fRadius = pending.m_fRadius;
		live.m_sWaypoint = pending.m_sWaypoint;
		foreach (vector routePoint : pending.m_aRoute)
			live.m_aRoute.Insert(routePoint);

		s_aLiveCrews.Insert(live);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsLiveCrew(SCR_AIGroup group)
	{
		if (!group || !s_aLiveCrews)
			return false;

		foreach (HAM_LiveCrew live : s_aLiveCrews)
		{
			if (live && live.m_Group == group)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void UnlockCrewSlots(SCR_BaseCompartmentManagerComponent compartments)
	{
		if (!compartments)
			return;

		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (slot.GetType() == ECompartmentType.PILOT || slot.GetType() == ECompartmentType.TURRET || slot.GetType() == ECompartmentType.CARGO)
				slot.SetCompartmentAccessible(true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AbandonCrew(HAM_PendingCrew pending)
	{
		if (!pending)
			return;

		if (pending.m_Crew)
			HAM_Util.DeleteGroup(pending.m_Crew);

		if (pending.m_CrewExtra)
			HAM_Util.DeleteGroup(pending.m_CrewExtra);

		if (!pending.m_Vehicle || HAM_Util.IsDead(pending.m_Vehicle) || HasPlayerOccupant(pending.m_Vehicle))
			return;

		HAM_Util.Delete(pending.m_Vehicle);
	}

	//------------------------------------------------------------------------------------------------
	protected static int CrewSeatCount(SCR_BaseCompartmentManagerComponent compartments)
	{
		if (!compartments)
			return 0;

		int count = 0;
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (slot.GetType() == ECompartmentType.PILOT || slot.GetType() == ECompartmentType.TURRET)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	//! Les places d'équipage sont prises : les hommes du petit groupe encore à pied sont en trop.
	protected static void DropSpareCrew(SCR_AIGroup crew, int seats)
	{
		if (!crew || seats < 1)
			return;

		array<AIAgent> agents = {};
		crew.GetAgents(agents);
		int seated = 0;
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body)
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				seated++;
		}

		if (seated < seats)
			return;

		foreach (AIAgent extra : agents)
		{
			if (!extra)
				continue;

			IEntity body = extra.GetControlledEntity();
			if (!body)
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				continue;

			HAM_Util.Delete(body);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CrewSeated(SCR_BaseCompartmentManagerComponent compartments)
	{
		if (!compartments)
			return false;

		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		bool pilotSlot = false;
		bool pilotIn = false;
		bool anyIn = false;
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (slot.GetOccupant())
				anyIn = true;

			if (slot.GetType() != ECompartmentType.PILOT)
				continue;

			pilotSlot = true;
			if (slot.GetOccupant())
				pilotIn = true;
		}

		if (pilotSlot)
			return pilotIn;

		return anyIn;
	}

	//------------------------------------------------------------------------------------------------
	//! Les deux groupes du Humvee prennent des places distinctes dans le même passage.
	protected static void SeatGroups(Vehicle vehicle, SCR_BaseCompartmentManagerComponent compartments, SCR_AIGroup crew, SCR_AIGroup extra)
	{
		if (!vehicle || !compartments)
			return;

		UnlockCrewSlots(compartments);
		array<BaseCompartmentSlot> seats = {};
		CollectCrewSeats(compartments, seats);
		array<AIAgent> agents = {};
		AppendAgents(crew, agents);
		AppendAgents(extra, agents);
		int seat = 0;
		foreach (AIAgent agent : agents)
		{
			if (seat >= seats.Count())
				return;

			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body || HAM_Util.IsDead(body))
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (!access || access.IsInCompartment())
				continue;

			access.GetInVehicle(vehicle, seats[seat], true, -1, ECloseDoorAfterActions.INVALID, false);
			seat++;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendAgents(SCR_AIGroup crew, notnull array<AIAgent> agents)
	{
		if (!crew)
			return;

		array<AIAgent> listed = {};
		crew.GetAgents(listed);
		foreach (AIAgent agent : listed)
		{
			if (agent)
				agents.Insert(agent);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectCrewSeats(SCR_BaseCompartmentManagerComponent compartments, notnull array<BaseCompartmentSlot> seats)
	{
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		AppendSeatKind(slots, seats, ECompartmentType.PILOT);
		AppendSeatKind(slots, seats, ECompartmentType.TURRET);
		AppendSeatKind(slots, seats, ECompartmentType.CARGO);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendSeatKind(notnull array<BaseCompartmentSlot> slots, notnull array<BaseCompartmentSlot> seats, ECompartmentType kind)
	{
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot || slot.GetOccupant() || slot.GetType() != kind)
				continue;

			seats.Insert(slot);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! seatKind : 0 toutes les places, 1 pilotes, 2 tourelles.
	protected static void SeatAgents(Vehicle vehicle, SCR_BaseCompartmentManagerComponent compartments, SCR_AIGroup crew, int seatKind, bool instant)
	{
		if (!vehicle || !compartments || !crew)
			return;

		UnlockCrewSlots(compartments);
		array<AIAgent> agents = {};
		crew.GetAgents(agents);
		array<AIAgent> order = {};
		AIAgent leader;
		if (seatKind == 1)
			leader = crew.GetLeaderAgent();

		if (leader)
			order.Insert(leader);

		foreach (AIAgent listed : agents)
		{
			if (!listed || listed == leader)
				continue;

			order.Insert(listed);
		}

		foreach (AIAgent agent : order)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body || HAM_Util.IsDead(body))
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (!access || access.IsInCompartment())
				continue;

			BaseCompartmentSlot slot = FreeCrewSlot(compartments, seatKind);
			if (!slot)
				return;

			access.GetInVehicle(vehicle, slot, instant, -1, ECloseDoorAfterActions.INVALID, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static BaseCompartmentSlot FreeCrewSlot(SCR_BaseCompartmentManagerComponent compartments, int seatKind)
	{
		array<BaseCompartmentSlot> slots = {};
		if (seatKind != 2)
		{
			compartments.GetFreeCompartmentsOfType(slots, ECompartmentType.PILOT, false);
			if (!slots.IsEmpty())
				return slots[0];
		}

		if (seatKind != 1)
		{
			slots.Clear();
			compartments.GetFreeCompartmentsOfType(slots, ECompartmentType.TURRET, false);
			if (!slots.IsEmpty())
				return slots[0];
		}

		if (seatKind == 0 || seatKind == 1)
		{
			slots.Clear();
			compartments.GetFreeCompartmentsOfType(slots, ECompartmentType.CARGO, false);
			if (!slots.IsEmpty())
				return slots[0];
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static int BoardSeatCount(SCR_BaseCompartmentManagerComponent compartments)
	{
		if (!compartments)
			return 0;

		int count = 0;
		array<BaseCompartmentSlot> slots = {};
		compartments.GetCompartments(slots);
		foreach (BaseCompartmentSlot slot : slots)
		{
			if (!slot)
				continue;

			if (slot.GetType() == ECompartmentType.PILOT || slot.GetType() == ECompartmentType.TURRET || slot.GetType() == ECompartmentType.CARGO)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static int SeatedCount(SCR_AIGroup crew)
	{
		if (!crew)
			return 0;

		int seated = 0;
		array<AIAgent> agents = {};
		crew.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (!body)
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				seated++;
		}

		return seated;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropBodiesOnFoot(SCR_AIGroup crew)
	{
		if (!crew)
			return;

		array<AIAgent> agents = {};
		crew.GetAgents(agents);
		foreach (AIAgent extra : agents)
		{
			if (!extra)
				continue;

			IEntity body = extra.GetControlledEntity();
			if (!body)
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				continue;

			HAM_Util.Delete(body);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropUnseated(SCR_AIGroup crew, SCR_AIGroup extra, int seats)
	{
		if (seats < 1)
			return;

		int seated = SeatedCount(crew) + SeatedCount(extra);
		if (seated < seats)
			return;

		DropBodiesOnFoot(crew);
		DropBodiesOnFoot(extra);
	}

	//------------------------------------------------------------------------------------------------
	//! Course légère : le groupe ne part pas au sprint. La vitesse du véhicule reste le réglage de croisière.
	protected static void SetPace(SCR_AIGroup group, EMovementType pace)
	{
		if (!group)
			return;

		AIGroupMovementComponent movement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (movement)
			movement.SetGroupCharactersWantedMovementType(pace);

		SCR_AIGroupSettingsComponent settings = SCR_AIGroupSettingsComponent.Cast(group.FindComponent(SCR_AIGroupSettingsComponent));
		if (settings)
			settings.AddSetting(SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.SCENARIO, pace), false, true);
	}

	//------------------------------------------------------------------------------------------------
	//! Rotor cassé ou moteur hors service. Le démarrage au sol ne compte pas : le moteur y est encore sain.
	protected static bool HeliLiftGone(Vehicle vehicle)
	{
		if (!vehicle)
			return false;

		VehicleHelicopterSimulation sim = VehicleHelicopterSimulation.Cast(vehicle.FindComponent(VehicleHelicopterSimulation));
		if (sim)
		{
			int rotors = sim.RotorCount();
			for (int rotor = 0; rotor < rotors; rotor++)
			{
				if (sim.RotorGetState(rotor) == RotorState.DESTROYED)
					return true;
			}
		}

		SCR_VehicleDamageManagerComponent damage = SCR_VehicleDamageManagerComponent.Cast(vehicle.FindComponent(SCR_VehicleDamageManagerComponent));
		if (damage)
		{
			if (damage.GetState() == EDamageState.DESTROYED || !damage.GetEngineFunctional())
				return true;

			if (HeliPartDestroyed(damage))
				return true;
		}

		// Impact qui coupe le moteur sans passer la zone à « détruit ». Le démarrage au sol, moteur sain, ne compte pas.
		if (!sim || sim.EngineIsOn() || sim.GetAltitudeAGL() <= 8)
			return false;

		if (!damage)
			return false;

		return damage.GetState() != EDamageState.UNDAMAGED;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HeliPartDestroyed(SCR_VehicleDamageManagerComponent damage)
	{
		array<HitZone> zones = {};
		damage.GetAllHitZonesInHierarchy(zones);
		foreach (HitZone zone : zones)
		{
			if (!zone || zone.GetDamageState() != EDamageState.DESTROYED)
				continue;

			if (SCR_RotorHitZone.Cast(zone) || SCR_EngineHitZone.Cast(zone))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Coupe le cadre du pilote automatique sans reprendre son code. La physique du jeu reprend l'appareil.
	protected static void CutHeliAutopilot(HAM_LiveCrew live)
	{
		if (!live || !live.m_Vehicle)
			return;

		array<Managed> components = {};
		live.m_Vehicle.FindComponents(GenericComponent, components);
		foreach (Managed item : components)
		{
			GenericComponent component = GenericComponent.Cast(item);
			if (!component)
				continue;

			string componentName = component.ClassName();
			if (!componentName.Contains("HeliAutopilot"))
				continue;

			component.ClearEventMask(live.m_Vehicle, EntityEvent.FRAME);
		}

		VehicleHelicopterSimulation sim = VehicleHelicopterSimulation.Cast(live.m_Vehicle.FindComponent(VehicleHelicopterSimulation));
		if (sim && sim.EngineIsOn())
			sim.EngineStop();

		Physics physics = live.m_Vehicle.GetPhysics();
		if (physics)
		{
			vector velocity = physics.GetVelocity();
			if (velocity[1] > -1)
				velocity[1] = -2;

			physics.SetVelocity(velocity);
		}

		HAM_Util.ClearWaypoints(live.m_Group);

		Print("[13RDPA H&M] Hélicoptère hors de vol : rotor ou moteur détruit, patrouille coupée", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReissueAirRoute(HAM_LiveCrew live)
	{
		if (!live || !live.m_Group)
			return;

		ClaimVehicle(live.m_Group, live.m_Vehicle);
		foreach (vector airPoint : live.m_aRoute)
		{
			HAM_Util.OrderAt(live.m_Group, live.m_sWaypoint, airPoint, live.m_fRadius);
		}

		SetAirPrecision(live.m_Group);
	}

	//------------------------------------------------------------------------------------------------
	//! Le groupe ne pilote que les véhicules qu'il a enregistrés.
	protected static void ClaimVehicle(SCR_AIGroup crew, Vehicle vehicle)
	{
		if (!crew || !vehicle)
			return;

		SCR_AIGroupUtilityComponent utility = crew.GetGroupUtilityComponent();
		SCR_AIVehicleUsageComponent usage = SCR_AIVehicleUsageComponent.Cast(vehicle.FindComponent(SCR_AIVehicleUsageComponent));
		if (!utility || !usage)
			return;

		utility.AddUsableVehicle(usage);
	}

	//------------------------------------------------------------------------------------------------
	//! L'arrivée se juge à l'horizontale : la hauteur du waypoint n'a pas à être exacte.
	protected static void SetAirPrecision(SCR_AIGroup crew)
	{
		if (!crew)
			return;

		array<AIWaypoint> waypoints = {};
		crew.GetWaypoints(waypoints);
		foreach (AIWaypoint waypoint : waypoints)
		{
			if (waypoint)
				waypoint.SetCompletionYPrecision(80);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Hélicoptère au sol. Un groupe transport prend les places pilote, l'autre les tourelles.
	static Vehicle SpawnAirPatrol(vector pos, notnull array<vector> route, float radius)
	{
		Ensure();
		Vehicle vehicle = Vehicle.Cast(HAM_Util.Spawn(HAM_Units.VEHICLE_UH1H, pos + Vector(0, 0.5, 0), Math.RandomFloat(0, 360)));
		if (!vehicle)
			return null;

		s_mScriptVehicles.Set(vehicle, true);
		vehicle.SetFlags(EntityFlags.ACTIVE, false);
		vector beside = pos + Vector(6, 0, 0);
		SCR_AIGroup pilots = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_TRANSPORT, beside, 0);
		SCR_AIGroup gunners = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_TRANSPORT, beside + Vector(0, 0, 4), 0);
		if (!pilots)
		{
			HAM_Util.Delete(vehicle);
			if (gunners)
				HAM_Util.DeleteGroup(gunners);

			return null;
		}

		HAM_PendingCrew pending = new HAM_PendingCrew();
		pending.m_Vehicle = vehicle;
		pending.m_sWaypoint = HAM_Units.WP_DEFEND;
		pending.m_fRadius = radius;
		pending.m_bEnemy = true;
		pending.m_bAir = true;
		pending.m_fSince = HAM_Util.Now();
		pending.m_iSpawnTries = 1;
		pending.m_Crew = pilots;
		pending.m_CrewExtra = gunners;
		pending.m_iCrewSeats = 0;
		foreach (vector airPoint : route)
		{
			pending.m_aRoute.Insert(airPoint);
		}

		s_aPendingCrews.Insert(pending);
		RegisterEnemy(pilots);
		if (gunners)
			RegisterEnemy(gunners);

		return vehicle;
	}

	//------------------------------------------------------------------------------------------------
	static SCR_AIGroup CrewGroup(SCR_BaseCompartmentManagerComponent compartments)
	{
		array<IEntity> occupants = {};
		compartments.GetOccupants(occupants);
		foreach (IEntity occupant : occupants)
		{
			if (!occupant)
				continue;

			AIControlComponent control = AIControlComponent.Cast(occupant.FindComponent(AIControlComponent));
			if (!control)
				continue;

			AIAgent agent = control.GetControlAIAgent();
			if (!agent)
				continue;

			SCR_AIGroup group = SCR_AIGroup.Cast(agent.GetParentGroup());
			if (group)
				return group;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Vrai si l'équipage d'un véhicule de la liste est encore en création.
	static bool IsCrewPending(Vehicle vehicle)
	{
		Ensure();
		foreach (HAM_PendingCrew pending : s_aPendingCrews)
		{
			if (pending.m_Vehicle == vehicle)
				return true;
		}

		return false;
	}
}
