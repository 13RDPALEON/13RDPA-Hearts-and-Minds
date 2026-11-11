modded class SCR_AISelectFireMode
{
    protected SCR_AICombatComponent m_HAM_CombatComponent;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        SCR_AIUtilityComponent utility = SCR_AIUtilityComponent.Cast(
            owner.FindComponent(SCR_AIUtilityComponent));

        if (utility)
            m_HAM_CombatComponent = utility.m_CombatComponent;
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_HAM_CombatComponent)
            return super.EOnTaskSimulate(owner, dt);

        int wanted = m_HAM_CombatComponent.HAM_GetFireModeEffective();

        EWeaponFiremodeType forced;
        if (!HAM_ResolveFiremodeType(wanted, forced))
            return super.EOnTaskSimulate(owner, dt);

        EWeaponFiremodeType saved = m_FiremodeType;
        m_FiremodeType = forced;

        ENodeResult result = super.EOnTaskSimulate(owner, dt);

        m_FiremodeType = saved;

        return result;
    }

    protected bool HAM_ResolveFiremodeType(int wanted, out EWeaponFiremodeType type)
    {
        switch (wanted)
        {
            case HAM_EAIFireModeChoice.SEMI:
            {
                type = EWeaponFiremodeType.Semiauto;
                return true;
            }
            case HAM_EAIFireModeChoice.BURST:
            {
                type = EWeaponFiremodeType.Burst;
                return true;
            }
            case HAM_EAIFireModeChoice.FULL_AUTO:
            {
                type = EWeaponFiremodeType.Auto;
                return true;
            }
        }

        return false;
    }
}
