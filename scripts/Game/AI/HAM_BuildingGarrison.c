class HAM_BuildingGarrison
{
    protected static const float HAM_EYE_HEIGHT      = 1.53;
    protected static const float HAM_TRACE_DOWN_DIST = 5.0;
    protected static const float HAM_CEIL_CHECK_DIST = 15.0;
    protected static const int   HAM_AGENTS_PER_FLOOR = 4;

    protected static const float HAM_AGENT_SPACING   = 3.5;
    protected static const float HAM_SPACING_FLOOR   = 0.6;

    protected static const float HAM_MIN_CLEARANCE   = 2.0;
    protected static const float HAM_LOFT_CLEARANCE  = 5.0;

    protected static const int   HAM_YAW_SAMPLES     = 48;
    protected static const float HAM_YAW_RANGE       = 20.0;
    protected static const float HAM_YAW_MIN_SCORE   = 1.5;
    protected static const float HAM_YAW_SPREAD_DIST = 6.0;
    protected static const float HAM_YAW_SPREAD_ANG  = 45.0;
    protected static const float HAM_YAW_SPREAD_MUL  = 0.35;

    protected static const float HAM_DOOR_FLOOR_TOL  = 2.5;
    protected static const float HAM_DOOR_MAX_DIST   = 15.0;
    protected static const float HAM_DOOR_VISIBLE    = 0.9;
    protected static const float HAM_DOOR_QUERY_PAD  = 6.0;

    protected static const ResourceName HAM_GROUP_BASE = "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et";
    protected static const ResourceName HAM_DEFEND_WP  = "{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et";

    protected static SCR_DestructibleBuildingEntity s_pHAM_QueryResult;

    protected static ref array<vector> s_aHAM_DoorPositions;

    static void HAM_GarrisonPlacedEntity(notnull SCR_EditableEntityComponent placedEntity, vector buildingPos)
    {
        SCR_AIGroup group = SCR_AIGroup.Cast(placedEntity.GetOwner());

        if (!group)
        {
            ChimeraCharacter character = ChimeraCharacter.Cast(placedEntity.GetOwner());
            if (character)
            {
                AIControlComponent control = AIControlComponent.Cast(character.FindComponent(AIControlComponent));
                if (control && control.GetAIAgent())
                    group = SCR_AIGroup.Cast(control.GetAIAgent().GetParentGroup());
            }
        }

        if (!group)
        {
            Print("[13RDPA H&M] Garnison : l'entité posée n'est ni un groupe ni un personnage IA — abandon.", LogLevel.WARNING);
            return;
        }

        SCR_DestructibleBuildingEntity building = HAM_FindBuildingAt(buildingPos);
        if (!building)
        {
            Print("[13RDPA H&M] Garnison : bâtiment introuvable à la position armée — abandon.", LogLevel.WARNING);
            return;
        }

        GetGame().GetCallqueue().CallLater(HAM_WaitMembersThenPlace, 1000, false, group, building, 12, -1);
    }

    protected static SCR_DestructibleBuildingEntity HAM_FindBuildingAt(vector pos)
    {
        s_pHAM_QueryResult = null;
        GetGame().GetWorld().QueryEntitiesBySphere(pos, 1.0, HAM_QueryCallback, null, EQueryEntitiesFlags.ALL);
        return s_pHAM_QueryResult;
    }

    protected static bool HAM_QueryCallback(IEntity entity)
    {
        SCR_DestructibleBuildingEntity building = SCR_DestructibleBuildingEntity.Cast(entity);
        if (!building)
            return true;

        s_pHAM_QueryResult = building;
        return false;
    }

    protected static void HAM_WaitMembersThenPlace(SCR_AIGroup group, SCR_DestructibleBuildingEntity building, int retriesLeft, int lastCount)
    {
        if (!group || !building)
            return;

        int ready = HAM_ReadyAgentCount(group);
        int expected = HAM_ExpectedAgentCount(group);

        bool complete;
        if (expected > 0)
            complete = ready >= expected;
        else
            complete = ready > 0 && ready == lastCount;

        if (!complete)
        {
            if (retriesLeft > 0)
            {
                GetGame().GetCallqueue().CallLater(HAM_WaitMembersThenPlace, 1000, false, group, building, retriesLeft - 1, ready);
                return;
            }

            if (ready == 0)
            {
                Print("[13RDPA H&M] Garnison : aucun membre spawné après attente, abandon.", LogLevel.WARNING);
                return;
            }

            PrintFormat("[13RDPA H&M] Garnison : effectif encore incomplet après attente, pose de %1 membre(s) sur %2 attendu(s).",
                ready, expected, level: LogLevel.WARNING);
        }

        HAM_PlaceAgents(group, building);
    }

    protected static int HAM_ExpectedAgentCount(SCR_AIGroup group)
    {
        int expected = 0;
        if (group.m_aUnitPrefabSlots)
            expected = group.m_aUnitPrefabSlots.Count();

        int cap = group.GetMaxUnitsToSpawn();
        if (cap > 0 && cap < expected)
            expected = cap;

        return expected;
    }

    protected static int HAM_ReadyAgentCount(SCR_AIGroup group)
    {
        array<AIAgent> agents = {};
        group.GetAgents(agents);

        int ready = 0;

        foreach (AIAgent agent : agents)
        {
            if (agent && agent.GetControlledEntity())
                ready++;
        }

        return ready;
    }

    protected static void HAM_PlaceAgents(SCR_AIGroup group, SCR_DestructibleBuildingEntity building)
    {
        array<AIAgent> agents = {};
        group.GetAgents(agents);
        if (agents.IsEmpty())
            return;

        array<vector> postPositions = {};
        array<float>  postYaws = {};

        array<Managed> comps = {};
        building.FindComponents(SCR_AISmartActionSentinelComponent, comps);
        foreach (Managed comp : comps)
        {
            SCR_AISmartActionSentinelComponent sentinel = SCR_AISmartActionSentinelComponent.Cast(comp);
            if (!sentinel)
                continue;

            vector postPos = building.CoordToParent(sentinel.GetActionOffset());

            bool isNew = true;
            foreach (vector other : postPositions)
            {
                if (vector.DistanceSq(postPos, other) < 0.1)
                {
                    isNew = false;
                    break;
                }
            }
            if (!isNew)
                continue;

            vector lookDir = building.CoordToParent(sentinel.GetLookPosition()) - postPos;
            postPositions.Insert(postPos);
            postYaws.Insert(lookDir.Normalized().ToYaw());
        }

        vector localMins, localMaxs;
        building.GetBounds(localMins, localMaxs);

        array<float> floorYs = {};
        HAM_DetectFloorWorldYs(building, localMins, localMaxs, floorYs);
        int floorCount = Math.Max(1, floorYs.Count());

        array<vector> doorPositions = {};
        HAM_CollectDoors(building, localMins, localMaxs, doorPositions);

        int maxAgents = postPositions.Count() + HAM_AGENTS_PER_FLOOR * floorCount;

        vector buildingOrigin = building.GetOrigin();
        float  buildingTopY   = buildingOrigin[1] + localMaxs[1];
        float  terrainY       = GetGame().GetWorld().GetSurfaceY(buildingOrigin[0], buildingOrigin[2]);

        AIPathfindingComponent pathComp = AIPathfindingComponent.Cast(group.FindComponent(AIPathfindingComponent));

        ref array<vector> placedPositions = new array<vector>();
        ref array<float>  placedYaws = new array<float>();
        int placedCount = 0;
        int deletedCount = 0;
        int skippedCount = 0;

        foreach (int agentIdx, AIAgent agent : agents)
        {
            if (!agent)
            {
                skippedCount++;
                continue;
            }

            IEntity agentEntity = agent.GetControlledEntity();
            if (!agentEntity)
            {
                skippedCount++;
                continue;
            }

            if (agentIdx >= maxAgents)
            {
                group.RemoveAgent(agent);
                RplComponent.DeleteRplEntity(agentEntity, false);
                deletedCount++;
                continue;
            }

            bool   posFound = false;
            bool   yawFromPost = false;

            vector finalPos;
            float  finalYaw = Math.RandomFloat(0, 360);
            float  wpRadius = HAM_AICombatStyle.GetDefendRadius(HAM_EAICombatStyle.CQB_DEFENSIVE);

            if (!postPositions.IsEmpty())
            {
                int postIdx = postPositions.GetRandomIndex();
                finalPos = postPositions[postIdx];
                finalYaw = postYaws[postIdx];
                postPositions.RemoveOrdered(postIdx);
                postYaws.RemoveOrdered(postIdx);
                wpRadius = 2;
                posFound = true;
                yawFromPost = true;
            }

            int targetFloorIndex = agentIdx - (agentIdx / floorCount) * floorCount;

            for (int attempt = 0; attempt < 70 && !posFound; attempt++)
            {
                vector localPos = HAM_GetRandomLocalCandidate(targetFloorIndex, floorCount, localMins, localMaxs);
                vector worldPos = building.CoordToParent(localPos);

                if (!floorYs.IsEmpty())
                {
                    int floorSlot = agentIdx + attempt / 10;
                    int floorCountNow = floorYs.Count();
                    worldPos[1] = floorYs[floorSlot - (floorSlot / floorCountNow) * floorCountNow] + HAM_EYE_HEIGHT;
                }

                TraceParam tp = new TraceParam();
                tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
                tp.Start = worldPos;
                tp.End   = worldPos - Vector(0, HAM_EYE_HEIGHT + HAM_TRACE_DOWN_DIST, 0);

                float hitDown = GetGame().GetWorld().TraceMove(tp, null);
                if (hitDown >= 0.999) continue;
                if (hitDown < 0.001)  continue;

                vector floorPt = tp.Start + (tp.End - tp.Start) * hitDown;
                floorPt[1] = floorPt[1] + 0.05;

                if (floorPt[1] < terrainY - 0.1)     continue;
                if (floorPt[1] > buildingTopY + 0.5) continue;

                if (tp.TraceEnt && tp.TraceEnt.GetRootParent() != building.GetRootParent()) continue;

                TraceParam tpUp = new TraceParam();
                tpUp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
                tpUp.Start = floorPt;
                tpUp.End   = floorPt + Vector(0, HAM_CEIL_CHECK_DIST, 0);
                float hitUp = GetGame().GetWorld().TraceMove(tpUp, null);
                if (hitUp >= 0.999) continue;

                if (!tpUp.TraceEnt) continue;
                if (tpUp.TraceEnt.GetRootParent() != building.GetRootParent()) continue;

                float clearance = HAM_CEIL_CHECK_DIST * hitUp;
                if (clearance < HAM_MIN_CLEARANCE) continue;

                if (clearance < HAM_LOFT_CLEARANCE && HAM_IsSlopedCeiling(floorPt, clearance, building))
                    continue;

                if (!HAM_HasRoomAround(floorPt))
                    continue;

                if (!HAM_SnapToNavmesh(pathComp, attempt < 40, floorPt))
                    continue;

                float spacing = HAM_AGENT_SPACING;
                if (attempt >= 45)      spacing = HAM_SPACING_FLOOR;
                else if (attempt >= 35) spacing = Math.Max(HAM_SPACING_FLOOR, HAM_AGENT_SPACING * 0.5);

                if (!HAM_IsFarEnoughFromPlaced(floorPt, placedPositions, spacing))
                    continue;

                finalPos = floorPt;
                posFound = true;
            }

            if (!posFound)
            {
                group.RemoveAgent(agent);
                RplComponent.DeleteRplEntity(agentEntity, false);
                deletedCount++;
                continue;
            }

            float yawScore = 0;

            if (!yawFromPost)
            {
                vector eyePos = finalPos + Vector(0, HAM_EYE_HEIGHT, 0);
                float doorYaw, doorDist;

                if (HAM_YawToVisibleDoor(eyePos, doorPositions, placedPositions, placedYaws, doorYaw, doorDist))
                {
                    finalYaw = doorYaw;
                }
                else
                {
                    finalYaw = HAM_ClearestYaw(eyePos, placedPositions, placedYaws, yawScore);
                }
            }

            EntitySpawnParams gParams = new EntitySpawnParams();
            gParams.TransformMode = ETransformMode.WORLD;
            Math3D.AnglesToMatrix(Vector(finalYaw, 0, 0), gParams.Transform);
            gParams.Transform[3] = finalPos;

            SCR_AIGroup indivGroup = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(Resource.Load(HAM_GROUP_BASE), GetGame().GetWorld(), gParams));
            if (!indivGroup)
            {
                skippedCount++;
                continue;
            }
            indivGroup.SetFaction(group.GetFaction());

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                indivGroup.FindComponent(SCR_AIGroupUtilityComponent));
            if (utility)
                utility.HAM_SetCombatStyle(HAM_EAICombatStyle.CQB_DEFENSIVE);

            group.RemoveAgent(agent);
            indivGroup.AddAgent(agent);

            SCR_EditableEntityComponent editableCharacter = SCR_EditableEntityComponent.Cast(
                agentEntity.FindComponent(SCR_EditableEntityComponent));
            if (editableCharacter)
                editableCharacter.SetTransform(gParams.Transform);
            else
                agentEntity.SetOrigin(finalPos);

            placedPositions.Insert(finalPos);
            placedYaws.Insert(finalYaw);
            placedCount++;

            EntitySpawnParams wpParams = new EntitySpawnParams();
            wpParams.TransformMode = ETransformMode.WORLD;
            Math3D.MatrixIdentity4(wpParams.Transform);
            wpParams.Transform[3] = finalPos;

            AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(Resource.Load(HAM_DEFEND_WP), GetGame().GetWorld(), wpParams));
            if (waypoint)
            {
                waypoint.SetCompletionRadius(wpRadius);

                SCR_TimedWaypoint timed = SCR_TimedWaypoint.Cast(waypoint);
                if (timed && timed.GetHoldingTime() >= 0)
                {
                    if (!timed.m_TimedWaypointParameters)
                        timed.m_TimedWaypointParameters = new SCR_AITimedWaypointParameters();
                    timed.SetHoldingTime(-1);
                }

                indivGroup.AddWaypointAt(waypoint, 0);
            }
        }

        SCR_EntityHelper.DeleteEntityAndChildren(group);

        PrintFormat("[13RDPA H&M] Garnison : %1 posté(s) sur %2 membre(s), bâtiment à %3 étage(s), %4 place(s) au total. Supprimés faute de place : %5. Ignorés sans corps : %6.",
            placedCount, agents.Count(), floorCount, maxAgents, deletedCount, skippedCount);
    }

    protected static vector HAM_GetRandomLocalCandidate(int floorIndex, int floorCount, vector localMins, vector localMaxs)
    {
        float width = localMaxs[0] - localMins[0];
        float depth = localMaxs[2] - localMins[2];

        float xMargin = Math.Clamp(width * 0.18, 0.6, 2.0);
        float zMargin = Math.Clamp(depth * 0.18, 0.6, 2.0);

        float minX = localMins[0] + xMargin;
        float maxX = localMaxs[0] - xMargin;
        float minZ = localMins[2] + zMargin;
        float maxZ = localMaxs[2] - zMargin;

        if (maxX <= minX) { minX = localMins[0] * 0.5; maxX = localMaxs[0] * 0.5; }
        if (maxZ <= minZ) { minZ = localMins[2] * 0.5; maxZ = localMaxs[2] * 0.5; }

        vector localPos;
        localPos[0] = Math.RandomFloatInclusive(minX, maxX);
        localPos[2] = Math.RandomFloatInclusive(minZ, maxZ);

        float localHeight = localMaxs[1] - localMins[1];
        float floorHeight = 3.5;
        if (floorCount > 1)
            floorHeight = Math.Max(2.8, localHeight / floorCount);

        localPos[1] = localMins[1] + floorIndex * floorHeight + HAM_EYE_HEIGHT;
        return localPos;
    }

    protected static void HAM_DetectFloorWorldYs(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, notnull array<float> outFloorYs)
    {
        array<float> colX = {0.5, 0.25, 0.75, 0.25, 0.75};
        array<float> colZ = {0.5, 0.25, 0.25, 0.75, 0.75};
        array<float> found    = {};
        array<int>   foundCol = {};

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
                tp.End   = Vector(start[0], bottomY, start[2]);
                float hit = GetGame().GetWorld().TraceMove(tp, null);

                if (hit < 0.001)
                {
                    start = start - Vector(0, 0.5, 0);
                    continue;
                }

                if (hit >= 0.999)
                    break;

                vector hitPt = tp.Start + (tp.End - tp.Start) * hit;
                if (!tp.TraceEnt || tp.TraceEnt.GetRootParent() == building.GetRootParent())
                {
                    found.Insert(hitPt[1]);
                    foundCol.Insert(c);
                }

                start = hitPt - Vector(0, 0.5, 0);
            }
        }

        for (int i = 1; i < found.Count(); i++)
        {
            float key    = found[i];
            int   keyCol = foundCol[i];
            int j = i - 1;
            while (j >= 0 && found[j] > key)
            {
                found[j + 1]    = found[j];
                foundCol[j + 1] = foundCol[j];
                j--;
            }
            found[j + 1]    = key;
            foundCol[j + 1] = keyCol;
        }

        int g = 0;
        while (g < found.Count())
        {
            float base = found[g];
            float sum  = 0;
            int   n    = 0;
            array<int> cols = {};

            int k = g;
            while (k < found.Count() && found[k] - base < 1.8)
            {
                sum += found[k];
                n++;
                if (!cols.Contains(foundCol[k]))
                    cols.Insert(foundCol[k]);
                k++;
            }

            if (cols.Count() >= 2)
                outFloorYs.Insert(sum / n);

            g = k;
        }

        if (outFloorYs.IsEmpty())
        {
            foreach (float y : found)
            {
                if (outFloorYs.IsEmpty() || y - outFloorYs[outFloorYs.Count() - 1] >= 1.8)
                    outFloorYs.Insert(y);
            }
        }
    }

    protected static bool HAM_IsSlopedCeiling(vector floorPt, float centerClearance, SCR_DestructibleBuildingEntity building)
    {
        const float PROBE_DIST = 1.2;
        const float PROBE_UP   = 15.0;
        const float MAX_DELTA  = 0.45;

        for (int i = 0; i < 8; i++)
        {
            vector offset = PROBE_DIST * Vector(i * 45, 0, 0).AnglesToVector();

            TraceParam tp = new TraceParam();
            tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            tp.Start = floorPt + offset;
            tp.End   = tp.Start + Vector(0, PROBE_UP, 0);

            float hit = GetGame().GetWorld().TraceMove(tp, null);
            if (hit >= 0.999)
                return true;

            if (Math.AbsFloat(PROBE_UP * hit - centerClearance) > MAX_DELTA)
                return true;
        }

        return false;
    }

    protected static bool HAM_HasRoomAround(vector floorPt)
    {
        const float PROBE_DIST = 1.2;
        const float PROBE_UP   = 0.8;
        const float PROBE_DOWN = 1.6;
        const float MAX_STEP   = 0.45;
        const int   NEEDED     = 3;

        int ok = 0;

        for (int i = 0; i < 4; i++)
        {
            vector offset = PROBE_DIST * Vector(i * 90, 0, 0).AnglesToVector();
            vector probeStart = floorPt + offset + Vector(0, PROBE_UP, 0);

            TraceParam tp = new TraceParam();
            tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            tp.Start = probeStart;
            tp.End   = probeStart - Vector(0, PROBE_UP + PROBE_DOWN, 0);

            float hit = GetGame().GetWorld().TraceMove(tp, null);
            if (hit >= 0.999) continue;
            if (hit < 0.001)  continue;

            vector probeFloor = tp.Start + (tp.End - tp.Start) * hit;
            if (Math.AbsFloat(probeFloor[1] - floorPt[1]) > MAX_STEP)
                continue;

            ok++;
        }

        return ok >= NEEDED;
    }

    protected static bool HAM_SnapToNavmesh(AIPathfindingComponent pathComp, bool strict, inout vector pos)
    {
        if (!pathComp) return true;

        NavmeshWorldComponent navmesh = pathComp.GetNavmeshComponent();
        if (navmesh && !navmesh.IsTileLoaded(pos))
            return true;

        vector extents = Vector(3, 4, 3);
        if (strict)
            extents = Vector(3, 1.5, 3);

        vector navPt;
        if (!pathComp.GetClosestPositionOnNavmesh(pos, extents, navPt))
            return false;

        pos = navPt;
        return true;
    }

    protected static void HAM_CollectDoors(SCR_DestructibleBuildingEntity building, vector localMins, vector localMaxs, notnull array<vector> outDoors)
    {
        s_aHAM_DoorPositions = outDoors;

        vector size = localMaxs - localMins;
        float radius = Math.Max(size[0], size[2]) * 0.5 + HAM_DOOR_QUERY_PAD;
        vector centre = building.CoordToParent((localMins + localMaxs) * 0.5);

        GetGame().GetWorld().QueryEntitiesBySphere(centre, radius, HAM_DoorQueryCallback, null, EQueryEntitiesFlags.ALL);

        s_aHAM_DoorPositions = null;
    }

    protected static bool HAM_DoorQueryCallback(IEntity entity)
    {
        if (!entity || !s_aHAM_DoorPositions)
            return true;

        DoorComponent door = DoorComponent.Cast(entity.FindComponent(DoorComponent));
        if (door)
            s_aHAM_DoorPositions.Insert(entity.GetOrigin());

        return true;
    }

    protected static bool HAM_YawToVisibleDoor(vector eyePos, array<vector> doorPositions, array<vector> placedPositions, array<float> placedYaws, out float outYaw, out float outDist)
    {
        float bestDistSq = HAM_DOOR_MAX_DIST * HAM_DOOR_MAX_DIST;
        float floorY = eyePos[1] - HAM_EYE_HEIGHT;
        float spreadDistSq = HAM_YAW_SPREAD_DIST * HAM_YAW_SPREAD_DIST;
        bool found = false;

        foreach (vector doorPos : doorPositions)
        {
            if (Math.AbsFloat(doorPos[1] - floorY) > HAM_DOOR_FLOOR_TOL)
                continue;

            vector target = doorPos;
            target[1] = eyePos[1];

            float distSq = vector.DistanceSq(eyePos, target);
            if (distSq >= bestDistSq)
                continue;

            float yaw = (target - eyePos).Normalized().ToYaw();

            bool taken = false;
            foreach (int idx, vector placedPos : placedPositions)
            {
                if (vector.DistanceSq(placedPos, eyePos) > spreadDistSq)
                    continue;

                if (HAM_YawDelta(yaw, placedYaws[idx]) < HAM_YAW_SPREAD_ANG)
                {
                    taken = true;
                    break;
                }
            }
            if (taken)
                continue;

            TraceParam tp = new TraceParam();
            tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            tp.Start = eyePos;
            tp.End   = target;

            if (GetGame().GetWorld().TraceMove(tp, null) < HAM_DOOR_VISIBLE)
                continue;

            bestDistSq = distSq;
            outYaw = yaw;
            outDist = Math.Sqrt(distSq);
            found = true;
        }

        return found;
    }

    protected static float HAM_ClearestYaw(vector eyePos, array<vector> placedPositions, array<float> placedYaws, out float outScore)
    {
        float bestYaw = Math.RandomFloat(0, 360);
        float bestScore = HAM_YAW_MIN_SCORE;
        outScore = 0;
        float step = 360.0 / HAM_YAW_SAMPLES;
        float spreadDistSq = HAM_YAW_SPREAD_DIST * HAM_YAW_SPREAD_DIST;

        for (int i = 0; i < HAM_YAW_SAMPLES; i++)
        {
            float yaw = i * step;

            vector mat[4];
            Math3D.AnglesToMatrix(Vector(yaw, 0, 0), mat);

            TraceParam tp = new TraceParam();
            tp.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
            tp.Start = eyePos;
            tp.End   = eyePos + mat[2] * HAM_YAW_RANGE;

            float score = GetGame().GetWorld().TraceMove(tp, null) * HAM_YAW_RANGE;

            foreach (int idx, vector placedPos : placedPositions)
            {
                if (vector.DistanceSq(placedPos, eyePos) > spreadDistSq)
                    continue;

                if (HAM_YawDelta(yaw, placedYaws[idx]) < HAM_YAW_SPREAD_ANG)
                    score = score * HAM_YAW_SPREAD_MUL;
            }

            if (score > bestScore)
            {
                bestScore = score;
                bestYaw = yaw;
                outScore = score;
            }
        }

        return bestYaw;
    }

    protected static float HAM_YawDelta(float a, float b)
    {
        float delta = Math.AbsFloat(a - b);

        while (delta > 360)
            delta = delta - 360;

        if (delta > 180)
            delta = 360 - delta;

        return delta;
    }

    protected static bool HAM_IsFarEnoughFromPlaced(vector candidatePos, array<vector> placedPositions, float minSpacing)
    {
        float minSpacingSq = minSpacing * minSpacing;

        foreach (vector placedPos : placedPositions)
        {
            if (vector.DistanceSq(candidatePos, placedPos) < minSpacingSq)
                return false;
        }

        return true;
    }
}
