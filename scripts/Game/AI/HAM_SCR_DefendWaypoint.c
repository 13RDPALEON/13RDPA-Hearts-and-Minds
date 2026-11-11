modded class SCR_DefendWaypoint
{
    protected HAM_EAIZoneFireMode m_eTRN_ZoneFireMode;
    protected HAM_EAIZoneStyleChoice m_eTRN_ZoneStyleChoice;

    int HAM_GetZoneFireMode() { return m_eTRN_ZoneFireMode; }
    void HAM_SetZoneFireMode(int mode) { m_eTRN_ZoneFireMode = mode; }

    int HAM_GetZoneStyleChoice() { return m_eTRN_ZoneStyleChoice; }

    void HAM_SetZoneStyleChoice(int choice)
    {
        m_eTRN_ZoneStyleChoice = choice;
        SCR_AIGroupUtilityComponent.HAM_ReapplyStyleForZone(this);
    }
}
