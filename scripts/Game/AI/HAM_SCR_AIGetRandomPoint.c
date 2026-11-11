modded class SCR_AIGetRandomPoint
{
    protected static const vector HAM_NAVMESH_HALF_EXTENTS = "2 4 2";

    protected static const float HAM_STOREY_SEARCH_HEIGHT = 8.0;

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        vector origin = vector.Zero;
        IEntity waypointEnt;

        if (GetVariableIn(WAYPOINT_PORT, waypointEnt))
        {
            AIWaypoint waypoint = AIWaypoint.Cast(waypointEnt);
            if (!waypoint)
                return NodeError(this, owner, WAYPOINT_PORT + " is null! or wrong type");

            m_Radius = waypoint.GetCompletionRadius();
            origin = waypoint.GetOrigin();
        }
        else
        {
            if (GetVariableType(true, RADIUS_PORT) == int)
            {
                int radius;
                GetVariableIn(RADIUS_PORT, radius);
                m_Radius = radius;
            }
            else if (GetVariableType(true, RADIUS_PORT) == float)
            {
                GetVariableIn(RADIUS_PORT, m_Radius);
            }

            if (GetVariableType(true, ORIGIN_PORT) == vector)
                GetVariableIn(ORIGIN_PORT, origin);
        }

        vector result = s_AIRandomGenerator.GenerateRandomPointInRadius(m_ExclusionRadius, m_Radius, origin, true);

        if (!HAM_ShouldSearchBuildings(owner) || !HAM_SnapToNavmesh(owner, origin, result))
            result[1] = origin[1];

        SetVariableOut(PORT_RESULT_VECTOR, result);
        return ENodeResult.SUCCESS;
    }

    protected bool HAM_ShouldSearchBuildings(AIAgent owner)
    {
        SCR_ChimeraAIAgent chimera = SCR_ChimeraAIAgent.Cast(owner);
        if (!chimera || !chimera.m_UtilityComponent)
            return false;

        if (!SCR_AIMoveAndInvestigateBehavior.Cast(chimera.m_UtilityComponent.GetCurrentBehavior()))
            return false;

        AIGroup group = owner.GetParentGroup();
        if (!group)
            return false;

        SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
            group.FindComponent(SCR_AIGroupUtilityComponent));

        if (!utility)
            return false;

        return utility.HAM_GetBuildingSearch();
    }

    protected bool HAM_SnapToNavmesh(AIAgent owner, vector origin, out vector result)
    {
        IEntity controlled = owner.GetControlledEntity();
        if (!controlled)
            return false;

        AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(
            controlled.FindComponent(AIPathfindingComponent));

        if (!pathfinding)
            return false;

        float elevation = 0;
        if (Math.RandomFloat01() < 0.5)
            elevation = Math.RandomFloat(0, HAM_STOREY_SEARCH_HEIGHT);

        vector candidate = result;
        candidate[1] = origin[1] + elevation;

        vector corrected;
        if (!pathfinding.GetClosestPositionOnNavmesh(candidate, HAM_NAVMESH_HALF_EXTENTS, corrected))
            return false;

        result = corrected;
        return true;
    }
}
