//------------------------------------------------------------------------------------------------
// Points libres à l'intérieur des bâtiments : CQB, intel, otage.
// Repris du placement intérieur du Conflict 13RDPA (traces sol/plafond dans l'emprise du bâtiment).
//------------------------------------------------------------------------------------------------

class HAM_Interior
{
	protected static const float EYE_HEIGHT = 1.53;
	protected static const float CEIL_CHECK = 12.0;
	protected static const float MIN_CLEARANCE = 2.0;

	protected static ref array<SCR_DestructibleBuildingEntity> s_aScan;

	//------------------------------------------------------------------------------------------------
	static void CollectBuildings(vector center, float radius, notnull array<SCR_DestructibleBuildingEntity> buildings)
	{
		buildings.Clear();
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		if (!s_aScan)
			s_aScan = {};

		s_aScan.Clear();
		world.QueryEntitiesBySphere(center, radius, CollectBuilding);
		foreach (SCR_DestructibleBuildingEntity building : s_aScan)
		{
			if (!building)
				continue;

			vector mins;
			vector maxs;
			building.GetBounds(mins, maxs);
			if (IsTooSmall(mins, maxs))
				continue;

			buildings.Insert(building);
		}

		s_aScan.Clear();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CollectBuilding(IEntity ent)
	{
		SCR_DestructibleBuildingEntity building = SCR_DestructibleBuildingEntity.Cast(ent);
		if (building)
			s_aScan.Insert(building);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsTooSmall(vector localMins, vector localMaxs)
	{
		float spanX = Math.AbsFloat(localMaxs[0] - localMins[0]);
		float spanZ = Math.AbsFloat(localMaxs[2] - localMins[2]);
		if (spanX < 4.5 || spanZ < 4.5)
			return true;

		return spanX * spanZ < 20.0;
	}

	//------------------------------------------------------------------------------------------------
	static bool BelongsToBuilding(IEntity ent, SCR_DestructibleBuildingEntity building)
	{
		if (!ent || !building)
			return false;

		IEntity rootA = ent.GetRootParent();
		IEntity rootB = building.GetRootParent();
		if (rootA && rootB && rootA == rootB)
			return true;

		return ent == building || ent.GetParent() == building;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsFarEnough(vector pos, array<vector> placed, float spacing)
	{
		if (!placed)
			return true;

		float spacingSq = spacing * spacing;
		foreach (vector other : placed)
		{
			if (vector.DistanceSq(pos, other) < spacingSq)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsCoreInside(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, vector worldPos)
	{
		vector local = building.CoordToLocal(worldPos);
		float insetX = Math.Max(0.3, (localMaxs[0] - localMins[0]) * 0.15);
		float insetZ = Math.Max(0.3, (localMaxs[2] - localMins[2]) * 0.15);
		if (local[0] < localMins[0] + insetX || local[0] > localMaxs[0] - insetX)
			return false;

		if (local[2] < localMins[2] + insetZ || local[2] > localMaxs[2] - insetZ)
			return false;

		return local[1] >= localMins[1] - 1.2 && local[1] <= localMaxs[1] - 0.4;
	}

	//------------------------------------------------------------------------------------------------
	//! Un point de sol du bâtiment, sous son plafond, à l'écart des points déjà pris.
	static bool FindSpot(SCR_DestructibleBuildingEntity building, array<vector> placed, out vector spot)
	{
		spot = vector.Zero;
		if (!building)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		vector localMins;
		vector localMaxs;
		building.GetBounds(localMins, localMaxs);
		vector origin = building.GetOrigin();
		float topY = origin[1] + localMaxs[1];
		float height = localMaxs[1] - localMins[1];
		int floors = Math.Max(1, Math.Floor((height - 1.5) / 3.5));

		for (int attempt = 0; attempt < 40; attempt++)
		{
			vector localPos;
			localPos[0] = Math.RandomFloat(localMins[0], localMaxs[0]);
			localPos[2] = Math.RandomFloat(localMins[2], localMaxs[2]);
			localPos[1] = localMins[1] + Math.RandomInt(0, floors) * (height / floors) + EYE_HEIGHT;
			vector worldPos = building.CoordToParent(localPos);

			TraceParam down = new TraceParam();
			down.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			down.Start = worldPos;
			down.End = worldPos - Vector(0, EYE_HEIGHT + 5.0, 0);
			float hitDown = world.TraceMove(down, null);
			if (hitDown >= 0.999 || hitDown < 0.001)
				continue;

			if (!down.TraceEnt || !BelongsToBuilding(down.TraceEnt, building))
				continue;

			vector floorPt = down.Start + (down.End - down.Start) * hitDown;
			floorPt[1] = floorPt[1] + 0.12;
			if (floorPt[1] < world.GetSurfaceY(floorPt[0], floorPt[2]) + 0.05)
				continue;

			if (floorPt[1] > topY + 0.5)
				continue;

			if (!IsCoreInside(building, localMins, localMaxs, floorPt))
				continue;

			TraceParam up = new TraceParam();
			up.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			up.Start = floorPt;
			up.End = floorPt + Vector(0, CEIL_CHECK, 0);
			float hitUp = world.TraceMove(up, null);
			if (hitUp >= 0.999)
				continue;

			if (up.TraceEnt && !BelongsToBuilding(up.TraceEnt, building))
				continue;

			if (CEIL_CHECK * hitUp < MIN_CLEARANCE)
				continue;

			float spacing = 3.0;
			if (attempt >= 25)
				spacing = 1.0;

			if (!IsFarEnough(floorPt, placed, spacing))
				continue;

			spot = floorPt;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Le volume du prop est déjà dans un mur, un meuble ou sous un plancher.
	static bool SpotEmbedded(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		vector origin = pos + Vector(0, 0.45, 0);
		if (CloseHit(world, origin, origin + Vector(0.7, 0, 0)))
			return true;

		if (CloseHit(world, origin, origin + Vector(-0.7, 0, 0)))
			return true;

		if (CloseHit(world, origin, origin + Vector(0, 0, 0.7)))
			return true;

		if (CloseHit(world, origin, origin + Vector(0, 0, -0.7)))
			return true;

		if (CloseHit(world, origin, origin + Vector(0, 1.15, 0)))
			return true;

		return UnderFloor(world, pos);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool UnderFloor(BaseWorld world, vector pos)
	{
		TraceParam up = new TraceParam();
		up.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		up.Start = pos + Vector(0, 0.2, 0);
		up.End = pos + Vector(0, 2.0, 0);
		float hit = world.TraceMove(up, null);
		if (hit < 0.08 || hit > 0.75)
			return false;

		if (!up.TraceEnt)
			return false;

		if (ChimeraCharacter.Cast(up.TraceEnt))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CloseHit(BaseWorld world, vector start, vector end)
	{
		TraceParam trace = new TraceParam();
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Start = start;
		trace.End = end;
		float hit = world.TraceMove(trace, null);
		if (hit >= 0.2)
			return false;

		if (trace.TraceEnt && ChimeraCharacter.Cast(trace.TraceEnt))
			return false;

		return true;
	}
}
