modded class SCR_FuelConsumptionComponent
{
    protected bool m_bTRN_FuelConsumption = true;

    protected float m_fTRN_FuelScaleOverride = -1;

    bool HAM_GetFuelConsumption()       { return m_bTRN_FuelConsumption; }
    void HAM_SetFuelConsumption(bool v) { m_bTRN_FuelConsumption = v; }

    float HAM_GetFuelScaleOverride() { return m_fTRN_FuelScaleOverride; }

    void HAM_SetFuelScaleOverride(float v)
    {
        if (v < -1)
            v = -1;

        m_fTRN_FuelScaleOverride = v;
    }

    float HAM_ResolveFuelScale()
    {
        if (!m_bTRN_FuelConsumption)
            return 0;

        if (m_fTRN_FuelScaleOverride >= 0)
            return m_fTRN_FuelScaleOverride;

        return HAM_InheritedFuelScale();
    }

    float HAM_InheritedFuelScale()
    {
        float groupScale = HAM_DriverGroupFuelScale();
        if (groupScale >= 0)
            return groupScale;

        return HAM_AISettingsComponent.HAM_FuelScale();
    }

    protected float HAM_DriverGroupFuelScale()
    {
        IEntity owner = GetOwner();
        if (!owner)
            return -1;

        SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(
            owner.FindComponent(SCR_BaseCompartmentManagerComponent));

        if (!compartments)
            return -1;

        array<BaseCompartmentSlot> slots = {};
        compartments.GetCompartments(slots);

        foreach (BaseCompartmentSlot slot : slots)
        {
            if (!PilotCompartmentSlot.Cast(slot))
                continue;

            IEntity occupant = slot.GetOccupant();
            if (!occupant)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(
                occupant.FindComponent(SCR_AICombatComponent));

            if (!combat)
                continue;

            SCR_AIGroupUtilityComponent utility = combat.HAM_GetGroupUtility();
            if (!utility)
                continue;

            return utility.HAM_GetGroupFuelScaleEffective();
        }

        return -1;
    }

    override void Update()
    {
        float scale = HAM_ResolveFuelScale();
        float global = GetGlobalFuelConsumptionScale();

        if (scale == global)
        {
            super.Update();
            return;
        }

        s_fGlobalFuelConsumptionScale = scale;
        super.Update();
        s_fGlobalFuelConsumptionScale = global;
    }
}
