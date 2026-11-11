modded class SCR_AICombatComponent : ScriptComponent
{
    static const float HAM_UNCAPPED_CRUISE_SPEED = 200;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Perception de ce personnage. 1 = suit le reglage global ; toute autre valeur l'emporte seule, jamais de cumul.", params: "0 3 0.1", category: "IA")]
    protected float m_fTRN_PerceptionMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Dispersion du tir de ce personnage. 1 = suit le reglage global ; toute autre valeur l'emporte seule, jamais de cumul.", params: "0 4 0.1", category: "IA")]
    protected float m_fTRN_AimErrorMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Temps de visee avant tir. 1 = suit le reglage global ; toute autre valeur l'emporte seule, jamais de cumul.", params: "0 4 0.1", category: "IA")]
    protected float m_fTRN_ReactionMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Delai entre detection et premier coup de feu. 1 = suit le reglage global ; toute autre valeur l'emporte seule, jamais de cumul.", params: "0 5 0.1", category: "IA")]
    protected float m_fTRN_EngageDelayMul;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Competence de tir de ce personnage. NONE = suit le reglage global.", enums: ParamEnumArray.FromEnum(EAISkill), category: "IA")]
    protected EAISkill m_eTRN_SkillOverride;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Posture imposee. FROM_GLOBAL = suit le reglage general du scenario. AUTOMATIC = l'IA decide seule (comportement d'origine).", enums: ParamEnumArray.FromEnum(HAM_EAIStanceMode), category: "IA")]
    protected HAM_EAIStanceMode m_eTRN_StanceMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Regard. FROM_GLOBAL = suit le reglage general du scenario. HOLD = tient sa direction, ne pivote plus vers un bruit lointain. AUTOMATIC = comportement d origine.", enums: ParamEnumArray.FromEnum(HAM_EAILookMode), category: "IA")]
    protected HAM_EAILookMode m_eTRN_LookMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Duree pendant laquelle ce personnage continue de regarder un contact qu il ne voit plus, en secondes. 0 = suit le reglage general du scenario", params: "0 60 1", category: "IA")]
    protected float m_fTRN_ContactMemory;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Port de l'arme de ce personnage. Suivre le reglage general = son groupe decide, a defaut le scenario. Automatique = comportement d'origine. Engagement = arme sortie seulement face a une cible ennemie. Alerte = arme sortie tant que l'IA reste en alerte. Maintenue = arme sortie apres le premier engagement.", enums: ParamEnumArray.FromEnum(HAM_EAIWeaponModeSetting), category: "IA")]
    protected HAM_EAIWeaponMode m_eTRN_WeaponMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Mode de tir de ce personnage. Suivre le reglage general = son groupe decide, a defaut le scenario. Au choix de l'IA = comportement d'origine. Les autres valeurs imposent le selecteur de l'arme quand l'arme possede ce mode.", enums: ParamEnumArray.FromEnum(HAM_EAIFireModeChoice), category: "IA")]
    protected HAM_EAIFireModeChoice m_eTRN_FireMode;

    protected vector m_vTRN_LostContactPos;

    protected float m_fTRN_LostContactUntil;

    protected HAM_EAICombatRole m_eTRN_CombatRole;

    protected Managed m_HAM_RoleOwner;

    protected bool m_bTRN_SkillApplied;

    static void HAM_ApplySettingsToAllAgents()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent comp = SCR_AICombatComponent.Cast(
                controlled.FindComponent(SCR_AICombatComponent));

            if (comp)
                comp.HAM_ApplyAISettings();
        }
    }

    static void HAM_ApplyStanceToAllAgents()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent comp = SCR_AICombatComponent.Cast(
                controlled.FindComponent(SCR_AICombatComponent));

            if (comp)
                comp.HAM_ApplyStanceNow();
        }
    }

    static void HAM_ApplyWeaponModeToAllAgents()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent comp = SCR_AICombatComponent.Cast(
                controlled.FindComponent(SCR_AICombatComponent));
            if (comp)
                comp.HAM_ApplyConfiguredWeaponMode();
        }
    }

    float HAM_GetPerceptionMul() { return m_fTRN_PerceptionMul; }
    float HAM_GetAimErrorMul()   { return m_fTRN_AimErrorMul; }
    int   HAM_GetSkillOverride() { return m_eTRN_SkillOverride; }

    static float HAM_ComposeSetting(float agentMul, float globalMul)
    {
        if (!HAM_IsNeutral(agentMul))
            return agentMul;

        return globalMul;
    }

    float HAM_GetReactionMul()
    {
        return HAM_ComposeSetting(m_fTRN_ReactionMul, HAM_AISettingsComponent.HAM_ReactionScale());
    }

    float HAM_GetReactionMulRaw() { return m_fTRN_ReactionMul; }

    void HAM_SetReactionMul(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_ReactionMul = v;
    }

    float HAM_GetEngageDelayMul()
    {
        return HAM_ComposeSetting(m_fTRN_EngageDelayMul, HAM_AISettingsComponent.HAM_EngageDelayScale());
    }

    float HAM_GetEffectiveAimErrorMul()
    {
        return HAM_ComposeSetting(m_fTRN_AimErrorMul, HAM_AISettingsComponent.HAM_AimErrorScale());
    }

    float HAM_GetEngageDelayMulRaw() { return m_fTRN_EngageDelayMul; }

    void HAM_SetEngageDelayMul(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_EngageDelayMul = v;
    }

    void HAM_SetPerceptionMul(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_PerceptionMul = v;
        HAM_ApplyAISettings();
    }

    void HAM_SetAimErrorMul(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_AimErrorMul = v;
    }

    void HAM_SetSkillOverride(int skill)
    {
        m_eTRN_SkillOverride = skill;
        HAM_ApplyAISettings();
    }

    int HAM_GetStanceMode() { return m_eTRN_StanceMode; }

    int HAM_GetLookMode() { return m_eTRN_LookMode; }

    float HAM_GetContactMemory() { return m_fTRN_ContactMemory; }

    void HAM_SetContactMemory(float seconds)
    {
        m_fTRN_ContactMemory = seconds;
    }

    float HAM_GetContactMemoryEffective()
    {
        if (m_fTRN_ContactMemory > 0)
            return m_fTRN_ContactMemory;

        return HAM_AISettingsComponent.HAM_ContactMemory();
    }

    override void EvaluateWeaponAndTarget(out bool outWeaponEvent, out bool outSelectedTargetChanged,
        out BaseTarget outPrevTarget, out BaseTarget outCurrentTarget,
        out bool outRetreatTargetChanged, out bool outCompartmentChanged)
    {
        super.EvaluateWeaponAndTarget(outWeaponEvent, outSelectedTargetChanged,
            outPrevTarget, outCurrentTarget, outRetreatTargetChanged, outCompartmentChanged);

        int weaponMode = HAM_GetWeaponModeEffective();
        if (outSelectedTargetChanged)
        {
            if (outCurrentTarget)
                m_fTRN_LostContactUntil = 0;
            else
                HAM_RememberLostContact(outPrevTarget);
        }
        else if (weaponMode != HAM_EAIWeaponMode.ENGAGEMENT_STICKY)
        {
            return;
        }

        if (weaponMode != HAM_EAIWeaponMode.AUTOMATIC && !HAM_HasManualWeaponOrder())
            HAM_ApplyConfiguredWeaponMode();
    }

    protected void HAM_RememberLostContact(BaseTarget lostTarget)
    {
        m_fTRN_LostContactUntil = 0;

        if (!lostTarget)
            return;

        if (lostTarget.GetTargetCategory() == ETargetCategory.FRIENDLY)
            return;

        if (lostTarget.IsDisarmed())
            return;

        float memory = HAM_GetContactMemoryEffective();
        if (memory <= 0)
            return;

        vector pos = lostTarget.GetLastSeenPosition();
        if (pos == vector.Zero)
            return;

        IEntity targetEntity = lostTarget.GetTargetEntity();
        if (targetEntity)
        {
            vector bmin, bmax;
            targetEntity.GetBounds(bmin, bmax);
            pos = pos + 0.5 * (bmin + bmax);
        }

        m_vTRN_LostContactPos = pos;
        m_fTRN_LostContactUntil = GetGame().GetWorld().GetWorldTime() + memory * 1000;
    }

    bool HAM_GetLostContactLook(out vector outPos, out float outRemaining)
    {
        if (m_fTRN_LostContactUntil <= 0)
            return false;

        if (GetCurrentTarget())
            return false;

        float remaining = m_fTRN_LostContactUntil - GetGame().GetWorld().GetWorldTime();
        if (remaining <= 0)
        {
            m_fTRN_LostContactUntil = 0;
            return false;
        }

        outPos = m_vTRN_LostContactPos;
        outRemaining = remaining * 0.001;
        return true;
    }

    void HAM_SetLookMode(int mode)
    {
        m_eTRN_LookMode = mode;
    }

    int HAM_GetLookModeEffective()
    {
        if (m_CharacterController && m_CharacterController.GetDisableMovementControls())
            return HAM_EAILookMode.HOLD;

        if (m_eTRN_LookMode != HAM_EAILookMode.FROM_GLOBAL)
            return m_eTRN_LookMode;

        return HAM_AISettingsComponent.HAM_LookMode();
    }

    int HAM_GetStanceModeEffective()
    {
        if (m_eTRN_StanceMode != HAM_EAIStanceMode.FROM_GLOBAL)
            return m_eTRN_StanceMode;

        return HAM_AISettingsComponent.HAM_StanceMode();
    }

    int HAM_GetCombatRole() { return m_eTRN_CombatRole; }

    void HAM_SetCombatRole(int role, Managed owner)
    {
        m_eTRN_CombatRole = role;
        m_HAM_RoleOwner = owner;
    }

    void HAM_ClearCombatRole(Managed owner)
    {
        if (m_HAM_RoleOwner != owner)
            return;

        m_eTRN_CombatRole = HAM_EAICombatRole.NONE;
        m_HAM_RoleOwner = null;
    }

    protected HAM_EAIWeaponMode m_eTRN_PreviousWeaponMode;
    protected bool m_bTRN_TemporaryWeaponMode;
    protected bool m_bTRN_WeaponEngagementSeen;
    protected IEntity m_pTRN_HolsteredWeapon;
    protected bool m_bTRN_WeaponUnequipped;
    protected bool m_bTRN_InitialWeaponModeApplied;
    protected int m_iTRN_InitialWeaponModeAttempts;

    protected static const int HAM_INITIAL_WEAPON_MODE_DELAY_MS = 50;
    protected static const int HAM_INITIAL_WEAPON_MODE_MAX_ATTEMPTS = 100;

    int HAM_GetWeaponMode() { return m_eTRN_WeaponMode; }

    int HAM_GetPersistedWeaponMode()
    {
        if (m_bTRN_TemporaryWeaponMode)
            return m_eTRN_PreviousWeaponMode;

        return m_eTRN_WeaponMode;
    }

    bool HAM_HasWeaponEngagement()
    {
        BaseTarget target = GetCurrentTarget();
        return target && target.GetTargetCategory() == ETargetCategory.ENEMY;
    }

    bool HAM_HasWeaponAlert()
    {
        if (!m_Utility || !m_Utility.m_ThreatSystem)
            return false;

        return m_Utility.m_ThreatSystem.GetState() != EAIThreatState.SAFE;
    }

    bool HAM_IsWeaponEquipped()
    {
        if (!m_CharacterController)
            return false;

        BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
        return weaponManager && weaponManager.GetCurrentWeapon();
    }

    bool HAM_HasManualWeaponOrder()
    {
        return m_eTRN_WeaponMode == HAM_EAIWeaponMode.FORCE_LOWERED
            || m_eTRN_WeaponMode == HAM_EAIWeaponMode.FORCE_RAISED;
    }

    int HAM_GetWeaponModeEffective()
    {
        if (m_eTRN_WeaponMode != HAM_EAIWeaponMode.FROM_GLOBAL)
            return m_eTRN_WeaponMode;

        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility && groupUtility.HAM_GetGroupWeaponMode() != HAM_EAIWeaponMode.FROM_GLOBAL)
            return groupUtility.HAM_GetGroupWeaponMode();

        HAM_AISettingsComponent settings = HAM_AISettingsComponent.GetInstance();
        if (!settings)
            return HAM_EAIWeaponMode.AUTOMATIC;

        return settings.HAM_GetWeaponMode();
    }

    int  HAM_GetFireMode()         { return m_eTRN_FireMode; }
    void HAM_SetFireMode(int mode) { m_eTRN_FireMode = mode; }

    int HAM_GetFireModeEffective()
    {
        if (m_eTRN_FireMode != HAM_EAIFireModeChoice.FROM_GLOBAL)
            return m_eTRN_FireMode;

        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility && groupUtility.HAM_GetGroupFireMode() != HAM_EAIFireModeChoice.FROM_GLOBAL)
            return groupUtility.HAM_GetGroupFireMode();

        HAM_AISettingsComponent settings = HAM_AISettingsComponent.GetInstance();
        if (!settings)
            return HAM_EAIFireModeChoice.AUTOMATIC;

        return settings.HAM_GetFireMode();
    }

    bool HAM_ShouldCarryWeapon()
    {
        int mode = HAM_GetWeaponModeEffective();
        if (mode != HAM_EAIWeaponMode.ENGAGEMENT_STICKY)
            m_bTRN_WeaponEngagementSeen = false;

        if (mode == HAM_EAIWeaponMode.ALERT_ONLY)
            return HAM_HasWeaponAlert();
        if (mode == HAM_EAIWeaponMode.ENGAGEMENT_ONLY || mode == HAM_EAIWeaponMode.TEMPORARY_ENGAGEMENT)
            return HAM_HasWeaponEngagement();
        if (mode == HAM_EAIWeaponMode.ENGAGEMENT_STICKY)
        {
            SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
            bool groupSticky = m_eTRN_WeaponMode == HAM_EAIWeaponMode.FROM_GLOBAL
                && groupUtility && groupUtility.HAM_GetGroupWeaponMode() == HAM_EAIWeaponMode.ENGAGEMENT_STICKY;

            if (HAM_HasWeaponEngagement())
            {
                m_bTRN_WeaponEngagementSeen = true;
                if (groupSticky)
                    groupUtility.HAM_MarkGroupWeaponEngagementSeen();
            }

            if (groupSticky)
                return groupUtility.HAM_HasGroupWeaponEngagementSeen();

            if (!m_bTRN_WeaponEngagementSeen)
                return false;

            return true;
        }

        return true;
    }

    void HAM_ApplyConfiguredWeaponMode()
    {
        if (!m_CharacterController || !Replication.IsServer())
            return;

        if (HAM_GetWeaponModeEffective() == HAM_EAIWeaponMode.AUTOMATIC || HAM_HasManualWeaponOrder())
            return;

        bool carry = HAM_ShouldCarryWeapon();
        HAM_ApplyWeaponCarriage(carry);

        if (carry && HAM_GetWeaponModeEffective() == HAM_EAIWeaponMode.TEMPORARY_ENGAGEMENT && HAM_HasWeaponEngagement())
            HAM_RestoreTemporaryWeaponMode();
    }

    void HAM_ApplyWeaponCarriage(bool carry)
    {
        if (!m_CharacterController || !Replication.IsServer())
            return;

        if (m_CharacterController.IsChangingItem())
            return;

        if (!carry)
        {
            BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
            BaseWeaponComponent currentWeapon;
            if (weaponManager)
                currentWeapon = weaponManager.GetCurrentWeapon();

            if (currentWeapon)
            {
                m_pTRN_HolsteredWeapon = currentWeapon.GetOwner();
                SCR_CharacterControllerComponent scrController = SCR_CharacterControllerComponent.Cast(m_CharacterController);
                if (scrController)
                    scrController.ActionUnequipItem();
                m_bTRN_WeaponUnequipped = true;
            }

            return;
        }

        BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
        BaseWeaponComponent currentWeapon;
        if (weaponManager)
            currentWeapon = weaponManager.GetCurrentWeapon();

        if (!currentWeapon && m_bTRN_WeaponUnequipped && m_pTRN_HolsteredWeapon)
        {
            m_CharacterController.TryEquipRightHandItem(m_pTRN_HolsteredWeapon, EEquipItemType.EEquipTypeWeapon, false);
        }
        else if (currentWeapon)
            m_bTRN_WeaponUnequipped = false;

    }

    void HAM_RequestTemporaryWeaponHolster()
    {
        if (m_bTRN_TemporaryWeaponMode)
            return;

        m_eTRN_PreviousWeaponMode = m_eTRN_WeaponMode;
        m_eTRN_WeaponMode = HAM_EAIWeaponMode.TEMPORARY_ENGAGEMENT;
        m_bTRN_TemporaryWeaponMode = true;
        HAM_ApplyWeaponCarriage(false);
    }

    bool HAM_CanDrawWeapon()
    {
        if (!m_CharacterController || HAM_IsWeaponEquipped())
            return false;

        return HAM_FindWeaponToDraw() != null;
    }

    protected IEntity HAM_FindWeaponToDraw()
    {
        BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
        if (!weaponManager)
            return null;

        array<WeaponSlotComponent> slots = {};
        weaponManager.GetWeaponsSlots(slots);

        if (m_pTRN_HolsteredWeapon)
        {
            foreach (WeaponSlotComponent slot : slots)
            {
                if (slot.GetWeaponEntity() == m_pTRN_HolsteredWeapon)
                    return m_pTRN_HolsteredWeapon;
            }
        }

        foreach (WeaponSlotComponent slot : slots)
        {
            if (slot.GetWeaponEntity())
                return slot.GetWeaponEntity();
        }

        return null;
    }

    void HAM_RequestWeaponDraw()
    {
        if (!m_CharacterController || !Replication.IsServer() || m_CharacterController.IsChangingItem())
            return;

        IEntity weapon = HAM_FindWeaponToDraw();
        if (!weapon)
            return;

        if (m_bTRN_TemporaryWeaponMode)
            HAM_RestoreTemporaryWeaponMode();

        int mode = HAM_GetWeaponModeEffective();
        if (mode != HAM_EAIWeaponMode.AUTOMATIC && !HAM_HasManualWeaponOrder() && !HAM_ShouldCarryWeapon())
            m_eTRN_WeaponMode = HAM_EAIWeaponMode.AUTOMATIC;

        m_pTRN_HolsteredWeapon = weapon;
        m_bTRN_WeaponUnequipped = false;
        m_CharacterController.TryEquipRightHandItem(weapon, EEquipItemType.EEquipTypeWeapon, false);
    }

    void HAM_RestoreTemporaryWeaponMode()
    {
        if (!m_bTRN_TemporaryWeaponMode)
            return;

        m_eTRN_WeaponMode = m_eTRN_PreviousWeaponMode;
        m_bTRN_TemporaryWeaponMode = false;
        m_bTRN_WeaponEngagementSeen = false;

    }

    void HAM_SetWeaponMode(int mode)
    {
        m_bTRN_TemporaryWeaponMode = false;
        m_bTRN_WeaponEngagementSeen = false;
        m_eTRN_WeaponMode = mode;

        if (!m_CharacterController)
            return;

        if (mode == HAM_EAIWeaponMode.FORCE_LOWERED || mode == HAM_EAIWeaponMode.FORCE_RAISED)
        {
            m_CharacterController.SetWeaponRaised(mode == HAM_EAIWeaponMode.FORCE_RAISED);
            return;
        }

        if (HAM_GetWeaponModeEffective() == HAM_EAIWeaponMode.AUTOMATIC)
            return;

        HAM_ApplyConfiguredWeaponMode();
    }

    bool HAM_HasOwnWeaponMode()
    {
        return m_eTRN_WeaponMode != HAM_EAIWeaponMode.FROM_GLOBAL;
    }

    protected HAM_EAIStayInVehicleMode m_eTRN_StayInVehicleMode;

    int  HAM_GetStayInVehicleMode() { return m_eTRN_StayInVehicleMode; }
    void HAM_SetStayInVehicleMode(int mode) { m_eTRN_StayInVehicleMode = mode; }

    int HAM_StayInVehicleModeEffective()
    {
        if (m_eTRN_StayInVehicleMode != HAM_EAIStayInVehicleMode.FROM_GLOBAL)
            return m_eTRN_StayInVehicleMode;

        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility)
        {
            int groupMode = groupUtility.HAM_GetGroupStayInVehicleMode();
            if (groupMode != HAM_EAIStayInVehicleMode.FROM_GLOBAL)
                return groupMode;
        }

        return HAM_AISettingsComponent.HAM_StayInVehicleMode();
    }

    bool HAM_ShouldStayInVehicle()
    {
        int mode = HAM_StayInVehicleModeEffective();
        if (mode == HAM_EAIStayInVehicleMode.FREE || mode == HAM_EAIStayInVehicleMode.FROM_GLOBAL)
            return false;

        if (mode == HAM_EAIStayInVehicleMode.ALL)
            return true;

        if (!m_CompartmentAccess)
            return false;

        BaseCompartmentSlot slot = m_CompartmentAccess.GetCompartment();
        if (!slot)
            return false;

        if (PilotCompartmentSlot.Cast(slot))
            return true;

        if (TurretCompartmentSlot.Cast(slot))
            return true;

        return false;
    }

    void HAM_SetStanceMode(int mode)
    {
        m_eTRN_StanceMode = mode;

        HAM_ApplyStanceNow();
    }

    void HAM_ApplyStanceNow()
    {
        if (!m_CharacterController)
            return;

        bool threatened = m_Utility && m_Utility.m_ThreatSystem
            && m_Utility.m_ThreatSystem.GetState() == EAIThreatState.THREATENED;

        ECharacterStance forced;
        if (HAM_AIStanceResolver.Resolve(HAM_GetStanceModeEffective(), m_CharacterController.GetStance(), threatened, forced))
            SCR_AIStanceHandling.SetStance(m_CharacterController, forced);
    }

    void HAM_AdoptGroupDefaults(float perception, float aimError, float reaction, float engageDelay, int skill, int stanceMode, int lookMode, float contactMemory)
    {
        if (m_fTRN_ContactMemory <= 0)
            m_fTRN_ContactMemory = contactMemory;

        if (m_eTRN_StanceMode == HAM_EAIStanceMode.FROM_GLOBAL)
            m_eTRN_StanceMode = stanceMode;

        if (m_eTRN_LookMode == HAM_EAILookMode.FROM_GLOBAL)
            m_eTRN_LookMode = lookMode;

        if (HAM_IsNeutral(m_fTRN_PerceptionMul))
            m_fTRN_PerceptionMul = perception;

        if (HAM_IsNeutral(m_fTRN_AimErrorMul))
            m_fTRN_AimErrorMul = aimError;

        if (HAM_IsNeutral(m_fTRN_ReactionMul))
            m_fTRN_ReactionMul = reaction;

        if (HAM_IsNeutral(m_fTRN_EngageDelayMul))
            m_fTRN_EngageDelayMul = engageDelay;

        if (m_eTRN_SkillOverride == EAISkill.NONE)
            m_eTRN_SkillOverride = skill;

        HAM_ApplyAISettings();
    }

    protected static bool HAM_IsNeutral(float value)
    {
        return Math.AbsFloat(value - 1.0) < 0.001;
    }

    void HAM_ApplyAISettings()
    {
        int skill = m_eTRN_SkillOverride;
        if (skill == EAISkill.NONE)
            skill = HAM_AISettingsComponent.HAM_AISkill();

        if (skill != EAISkill.NONE)
        {
            SetAISkill(skill);
            m_bTRN_SkillApplied = true;
        }
        else if (m_bTRN_SkillApplied)
        {
            ResetAISkill();
            m_bTRN_SkillApplied = false;
        }

        m_fPerceptionFactor = HAM_ComposeSetting(m_fTRN_PerceptionMul, HAM_AISettingsComponent.HAM_PerceptionFactor());

        if (m_Perception && m_Utility && m_Utility.m_ThreatSystem)
            UpdatePerceptionFactor(m_Perception, m_Utility.m_ThreatSystem);

        HAM_ApplyNavPreset();
    }

    protected void HAM_ApplyInitialWeaponMode()
    {
        if (m_bTRN_InitialWeaponModeApplied)
            return;

        if (HAM_GetWeaponModeEffective() == HAM_EAIWeaponMode.AUTOMATIC)
            return;

        if (!m_CharacterController)
            return;

        if (m_CharacterController.IsChangingItem())
        {
            HAM_ScheduleInitialWeaponModeRetry();
            return;
        }

        bool carry = HAM_ShouldCarryWeapon();
        HAM_ApplyConfiguredWeaponMode();

        if (!carry && HAM_IsWeaponEquipped())
        {
            HAM_ScheduleInitialWeaponModeRetry();
            return;
        }

        m_bTRN_InitialWeaponModeApplied = true;
    }

    protected void HAM_ScheduleInitialWeaponModeRetry()
    {
        m_iTRN_InitialWeaponModeAttempts++;
        if (m_iTRN_InitialWeaponModeAttempts > HAM_INITIAL_WEAPON_MODE_MAX_ATTEMPTS)
        {
            m_bTRN_InitialWeaponModeApplied = true;
            return;
        }

        ScriptCallQueue callQueue = GetGame().GetCallqueue();
        callQueue.Remove(HAM_ApplyInitialWeaponMode);
        callQueue.CallLater(HAM_ApplyInitialWeaponMode, HAM_INITIAL_WEAPON_MODE_DELAY_MS, false);
    }

    override protected void Event_OnInventoryChanged(IEntity item, BaseInventoryStorageComponent storageOwner)
    {
        super.Event_OnInventoryChanged(item, storageOwner);

        if (m_bTRN_InitialWeaponModeApplied || !Replication.IsServer())
            return;

        m_iTRN_InitialWeaponModeAttempts = 0;
        ScriptCallQueue callQueue = GetGame().GetCallqueue();
        callQueue.Remove(HAM_ApplyInitialWeaponMode);
        callQueue.CallLater(HAM_ApplyInitialWeaponMode, HAM_INITIAL_WEAPON_MODE_DELAY_MS, false);
    }

    void HAM_ApplyNavPreset()
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;

        AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(
            owner.FindComponent(AIPathfindingComponent));

        if (pathfinding)
            pathfinding.SetAreaCosts(HAM_FootNavConfigEffective());

        if (!m_CompartmentAccess)
            return;

        BaseCompartmentSlot slot = m_CompartmentAccess.GetCompartment();
        if (slot && PilotCompartmentSlot.Cast(slot))
            HAM_PushVehicleNav(slot.GetVehicle());
    }

    protected ResourceName HAM_FootNavConfigEffective()
    {
        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility)
            return groupUtility.HAM_GetFootNavConfig();

        return HAM_AISettingsComponent.HAM_FootNavConfig();
    }

    protected ResourceName HAM_VehicleNavConfigEffective()
    {
        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility)
            return groupUtility.HAM_GetVehicleNavConfig();

        return HAM_AISettingsComponent.HAM_VehicleNavConfig();
    }

    protected float HAM_VehicleCruiseSpeedEffective()
    {
        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility)
            return groupUtility.HAM_GetVehicleCruiseSpeed();

        return HAM_AISettingsComponent.HAM_VehicleCruiseSpeed();
    }

    protected void HAM_PushVehicleNav(IEntity vehicle)
    {
        if (!vehicle)
            return;

        AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(
            vehicle.FindComponent(AIPathfindingComponent));

        if (!pathfinding)
        {
            Print(string.Format("[HAM_AI] Nav véhicule : AUCUN composant de pathfinding trouvé sur %1", vehicle), LogLevel.WARNING);
            return;
        }

        ResourceName conf = HAM_VehicleNavConfigEffective();
        pathfinding.SetAreaCosts(conf);

        AICarMovementComponent carMove = AICarMovementComponent.Cast(
            vehicle.FindComponent(AICarMovementComponent));

        if (!carMove)
            return;

        float speed = 0;
        Vehicle veh = Vehicle.Cast(vehicle);
        if (veh)
            speed = veh.HAM_GetCruiseSpeedOverride();

        if (speed == 0)
            speed = HAM_VehicleCruiseSpeedEffective();

        if (speed <= 0)
            speed = HAM_UNCAPPED_CRUISE_SPEED;

        carMove.SetCruiseSpeed(speed);
    }

    override protected void Event_OnCompartmentEntered(IEntity vehicle, BaseCompartmentManagerComponent manager, int mgrID, int slotID)
    {
        super.Event_OnCompartmentEntered(vehicle, manager, mgrID, slotID);

        if (m_CurrentCompartmentSlot && PilotCompartmentSlot.Cast(m_CurrentCompartmentSlot))
            HAM_PushVehicleNav(m_CurrentCompartmentSlot.GetVehicle());
    }

    protected AIGroup m_HAM_CachedGroup;
    protected SCR_AIGroupUtilityComponent m_HAM_GroupUtility;

    SCR_AIGroupUtilityComponent HAM_GetGroupUtility()
    {
        if (!m_Utility)
            return null;

        AIAgent agent = m_Utility.GetAIAgent();
        if (!agent)
            return null;

        AIGroup group = agent.GetParentGroup();
        if (group != m_HAM_CachedGroup)
        {
            m_HAM_CachedGroup = group;
            m_HAM_GroupUtility = null;

            if (group)
                m_HAM_GroupUtility = SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
        }

        return m_HAM_GroupUtility;
    }

    override void UpdatePerceptionFactor(PerceptionComponent perceptionComp, SCR_AIThreatSystem threatSystem)
    {
        super.UpdatePerceptionFactor(perceptionComp, threatSystem);

        if (threatSystem.GetState() != EAIThreatState.THREATENED)
            return;

        float mul;
        SCR_AIGroupUtilityComponent groupUtility = HAM_GetGroupUtility();
        if (groupUtility)
            mul = groupUtility.HAM_GetThreatenedPerceptionMul();
        else
            mul = HAM_AISettingsComponent.HAM_ThreatenedPerceptionMul();

        if (mul == 1.0)
            return;

        perceptionComp.SetPerceptionFactor(
            PERCEPTION_FACTOR_THREATENED * m_fEquipmentPerceptionFactor * m_fPerceptionFactor * mul);
    }

    protected bool HAM_HasActiveNVG()
    {
        SCR_GadgetManagerComponent gadgetManager = SCR_GadgetManagerComponent.GetGadgetManager(GetOwner());
        if (!gadgetManager)
            return false;

        array<SCR_GadgetComponent> gadgets = gadgetManager.GetGadgetsByType(EGadgetType.NIGHT_VISION);
        if (!gadgets)
            return false;

        foreach (SCR_GadgetComponent gadget : gadgets)
        {
            if (gadget && gadget.IsToggledOn())
                return true;
        }

        return false;
    }

    void HAM_RefreshEquipmentPerception()
    {
        RecalculateEquipmentPerceptionFactor();

        if (m_Perception && m_Utility && m_Utility.m_ThreatSystem)
            UpdatePerceptionFactor(m_Perception, m_Utility.m_ThreatSystem);
    }

    override protected void RecalculateEquipmentPerceptionFactor()
    {
        super.RecalculateEquipmentPerceptionFactor();

        if (!HAM_AINVGToggle.IsNight())
            return;

        if (HAM_HasActiveNVG())
        {
            float value = HAM_AISettingsComponent.HAM_NVGPerceptionMul();
            if (value > m_fEquipmentPerceptionFactor)
                m_fEquipmentPerceptionFactor = value;

            return;
        }

        float nightValue = HAM_AISettingsComponent.HAM_NightPerceptionMul();
        m_fEquipmentPerceptionFactor = m_fEquipmentPerceptionFactor * nightValue;
    }

    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);

        HAM_ApplyAISettings();

        if (!m_CharacterController || !Replication.IsServer())
            return;

        BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
        if (weaponManager)
            weaponManager.m_OnWeaponChangeCompleteInvoker.Insert(HAM_OnWeaponChangeComplete);

        if (m_Utility && m_Utility.m_ThreatSystem)
            m_Utility.m_ThreatSystem.GetOnThreatStateChanged().Insert(HAM_OnThreatStateChanged);

        HAM_ApplyConfiguredWeaponMode();
    }

    override void OnDelete(IEntity owner)
    {
        ArmaReforgerScripted game = GetGame();
        if (game)
        {
            ScriptCallQueue callQueue = game.GetCallqueue();
            if (callQueue)
            {
                callQueue.Remove(HAM_ApplyInitialWeaponMode);
                callQueue.Remove(HAM_ApplyConfiguredWeaponMode);
            }
        }

        if (m_Utility && m_Utility.m_ThreatSystem)
            m_Utility.m_ThreatSystem.GetOnThreatStateChanged().Remove(HAM_OnThreatStateChanged);

        if (m_CharacterController)
        {
            BaseWeaponManagerComponent weaponManager = m_CharacterController.GetWeaponManagerComponent();
            if (weaponManager)
                weaponManager.m_OnWeaponChangeCompleteInvoker.Remove(HAM_OnWeaponChangeComplete);
        }

        super.OnDelete(owner);
    }

    protected void HAM_ScheduleConfiguredWeaponMode()
    {
        ScriptCallQueue callQueue = GetGame().GetCallqueue();
        callQueue.Remove(HAM_ApplyConfiguredWeaponMode);
        callQueue.CallLater(HAM_ApplyConfiguredWeaponMode, 0, false);
    }

    protected void HAM_OnWeaponChangeComplete(BaseWeaponComponent weapon)
    {
        if (!weapon)
            return;

        HAM_ScheduleConfiguredWeaponMode();
    }

    protected void HAM_OnThreatStateChanged(EAIThreatState previousState, EAIThreatState newState)
    {
        if (HAM_GetWeaponModeEffective() != HAM_EAIWeaponMode.ALERT_ONLY)
            return;

        HAM_ScheduleConfiguredWeaponMode();
    }
}
