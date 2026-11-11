// ====================================================================================
// HAM_SCR_NameTagDisplay.c — 13RDPA
//
// Toggle GM « Masquer les nametags » : endort l'affichage vanilla (purge des tags +
// m_bSleepDisplay) tant que le réglage est actif ; le réveille au retour.
// Gère aussi le rafraîchissement des tags quand le toggle « icône inconscient »
// change (les tags re-créés repassent par notre HAM_SCR_NameTagData).
//
// Slider GM « Distance des nametags » : met à l'échelle les zones du config statique
// s_NametagCfg pour que la zone la plus lointaine se termine à la distance choisie
// (mécanique vanilla AdjustZoneRanges/RestoreZoneRanges, utilisée pour le zoom).
// 0 = distances d'origine. Appliqué au CHANGEMENT seulement — stable tant que la
// config n'active pas m_bAdjustZoneRangeOnZoom (nos configs : false ; si un jour une
// config l'active, le zoom optique écrasera temporairement notre échelle en visée).
// ====================================================================================
modded class SCR_NameTagDisplay
{
    protected bool m_bTRN_TagsHidden;
    protected bool m_bTRN_LastHideUnconscious;
    protected float m_fTRN_LastNameTagDist;

    //! Met à l'échelle toutes les zones (fins ET débuts, contrairement au zoom vanilla
    //! qui ne scale que les fins — indispensable quand on RÉDUIT la distance, sinon une
    //! zone peut finir avant son propre début et disparaître).
    protected void HAM_ApplyNameTagDistance(float desired)
    {
        if (!s_NametagCfg || s_NametagCfg.m_fFarthestZoneRangeOriginal <= 0)
            return;

        float modifier = desired / s_NametagCfg.m_fFarthestZoneRangeOriginal;

        for (int i = 0; i < s_NametagCfg.m_iZoneCount; i++)
        {
            SCR_NameTagZone zone = s_NametagCfg.m_aZones[i];
            float scaledEnd = zone.m_iZoneEnd * modifier;
            float scaledStart = zone.GetZoneStart() * modifier;
            zone.m_iZoneEndSq = scaledEnd * scaledEnd;
            zone.m_iZoneStartSq = scaledStart * scaledStart;
        }

        s_NametagCfg.m_fFarthestZoneRange = desired;
        s_NametagCfg.m_fFarthestZoneRangePow2 = desired * desired;
    }

    //! Retour aux distances d'origine de la config (fins via vanilla + débuts)
    protected void HAM_RestoreNameTagDistance()
    {
        if (!s_NametagCfg)
            return;

        s_NametagCfg.RestoreZoneRanges();

        for (int i = 0; i < s_NametagCfg.m_iZoneCount; i++)
        {
            SCR_NameTagZone zone = s_NametagCfg.m_aZones[i];
            int start = zone.GetZoneStart();
            zone.m_iZoneStartSq = start * start;
        }
    }

    override void DisplayUpdate(IEntity owner, float timeSlice)
    {
        bool hideTags = HAM_MilsimSettingsComponent.HAM_HideNameTags();
        bool hideUnconscious = HAM_MilsimSettingsComponent.HAM_HideUnconsciousIcon();

        // Slider distance : appliqué uniquement quand la valeur répliquée change
        float desiredDist = HAM_MilsimSettingsComponent.HAM_NameTagMaxDistance();
        if (desiredDist != m_fTRN_LastNameTagDist)
        {
            m_fTRN_LastNameTagDist = desiredDist;
            if (desiredDist <= 0)
                HAM_RestoreNameTagDistance();
            else
                HAM_ApplyNameTagDistance(desiredDist);
        }

        if (hideTags)
        {
            // Re-purge à chaque réveil vanilla (respawn, changement d'entité...)
            if (!m_bSleepDisplay)
            {
                CleanupAllTags();
                m_CurrentPlayerTag = null;
                m_bSleepDisplay = true;
            }
            m_bTRN_TagsHidden = true;
            m_bTRN_LastHideUnconscious = hideUnconscious;
            return;
        }

        // Retour des nametags, ou changement du toggle icône inconscient :
        // reconstruit les tags (leur init repasse par ActivateEntityState)
        if (m_bTRN_TagsHidden || hideUnconscious != m_bTRN_LastHideUnconscious)
        {
            m_bTRN_TagsHidden = false;
            m_bTRN_LastHideUnconscious = hideUnconscious;

            IEntity controlled = null;
            if (m_PlayerController)
                controlled = m_PlayerController.GetControlledEntity();
            if (controlled)
                RefreshTags(controlled);
        }

        super.DisplayUpdate(owner, timeSlice);
    }
}
