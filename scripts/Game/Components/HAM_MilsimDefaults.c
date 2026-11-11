// ====================================================================================
// HAM_MilsimDefaults.c — 13RDPA
//
// Config de PRÉRÉGLAGES : les valeurs par défaut des 20 réglages GM, éditables dans
// le Workbench via Configs/Milsim/HAM_MilsimDefaults.conf (pattern du système de
// nametags vanilla : composant → .conf configRoot).
//
// Un mod ou un conflit peut livrer son propre preset en OVERRIDANT ce .conf (même
// GUID {6AA3347013000019}) — ou en pointant m_sTRN_DefaultsConfig du composant vers
// un autre .conf via override de prefab.
//
// Chargé par HAM_MilsimSettingsComponent à l'init, CÔTÉ AUTORITÉ seulement (les
// clients reçoivent les valeurs par réplication). Si une save de campagne existe,
// la persistance repasse derrière et rétablit les derniers réglages du GM.
//
// ⚠️ Les défauts des [Attribute] ci-dessous doivent rester ALIGNÉS sur ceux des
// RplProp du composant (même comportement avec ou sans .conf).
// ====================================================================================
[BaseContainerProps(configRoot: true)]
class HAM_MilsimDefaults
{
    // ── Affichage ──
    [Attribute("1", UIWidgets.CheckBox, "Masquer les noms au-dessus des joueurs et des IA", category: "Affichage")]
    bool m_bHideNameTags;

    [Attribute("1", UIWidgets.CheckBox, "Masquer l'icône au-dessus du personnel inconscient", category: "Affichage")]
    bool m_bHideUnconsciousIcon;

    [Attribute("0", UIWidgets.EditBox, "Distance max d'affichage des nametags (m). 0 = distances d'origine de la config nametag", "0 500 5", category: "Affichage")]
    float m_fNameTagMaxDistance;

    // ── VON ──
    [Attribute("1", UIWidgets.CheckBox, "Masquer le nom et le rôle du locuteur dans l'overlay VON", category: "VON")]
    bool m_bHideVONNames;

    [Attribute("1", UIWidgets.CheckBox, "Interdire de transmettre (radio et voix) quand on est inconscient", category: "VON")]
    bool m_bBlockUnconsciousVON;

    [Attribute("1", UIWidgets.CheckBox, "Aucun élément d'UI quand un ENNEMI parle à proximité", category: "VON")]
    bool m_bHideEnemyProxVONUI;

    [Attribute("1", UIWidgets.CheckBox, "Aucun élément d'UI pour toute voix de proximité entrante (alliés compris)", category: "VON")]
    bool m_bHideAllProxVONUI;

    // ── Nettoyage ──
    [Attribute("1", UIWidgets.CheckBox, "Les véhicules détruits ne sont jamais nettoyés", category: "Nettoyage")]
    bool m_bKeepWrecks;

    [Attribute("1", UIWidgets.CheckBox, "Les corps (joueurs + IA) ne sont jamais nettoyés", category: "Nettoyage")]
    bool m_bKeepCorpses;

    [Attribute("0", UIWidgets.CheckBox, "Retire les véhicules inoccupés loin des joueurs et les fait revenir à leur approche", category: "Nettoyage")]
    bool m_bVehicleVirtualization;

    // ── Monde ──
    [Attribute("0", UIWidgets.CheckBox, "Les bâtiments ne s'effondrent jamais", category: "Monde")]
    bool m_bIndestructibleBuildings;

    [Attribute("0", UIWidgets.CheckBox, "Bloquer le menu radial carte, les marqueurs rapides et les marqueurs SL", category: "Monde")]
    bool m_bDisableVanillaMarkers;

    // ── Conflit ──
    [Attribute("0", UIWidgets.CheckBox, "Les demandes catalogue (véhicules...) n'exigent plus de rang", category: "Conflit")]
    bool m_bNoMinimumRank;

    [Attribute("0", UIWidgets.CheckBox, "Stopper la régénération auto de supply du QG", category: "Conflit")]
    bool m_bNoSupplyRegenHQ;

    [Attribute("0", UIWidgets.CheckBox, "Stopper la régénération auto de supply des points de réappro", category: "Conflit")]
    bool m_bNoSupplyRegenHubs;

    [Attribute("0", UIWidgets.CheckBox, "Stopper la régénération auto de supply des sources", category: "Conflit")]
    bool m_bNoSupplyRegenSources;

