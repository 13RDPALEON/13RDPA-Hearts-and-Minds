// ====================================================================================
// HAM_MilsimStealthGM.c — 13RDPA
//
// Toggle GM « GM furtif » (méthode du mod KittenEye_StealthGM, rendue togglable) :
//   - SendPing : les joueurs qui pinguent reçoivent la notification vanilla
//     « pas de GM à pinguer » (même appel que le fallback vanilla L135) → la présence
//     d'un GM n'est jamais trahie. Limite : bloque aussi les pings GM→GM.
//   - Liste des joueurs : l'indicateur « Game Master » est masqué sur toutes les
//     lignes.
// ====================================================================================
modded class SCR_PingEditorComponent
{
    override void SendPing(bool unlimitedOnly = false, vector position = vector.Zero, SCR_EditableEntityComponent target = null)
    {
        if (HAM_MilsimSettingsComponent.HAM_StealthGM())
        {
            SCR_NotificationsComponent.SendLocal(ENotification.EDITOR_PING_NO_GM_TO_PING);
            return;
        }

        super.SendPing(unlimitedOnly, position, target);
    }
}

modded class SCR_PlayerListMenu
{
    override protected void UpdateGameMasterIndicator(notnull SCR_PlayerListEntry entry, bool editorIslimited)
    {
        if (HAM_MilsimSettingsComponent.HAM_StealthGM())
        {
            Widget gameMasterIndicator = entry.m_wRow.FindAnyWidget(m_sGameMasterIndicatorName);
            if (gameMasterIndicator)
                gameMasterIndicator.SetVisible(false);
            return;
        }

        super.UpdateGameMasterIndicator(entry, editorIslimited);
    }
}
