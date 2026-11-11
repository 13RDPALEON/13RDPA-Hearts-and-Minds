modded class SCR_AIGetAimErrorOffset : AITaskScripted
{
    override float GetRandomFactor(EAISkill skill, float mu)
    {
        float scale;

        if (m_CombatComponent)
            scale = m_CombatComponent.HAM_GetEffectiveAimErrorMul();
        else
            scale = HAM_AISettingsComponent.HAM_AimErrorScale();

        return super.GetRandomFactor(skill, mu) * scale;
    }
}
