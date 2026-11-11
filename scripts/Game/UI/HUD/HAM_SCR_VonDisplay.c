// ====================================================================================
// HAM_SCR_VonDisplay.c — 13RDPA
//
// Toggle GM « Masquer les noms VON » : après le pipeline vanilla, vide et masque le
// nom, le rôle et les icônes du locuteur sur les transmissions ENTRANTES (radio ET
// voix de proximité — les deux passent par UpdateTransmission ; les sortantes
// n'affichent déjà pas de nom).
//
// Toggles GM « Masquer l'UI VON de proximité (ennemie / entière) » : supprime
// l'ENTRÉE UI complète des voix directes entrantes (on entend toujours, mais aucun
// élément d'interface ne trahit qui/qu'on parle à proximité). Retour false AVANT le
// pipeline vanilla, même contrat que l'early-out m_bIsVONDirectDisabled.
//
// FOF Voice modde aussi SCR_VonDisplay mais seulement UpdateWidgets → pas de
// télescopage de chaîne sur UpdateTransmission.
// ====================================================================================
modded class SCR_VonDisplay
{
    //! Le locuteur direct est-il d'une faction ennemie du joueur local ?
    protected bool HAM_IsEnemyProxSender(TransmissionData data)
    {
        PlayerController playerController = GetGame().GetPlayerController();
        if (!playerController)
            return false;

        int controlledID = playerController.GetPlayerId();
        SCR_FactionManager factionMgr = SCR_FactionManager.Cast(GetGame().GetFactionManager());
        if (!controlledID || !factionMgr)
            return false;

        Faction playerFaction = factionMgr.SGetPlayerFaction(controlledID);
        Faction senderFaction = factionMgr.SGetPlayerFaction(data.m_iPlayerID);
        if (!playerFaction || !senderFaction)
            return false;

        return playerFaction.IsFactionEnemy(senderFaction);
    }

    override protected bool UpdateTransmission(TransmissionData data, BaseTransceiver radioTransceiver, int frequency, bool IsReceiving)
    {
        // Suppression d'entrées UI de proximité (avant le pipeline vanilla)
        if (IsReceiving && !radioTransceiver && data)
        {
            bool suppress = HAM_MilsimSettingsComponent.HAM_HideAllProxVONUI();
            if (!suppress && HAM_MilsimSettingsComponent.HAM_HideEnemyProxVONUI() && HAM_IsEnemyProxSender(data))
                suppress = true;

            if (suppress)
            {
                data.m_RadioTransceiver = radioTransceiver;
                data.m_bForceUpdate = false;
                return false;
            }
        }

        bool shown = super.UpdateTransmission(data, radioTransceiver, frequency, IsReceiving);

        if (!shown || !IsReceiving)
            return shown;

        if (!HAM_MilsimSettingsComponent.HAM_HideVONNames())
            return shown;

        if (!data || !data.m_Widgets)
            return shown;

        if (data.m_Widgets.m_wName)
        {
            data.m_Widgets.m_wName.SetText(string.Empty);
            data.m_Widgets.m_wName.SetVisible(false);
        }

        if (data.m_Widgets.m_wRole)
        {
            data.m_Widgets.m_wRole.SetText(string.Empty);
            data.m_Widgets.m_wRole.SetVisible(false);
        }

        if (data.m_Widgets.m_wSquadLeaderIcon)
            data.m_Widgets.m_wSquadLeaderIcon.SetVisible(false);

        if (data.m_Widgets.m_wPlatformImage)
            data.m_Widgets.m_wPlatformImage.SetVisible(false);

        if (data.m_Widgets.m_wPlatformImageGlow)
            data.m_Widgets.m_wPlatformImageGlow.SetVisible(false);

        return shown;
    }
}
