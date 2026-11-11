// ====================================================================================
// HAM_MilsimVanillaMarkers.c — 13RDPA
//
// Toggle GM « Désactiver les marqueurs vanilla » : 3 points de blocage (méthode du
// mod DisableVanillaMarker, rendue togglable) :
//   - menu radial de la carte (placement de marqueurs)
//   - marqueur rapide (quick marker)
//   - marqueurs automatiques de chef de groupe (SL)
//
// Limite : les marqueurs SL déjà enregistrés restent affichés si le GM active le
// toggle en cours de partie (blocage à l'enregistrement seulement).
//
// ⚠️ SCR_MapMarkerEntrySquadLeader est une classe [BaseContainerProps] désérialisée
// depuis la config marqueurs → les attributs sont RÉPÉTÉS au-dessus du modded pour
// garder la classe enregistrée (pattern FOF Loadout 1.7, prouvé en prod ; cf. mémoire
// modded-basecontainerprops-world-data-break). SCR_MapRadialUI/SCR_MapMarkersUI :
// [BaseContainerProps()] porté par la base SCR_MapUIBaseComponent, déclarations
// vanilla nues → modded nu, comme le mod d'origine. À VALIDER EN PLAY (ouverture
// carte : aucun « Unknown keyword/data » dans les logs).
// ====================================================================================
modded class SCR_MapRadialUI
{
    override protected void OnInputMenuOpen(float value, EActionTrigger reason)
    {
        if (HAM_MilsimSettingsComponent.HAM_DisableVanillaMarkers())
            return;

        super.OnInputMenuOpen(value, reason);
    }
}

modded class SCR_MapMarkersUI
{
    override protected void OnInputQuickMarkerMenu(float value, EActionTrigger reason)
    {
        if (HAM_MilsimSettingsComponent.HAM_DisableVanillaMarkers())
            return;

        super.OnInputQuickMarkerMenu(value, reason);
    }
}

[BaseContainerProps(), SCR_MapMarkerTitle()]
modded class SCR_MapMarkerEntrySquadLeader : SCR_MapMarkerEntryDynamic
{
    override void RegisterMarker(SCR_MapMarkerSquadLeader marker, SCR_AIGroup group)
    {
        if (HAM_MilsimSettingsComponent.HAM_DisableVanillaMarkers())
            return;

        if (HAM_HideOtherAlliedGroup(group))
            return;

        super.RegisterMarker(marker, group);
    }

    protected bool HAM_HideOtherAlliedGroup(SCR_AIGroup group)
    {
        if (!group)
            return false;

        int playerId = SCR_PlayerController.GetLocalPlayerId();
        SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
        if (groups && groups.GetPlayerGroup(playerId) == group)
            return false;

        Faction groupFaction = group.GetFaction();
        Faction playerFaction = SCR_FactionManager.SGetLocalPlayerFaction();
        if (!groupFaction || !playerFaction)
            return false;

        return groupFaction.GetFactionKey() == playerFaction.GetFactionKey();
    }
}

