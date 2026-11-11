class HAM_AIImmobilizeRules
{
    static bool IsImmobilized(SCR_AIUtilityComponent utility)
    {
        if (!utility)
            return false;

        CharacterControllerComponent controller = utility.GetCharacterController();
        if (!controller)
            return false;

        return controller.GetDisableMovementControls();
    }
}

modded class SCR_AIMoveBehaviorBase
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}

modded class SCR_AIMoveInFormationBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}

modded class SCR_AIRetreatWhileLookAtBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}

modded class SCR_AIRetreatFromTargetBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}

modded class SCR_AIFindFirePositionBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}

modded class SCR_AIDefendBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_AIImmobilizeRules.IsImmobilized(m_Utility))
            return 0;

        return super.CustomEvaluate();
    }
}
