modded class SCR_AISetStance : AITaskScripted
{
    protected SCR_AICombatComponent m_HAM_Combat;
    protected SCR_AIUtilityComponent m_HAM_Utility;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        m_HAM_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));

        IEntity controlled = owner.GetControlledEntity();
        if (controlled)
            m_HAM_Combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_HAM_Combat || !m_CharacterController)
            return super.EOnTaskSimulate(owner, dt);

        int mode = m_HAM_Combat.HAM_GetStanceModeEffective();
        if (mode == HAM_EAIStanceMode.AUTOMATIC)
            return super.EOnTaskSimulate(owner, dt);

        ECharacterStance desired;
        if (!GetVariableIn(STANCE_PORT, desired))
            desired = m_eStance;

        bool threatened = m_HAM_Utility && m_HAM_Utility.m_ThreatSystem
            && m_HAM_Utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;

        ECharacterStance forced;
        if (!HAM_AIStanceResolver.Resolve(mode, desired, threatened, forced))
            return super.EOnTaskSimulate(owner, dt);

        SCR_AIStanceHandling.SetStance(m_CharacterController, forced);

        return ENodeResult.SUCCESS;
    }
}
