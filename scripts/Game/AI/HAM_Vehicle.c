modded class Vehicle
{
    protected float m_fTRN_CruiseSpeedOverride;

    float HAM_GetCruiseSpeedOverride() { return m_fTRN_CruiseSpeedOverride; }

    void HAM_SetCruiseSpeedOverride(float v)
    {
        m_fTRN_CruiseSpeedOverride = v;

        SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(
            FindComponent(SCR_BaseCompartmentManagerComponent));

        if (compartments)
        {
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

                if (combat)
                {
                    combat.HAM_ApplyNavPreset();
                    return;
                }
            }
        }

        AICarMovementComponent carMove = AICarMovementComponent.Cast(
            FindComponent(AICarMovementComponent));

        if (!carMove)
            return;

        if (v <= 0)
            v = SCR_AICombatComponent.HAM_UNCAPPED_CRUISE_SPEED;

        carMove.SetCruiseSpeed(v);
    }
}
