modded class SCR_AIAttackBehavior : SCR_AIBehaviorBase
{
    override protected void InitWaitTime(SCR_AIUtilityComponent utility)
    {
        super.InitWaitTime(utility);

        if (!utility)
            return;

        SCR_AICombatComponent combat = utility.m_CombatComponent;
        if (!combat)
            return;

        m_fWaitTime.m_Value = m_fWaitTime.m_Value * combat.HAM_GetEngageDelayMul();
    }
}
