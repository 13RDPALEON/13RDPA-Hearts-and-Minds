// ====================================================================================
// HAM_SCR_VONController.c — 13RDPA
//
// Micro-amélioration (reprise du mod VONDisabledIncapacitated) : vanilla coupe le
// toggle de voix de proximité (m_bIsToggledDirect) quand le perso tombe inconscient
// ou meurt, et ne le RESTAURE jamais — après réanimation le joueur doit réactiver sa
// voix à la main. Ici : on mémorise l'état du toggle à l'entrée en inconscience et on
// le restaure au retour en vie. Toujours actif (confort, pas de checkbox GM).
//
// Contrairement au mod d'origine (réimplémentation complète SANS super, hostile aux
// chaînes modded), on laisse vanilla faire puis on corrige après. Chaîne existante
// sur SCR_VONController (FOF Voice, FOFGameMode patch, HAM_Medical) : aucun ne touche
// OnLifeStateChanged → pas de télescopage.
// ====================================================================================
modded class SCR_VONController
{
    protected bool m_bTRN_PrevDirectToggle;

    override protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
    {
        bool wasToggledDirect = m_bIsToggledDirect;
        bool wasUnconscious = m_bIsUnconscious;

        super.OnLifeStateChanged(previousLifeState, newLifeState);

        // Entrée en inconscience/mort : vanilla vient de couper la proximité → retenir
        if (m_bIsUnconscious && !wasUnconscious)
            m_bTRN_PrevDirectToggle = wasToggledDirect;

        // Retour en vie : restaurer le toggle tel qu'il était avant le KO
        if (newLifeState == ECharacterLifeState.ALIVE && m_bTRN_PrevDirectToggle)
        {
            m_bTRN_PrevDirectToggle = false;
            SetVONProximityToggle(true);
            UpdateSystemState();
        }
    }
}