    [Attribute("0", UIWidgets.CheckBox, "Stopper la régénération auto de supply des bases ordinaires", category: "Conflit")]
    bool m_bNoSupplyRegenBases;

    // Points de supply (dépôts) : 5e régime, à part des 4 ci-dessus. Leur régénération est
    // portée par le CONTENEUR de la caisse (SCR_ResourceContainer.m_bEnableResourceGain /
    // m_fResourceGain / m_fResourceGainTickrate) et non par SupplyIncomeTimer — les 4 cases
    // ci-dessus ne les atteignent donc pas.
    [Attribute("0", UIWidgets.CheckBox, "Stopper la régénération auto des points de supply (dépôts)", category: "Conflit")]
    bool m_bNoSupplyRegenDepots;

    [Attribute("0", UIWidgets.Slider, "Points de supply : quantité régénérée à chaque cycle (0 = valeur d'origine du dépôt)", "0 2000 10", category: "Conflit")]
    float m_fDepotRegenAmount;

    [Attribute("0", UIWidgets.Slider, "Points de supply : délai entre deux régénérations en secondes (0 = valeur d'origine du dépôt)", "0 600 1", category: "Conflit")]
    float m_fDepotRegenTickrate;

    // ── Caméra ──
    [Attribute("0", UIWidgets.CheckBox, "Interdire la 3e personne à pied", category: "Camera")]
    bool m_bForceFirstPersonFoot;

    [Attribute("0", UIWidgets.CheckBox, "Interdire la 3e personne en véhicule et tourelle", category: "Camera")]
    bool m_bForceFirstPersonVehicle;

    // ── GM ──
    [Attribute("1", UIWidgets.CheckBox, "Bloquer l'accès des joueurs à Arma Vision (mode photo)", category: "GM")]
    bool m_bBlockPhotoMode;

    [Attribute("1", UIWidgets.CheckBox, "Cacher la présence du GM (ping + liste des joueurs)", category: "GM")]
    bool m_bStealthGM;

    [Attribute("100", UIWidgets.Slider, "Probabilité (%) que l'arme tombe au sol quand un joueur devient inconscient. 0 = elle est toujours rangée sur le corps", "0 100 5", category: "Personnage")]
    float m_fWeaponDropChance;

    [Attribute("0", UIWidgets.CheckBox, "Afficher un compas en bas de l'écran des joueurs", category: "Affichage")]
    bool m_bShowCompass;
}

// ====================================================================================
// REGISTRE DE PRESETS — liste (nom + .conf HAM_MilsimDefaults) affichée par le menu
// déroulant « Preset de réglages » en tête de la catégorie GM. Overridable (même
// GUID {6AA334701300001A}) pour ajouter/retirer des presets côté serveur/conflit.
// ====================================================================================
[BaseContainerProps()]
class HAM_MilsimPresetEntry
{
    [Attribute("Preset", UIWidgets.EditBox, "Nom affiché dans le menu déroulant")]
    string m_sName;

    [Attribute("", UIWidgets.ResourceNamePicker, "Config HAM_MilsimDefaults de ce preset", params: "conf")]
    ResourceName m_sConfig;
}

[BaseContainerProps(configRoot: true)]
class HAM_MilsimPresets
{
    // ⚠️ UIWidgets.Object OBLIGATOIRE sur un tableau d'objets — sans lui le tableau
    // n'est pas désérialisé depuis le .conf (registre « chargé » mais liste vide,
    // dropdown GM vierge — bug constaté 2026-08-06).
    [Attribute("", UIWidgets.Object, "Liste des presets (nom + .conf HAM_MilsimDefaults)")]
    ref array<ref HAM_MilsimPresetEntry> m_aPresets;

    //! Charge un registre de presets depuis sa ressource (null si introuvable)
    static HAM_MilsimPresets HAM_Load(ResourceName path)
    {
        if (path.IsEmpty())
            return null;

        Resource container = BaseContainerTools.LoadContainer(path);
        if (!container)
        {
            Print("[HAM_Milsim] Registre de presets introuvable : " + path, LogLevel.WARNING);
            return null;
        }

        HAM_MilsimPresets presets = HAM_MilsimPresets.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
        if (presets && (!presets.m_aPresets || presets.m_aPresets.IsEmpty()))
            Print("[HAM_Milsim] Registre de presets chargé mais VIDE : " + path, LogLevel.WARNING);

        return presets;
    }
}
