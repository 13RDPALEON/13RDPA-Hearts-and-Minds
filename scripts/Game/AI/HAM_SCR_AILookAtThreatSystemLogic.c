modded class SCR_AILookAtThreatSystemLogic
{
    protected static const float HAM_HOLD_DISTANCE = 25;

    protected static const float HAM_DANGER_THRESHOLD_RAISE_WEAPON = 0.5;

    protected SCR_AICombatComponent m_HAM_Combat;

    protected CharacterControllerComponent m_HAM_Controller;

    protected SCR_AIUtilityComponent m_HAM_Utility;

    protected vector m_vTRN_HeldPos;

    protected int m_iTRN_HeldSector = -1;

    protected bool m_bTRN_WeaponRaised;

    protected float m_fTRN_NextTrace;

    override void OnInit(AIAgent owner)
    {
        super.OnInit(owner);

        m_HAM_Combat = HAM_AILookRules.CombatOf(owner);

        m_HAM_Utility = SCR_AIUtilityComponent.Cast(owner.FindComponent(SCR_AIUtilityComponent));

        IEntity controlled = owner.GetControlledEntity();
        if (controlled)
            m_HAM_Controller = CharacterControllerComponent.Cast(
                controlled.FindComponent(CharacterControllerComponent));
    }

    protected override void SwitchState_ObserveMinorSector()
    {
        if (m_HAM_Combat && HAM_AILookRules.IsSteadyOrMore(m_HAM_Combat.HAM_GetLookModeEffective()))
        {
            SwitchState_Idle();
            return;
        }

        super.SwitchState_ObserveMinorSector();
    }

    override ENodeResult EOnTaskSimulate(AIAgent owner, float dt)
    {
        ENodeResult result = super.EOnTaskSimulate(owner, dt);

        int mode = 0;
        if (m_HAM_Combat)
            mode = m_HAM_Combat.HAM_GetLookModeEffective();

        if (!HAM_AILookRules.IsHold(mode))
            m_vTRN_HeldPos = vector.Zero;

        HAM_UpdateWeaponRaised(mode);

        HAM_ReArmLostContactLook();

        if (!HAM_MustHoldGaze(mode))
            return result;

        if (m_vTRN_HeldPos == vector.Zero)
            m_vTRN_HeldPos = HAM_ForwardPos();

        SetVariableOut(PORT_CURRENT_POSITION, m_vTRN_HeldPos);

        return result;
    }

    protected void HAM_UpdateWeaponRaised(int mode)
    {
        if (!m_HAM_Controller || !m_ThreatFilter || !m_MyEntity || !m_HAM_Combat)
            return;

        int weaponMode = m_HAM_Combat.HAM_GetWeaponMode();
        if (weaponMode == HAM_EAIWeaponMode.FORCE_LOWERED || weaponMode == HAM_EAIWeaponMode.FORCE_RAISED)
        {
            m_bTRN_WeaponRaised = false;
            return;
        }

        if (!HAM_AILookRules.IsFocusedOrMore(mode))
        {
            HAM_ReleaseWeaponRaised();
            return;
        }

        bool raise = false;
        if (m_iSectorMajor != -1)
        {
            vector sectorPos = m_ThreatFilter.GetSectorPos(m_iSectorMajor);
            if (!HAM_AILookRules.SectorTooFar(m_MyEntity.GetOrigin(), sectorPos))
                raise = m_ThreatFilter.GetSectorDanger(m_iSectorMajor) > HAM_DANGER_THRESHOLD_RAISE_WEAPON;
        }

        if (!raise)
        {
            HAM_ReleaseWeaponRaised();
            return;
        }

        m_bTRN_WeaponRaised = true;
        m_HAM_Controller.SetWeaponRaised(true);
    }

    protected void HAM_ReleaseWeaponRaised()
    {
        if (!m_bTRN_WeaponRaised)
            return;

        if (m_HAM_Combat && m_HAM_Combat.GetCurrentTarget())
            return;

        m_bTRN_WeaponRaised = false;
        m_HAM_Controller.SetWeaponRaised(false);
    }

    protected void HAM_ReArmLostContactLook()
    {
        if (!m_HAM_Combat || !m_HAM_Utility || !m_HAM_Utility.m_LookAction)
            return;

        vector pos;
        float remaining;
        if (!m_HAM_Combat.HAM_GetLostContactLook(pos, remaining))
            return;

        m_HAM_Utility.m_LookAction.LookAt(pos, SCR_AILookAction.PRIO_DANGER_EVENT, remaining);
    }

    protected vector HAM_ForwardPos()
    {
        vector mat[4];
        m_MyEntity.GetWorldTransform(mat);

        return m_MyEntity.GetOrigin() + mat[2] * HAM_HOLD_DISTANCE;
    }

    protected bool HAM_MustHoldGaze(int mode)
    {
        if (!m_ThreatFilter || !m_MyEntity || !m_HAM_Combat)
            return false;

        if (m_iSectorMajor == -1)
            return false;

        bool hold = false;
        if (!HAM_AILookRules.SectorConcernsMe(m_ThreatFilter.GetSectorFlags(m_iSectorMajor), mode))
        {
            hold = HAM_AILookRules.IsHold(mode);
            if (!hold)
                hold = HAM_AILookRules.SectorTooFar(m_MyEntity.GetOrigin(), m_ThreatFilter.GetSectorPos(m_iSectorMajor));
        }

        HAM_TraceSector(mode, hold);

        return hold;
    }

    protected void HAM_TraceSector(int mode, bool held)
    {
        if (!HAM_AILookTrace.TRACE)
            return;

        float now = GetGame().GetWorld().GetWorldTime();
        if (now < m_fTRN_NextTrace)
            return;

        m_fTRN_NextTrace = now + HAM_AILookTrace.PERIOD_MS;

        PrintFormat("[HAM_AI][SECTEUR] %1 mode=%2 majeur=%3 mineur=%4 drapeaux=%5 distance=%6 tenu=%7",
            HAM_AILookRules.PrefabName(m_MyEntity), mode, m_iSectorMajor, m_iSectorMinor,
            m_ThreatFilter.GetSectorFlags(m_iSectorMajor),
            vector.Distance(m_MyEntity.GetOrigin(), m_ThreatFilter.GetSectorPos(m_iSectorMajor)), held);
    }
}
