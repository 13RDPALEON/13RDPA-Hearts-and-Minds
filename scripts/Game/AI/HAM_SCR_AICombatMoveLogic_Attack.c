modded class SCR_AICombatMoveLogic_Attack : SCR_AICombatMoveLogicBase
{
    protected AIGroup m_HAM_CachedGroup;
    protected SCR_AIGroupUtilityComponent m_HAM_GroupUtility;

    protected vector m_vTRN_ApproachTargetPos;

    protected HAM_EAICombatStyle HAM_GetStyle()
    {
        if (!m_Utility)
            return HAM_EAICombatStyle.DEFAULT;

        AIAgent agent = m_Utility.GetAIAgent();
        if (!agent)
            return HAM_EAICombatStyle.DEFAULT;

        AIGroup group = agent.GetParentGroup();
        if (group != m_HAM_CachedGroup)
        {
            m_HAM_CachedGroup = group;
            m_HAM_GroupUtility = null;

            if (group)
                m_HAM_GroupUtility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
        }

        if (!m_HAM_GroupUtility)
            return HAM_EAICombatStyle.DEFAULT;

        return m_HAM_GroupUtility.HAM_GetCombatStyle();
    }

    protected bool HAM_FiresBeforeSeekingCover()
    {
        if (!m_HAM_GroupUtility)
            return false;

        return m_HAM_GroupUtility.HAM_FiresBeforeSeekingCover();
    }

    protected override float ResolveStoppedWaitTime(bool inCover, EAIThreatState threat, EWeaponType weaponType)
    {
        float waitTime = super.ResolveStoppedWaitTime(inCover, threat, weaponType);

        HAM_GetStyle();
        if (!m_HAM_GroupUtility || !m_HAM_GroupUtility.HAM_OverridesProgression())
            return waitTime;

        return waitTime * (m_HAM_GroupUtility.HAM_GetHoldTimeS() / 5.0);
    }

    protected bool HAM_IsReloadingExposed()
    {
        if (!m_HAM_GroupUtility || !m_HAM_GroupUtility.HAM_GetSeekCoverToReload())
            return false;

        if (m_State.m_bInCover)
            return false;

        return m_CharacterController && m_CharacterController.IsReloading();
    }

    protected override bool ResolveFailMoveIfNoCover()
    {
        HAM_GetStyle();

        if (HAM_IsReloadingExposed())
            return true;

        bool firesFirst = HAM_FiresBeforeSeekingCover();

        if (m_HAM_GroupUtility && m_HAM_GroupUtility.HAM_OverridesProgression()
            && !m_HAM_GroupUtility.HAM_GetRequiresCover())
            return false;

        if (firesFirst && IsFirstExecution())
            return m_State.m_bInCover;

        return super.ResolveFailMoveIfNoCover();
    }

    protected override void ResolveMoveRequestMovePosAndDir(vector targetPos, out vector outMovePos,
        out vector outAvoidStraightPathDir, out SCR_EAICombatMoveDirection outDirection,
        out float outCoverSearchSectorHalfAngleRad)
    {
        m_vTRN_ApproachTargetPos = targetPos;

        super.ResolveMoveRequestMovePosAndDir(targetPos, outMovePos, outAvoidStraightPathDir,
            outDirection, outCoverSearchSectorHalfAngleRad);

        if (!m_CombatComp || m_CombatComp.HAM_GetCombatRole() != HAM_EAICombatRole.COVER)
            return;

        HAM_GetStyle();
        if (!m_HAM_GroupUtility || !m_HAM_GroupUtility.HAM_GetCombatRoles())
            return;

        if (outDirection == SCR_EAICombatMoveDirection.CUSTOM_POS
            || outDirection == SCR_EAICombatMoveDirection.FORWARD)
        {
            outDirection = SCR_EAICombatMoveDirection.ANYWHERE;
        }
    }

    protected override vector GetAvoidStraightPathDir()
    {
        HAM_GetStyle();
        if (!m_HAM_GroupUtility || !m_HAM_GroupUtility.HAM_GetFlankingApproach())
            return super.GetAvoidStraightPathDir();

        if (m_CombatComp && m_CombatComp.HAM_GetCombatRole() == HAM_EAICombatRole.COVER)
            return vector.Zero;

        if (!m_MyEntity || m_vTRN_ApproachTargetPos == vector.Zero)
            return vector.Zero;

        vector dir = vector.Direction(m_MyEntity.GetOrigin(), m_vTRN_ApproachTargetPos);
        dir[1] = 0;

        if (dir.LengthSq() < 0.01)
            return vector.Zero;

        dir.Normalize();
        return dir;
    }

    protected override bool MoveToNextPosCondition()
    {
        HAM_GetStyle();

        if (HAM_IsReloadingExposed() && !m_State.IsExecutingRequest())
            return true;

        bool firesFirst = HAM_FiresBeforeSeekingCover();

        if ((!m_HAM_GroupUtility || !m_HAM_GroupUtility.HAM_OverridesProgression()) && !firesFirst)
            return super.MoveToNextPosCondition();

        float approachFloor = -1;
        if (m_HAM_GroupUtility)
            approachFloor = m_HAM_GroupUtility.HAM_GetApproachFloor();
        if (approachFloor < 0)
            approachFloor = Math.Max(m_fWeaponMinDist + 5.0, 15.0);

        if (m_Target && m_fTargetDist < approachFloor && m_Target.GetTimeSinceSeen() < 15)
            return false;

        if (m_State.IsExecutingRequest())
            return false;

        if (IsFirstExecution() && !m_State.m_bInCover && !firesFirst)
            return true;

        bool holdFire = m_CombatComp.GetCombatMode() == EAIGroupCombatMode.HOLD_FIRE;
        bool formationLocked = m_Utility.ShouldKeepFormation();

        if (holdFire || (formationLocked && HAM_GetStyle() != HAM_EAICombatStyle.CQB_OFFENSIVE))
        {
            if (!IsFirstExecution())
                return false;
        }

        float stoppedWaitTime = ResolveStoppedWaitTime(m_State.m_bInCover, m_eThreatState, m_eWeaponType);
        return m_State.m_fTimerStopped_s > stoppedWaitTime;
    }
}
