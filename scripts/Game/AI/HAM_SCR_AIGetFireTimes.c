modded class SCR_AIGetFireTimes : AITaskScripted
{
    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_WeaponConfig || !m_CombatComponent)
            return super.EOnTaskSimulate(owner, dt);

        float mul = m_CombatComponent.HAM_GetReactionMul();
        if (mul == 1.0)
            return super.EOnTaskSimulate(owner, dt);

        if (mul < 0.3)
            mul = 0.3;

        float savedStabilization = m_WeaponConfig.m_fBaseStabilizationTime;
        m_WeaponConfig.m_fBaseStabilizationTime = savedStabilization * mul;

        ENodeResult result = super.EOnTaskSimulate(owner, dt);

        m_WeaponConfig.m_fBaseStabilizationTime = savedStabilization;

        return result;
    }
}
