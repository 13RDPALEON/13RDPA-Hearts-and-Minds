[BaseContainerProps()]
modded class SCR_AIDangerReaction_DoorMovement
{
    override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
    {
        bool reacted = super.PerformReaction(utility, threatSystem, dangerEvent, dangerEventCount);
        if (!reacted)
            return false;

        SCR_AICombatComponent combat = utility.m_CombatComponent;
        if (!combat || !HAM_AILookRules.IsHold(combat.HAM_GetLookModeEffective()))
            return true;

        SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());
        if (!agent)
            return true;

        IEntity actionStarter = dangerEvent.GetVictim();
        if (!actionStarter || !agent.IsPerceivedEnemy(actionStarter))
            return true;

        utility.m_LookAction.LookAt(actionStarter.GetOrigin(), SCR_AILookAction.PRIO_ENEMY_TARGET);

        return true;
    }
}
