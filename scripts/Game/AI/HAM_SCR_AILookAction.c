class HAM_AILookTrace
{
    static const bool TRACE = false;

    static const float PERIOD_MS = 1000;

    static string BehaviorName(SCR_AIUtilityComponent utility)
    {
        if (!utility)
            return "sans utilitaire";

        SCR_AIBehaviorBase behavior = utility.GetCurrentBehavior();
        if (!behavior)
            return "aucun";

        return behavior.Type().ToString();
    }

    static int LookMode(SCR_AIUtilityComponent utility)
    {
        if (!utility || !utility.m_CombatComponent)
            return -1;

        return utility.m_CombatComponent.HAM_GetLookModeEffective();
    }
}

modded class SCR_AILookAction
{
    protected float m_fTRN_NextTrace;

    protected void HAM_TraceLook(string source, float priority, bool ignored)
    {
        if (!HAM_AILookTrace.TRACE)
            return;

        float now = GetGame().GetWorld().GetWorldTime();
        if (now < m_fTRN_NextTrace)
            return;

        m_fTRN_NextTrace = now + HAM_AILookTrace.PERIOD_MS;

        PrintFormat("[HAM_AI][REGARD] demande=%1 priorite=%2 comportement=%3 mode=%4 ecartee=%5",
            source, priority, HAM_AILookTrace.BehaviorName(m_Utility), HAM_AILookTrace.LookMode(m_Utility), ignored);
    }

    protected bool HAM_ShouldIgnoreLook(float priority)
    {
        int mode = HAM_AILookTrace.LookMode(m_Utility);

        if (!HAM_AILookRules.IsSteadyOrMore(mode))
            return false;

        if (HAM_AILookRules.IsHold(mode))
            return priority < PRIO_ENEMY_TARGET;

        return priority < PRIO_DANGER_EVENT;
    }

    override void LookAt(vector pos, float priority, float duration = 0.8)
    {
        bool ignored = HAM_ShouldIgnoreLook(priority);

        HAM_TraceLook("position", priority, ignored);

        if (ignored)
            return;

        super.LookAt(pos, priority, duration);
    }

    override void LookAt(IEntity ent, float priority, float duration = 0.8)
    {
        bool ignored = HAM_ShouldIgnoreLook(priority);

        HAM_TraceLook("entite", priority, ignored);

        if (ignored)
            return;

        super.LookAt(ent, priority, duration);
    }
}
