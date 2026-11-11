//------------------------------------------------------------------------------------------------
// Outils serveur partagés : temps, sol, spawn de prefabs, groupes IA, joueurs, routes.
//------------------------------------------------------------------------------------------------

class HAM_Util
{
	//------------------------------------------------------------------------------------------------
	//! Trace détaillée du front, seulement avec "debug": true dans le JSON de réglages.
	static void Debug(string message)
	{
		if (HAM_Settings.Get().m_bDebug)
			Print("[13RDPA H&M debug] " + message, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Ce qui naît sur le front (garnisons, patrouilles, CQB, véhicules, engins), toujours journalisé.
	static void Info(string message)
	{
		Print("[13RDPA H&M] " + message, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Eau au-dessus du sol en ce point (mer, lac, rivière), sondée à plusieurs hauteurs comme le Conflit :
	//! un lac a un volume bas qu'une sonde trop haute ne voit pas.
	static bool IsWater(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		float ground = world.GetSurfaceY(pos[0], pos[2]);
		return WaterCovers(world, pos, ground, 2) || WaterCovers(world, pos, ground, 12) || WaterCovers(world, pos, ground, 40) || WaterCovers(world, pos, ground, 120);
	}

	//------------------------------------------------------------------------------------------------
	//! Véhicules : le point et un anneau de 28 m autour doivent être au sec.
	static bool IsNearWater(vector pos)
	{
		if (IsWater(pos))
			return true;

		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			if (IsWater(pos + dir * 28.0))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool WaterCovers(BaseWorld world, vector pos, float ground, float lift)
	{
		vector probe = pos;
		probe[1] = ground + lift;
		vector surface;
		EWaterSurfaceType waterType;
		vector transformWS[4];
		vector obb;
		if (!ChimeraWorldUtils.TryGetWaterSurface(world, probe, surface, waterType, transformWS, obb))
			return false;

		return surface[1] > ground - 0.4;
	}

	protected static int s_iDrySlot;

	//------------------------------------------------------------------------------------------------
	//! Point au sec autour du centre, entre minDist et maxDist.
	//! Si tout est dans l'eau, un cran d'anneau : deux échecs de suite ne retombent pas au même point.
	static vector DryAround(vector center, float minDist, float maxDist, int tries = 12)
	{
		for (int i = 0; i < tries; i++)
		{
			vector candidate = RandomAround(center, minDist, maxDist);
			if (!IsWater(candidate))
				return candidate;
		}

		if (!IsWater(center))
			return center;

		int slot = s_iDrySlot;
		s_iDrySlot = s_iDrySlot + 1;
		if (s_iDrySlot > 48)
			s_iDrySlot = 0;

		float inner = minDist;
		if (inner < 2)
			inner = 2;

		return RingAround(center, slot, 2, 0, inner);
	}

	//------------------------------------------------------------------------------------------------
	//! Anneau autour du centre. Chaque cran est à au moins gap mètres du précédent, jamais sur le centre.
	static vector RingAround(vector center, int slot, float gap, float spin, float firstRadius)
	{
		if (gap < 2)
			gap = 2;

		if (firstRadius < gap)
			firstRadius = gap;

		if (slot < 0)
			slot = 0;

		float radius = firstRadius;
		int room = SeatsOnRing(radius, gap);
		int left = slot;
		int guard = 0;
		while (left >= room && guard < 12)
		{
			left = left - room;
			radius = radius + gap;
			room = SeatsOnRing(radius, gap);
			guard = guard + 1;
		}

		float angle = spin + (Math.PI2 * left / room);
		vector pos = Ground(center + Vector(Math.Cos(angle) * radius, 0, Math.Sin(angle) * radius));
		if (IsWater(pos))
			pos = Ground(center + Vector(Math.Cos(angle) * (radius + gap), 0, Math.Sin(angle) * (radius + gap)));

		return pos;
	}

	//------------------------------------------------------------------------------------------------
	protected static int SeatsOnRing(float radius, float gap)
	{
		if (gap < 2)
			gap = 2;

		float circ = Math.PI2 * radius;
		int room = 4;
		float used = gap * 4;
		while (used + gap <= circ && room < 24)
		{
			room = room + 1;
			used = used + gap;
		}

		return room;
	}

	//------------------------------------------------------------------------------------------------
	static void PlaceUpright(IEntity body, vector pos)
	{
		if (!body)
			return;

		vector mat[4];
		body.GetWorldTransform(mat);
		mat[3] = Ground(pos);
		body.SetWorldTransform(mat);
	}

	protected static ref array<IEntity> s_aRingBodies;

	//------------------------------------------------------------------------------------------------
	//! Décale les personnages posés sur une origine commune (habitacle, capot), pas ceux déjà à l'écart ni assis.
	static int SeparateNearOrigin(vector origin, float standOff, float gap)
	{
		s_aRingBodies = {};
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		float reach = standOff + 3;
		world.QueryEntitiesBySphere(origin, reach, SeeRingBody);
		int slot = 0;
		int moved = 0;
		foreach (IEntity body : s_aRingBodies)
		{
			if (!body || IsDead(body))
				continue;

			CompartmentAccessComponent access = CompartmentAccessComponent.Cast(body.FindComponent(CompartmentAccessComponent));
			if (access && access.IsInCompartment())
				continue;

			if (vector.DistanceXZ(body.GetOrigin(), origin) > 2.5)
				continue;

			PlaceUpright(body, RingAround(origin, slot, gap, 0.4, standOff));
			slot = slot + 1;
			moved = moved + 1;
		}

		return moved;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeRingBody(IEntity ent)
	{
		if (!ent)
			return true;

		ChimeraCharacter character = ChimeraCharacter.Cast(ent);
		if (!character)
			return true;

		PlayerManager players = GetGame().GetPlayerManager();
		if (players && players.GetPlayerIdFromControlledEntity(ent) > 0)
			return true;

		if (!s_aRingBodies)
			s_aRingBodies = {};

		s_aRingBodies.Insert(ent);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Ramène une position sur le navmesh du groupe ; faux s'il n'y a pas de point proche.
	//! Sans pathfinding ou tuile pas chargée : on ne bloque pas.
	static bool SnapToNavmesh(SCR_AIGroup group, inout vector pos, float extentXZ = 20.0, float extentY = 12.0)
	{
		if (!group)
			return true;

		AIPathfindingComponent path = AIPathfindingComponent.Cast(group.FindComponent(AIPathfindingComponent));
		if (!path)
			return true;

		NavmeshWorldComponent navmesh = path.GetNavmeshComponent();
		if (navmesh && !navmesh.IsTileLoaded(pos))
			return true;

		vector navPt;
		if (!path.GetClosestPositionOnNavmesh(pos, Vector(extentXZ, extentY, extentXZ), navPt))
			return false;

		pos = navPt;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static string PrefabFileName(ResourceName prefab)
	{
		string path = prefab;
		int slash = path.LastIndexOf("/");
		return path.Substring(slash + 1, path.Length() - slash - 1);
	}

	//------------------------------------------------------------------------------------------------
	//! Temps du monde en secondes.
	static float Now()
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetWorldTime() * 0.001;
	}

	//------------------------------------------------------------------------------------------------
	static float SurfaceY(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return pos[1];

		return world.GetSurfaceY(pos[0], pos[2]);
	}

	//------------------------------------------------------------------------------------------------
	static vector Ground(vector pos)
	{
		pos[1] = SurfaceY(pos);
		return pos;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector s_vSite;
	protected static float s_fSiteClear;
	protected static bool s_bSiteBlocked;
	protected static int s_iSiteTrees;

	//------------------------------------------------------------------------------------------------
	//! Sol pour un prefab : au sec, pente sous 6°, hors forêt dense, hors bâtiment.
	//! Cherche autour du point voulu, jusqu'à 48 m. Faux si rien ne convient.
	static bool FindSite(vector preferred, float footprint, out vector spot)
	{
		spot = Ground(preferred);
		if (footprint < 4)
			footprint = 4;

		if (SiteOk(spot, footprint))
			return true;

		for (float ring = 6; ring <= 48; ring += 6)
		{
			for (int step = 0; step < 8; step++)
			{
				vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
				vector candidate = Ground(preferred + dir * ring);
				if (!SiteOk(candidate, footprint))
					continue;

				spot = candidate;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Clairière IED : sec, aucun arbre dans 20 m, pente sous environ 6°. Jusqu'à 120 m.
	static bool FindIedClearing(vector preferred, out vector spot)
	{
		spot = Ground(preferred);
		if (IedClearing(spot))
			return true;

		for (float ring = 16; ring <= 120; ring += 16)
		{
			for (int step = 0; step < 8; step++)
			{
				vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
				vector candidate = Ground(preferred + dir * ring);
				if (!IedClearing(candidate))
					continue;

				spot = candidate;
				return true;
			}
		}

		return false;
	}

	protected static bool IedClearing(vector pos)
	{
		if (IedWet(pos))
			return false;

		if (IedSlope(pos) > 0.10)
			return false;

		return !IedHasTree(pos);
	}

	protected static bool IedWet(vector pos)
	{
		if (IsWater(pos))
			return true;

		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			if (IsWater(pos + dir * 20))
				return true;
		}

		return false;
	}

	protected static float IedSlope(vector pos)
	{
		float ground = SurfaceY(pos);
		float worst = 0;
		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			float rise = Math.AbsFloat(SurfaceY(pos + dir * 20) - ground) / 20;
			if (rise > worst)
				worst = rise;
		}

		return worst;
	}

	//! Aire d'hélicoptère : sec, pente faible, aucun arbre dans 50 m.
	static bool FindHeliPad(vector center, out vector spot)
	{
		spot = Ground(center);
		if (HeliPadOk(spot))
			return true;

		for (float ring = 40; ring <= 480; ring += 40)
		{
			for (int step = 0; step < 12; step++)
			{
				float angle = Math.PI2 * step / 12;
				vector candidate = Ground(center + Vector(Math.Cos(angle) * ring, 0, Math.Sin(angle) * ring));
				if (!HeliPadOk(candidate))
					continue;

				spot = candidate;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HeliPadOk(vector pos)
	{
		if (IsNearWater(pos))
			return false;

		if (SlopeAt(pos, 24) > 0.10)
			return false;

		if (!OutOfBuildings(pos))
			return false;

		return !HeliHasTree(pos);
	}

	//------------------------------------------------------------------------------------------------
	protected static int s_iHeliTrees;

	protected static bool HeliHasTree(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return true;

		s_iHeliTrees = 0;
		world.QueryEntitiesBySphere(pos, 50, HeliCountTree);
		return s_iHeliTrees > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HeliCountTree(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		string name = ent.GetPrefabData().GetPrefabName();
		name.ToLower();
		if (!name.Contains("tree") && !name.Contains("pine") && !name.Contains("spruce") && !name.Contains("oak") && !name.Contains("birch"))
			return true;

		s_iHeliTrees++;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IedHasTree(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return true;

		s_iSiteTrees = 0;
		world.QueryEntitiesBySphere(pos, 20, IedCountTree);
		return s_iSiteTrees > 0;
	}

	protected static bool IedCountTree(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		string name = ent.GetPrefabData().GetPrefabName();
		name.ToLower();
		if (!name.Contains("tree") && !name.Contains("pine") && !name.Contains("spruce") && !name.Contains("oak") && !name.Contains("birch"))
			return true;

		s_iSiteTrees++;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteOk(vector pos, float footprint)
	{
		if (SiteWet(pos, footprint))
			return false;

		if (SiteSteep(pos, footprint))
			return false;

		if (SiteForest(pos, footprint))
			return false;

		if (SiteBuilding(pos, footprint))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteWet(vector pos, float footprint)
	{
		if (IsWater(pos))
			return true;

		float reach = footprint * 0.5;
		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			if (IsWater(pos + dir * reach))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! 0,10 m de dénivelé par mètre : environ 6°. Au-delà, un préfab de zone se pose de travers.
	protected static bool SiteSteep(vector pos, float footprint)
	{
		return SlopeAt(pos, footprint) > 0.10;
	}

	//------------------------------------------------------------------------------------------------
	protected static float SlopeAt(vector pos, float footprint)
	{
		float reach = Math.Max(4, footprint * 0.5);
		float ground = SurfaceY(pos);
		float worst = 0;
		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			float rise = Math.AbsFloat(SurfaceY(pos + dir * reach) - ground) / reach;
			if (rise > worst)
				worst = rise;
		}

		return worst;
	}

	//------------------------------------------------------------------------------------------------
	//! Le moins pentu dans les 48 m, même si aucun n'est vraiment plat.
	static vector Flattest(vector preferred, float footprint)
	{
		vector best = Ground(preferred);
		float bestSlope = SlopeAt(best, footprint);
		for (float ring = 6; ring <= 48; ring += 6)
		{
			for (int step = 0; step < 8; step++)
			{
				vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
				vector candidate = Ground(preferred + dir * ring);
				if (SiteWet(candidate, footprint))
					continue;

				float slope = SlopeAt(candidate, footprint);
				if (slope >= bestSlope)
					continue;

				bestSlope = slope;
				best = candidate;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteForest(vector pos, float footprint)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		s_iSiteTrees = 0;
		s_vSite = pos;
		world.QueryEntitiesBySphere(pos, Math.Max(12, footprint), SiteCountTree);
		return s_iSiteTrees >= 4;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteCountTree(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		string name = ent.GetPrefabData().GetPrefabName();
		name.ToLower();
		if (!name.Contains("tree") && !name.Contains("pine") && !name.Contains("spruce") && !name.Contains("oak") && !name.Contains("birch") && !name.Contains("bush") && !name.Contains("vegetation") && !name.Contains("forest"))
			return true;

		s_iSiteTrees++;
		return s_iSiteTrees < 4;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteBuilding(vector pos, float footprint)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		s_bSiteBlocked = false;
		s_vSite = pos;
		s_fSiteClear = footprint;
		world.QueryEntitiesBySphere(pos, footprint + 40, SiteSeeBuilding);
		return s_bSiteBlocked;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SiteSeeBuilding(IEntity ent)
	{
		SCR_DestructibleBuildingEntity building = SCR_DestructibleBuildingEntity.Cast(ent);
		if (!building)
			return true;

		vector mins;
		vector maxs;
		building.GetBounds(mins, maxs);
		float half = Math.Max(maxs[0] - mins[0], maxs[2] - mins[2]) * 0.5;
		if (vector.DistanceXZ(s_vSite, building.GetOrigin()) < half + s_fSiteClear)
		{
			s_bSiteBlocked = true;
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Point libre sur le terrain près du centre ; le centre au sol si rien n'est trouvé.
	static vector FreeSpot(vector center, float radius)
	{
		vector spot;
		if (SCR_WorldTools.FindEmptyTerrainPosition(spot, center, radius, 1.5, 2.5))
			return spot;

		return Ground(center);
	}

	protected static bool s_bArrivalBlocked;
	protected static vector s_vArrival;
	protected static float s_fArrivalClear = 7;

	//------------------------------------------------------------------------------------------------
	//! Arrivée de téléport : autour de la zone, jamais dans son préfab, sur un sol sec et sans collision.
	static vector SafeArrival(vector target)
	{
		vector center = Ground(target);
		for (int ring = 0; ring < 3; ring++)
		{
			float dist = 14;
			if (ring == 1)
				dist = 24;
			else if (ring == 2)
				dist = 36;

			for (int step = 0; step < 16; step++)
			{
				float angle = Math.PI2 * step / 16;
				vector candidate = Ground(center + Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist));
				if (IsNearWater(candidate))
					continue;

				if (!StandClear(candidate))
					continue;

				candidate[1] = candidate[1] + 0.15;
				return candidate;
			}
		}

		vector fallback = RingAround(center, 3, 2.5, 0.7, 30);
		if (IsNearWater(fallback) || !StandClear(fallback))
			fallback = FreeSpot(center + Vector(30, 0, 0), 16);

		fallback[1] = fallback[1] + 0.15;
		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	//! Le volume d'un joueur debout ne traverse ni le terrain, ni un préfab de zone.
	protected static bool StandClear(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		if (BodyTraceBlocked(world, pos + Vector(0, 0.35, 0), pos + Vector(0, 2.05, 0)))
			return false;

		vector chest = pos + Vector(0, 1.1, 0);
		if (BodyTraceBlocked(world, chest, chest + Vector(0.7, 0, 0)))
			return false;

		if (BodyTraceBlocked(world, chest, chest + Vector(-0.7, 0, 0)))
			return false;

		if (BodyTraceBlocked(world, chest, chest + Vector(0, 0, 0.7)))
			return false;

		if (BodyTraceBlocked(world, chest, chest + Vector(0, 0, -0.7)))
			return false;

		s_bArrivalBlocked = false;
		s_vArrival = pos;
		s_fArrivalClear = 7;
		world.QueryEntitiesBySphere(pos, 7, SeeArrivalPrefab);
		return !s_bArrivalBlocked;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool BodyTraceBlocked(BaseWorld world, vector from, vector to)
	{
		TraceParam trace = new TraceParam();
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Start = from;
		trace.End = to;
		float hit = world.TraceMove(trace, null);
		return hit > 0.02 && hit < 0.98;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeArrivalPrefab(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		string name = ent.GetPrefabData().GetPrefabName();
		bool zonePrefab = name.Contains("Headquarters_") || name.Contains("LivingArea_") || name.Contains("FuelStorage_") || name.Contains("SupplyCache_") || name.Contains("Tent_") || name.Contains("Storage_Supplies") || name.Contains("CanvasCover") || name.Contains("Sandbag_") || name.Contains("BuildingService");
		if (!zonePrefab)
			return true;

		if (vector.DistanceXZ(ent.GetOrigin(), s_vArrival) < s_fArrivalClear)
		{
			s_bArrivalBlocked = true;
			return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Emplacement de camion : sol sec, peu pentu, hors bâtiment et hors préfab de zone.
	//! Le centre n'est gardé que s'il passe ces tests.
	static vector ParkSpot(vector preferred)
	{
		vector center = Ground(preferred);
		vector found;
		if (FindPark(center, found))
			return found;

		vector shifted = DryAround(center, 24, 80, 16);
		if (FindPark(shifted, found))
			return found;

		if (!IsNearWater(shifted) && !SiteBuilding(shifted, 8))
		{
			shifted[1] = shifted[1] + 0.2;
			return shifted;
		}

		for (int step = 0; step < 8; step++)
		{
			vector dir = Vector(step * 45.0, 0, 0).AnglesToVector();
			vector candidate = Ground(center + dir * 70);
			if (IsNearWater(candidate) || SiteBuilding(candidate, 8))
				continue;

			candidate[1] = candidate[1] + 0.2;
			return candidate;
		}

		vector away = DryAround(center, 40, 120, 16);
		if (IsNearWater(away))
			away = DryAround(center + Vector(0, 0, 90), 20, 60, 8);

		away[1] = away[1] + 0.2;
		return away;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindPark(vector center, out vector spot)
	{
		spot = center;
		if (ParkOk(center))
		{
			spot[1] = spot[1] + 0.2;
			return true;
		}

		for (int ring = 0; ring < 4; ring++)
		{
			float dist = 18;
			if (ring == 1)
				dist = 30;
			else if (ring == 2)
				dist = 44;
			else if (ring == 3)
				dist = 60;

			for (int step = 0; step < 16; step++)
			{
				float angle = Math.PI2 * step / 16;
				vector candidate = Ground(center + Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist));
				if (!ParkOk(candidate))
					continue;

				spot = candidate;
				spot[1] = spot[1] + 0.2;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ParkOk(vector pos)
	{
		if (IsNearWater(pos))
			return false;

		if (SiteSteep(pos, 10))
			return false;

		if (SiteBuilding(pos, 8))
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		s_bArrivalBlocked = false;
		s_vArrival = pos;
		s_fArrivalClear = 12;
		world.QueryEntitiesBySphere(pos, 12, SeeArrivalPrefab);
		return !s_bArrivalBlocked;
	}

	//------------------------------------------------------------------------------------------------
	//! Dehors, au sol, pas dans un bâtiment ni dessous. L'angle est tiré au hasard, pas en arc.
	static vector CivilianSpot(vector center, float minDist, float maxDist)
	{
		float inner = minDist;
		if (inner < 6)
			inner = 6;

		float outer = maxDist;
		if (outer < inner + 4)
			outer = inner + 4;

		float span = outer - inner;
		for (int i = 0; i < 16; i++)
		{
			float angle = Math.RandomFloat(0, Math.PI2);
			float dist = inner + Math.RandomFloat(0, span);
			vector pos = Ground(center + Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist));
			if (IsNearWater(pos) || !OutOfBuildings(pos))
				continue;

			return pos;
		}

		vector fallback = DryAround(center, outer, outer + 40, 10);
		if (!IsNearWater(fallback) && OutOfBuildings(fallback))
			return fallback;

		return center;
	}

	//------------------------------------------------------------------------------------------------
	static bool OutOfBuildings(vector pos)
	{
		if (IsWater(pos))
			return false;

		if (SiteBuilding(pos, 1))
			return false;

		if (HAM_Interior.SpotEmbedded(pos))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static vector RandomAround(vector center, float minRadius, float maxRadius)
	{
		for (int tryIdx = 0; tryIdx < 8; tryIdx++)
		{
			float angle = Math.RandomFloat(0, Math.PI2);
			float dist = Math.RandomFloat(minRadius, maxRadius);
			vector pos = Ground(center + Vector(Math.Cos(angle) * dist, 0, Math.Sin(angle) * dist));
			if (!IsWater(pos))
				return pos;
		}

		return Ground(center);
	}

	//------------------------------------------------------------------------------------------------
	static IEntity Spawn(ResourceName prefab, vector pos, float yaw = 0)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print("[13RDPA H&M] Prefab introuvable : " + prefab, LogLevel.WARNING);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.AnglesToMatrix(Vector(yaw, 0, 0), params.Transform);
		params.Transform[3] = pos;

		IEntity spawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		Forget(spawned);
		return spawned;
	}

	//------------------------------------------------------------------------------------------------
	//! Préfab de zone : sol le plus plat, posé droit. Les pièces ne suivent pas la pente.
	static IEntity SpawnLevelled(ResourceName prefab, vector pos, float yaw = 0)
	{
		HAM_CompositionAlign.Begin();
		IEntity spawned = Spawn(prefab, pos, yaw);
		HAM_CompositionAlign.End();
		LevelUpright(spawned);
		return spawned;
	}

	//------------------------------------------------------------------------------------------------
	static void LevelUpright(IEntity ent)
	{
		if (!ent)
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		vector transform[4];
		ent.GetWorldTransform(transform);
		vector angles = Math3D.MatrixToAngles(transform);
		angles[1] = 0;
		angles[2] = 0;
		Math3D.AnglesToMatrix(angles, transform);
		vector at = transform[3];
		at[1] = world.GetSurfaceY(at[0], at[2]);
		transform[3] = at;
		ent.SetWorldTransform(transform);
	}

	//------------------------------------------------------------------------------------------------
	//! Retire l'entité et ses enfants de la sauvegarde monde : le front a sa propre sauvegarde.
	//! Le jeu peut n'inscrire l'entité qu'un peu après la pose. Un second passage l'enlève quand même.
	static void Forget(IEntity ent)
	{
		DropTracked(ent);
		if (!ent || !GetGame() || !GetGame().GetCallqueue())
			return;

		GetGame().GetCallqueue().CallLater(DropTracked, 2000, false, ent);
	}

	//------------------------------------------------------------------------------------------------
	static void DropTracked(IEntity ent)
	{
		if (!ent || !Replication.IsServer())
			return;

		PersistenceSystem system = PersistenceSystem.GetInstance();
		if (system && system.IsTracked(ent))
			system.StopTracking(ent, true);

		IEntity child = ent.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			DropTracked(child);
			child = next;
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Delete(IEntity ent)
	{
		if (ent)
			SCR_EntityHelper.DeleteEntityAndChildren(ent);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDead(IEntity ent)
	{
		if (!ent)
			return true;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(ent.FindComponent(CharacterControllerComponent));
		if (controller)
			return controller.IsDead();

		DamageManagerComponent damage = DamageManagerComponent.Cast(ent.FindComponent(DamageManagerComponent));
		if (damage)
			return damage.GetState() == EDamageState.DESTROYED;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static string FactionKey(IEntity ent)
	{
		if (!ent)
			return string.Empty;

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return string.Empty;

		Faction faction = affiliation.GetAffiliatedFaction();
		if (!faction)
			return string.Empty;

		return faction.GetFactionKey();
	}

	//------------------------------------------------------------------------------------------------
	static Faction FactionByKey(string key)
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return null;

		return manager.GetFactionByKey(key);
	}

	//------------------------------------------------------------------------------------------------
	static int PlayerIdOf(IEntity ent)
	{
		if (!ent)
			return 0;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return 0;

		return players.GetPlayerIdFromControlledEntity(ent);
	}

	//------------------------------------------------------------------------------------------------
	//! Un personnage tenu par un joueur. La caméra Zeus n'en est pas un : elle ne compte ni pour le spawn, ni pour le despawn.
	static bool IsPlayerBody(IEntity body)
	{
		if (!body || IsDead(body))
			return false;

		return CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent)) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Joueurs vivants qui contrôlent un personnage.
	static void AlivePlayers(notnull array<int> ids, notnull array<IEntity> bodies)
	{
		ids.Clear();
		bodies.Clear();
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> all = {};
		players.GetPlayers(all);
		foreach (int playerId : all)
		{
			IEntity body = players.GetPlayerControlledEntity(playerId);
			if (!IsPlayerBody(body))
				continue;

			ids.Insert(playerId);
			bodies.Insert(body);
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool AnyNear(notnull array<IEntity> bodies, vector pos, float radius)
	{
		float radiusSq = radius * radius;
		foreach (IEntity body : bodies)
		{
			if (body && vector.DistanceSqXZ(body.GetOrigin(), pos) <= radiusSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static int CountNear(notnull array<IEntity> bodies, vector pos, float radius)
	{
		int count = 0;
		float radiusSq = radius * radius;
		foreach (IEntity body : bodies)
		{
			if (body && vector.DistanceSqXZ(body.GetOrigin(), pos) <= radiusSq)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	static float NearestDistance(notnull array<IEntity> bodies, vector pos)
	{
		float best = float.MAX;
		foreach (IEntity body : bodies)
		{
			if (!body)
				continue;

			float dist = vector.DistanceXZ(body.GetOrigin(), pos);
			if (dist < best)
				best = dist;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static AIWaypoint SpawnWaypoint(ResourceName prefab, vector pos, float radius)
	{
		AIWaypoint waypoint = AIWaypoint.Cast(Spawn(prefab, Ground(pos)));
		if (waypoint && radius > 0)
			waypoint.SetCompletionRadius(radius);

		return waypoint;
	}

	//------------------------------------------------------------------------------------------------
	//! Groupe IA depuis un prefab de groupe, limité à maxUnits membres.
	static SCR_AIGroup SpawnGroup(ResourceName prefab, vector pos, int maxUnits)
	{
		if (prefab.IsEmpty() || maxUnits < 1)
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print("[13RDPA H&M] Groupe introuvable : " + prefab, LogLevel.WARNING);
			return null;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = Ground(pos);

		SCR_AIGroup.IgnoreSpawning(true);
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);
		if (!group)
			return null;

		group.SetMaxUnitsToSpawn(maxUnits);
		// 1.9 : un groupe sans importance est LOW (moitié de la limite, et la file peut le dissoudre).
		// H&M les faisait vivre dans la limite entière. La politique reste Manual, comme avant.
		group.SetImportance(SCR_EAISpawnImportance.CRITICAL);
		group.SpawnUnits();
		ApplyGroupSkill(group, prefab);
		KeepAwake(group);
		Forget(group);
		GetGame().GetCallqueue().CallLater(ForgetGroup, 2500, false, group);
		return group;
	}

	//------------------------------------------------------------------------------------------------
	//! Groupe entier. budget > 0 et plus grand que le préfab : rien ne naît, le groupe n'est pas coupé.
	static SCR_AIGroup SpawnFullGroup(ResourceName prefab, vector pos, int budget = 0, bool onGround = true)
	{
		if (prefab.IsEmpty())
			return null;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print("[13RDPA H&M] Groupe introuvable : " + prefab, LogLevel.WARNING);
			return null;
		}

		vector at = pos;
		if (onGround)
			at = Ground(pos);

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = at;

		SCR_AIGroup.IgnoreSpawning(true);
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		SCR_AIGroup.IgnoreSpawning(false);
		if (!group)
			return null;

		int slots = 1;
		if (group.m_aUnitPrefabSlots)
			slots = group.m_aUnitPrefabSlots.Count();

		if (slots < 1)
			slots = 1;

		if (budget > 0 && slots > budget)
		{
			DeleteGroup(group);
			return null;
		}

		group.SetMaxUnitsToSpawn(slots);
		group.SetImportance(SCR_EAISpawnImportance.CRITICAL);
		group.SpawnUnits();
		ApplyGroupSkill(group, prefab);
		KeepAwake(group);
		Forget(group);
		GetGame().GetCallqueue().CallLater(ForgetGroup, 2500, false, group);
		return group;
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyGroupSkill(SCR_AIGroup group, ResourceName prefab)
	{
		ApplySkillValue(group, HAM_Units.SkillOf(prefab));
	}

	//------------------------------------------------------------------------------------------------
	static void ApplySkillValue(SCR_AIGroup group, float skill)
	{
		if (!group || skill < 0)
			return;

		array<Managed> found = {};
		group.FindComponents(SCR_AIConfigComponent, found);
		foreach (Managed item : found)
		{
			SCR_AIConfigComponent config = SCR_AIConfigComponent.Cast(item);
			if (config)
				config.m_Skill = skill;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Le groupe et ses hommes restent simulés loin de la caméra. Le LOD max les fige sinon.
	static void KeepAwake(SCR_AIGroup group)
	{
		if (!group)
			return;

		WakeAgent(group);
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			WakeAgent(agent);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void WakeAgent(AIAgent agent)
	{
		if (!agent)
			return;

		int maxLod = AIAgent.GetMaxLOD();
		if (maxLod > 0 && agent.GetLOD() >= maxLod)
			agent.SetLOD(maxLod - 1);

		agent.PreventMaxLOD(-1);
		IEntity body = agent.GetControlledEntity();
		if (body)
			body.SetFlags(EntityFlags.ACTIVE, false);
	}

	//------------------------------------------------------------------------------------------------
	static void ForgetGroup(SCR_AIGroup group)
	{
		if (!group)
			return;

		Forget(group);
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (agent)
				Forget(agent.GetControlledEntity());
		}

		KeepAwake(group);
	}

	//------------------------------------------------------------------------------------------------
	static void ClearWaypoints(SCR_AIGroup group)
	{
		if (!group)
			return;

		array<AIWaypoint> waypoints = {};
		group.GetWaypoints(waypoints);
		foreach (AIWaypoint waypoint : waypoints)
		{
			group.RemoveWaypoint(waypoint);
			Delete(waypoint);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Order(SCR_AIGroup group, ResourceName waypointPrefab, vector pos, float radius)
	{
		if (!group)
			return;

		AIWaypoint waypoint = SpawnWaypoint(waypointPrefab, pos, radius);
		if (waypoint)
			group.AddWaypoint(waypoint);
	}

	//------------------------------------------------------------------------------------------------
	//! Même ordre, sans ramener le point au sol. Les patrouilles d'hélicoptère s'en servent.
	static void OrderAt(SCR_AIGroup group, ResourceName waypointPrefab, vector pos, float radius)
	{
		if (!group)
			return;

		AIWaypoint waypoint = AIWaypoint.Cast(Spawn(waypointPrefab, pos));
		if (!waypoint)
			return;

		if (radius > 0)
			waypoint.SetCompletionRadius(radius);

		group.AddWaypoint(waypoint);
	}

	//------------------------------------------------------------------------------------------------
	static void AliveBodies(SCR_AIGroup group, notnull array<IEntity> bodies)
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
			if (body && !IsDead(body))
				bodies.Insert(body);
		}
	}

	//------------------------------------------------------------------------------------------------
	static int AliveCount(SCR_AIGroup group)
	{
		array<IEntity> bodies = {};
		AliveBodies(group, bodies);
		return bodies.Count();
	}

	//------------------------------------------------------------------------------------------------
	static int AliveCountAll(array<SCR_AIGroup> groups)
	{
		int total = 0;
		if (!groups)
			return 0;

		foreach (SCR_AIGroup group : groups)
		{
			total += AliveCount(group);
		}

		return total;
	}

	//------------------------------------------------------------------------------------------------
	static bool AnyAliveNear(array<SCR_AIGroup> groups, vector pos, float radius)
	{
		if (!groups)
			return false;

		float radiusSq = radius * radius;
		array<IEntity> bodies = {};
		foreach (SCR_AIGroup group : groups)
		{
			bodies.Clear();
			AliveBodies(group, bodies);
			foreach (IEntity body : bodies)
			{
				if (vector.DistanceSqXZ(body.GetOrigin(), pos) <= radiusSq)
					return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Supprime les membres encore présents puis le groupe. Les corps déjà tombés restent au sol.
	static void DeleteGroup(SCR_AIGroup group)
	{
		if (!group)
			return;

		ClearWaypoints(group);
		array<AIAgent> agents = {};
		group.GetAgents(agents);
		foreach (AIAgent agent : agents)
		{
			if (!agent)
				continue;

			IEntity body = agent.GetControlledEntity();
			if (body && !IsDead(body))
				Delete(body);
		}

		Delete(group);
	}

	//------------------------------------------------------------------------------------------------
	static void DeleteGroups(array<SCR_AIGroup> groups)
	{
		if (!groups)
			return;

		foreach (SCR_AIGroup group : groups)
		{
			DeleteGroup(group);
		}

		groups.Clear();
	}

	//------------------------------------------------------------------------------------------------
	static void PruneGroups(array<SCR_AIGroup> groups)
	{
		if (!groups)
			return;

		for (int i = groups.Count() - 1; i >= 0; i--)
		{
			SCR_AIGroup group = groups[i];
			if (!group || AliveCount(group) < 1)
				groups.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Point de route le plus proche dans maxDist mètres. Avec minWidth, les routes plus étroites
	//! (sentiers, trottoirs) sont écartées ; une route sans largeur connue compte, comme dans Conflict.
	static bool RoadPoint(vector sample, float maxDist, out vector roadPoint, float minWidth = 0)
	{
		roadPoint = Ground(sample);
		SCR_AIWorld aiWorld = SCR_AIWorld.Cast(GetGame().GetAIWorld());
		if (!aiWorld)
			return false;

		RoadNetworkManager roads = aiWorld.GetRoadNetworkManager();
		if (!roads)
			return false;

		BaseRoad road;
		float dist;
		if (!FindRoad(roads, roadPoint, minWidth, road, dist))
			return false;

		if (!road || dist > maxDist)
			return false;

		array<vector> points = {};
		road.GetPoints(points);
		bool found = false;
		float bestSq = 0;
		foreach (vector point : points)
		{
			vector dry = Ground(point);
			if (IsWater(dry))
				continue;

			float distSq = vector.DistanceSqXZ(sample, dry);
			if (distSq > maxDist * maxDist * 4)
				continue;

			if (found && distSq >= bestSq)
				continue;

			found = true;
			bestSq = distSq;
			roadPoint = dry;
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindRoad(RoadNetworkManager roads, vector sample, float minWidth, out BaseRoad road, out float dist)
	{
		road = null;
		dist = 1000;

		BaseRoad closest;
		float closestDist;
		if (roads.GetClosestRoad(sample, closest, closestDist) && closest && IsWideEnough(closest, minWidth))
		{
			road = closest;
			dist = closestDist;
			return true;
		}

		if (minWidth <= 0)
			return false;

		array<BaseRoad> around = {};
		roads.GetRoadsInAABB(sample - "45 40 45", sample + "45 40 45", around);

		float bestDist = 45;
		foreach (BaseRoad candidate : around)
		{
			if (!candidate || !IsWideEnough(candidate, minWidth))
				continue;

			array<vector> points = {};
			candidate.GetPoints(points);
			foreach (vector point : points)
			{
				float d = vector.Distance(sample, point);
				if (d >= bestDist)
					continue;

				bestDist = d;
				road = candidate;
			}
		}

		if (!road)
			return false;

		dist = bestDist;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsWideEnough(BaseRoad road, float minWidth)
	{
		if (minWidth <= 0)
			return true;

		float width = road.GetWidth();
		if (width <= 0)
			return true;

		return width >= minWidth;
	}

	//------------------------------------------------------------------------------------------------
	static RplId IdOf(IEntity ent)
	{
		if (!ent)
			return RplId.Invalid();

		RplComponent rpl = RplComponent.Cast(ent.FindComponent(RplComponent));
		if (!rpl)
			return RplId.Invalid();

		return rpl.Id();
	}

	//------------------------------------------------------------------------------------------------
	static IEntity EntityOf(RplId id)
	{
		if (!id.IsValid())
			return null;

		RplComponent rpl = RplComponent.Cast(Replication.FindItem(id));
		if (!rpl)
			return null;

		return rpl.GetEntity();
	}

	//------------------------------------------------------------------------------------------------
	static string FormatInt(float value)
	{
		int rounded = Math.Round(value);
		return rounded.ToString();
	}
}
