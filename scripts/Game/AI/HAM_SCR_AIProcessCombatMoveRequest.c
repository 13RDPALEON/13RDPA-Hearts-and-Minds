modded class SCR_AIProcessCombatMoveRequest
{
    protected CharacterControllerComponent m_HAM_Controller;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        IEntity controlled = owner.GetControlledEntity();
        if (controlled)
            m_HAM_Controller = CharacterControllerComponent.Cast(controlled.FindComponent(CharacterControllerComponent));
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        ENodeResult result = super.EOnTaskSimulate(owner, dt);

        if (result != ENodeResult.SUCCESS)
            return result;

        if (!m_HAM_Controller || !m_HAM_Controller.GetDisableMovementControls())
            return result;

        SCR_AICombatMoveRequestBase rq;
        if (!GetVariableIn(PORT_REQUEST, rq))
            return result;

        if (rq.m_eType != SCR_EAICombatMoveRequestType.MOVE)
            return result;

        SetVariableOut(PORT_TYPE, SCR_EAICombatMoveRequestType.STOP);

        return result;
    }
}
