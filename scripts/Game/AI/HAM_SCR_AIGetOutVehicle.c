modded class SCR_AIGetOutVehicle
{
    override void OnActionSelected()
    {
        super.OnActionSelected();

        if (!m_Utility || !m_Utility.m_CombatComponent)
            return;

        if (!m_Utility.m_CombatComponent.HAM_ShouldStayInVehicle())
            return;

        if (m_Utility.FindActionOfType(SCR_AIPerformActionBehavior))
            return;

        AIActionBase getIn = m_Utility.FindActionOfType(SCR_AIGetInVehicle);
        if (getIn)
            getIn.Fail();

        Fail();
    }
}
