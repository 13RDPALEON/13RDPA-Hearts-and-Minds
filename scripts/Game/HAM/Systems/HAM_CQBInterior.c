//------------------------------------------------------------------------------------------------
// 13RDPA — placement intérieur CQB (points libres + étages).
// Inspiré des traces plafond/sol, sans dépendance SNK.
//------------------------------------------------------------------------------------------------

class HAM_CQBInterior
{
	protected static const float EYE_HEIGHT = 1.53;
	protected static const float TRACE_DOWN = 5.0;
	protected static const float CEIL_CHECK = 12.0;
	protected static const float AGENT_SPACING = 3.6;
	protected static const float MIN_CLEARANCE = 2.0;

	//------------------------------------------------------------------------------------------------
	static void DetectFloorWorldYs(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, notnull array<float> outFloorYs)
	{
		outFloorYs.Clear();
		if (!building)
			return;

		array<float> colX = {0.5, 0.25, 0.75, 0.25, 0.75};
		array<float> colZ = {0.5, 0.25, 0.25, 0.75, 0.75};
		array<float> found = {};

		float spanX = localMaxs[0] - localMins[0];
		float spanZ = localMaxs[2] - localMins[2];

		for (int c = 0; c < colX.Count(); c++)
		{
			vector localTop;
			localTop[0] = localMins[0] + spanX * colX[c];
			localTop[1] = localMaxs[1] - 0.3;
			localTop[2] = localMins[2] + spanZ * colZ[c];
			vector start = building.CoordToParent(localTop);

			vector localBottom = localTop;
			localBottom[1] = localMins[1] - 0.5;
			vector bottomWorld = building.CoordToParent(localBottom);
			float bottomY = bottomWorld[1];

			for (int guard = 0; guard < 16; guard++)
			{
				if (start[1] <= bottomY + 0.5)
					break;

				TraceParam tp = new TraceParam();
				tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
				tp.Start = start;
				tp.End = Vector(start[0], bottomY, start[2]);
				float hit = GetGame().GetWorld().TraceMove(tp, null);

				if (hit < 0.001)
				{
					start = start - Vector(0, 0.5, 0);
					continue;
				}

				if (hit >= 0.999)
					break;

				vector hitPt = tp.Start + (tp.End - tp.Start) * hit;
				if (!tp.TraceEnt || !BelongsToBuilding(tp.TraceEnt, building))
				{
					start = hitPt - Vector(0, 0.5, 0);
					continue;
				}

				found.Insert(hitPt[1]);

				start = hitPt - Vector(0, 0.5, 0);
			}
		}

		// Tri croissant
		for (int i = 1; i < found.Count(); i++)
		{
			float key = found[i];
			int j = i - 1;
			while (j >= 0 && found[j] > key)
			{
				found[j + 1] = found[j];
				j--;
			}
			found[j + 1] = key;
		}

		// Cluster étages (~0.6 m)
		foreach (float y : found)
		{
			bool merge = false;
			for (int k = 0; k < outFloorYs.Count(); k++)
			{
				if (Math.AbsFloat(outFloorYs[k] - y) < 0.6)
				{
					outFloorYs[k] = (outFloorYs[k] + y) * 0.5;
					merge = true;
					break;
				}
			}

			if (!merge)
				outFloorYs.Insert(y);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Grille régulière dans l'emprise du bâtiment. Évite le paquet dans un couloir.
	static void FillSpreadGrid(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, int wanted, notnull array<vector> spots)
	{
		spots.Clear();
		if (!building || wanted < 1)
			return;

		float spanX = localMaxs[0] - localMins[0];
		float spanZ = localMaxs[2] - localMins[2];
		if (spanX < 1.0)
			spanX = 1.0;
		if (spanZ < 1.0)
			spanZ = 1.0;

		float spacing = AGENT_SPACING;
		float minSpan = Math.Min(spanX, spanZ);
		float fit = minSpan / Math.Max(2, wanted);
		if (fit < spacing)
			spacing = Math.Max(1.5, fit);

		vector centerLocal;
		centerLocal[0] = (localMins[0] + localMaxs[0]) * 0.5;
		centerLocal[1] = localMins[1] + 1.2;
		centerLocal[2] = (localMins[2] + localMaxs[2]) * 0.5;
		float radius = Math.Max(1.2, minSpan * 0.32);

		for (int i = 0; i < wanted; i++)
		{
			float ang = (i * Math.PI2) / wanted;
			vector localPos = centerLocal;
			localPos[0] = centerLocal[0] + Math.Sin(ang) * radius;
			localPos[2] = centerLocal[2] + Math.Cos(ang) * radius;
			vector worldPos = building.CoordToParent(localPos);

			TraceParam tpDown = new TraceParam();
			tpDown.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpDown.Start = worldPos + Vector(0, 2.0, 0);
			tpDown.End = worldPos - Vector(0, 6.0, 0);
			float hitDown = GetGame().GetWorld().TraceMove(tpDown, null);
			vector floorPt = worldPos;
			if (hitDown > 0.02 && hitDown < 0.98)
			{
				floorPt = tpDown.Start + (tpDown.End - tpDown.Start) * hitDown;
				floorPt[1] = floorPt[1] + 0.08;
				if (tpDown.TraceEnt && !BelongsToBuilding(tpDown.TraceEnt, building))
					floorPt = worldPos;
			}

			if (!IsFarEnough(floorPt, spots, 2.0))
			{
				float side = 2.5;
				if ((i / 2) * 2 == i)
					side = -2.5;

				localPos[0] = localPos[0] + (i + 1) * 2.5;
				localPos[2] = localPos[2] + side;
				floorPt = building.CoordToParent(localPos);
				floorPt[1] = floorPt[1] + 0.08;
			}

			spots.Insert(floorPt);
		}
	}

	//------------------------------------------------------------------------------------------------
	static vector RandomLocalCandidate(int floorIndex, int floorCount, vector localMins, vector localMaxs)
	{
		float margin = 0.35;
		float width = localMaxs[0] - localMins[0];
		float depth = localMaxs[2] - localMins[2];
		float usableW = Math.Max(0.5, width * (1.0 - margin));
		float usableD = Math.Max(0.5, depth * (1.0 - margin));

		vector localPos;
		localPos[0] = localMins[0] + width * 0.5 * margin + Math.RandomFloat01() * usableW;
		localPos[2] = localMins[2] + depth * 0.5 * margin + Math.RandomFloat01() * usableD;

		float localH = localMaxs[1] - localMins[1];
		float floorStep = localH / Math.Max(1, floorCount);
		localPos[1] = localMins[1] + floorIndex * floorStep + EYE_HEIGHT;
		return localPos;
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
	static bool IsCoreInside(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, vector worldPos)
	{
		if (!building)
			return false;

		vector local = building.CoordToLocal(worldPos);
		float spanX = localMaxs[0] - localMins[0];
		float spanZ = localMaxs[2] - localMins[2];
		float insetX = spanX * 0.18;
		float insetZ = spanZ * 0.18;
		if (spanX < 7.0)
			insetX = spanX * 0.12;
		if (spanZ < 7.0)
			insetZ = spanZ * 0.12;
		if (insetX < 0.25)
			insetX = 0.25;
		if (insetZ < 0.25)
			insetZ = 0.25;
		if (spanX >= 7.0 && insetX < 0.6)
			insetX = 0.6;
		if (spanZ >= 7.0 && insetZ < 0.6)
			insetZ = 0.6;

		if (local[0] < localMins[0] + insetX)
			return false;
		if (local[0] > localMaxs[0] - insetX)
			return false;
		if (local[2] < localMins[2] + insetZ)
			return false;
		if (local[2] > localMaxs[2] - insetZ)
			return false;
		if (local[1] < localMins[1] - 1.2)
			return false;
		if (local[1] > localMaxs[1] - 0.4)
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Petites cabanes : le sol est souvent le terrain. On reste au centre, sous le toit, en ligne sur le grand côté.
	static bool FillCoreSpots(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, int wanted, notnull array<vector> spots)
	{
		spots.Clear();
		if (!building || wanted < 1)
			return false;

		float spanX = Math.AbsFloat(localMaxs[0] - localMins[0]);
		float spanZ = Math.AbsFloat(localMaxs[2] - localMins[2]);
		if (Math.Min(spanX, spanZ) >= 7.0)
			return false;

		vector centerLocal;
		centerLocal[0] = (localMins[0] + localMaxs[0]) * 0.5;
		centerLocal[1] = localMins[1] + 1.3;
		centerLocal[2] = (localMins[2] + localMaxs[2]) * 0.5;

		int axis = 2;
		if (spanX >= spanZ)
			axis = 0;
		float step = Math.Clamp(Math.Max(spanX, spanZ) * 0.5 / wanted, 0.4, 1.2);
		for (int i = 0; i < wanted; i++)
		{
			// Centre, puis un pas de chaque côté, puis deux pas, etc.
			float offset = ((i + 1) / 2) * step;
			int side = i - (i / 2) * 2;
			if (side == 0)
				offset = -offset;

			vector localPos = centerLocal;
			localPos[axis] = centerLocal[axis] + offset;

			vector worldPos = building.CoordToParent(localPos);
			TraceParam tpDown = new TraceParam();
			tpDown.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpDown.Start = worldPos + Vector(0, 1.2, 0);
			tpDown.End = worldPos - Vector(0, 3.0, 0);
			float hitDown = GetGame().GetWorld().TraceMove(tpDown, null);
			if (hitDown <= 0.02 || hitDown >= 0.98)
				continue;
			if (!tpDown.TraceEnt || !BelongsToBuilding(tpDown.TraceEnt, building))
				continue;

			vector floorPt = tpDown.Start + (tpDown.End - tpDown.Start) * hitDown;
			floorPt[1] = floorPt[1] + 0.12;

			if (!IsCoreInside(building, localMins, localMaxs, floorPt))
				continue;

			TraceParam tpUp = new TraceParam();
			tpUp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpUp.Start = floorPt + Vector(0, 0.4, 0);
			tpUp.End = floorPt + Vector(0, 4.5, 0);
			float hitUp = GetGame().GetWorld().TraceMove(tpUp, null);
			if (hitUp >= 0.999)
				continue;
			if (!tpUp.TraceEnt || !BelongsToBuilding(tpUp.TraceEnt, building))
				continue;
			if (!IsEnclosed(building, floorPt))
				continue;

			spots.Insert(floorPt);
		}

		return !spots.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	// Sous le toit de CE bâtiment, avec des murs proches. Le préau, la cour et le tas de bûches
	// sont dans le volume du préfab : un simple test de boîte les acceptait.
	static bool IsEnclosed(SCR_DestructibleBuildingEntity building, vector floorPt)
	{
		if (!building)
			return false;

		TraceParam tpUp = new TraceParam();
		tpUp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		tpUp.Start = floorPt + Vector(0, 0.35, 0);
		tpUp.End = floorPt + Vector(0, 8.0, 0);
		float hitUp = GetGame().GetWorld().TraceMove(tpUp, null);
		if (hitUp >= 0.999)
			return false;
		if (!tpUp.TraceEnt || !BelongsToBuilding(tpUp.TraceEnt, building))
			return false;

		float clearance = 8.0 * hitUp;
		if (clearance < 1.7)
			return false;

		// Un point dans la pièce a le même sol et le même plafond à 1,2 m. Le préau, le rocher et la cour n'en ont pas.
		int room = 0;
		for (int dir = 0; dir < 4; dir++)
		{
			vector offset = Vector(1.2, 0, 0);
			if (dir == 1)
				offset = Vector(-1.2, 0, 0);
			if (dir == 2)
				offset = Vector(0, 0, 1.2);
			if (dir == 3)
				offset = Vector(0, 0, -1.2);

			vector probe = floorPt + offset;
			TraceParam tpFloor = new TraceParam();
			tpFloor.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpFloor.Start = probe + Vector(0, 0.8, 0);
			tpFloor.End = probe - Vector(0, 1.6, 0);
			float hitFloor = GetGame().GetWorld().TraceMove(tpFloor, null);
			if (hitFloor >= 0.999 || hitFloor <= 0.001)
				continue;

			vector probeFloor = tpFloor.Start + (tpFloor.End - tpFloor.Start) * hitFloor;
			if (Math.AbsFloat(probeFloor[1] - floorPt[1]) > 0.45)
				continue;

			TraceParam tpCeil = new TraceParam();
			tpCeil.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpCeil.Start = probe + Vector(0, 0.35, 0);
			tpCeil.End = probe + Vector(0, 8.0, 0);
			float hitCeil = GetGame().GetWorld().TraceMove(tpCeil, null);
			if (hitCeil >= 0.999)
				continue;
			if (!tpCeil.TraceEnt || !BelongsToBuilding(tpCeil.TraceEnt, building))
				continue;
			if (Math.AbsFloat(8.0 * hitCeil - clearance) > 0.6)
				continue;

			room++;
		}

		return room >= 3;
	}

	//------------------------------------------------------------------------------------------------
	//! Au-dessus du sol : étage ou grenier. Le rez-de-chaussée reste sous 1,5 m du terrain.
	static bool IsRaisedFloor(vector floorPt)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return false;

		return floorPt[1] >= world.GetSurfaceY(floorPt[0], floorPt[2]) + 1.5;
	}

	//------------------------------------------------------------------------------------------------
	//! Grenier : le plafond est le toit lui-même (plus rien du bâtiment au-dessus), en pente ou bas.
	//! Les greniers n'ont souvent pas d'accès : un homme posé là serait inatteignable.
	//! Le rez-de-chaussée n'est jamais un grenier (on y entre par les portes, même sous une charpente).
	static bool IsUnderRoof(SCR_DestructibleBuildingEntity building, vector floorPt)
	{
		if (!building)
			return false;

		BaseWorld world = GetGame().GetWorld();
		if (floorPt[1] < world.GetSurfaceY(floorPt[0], floorPt[2]) + 1.5)
			return false;

		float clearance;
		if (!CeilingAbove(building, floorPt, clearance))
			return false;

		vector start = floorPt + Vector(0, 0.35 + clearance + 0.3, 0);
		for (int guard = 0; guard < 6; guard++)
		{
			TraceParam tpAbove = new TraceParam();
			tpAbove.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpAbove.Start = start;
			tpAbove.End = start + Vector(0, 10.0, 0);
			float hitAbove = world.TraceMove(tpAbove, null);
			if (hitAbove < 0.001)
			{
				start = start + Vector(0, 0.3, 0);
				continue;
			}

			if (hitAbove < 0.999 && tpAbove.TraceEnt && BelongsToBuilding(tpAbove.TraceEnt, building))
				return false;

			break;
		}

		if (clearance < 2.2)
			return true;

		for (int dir = 0; dir < 4; dir++)
		{
			vector offset = Vector(1.5, 0, 0);
			if (dir == 1)
				offset = Vector(-1.5, 0, 0);
			if (dir == 2)
				offset = Vector(0, 0, 1.5);
			if (dir == 3)
				offset = Vector(0, 0, -1.5);

			float probeClearance;
			if (!CeilingAbove(building, floorPt + offset, probeClearance))
				continue;

			if (Math.AbsFloat(probeClearance - clearance) > 0.35)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CeilingAbove(SCR_DestructibleBuildingEntity building, vector floorPt, out float clearance)
	{
		TraceParam tpUp = new TraceParam();
		tpUp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		tpUp.Start = floorPt + Vector(0, 0.35, 0);
		tpUp.End = floorPt + Vector(0, 8.0, 0);
		float hitUp = GetGame().GetWorld().TraceMove(tpUp, null);
		if (hitUp >= 0.999 || !tpUp.TraceEnt || !BelongsToBuilding(tpUp.TraceEnt, building))
			return false;

		clearance = 7.65 * hitUp;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsFarEnough(vector pos, array<vector> placed, float spacing)
	{
		float spacingSq = spacing * spacing;
		foreach (vector p : placed)
		{
			if (vector.DistanceSq(pos, p) < spacingSq)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Cherche un point intérieur libre pour un agent. false = échec.
	static bool FindInteriorSpot(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, array<float> floorYs, int floorHint, array<vector> placed, bool enclosed, out vector outPos)
	{
		if (!building)
			return false;

		outPos = building.GetOrigin();
		vector bOrigin = building.GetOrigin();
		float buildingTopY = bOrigin[1] + localMaxs[1];
		float terrainY = GetGame().GetWorld().GetSurfaceY(bOrigin[0], bOrigin[2]);

		int floorCount = Math.Max(1, Math.Max(floorYs.Count(), Math.Max(1, (localMaxs[1] - localMins[1] - 1.5) / 3.5)));
		int targetFloor = floorHint - (floorHint / floorCount) * floorCount;

		for (int attempt = 0; attempt < 60; attempt++)
		{
			vector localPos = RandomLocalCandidate(targetFloor, floorCount, localMins, localMaxs);
			vector worldPos = building.CoordToParent(localPos);

			if (!floorYs.IsEmpty())
			{
				int floorIndex = floorHint + attempt / 8;
				int floorTotal = floorYs.Count();
				worldPos[1] = floorYs[floorIndex - (floorIndex / floorTotal) * floorTotal] + EYE_HEIGHT;
			}

			TraceParam tpDown = new TraceParam();
			tpDown.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpDown.Start = worldPos;
			tpDown.End = worldPos - Vector(0, EYE_HEIGHT + TRACE_DOWN, 0);
			float hitDown = GetGame().GetWorld().TraceMove(tpDown, null);
			if (hitDown >= 0.999 || hitDown < 0.001)
				continue;

			vector floorPt = tpDown.Start + (tpDown.End - tpDown.Start) * hitDown;
			floorPt[1] = floorPt[1] + 0.15;

			if (!tpDown.TraceEnt || !BelongsToBuilding(tpDown.TraceEnt, building))
				continue;

			float spotTerrain = GetGame().GetWorld().GetSurfaceY(floorPt[0], floorPt[2]);
			if (floorPt[1] < spotTerrain + 0.08)
				continue;
			if (floorPt[1] > buildingTopY + 0.5)
				continue;
			if (!IsCoreInside(building, localMins, localMaxs, floorPt))
				continue;

			TraceParam tpUp = new TraceParam();
			tpUp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
			tpUp.Start = floorPt;
			tpUp.End = floorPt + Vector(0, CEIL_CHECK, 0);
			float hitUp = GetGame().GetWorld().TraceMove(tpUp, null);
			if (hitUp >= 0.999)
				continue;

			if (tpUp.TraceEnt && !BelongsToBuilding(tpUp.TraceEnt, building))
				continue;

			float clearance = CEIL_CHECK * hitUp;
			if (clearance < MIN_CLEARANCE)
				continue;

			float spacing = AGENT_SPACING;
			if (attempt >= 40)
				spacing = 0.7;

			if (!IsFarEnough(floorPt, placed, spacing))
				continue;
			if (enclosed && !IsEnclosed(building, floorPt))
				continue;
			if (IsUnderRoof(building, floorPt))
				continue;

			outPos = floorPt;
			return true;
		}

		return false;
	}

	static bool IsTooSmall(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs)
	{
		if (!building)
			return true;

		float spanX = localMaxs[0] - localMins[0];
		float spanZ = localMaxs[2] - localMins[2];
		if (spanX < 0.0)
			spanX = -spanX;
		if (spanZ < 0.0)
			spanZ = -spanZ;

		if (spanX < 4.5)
			return true;
		if (spanZ < 4.5)
			return true;
		if (spanX * spanZ < 20.0)
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	// Surface couverte, en m². Le préau et la cour ne comptent pas : seul un point sous un toit fermé est gardé.
	static float EnclosedFloorArea(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs)
	{
		if (!building)
			return 0;

		float spanX = localMaxs[0] - localMins[0];
		float spanZ = localMaxs[2] - localMins[2];
		if (spanX < 0.0)
			spanX = -spanX;
		if (spanZ < 0.0)
			spanZ = -spanZ;
		if (spanX < 0.5 || spanZ < 0.5)
			return 0;

		float step = 2.0;
		int counted = 0;
		int samples = 0;
		float x = localMins[0] + step * 0.5;
		int guardX = 0;
		while (x < localMaxs[0] && guardX < 14)
		{
			guardX++;
			float z = localMins[2] + step * 0.5;
			int guardZ = 0;
			while (z < localMaxs[2] && guardZ < 14)
			{
				guardZ++;
				samples++;
				vector localPos;
				localPos[0] = x;
				localPos[1] = localMaxs[1] - 0.4;
				localPos[2] = z;
				vector worldPos = building.CoordToParent(localPos);
				TraceParam tp = new TraceParam();
				tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
				tp.Start = worldPos;
				tp.End = worldPos - Vector(0, 12.0, 0);
				float hit = GetGame().GetWorld().TraceMove(tp, null);
				if (hit > 0.001 && hit < 0.999 && tp.TraceEnt && BelongsToBuilding(tp.TraceEnt, building))
				{
					vector floorPt = tp.Start + (tp.End - tp.Start) * hit;
					floorPt[1] = floorPt[1] + 0.15;
					if (IsEnclosed(building, floorPt))
						counted++;
				}
				z = z + step;
			}
			x = x + step;
		}

		if (samples < 1)
			return 0;

		float cell = step * step;
		return counted * cell;
	}
}
