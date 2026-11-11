modded class SCR_AIObserveThreatSystemBehavior
{
    override float CustomEvaluate()
    {
        if (HAM_ShouldIgnoreSector())
            return 0;

        return super.CustomEvaluate();
    }

    protected bool HAM_ShouldIgnoreSector()
    {
        if (m_iCurrentSector == -1)
            return false;

        if (!m_Utility || !m_Utility.m_OwnerEntity || !m_Utility.m_SectorThreatFilter)
            return false;

        SCR_AICombatComponent combat = m_Utility.m_CombatComponent;
        if (!combat)
            return false;

        int mode = combat.HAM_GetLookModeEffective();

        if (HAM_AILookRules.SectorConcernsMe(m_eCurrentSectorFlags, mode))
            return false;

        if (HAM_AILookRules.IsFocusedOrMore(mode))
            return true;

        return HAM_AILookRules.SectorTooFar(m_Utility.m_OwnerEntity.GetOrigin(),
            m_Utility.m_SectorThreatFilter.GetSectorPos(m_iCurrentSector));
    }
}
