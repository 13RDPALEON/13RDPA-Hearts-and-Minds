modded class SCR_AIGroupFireteamManager
{
    void HAM_GetFootFireteams(notnull array<SCR_AIGroupFireteam> outFireteams)
    {
        outFireteams.Clear();

        foreach (SCR_AIGroupFireteam ft : m_aFireteams)
        {
            if (!ft)
                continue;

            if (ft.Type() != SCR_AIGroupFireteam)
                continue;

            if (ft.GetMemberCount() == 0)
                continue;

            outFireteams.Insert(ft);
        }
    }

    override void RebalanceAllFireteams()
    {
        super.RebalanceAllFireteams();
        HAM_RefreshTeams();
    }

    override void OnAgentAdded(AIAgent agent)
    {
        super.OnAgentAdded(agent);
        HAM_RefreshTeams();

        HAM_PropagateDefaults();
    }

    override void OnAgentRemoved(AIAgent agent)
    {
        super.OnAgentRemoved(agent);
        HAM_RefreshTeams();
    }

    protected void HAM_PropagateDefaults()
    {
        if (!m_Group)
            return;

        SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
            m_Group.FindComponent(SCR_AIGroupUtilityComponent));

        if (utility)
            utility.HAM_PropagateCombatDefaults();
    }

    protected void HAM_RefreshTeams()
    {
        if (!m_Group)
            return;

        SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
            m_Group.FindComponent(SCR_AIGroupUtilityComponent));

        if (!utility)
            return;

        if (utility.HAM_GetCombatStyle() == HAM_EAICombatStyle.DEFAULT)
            return;

        utility.HAM_ApplyArticulation();
    }

    void HAM_EnforceTeamSizes()
    {
        array<SCR_AIGroupFireteam> teams = {};
        HAM_GetFreeFootFireteams(teams);

        int total = 0;
        foreach (SCR_AIGroupFireteam team : teams)
            total += team.GetMemberCount();

        if (total < 2)
            return;

        array<int> wanted = {};
        HAM_BuildTeamSizes(total, wanted);

        while (teams.Count() < wanted.Count())
            teams.Insert(CreateFireteam(m_aFireteams, SCR_AIGroupFireteam));

        while (wanted.Count() < teams.Count())
            wanted.Insert(0);

        for (int guard = 0; guard < 128; guard++)
        {
            int source = -1;
            int destination = -1;

            foreach (int i, SCR_AIGroupFireteam team : teams)
            {
                int count = team.GetMemberCount();

                if (source == -1 && count > wanted[i])
                    source = i;

                if (destination == -1 && count < wanted[i])
                    destination = i;
            }

            if (source == -1 || destination == -1)
                break;

            teams[destination].MoveMembersFrom(teams[source], 1);
        }

        for (int i = teams.Count() - 1; i >= 0; i--)
        {
            if (teams[i].GetMemberCount() == 0)
                RemoveFireteam(m_aFireteams, teams[i]);
        }
    }

    protected static void HAM_BuildTeamSizes(int total, notnull array<int> outSizes)
    {
        outSizes.Clear();

        if (total < 2)
            return;

        if (total == 3)
        {
            outSizes.Insert(3);
            return;
        }

        int teamCount = total / 2;
        int rest = total - (teamCount * 2);

        for (int i = 0; i < teamCount; i++)
        {
            int size = 2;
            if (i < rest)
                size++;

            outSizes.Insert(size);
        }
    }

    protected void HAM_GetFreeFootFireteams(notnull array<SCR_AIGroupFireteam> outFireteams)
    {
        HAM_GetFootFireteams(outFireteams);

        for (int i = outFireteams.Count() - 1; i >= 0; i--)
        {
            if (outFireteams[i].IsLocked())
                outFireteams.Remove(i);
        }
    }
}
