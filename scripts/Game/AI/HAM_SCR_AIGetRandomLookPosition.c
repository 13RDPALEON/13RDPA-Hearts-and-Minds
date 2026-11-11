modded class SCR_AIGetRandomLookPosition
{
    protected SCR_AICombatComponent m_HAM_Combat;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        m_HAM_Combat = HAM_AILookRules.CombatOf(owner);
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_HAM_Combat || !HAM_AILookRules.IsSteadyOrMore(m_HAM_Combat.HAM_GetLookModeEffective()))
            return super.EOnTaskSimulate(owner, dt);

        vector inPos;
        if (!GetVariableIn(PORT_IN_POS, inPos))
            return ENodeResult.FAIL;

        SetVariableOut(PORT_OUT_POS, inPos);

        return ENodeResult.SUCCESS;
    }
}
