modded class SCR_AISetWeaponRaised : SCR_AICharacterStats
{
    protected SCR_AICombatComponent m_HAM_Combat;
    protected SCR_CharacterControllerComponent m_HAM_Controller;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        IEntity controlled = owner.GetControlledEntity();
        if (!controlled)
            return;

        m_HAM_Combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
        m_HAM_Controller = SCR_CharacterControllerComponent.Cast(controlled.FindComponent(SCR_CharacterControllerComponent));
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_HAM_Combat || !m_HAM_Controller)
            return super.EOnTaskSimulate(owner, dt);

        int mode = m_HAM_Combat.HAM_GetWeaponMode();
        if (mode != HAM_EAIWeaponMode.FORCE_LOWERED && mode != HAM_EAIWeaponMode.FORCE_RAISED)
            return super.EOnTaskSimulate(owner, dt);

        m_HAM_Controller.SetWeaponRaised(mode == HAM_EAIWeaponMode.FORCE_RAISED);

        return ENodeResult.SUCCESS;
    }
}
