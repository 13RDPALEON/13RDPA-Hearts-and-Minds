modded class SCR_AIGroupTargetClusterProcessor
{
    protected SCR_AIGroupUtilityComponent HAM_GetGroupUtility()
    {
        return m_Utility;
    }

    override EAITargetClusterState EvaluateNewDesiredState(SCR_AITargetClusterState s)
    {
        EAITargetClusterState state = super.EvaluateNewDesiredState(s);

        if (state != EAITargetClusterState.INVESTIGATING)
            return state;

        SCR_AIGroupUtilityComponent utility = HAM_GetGroupUtility();
        if (utility && !utility.HAM_GetInvestigateEnabled())
            return EAITargetClusterState.NONE;

        return state;
    }

    override float CalculateMaxAgeThreshold_s(SCR_AITargetClusterState s, EAITargetClusterState newState)
    {
        float duration = super.CalculateMaxAgeThreshold_s(s, newState);

        SCR_AIGroupUtilityComponent utility = HAM_GetGroupUtility();
        if (!utility)
            return duration;

        return duration * (utility.HAM_GetInvestigateDurationS() / 120.0);
    }
}

modded class SCR_AIInvestigateClusterActivity : SCR_AIFireteamsClusterActivity
{
    override protected void SendInvestigateMessages(AICommunicationComponent comms, vector pos, float radius)
    {
        float minRadius = 20;

        if (m_Utility)
            minRadius = m_Utility.HAM_GetInvestigateRadiusMin();

        super.SendInvestigateMessages(comms, pos, Math.Max(radius, minRadius));
    }
}
