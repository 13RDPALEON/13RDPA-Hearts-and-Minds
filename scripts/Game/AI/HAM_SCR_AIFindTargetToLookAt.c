modded class SCR_AIFindTagetToLookAt : AITaskScripted
{
    protected bool HAM_ShouldIgnoreForLook(BaseTarget target)
    {
        if (!target.IsDisarmed())
            return false;

        IEntity targetEntity = target.GetTargetEntity();
        if (!targetEntity)
            return true;

        ChimeraCharacter character = ChimeraCharacter.Cast(targetEntity);
        if (!character)
            return true;

        CharacterControllerComponent controller = character.GetCharacterController();
        if (!controller)
            return true;

        return controller.GetLifeState() != ECharacterLifeState.ALIVE;
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        if (!m_ConfigComp || !m_ConfigComp.m_EnablePerception || !m_PerceptionComp)
            return ENodeResult.FAIL;

        BaseTarget outTarget = null;

        float updateInterval = m_PerceptionComp.GetUpdateInterval();
        float timeSinceSeenMax = Math.Max(m_fTimeSinceSeenMax_S, updateInterval) + 0.02;
        float timeSinceDetectedMax = Math.Max(m_fTimeSinceDetectedMax_S, updateInterval) + 0.02;

        outTarget = m_PerceptionComp.GetClosestTarget(ETargetCategory.DETECTED, timeSinceSeenMax, timeSinceDetectedMax);

        if (!outTarget)
            outTarget = m_PerceptionComp.GetClosestTarget(ETargetCategory.ENEMY, timeSinceSeenMax, timeSinceDetectedMax);

        if (outTarget && HAM_ShouldIgnoreForLook(outTarget))
            outTarget = null;

        SetVariableOut(PORT_BASE_TARGET, outTarget);

        if (outTarget)
            return ENodeResult.SUCCESS;

        return ENodeResult.FAIL;
    }
}
