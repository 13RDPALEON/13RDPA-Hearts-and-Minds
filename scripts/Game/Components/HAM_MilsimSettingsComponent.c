// ====================================================================================
// HAM_MilsimSettingsComponent.c
//
// Réglages milsim togglables par le GM — À ATTACHER AU GAMEMODE (override empilé de
// GameMode_Base.et vanilla). 6 checkboxes GM (catégorie globale « 13RDPA »),
// répliquées serveur → clients.
//
//   - Masquer les nametags (noms au-dessus des joueurs/IA)
//   - Masquer l'icône inconscient (état UNCONSCIOUS des nametags)
//   - Masquer les noms VON (radio + voix de proximité)
//   - Conserver les épaves (véhicules détruits jamais nettoyés)
//   - Conserver les corps (joueurs + IA morts jamais nettoyés)
//   - Interdire de parler inconscient (VON coupé, radio ET voix)
//   - Masquer l'UI VON de proximité ennemie (on entend, aucun élément UI)
//   - Masquer l'UI VON de proximité entière (alliés compris)
//   - Bâtiments indestructibles (OFF par défaut)
//   - Désactiver Arma Vision / mode photo pour les joueurs (GM non affecté)
//   - Désactiver les marqueurs vanilla (radial + marqueurs rapides + SL) (OFF par défaut)
//   - Supprimer le rang minimum des demandes catalogue (OFF par défaut)
//   - GM furtif (ping « pas de GM » + indicateur GM masqué dans la liste joueurs)
//
// L'affichage est purement LOCAL ; les réglages sont répliqués via RplProp
// (composant NEUF — jamais de RplProp sur une classe moddée, crash connu 1.7).
// Pattern : HAM_MapSettingsComponent (SNK Conflict Mod).
//
// VON inconscient : le flag vanilla m_bPermitUnconsciousVON n'est PAS répliqué à
// chaud (SCR_VONController le lit dans un cache statique local). On l'applique donc
// LOCALEMENT sur chaque machine (serveur à l'init/changement, clients à l'OnRep).
// ====================================================================================
[ComponentEditorProps(category: "13RDPA/Milsim", description: "Reglages milsim togglables par le GM — a attacher au GameMode")]
class HAM_MilsimSettingsComponentClass : ScriptComponentClass {}

class HAM_MilsimSettingsComponent : ScriptComponent
{
    // Config de préréglages : fournit les DÉFAUTS des 20 réglages au démarrage du
    // scénario (chargée côté autorité à l'init ; un mod/conflit peut overrider le
    // .conf — même GUID — ou pointer ici vers un autre .conf via override de prefab).
    // Si une save de campagne existe, la persistance repasse derrière ces défauts.
    [Attribute("{95764C73AE5FD96C}Configs/Milsim/HAM_MilsimDefaults.conf", UIWidgets.ResourceNamePicker, "Config des reglages par defaut (preset)", params: "conf")]
    protected ResourceName m_sTRN_DefaultsConfig;

    // Registre des presets proposés par le menu déroulant GM « Preset de réglages »
    [Attribute("{8FEFDA3BBA4FAB8F}Configs/Milsim/HAM_MilsimPresets.conf", UIWidgets.ResourceNamePicker, "Registre des presets du menu deroulant GM", params: "conf")]
    protected ResourceName m_sTRN_PresetsConfig;

    // Dernier preset choisi dans le menu déroulant (affichage ; 0 = premier du registre)
    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected int m_iTRN_PresetIndex = 0;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_HideNameTags = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_HideUnconsciousIcon = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_HideVONNames = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_KeepWrecks = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_KeepCorpses = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_VehicleVirtualization = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_BlockUnconsciousVON = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_HideEnemyProxVONUI = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_HideAllProxVONUI = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_IndestructibleBuildings = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_BlockPhotoMode = true;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_DisableVanillaMarkers = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoMinimumRank = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_StealthGM = true;

    // Distance max d'affichage des nametags en mètres. 0 = distances d'origine de la
    // config (aucune modification). Appliqué côté client par le modded
    // SCR_NameTagDisplay (mise à l'échelle proportionnelle des zones, mécanique
    // vanilla AdjustZoneRanges/RestoreZoneRanges).
    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected float m_fTRN_NameTagMaxDistance = 0;

    // Régénération automatique de supply (Conflit) — 4 catégories de bases, défaut
    // false = régénération vanilla intacte. Lus SERVEUR par le modded
    // SCR_CampaignMilitaryBaseComponent.SupplyIncomeTimer (répliqués quand même pour
    // que le GM voie l'état réel dans sa fenêtre d'attributs, où qu'il soit).
    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoSupplyRegenHQ = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoSupplyRegenHubs = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoSupplyRegenSources = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoSupplyRegenBases = false;

    // Points de supply (dépôts) — 5e régime, distinct des 4 ci-dessus : leur régénération
    // est portée par le CONTENEUR de la caisse (SCR_ResourceContainer.m_bEnableResourceGain
    // / m_fResourceGain / m_fResourceGainTickrate) et non par SupplyIncomeTimer, que ces 4
    // cases bloquent. Appliqué SERVEUR par le modded SCR_ResourceSystem, sur les conteneurs
    // à gain — sans savoir qui les a armés (aucune dépendance à SNK Conflict).
    // Les deux valeurs à 0 = on ne touche pas à la cadence d'origine du dépôt.
    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_NoSupplyRegenDepots = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected float m_fTRN_DepotRegenAmount = 0;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected float m_fTRN_DepotRegenTickrate = 0;

    // Vue forcée à la 1re personne — séparée à pied / en véhicule (tourelle incluse).
    // Défaut false = 3e personne autorisée (vanilla). Lu côté client par le modded
    // SCR_CharacterCameraHandlerComponent (blocage du switch + rappel en continu dans
    // CameraSelector pour les transitions et les joueurs déjà en 3e personne).
    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_ForceFirstPersonFoot = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_ForceFirstPersonVehicle = false;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected float m_fTRN_WeaponDropChance = 100;

    [RplProp(onRplName: "OnRep_HAM_MilsimSettings")]
    protected bool m_bTRN_ShowCompass = false;

    protected static HAM_MilsimSettingsComponent s_pInstance;
    //! Gel automatique (init, JSON) : ne compte pas comme une édition du maître du jeu.
    protected bool m_bSilentGmNote;

    // ==================================================================================
    static HAM_MilsimSettingsComponent GetInstance()
    {
        if (!s_pInstance)
        {
            BaseGameMode gamemode = GetGame().GetGameMode();
            if (gamemode)
                s_pInstance = HAM_MilsimSettingsComponent.Cast(gamemode.FindComponent(HAM_MilsimSettingsComponent));
        }
        return s_pInstance;
    }

    // ==================================================================================
    // GETTERS STATIQUES null-safe — utilisés par les effecteurs modded chaque frame.
    // Composant absent (scénario sans notre override) → false → comportement vanilla.
    // ==================================================================================
    static bool HAM_HideNameTags()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_HideNameTags;
    }

    static bool HAM_HideUnconsciousIcon()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_HideUnconsciousIcon;
    }

    static bool HAM_HideVONNames()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_HideVONNames;
    }

    static bool HAM_KeepWrecks()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_KeepWrecks;
    }

    static bool HAM_KeepCorpses()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_KeepCorpses;
    }

    static bool HAM_VehicleVirtualization()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_VehicleVirtualization;
    }

    static bool HAM_HideEnemyProxVONUI()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_HideEnemyProxVONUI;
    }

    static bool HAM_HideAllProxVONUI()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_HideAllProxVONUI;
    }

    static bool HAM_IndestructibleBuildings()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_IndestructibleBuildings;
    }

    static bool HAM_BlockPhotoMode()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_BlockPhotoMode;
    }

    static bool HAM_DisableVanillaMarkers()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_DisableVanillaMarkers;
    }

    static bool HAM_NoMinimumRank()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoMinimumRank;
    }

    static bool HAM_StealthGM()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_StealthGM;
    }

    static float HAM_NameTagMaxDistance()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_fTRN_NameTagMaxDistance;
    }

    static bool HAM_NoSupplyRegenHQ()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoSupplyRegenHQ;
    }

    static bool HAM_NoSupplyRegenHubs()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoSupplyRegenHubs;
    }

    static bool HAM_NoSupplyRegenSources()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoSupplyRegenSources;
    }

    static bool HAM_NoSupplyRegenBases()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoSupplyRegenBases;
    }

    static bool HAM_NoSupplyRegenDepots()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_NoSupplyRegenDepots;
    }

    static float HAM_DepotRegenAmount()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_fTRN_DepotRegenAmount;
    }

    static float HAM_DepotRegenTickrate()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_fTRN_DepotRegenTickrate;
    }

    static bool HAM_ForceFirstPersonFoot()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_ForceFirstPersonFoot;
    }

    static bool HAM_ForceFirstPersonVehicle()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_ForceFirstPersonVehicle;
    }

    static float HAM_WeaponDropChance()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return 100;
        return inst.m_fTRN_WeaponDropChance;
    }

    static bool HAM_ShowCompass()
    {
        HAM_MilsimSettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_ShowCompass;
    }

    // ==================================================================================
    // GETTERS INSTANCE — utilisés par ReadVariable des attributs GM
    // ==================================================================================
    bool HAM_GetHideNameTags()             { return m_bTRN_HideNameTags; }
    bool HAM_GetHideUnconsciousIcon()      { return m_bTRN_HideUnconsciousIcon; }
    bool HAM_GetHideVONNames()             { return m_bTRN_HideVONNames; }
    bool HAM_GetKeepWrecks()               { return m_bTRN_KeepWrecks; }
    bool HAM_GetKeepCorpses()              { return m_bTRN_KeepCorpses; }
    bool HAM_GetVehicleVirtualization()    { return m_bTRN_VehicleVirtualization; }
    bool HAM_GetBlockUnconsciousVON()      { return m_bTRN_BlockUnconsciousVON; }
    bool HAM_GetHideEnemyProxVONUI()       { return m_bTRN_HideEnemyProxVONUI; }
    bool HAM_GetHideAllProxVONUI()         { return m_bTRN_HideAllProxVONUI; }
    bool HAM_GetIndestructibleBuildings()  { return m_bTRN_IndestructibleBuildings; }
    bool HAM_GetBlockPhotoMode()           { return m_bTRN_BlockPhotoMode; }
    bool HAM_GetDisableVanillaMarkers()    { return m_bTRN_DisableVanillaMarkers; }
    bool HAM_GetNoMinimumRank()            { return m_bTRN_NoMinimumRank; }
    bool HAM_GetStealthGM()                { return m_bTRN_StealthGM; }
    float HAM_GetNameTagMaxDistance()      { return m_fTRN_NameTagMaxDistance; }
    bool HAM_GetNoSupplyRegenHQ()          { return m_bTRN_NoSupplyRegenHQ; }
    bool HAM_GetNoSupplyRegenHubs()        { return m_bTRN_NoSupplyRegenHubs; }
    bool HAM_GetNoSupplyRegenSources()     { return m_bTRN_NoSupplyRegenSources; }
    bool HAM_GetNoSupplyRegenBases()       { return m_bTRN_NoSupplyRegenBases; }
    bool HAM_GetNoSupplyRegenDepots()      { return m_bTRN_NoSupplyRegenDepots; }
    float HAM_GetDepotRegenAmount()        { return m_fTRN_DepotRegenAmount; }
    float HAM_GetDepotRegenTickrate()      { return m_fTRN_DepotRegenTickrate; }
    bool HAM_GetForceFirstPersonFoot()     { return m_bTRN_ForceFirstPersonFoot; }
    bool HAM_GetForceFirstPersonVehicle()  { return m_bTRN_ForceFirstPersonVehicle; }
    float HAM_GetWeaponDropChance()        { return m_fTRN_WeaponDropChance; }
    bool HAM_GetShowCompass()              { return m_bTRN_ShowCompass; }

    // ==================================================================================
    // SETTERS GM (exécutés côté serveur par le manager d'attributs)
    // ==================================================================================
    void HAM_GMSetHideNameTags(bool v)            { m_bTRN_HideNameTags = v;            HAM_SettingChanged(); }
    void HAM_GMSetHideUnconsciousIcon(bool v)     { m_bTRN_HideUnconsciousIcon = v;     HAM_SettingChanged(); }
    void HAM_GMSetHideVONNames(bool v)            { m_bTRN_HideVONNames = v;            HAM_SettingChanged(); }
    void HAM_GMSetKeepWrecks(bool v)              { m_bTRN_KeepWrecks = v;              HAM_SettingChanged(); }
    void HAM_GMSetKeepCorpses(bool v)             { m_bTRN_KeepCorpses = v;             HAM_SettingChanged(); }
    void HAM_GMSetVehicleVirtualization(bool v)   { m_bTRN_VehicleVirtualization = v;   HAM_SettingChanged(); }
    void HAM_GMSetBlockUnconsciousVON(bool v)     { m_bTRN_BlockUnconsciousVON = v;     HAM_SettingChanged(); }
    void HAM_GMSetHideEnemyProxVONUI(bool v)      { m_bTRN_HideEnemyProxVONUI = v;      HAM_SettingChanged(); }
    void HAM_GMSetHideAllProxVONUI(bool v)        { m_bTRN_HideAllProxVONUI = v;        HAM_SettingChanged(); }
    void HAM_GMSetIndestructibleBuildings(bool v) { m_bTRN_IndestructibleBuildings = v; HAM_SettingChanged(); }
    void HAM_GMSetBlockPhotoMode(bool v)          { m_bTRN_BlockPhotoMode = v;          HAM_SettingChanged(); }
    void HAM_GMSetDisableVanillaMarkers(bool v)   { m_bTRN_DisableVanillaMarkers = v;   HAM_SettingChanged(); }
    void HAM_GMSetNoMinimumRank(bool v)           { m_bTRN_NoMinimumRank = v;           HAM_SettingChanged(); }
    void HAM_GMSetStealthGM(bool v)               { m_bTRN_StealthGM = v;               HAM_SettingChanged(); }
    void HAM_GMSetNameTagMaxDistance(float v)     { m_fTRN_NameTagMaxDistance = v;      HAM_SettingChanged(); }
    //! Revenu automatique coupé hors dépôts vanilla. Les dépôts restent à 10 toutes les 600 s.
    //! Le chargement d'un camion n'est pas concerné.
    void HAM_FreezeSupplyRegen()
    {
        m_bTRN_NoSupplyRegenHQ = true;
        m_bTRN_NoSupplyRegenHubs = true;
        m_bTRN_NoSupplyRegenSources = true;
        m_bTRN_NoSupplyRegenBases = true;
        m_bTRN_NoSupplyRegenDepots = false;
        m_fTRN_DepotRegenAmount = 10;
        m_fTRN_DepotRegenTickrate = 600;
        m_bSilentGmNote = true;
        HAM_SettingChanged();
        m_bSilentGmNote = false;
        SCR_ResourceSystem.HAM_RefreshDepotRegen();
    }

    void HAM_GMSetForceFirstPersonFoot(bool v)    { m_bTRN_ForceFirstPersonFoot = v;    HAM_SettingChanged(); }
    void HAM_GMSetForceFirstPersonVehicle(bool v) { m_bTRN_ForceFirstPersonVehicle = v; HAM_SettingChanged(); }
    void HAM_GMSetWeaponDropChance(float v)       { m_fTRN_WeaponDropChance = Math.Clamp(v, 0, 100); HAM_SettingChanged(); }
    void HAM_GMSetShowCompass(bool v)             { m_bTRN_ShowCompass = v;             HAM_SettingChanged(); }

    protected void HAM_SettingChanged()
    {
        if (!m_bSilentGmNote)
            HAM_ProfileSettingsApplier.NoteGmEdit();
        Replication.BumpMe();        // → OnRep sur les clients
        HAM_ApplyUnconsciousVON();   // application immédiate sur l'hôte/serveur
        HAM_ApplyPhotoModeBlock();
    }

    //! Callback RplProp — un réglage a changé côté serveur.
    //! Nametags/VON UI/marqueurs/rang : les effecteurs modded lisent les getters à
    //! chaque frame / événement, rien à pousser. Seuls le VON inconscient et la
    //! fermeture forcée du mode photo demandent une application.
    void OnRep_HAM_MilsimSettings()
    {
        HAM_ApplyUnconsciousVON();
        HAM_ApplyPhotoModeBlock();
    }

    // ==================================================================================
    // ARMA VISION — fermeture forcée du mode photo local si un joueur limité y est
    // déjà quand le GM active le blocage (le blocage d'ENTRÉE est fait par le modded
    // SCR_EditorManagerEntity.HasMode). Serveur dédié : pas d'éditeur local → no-op.
    // ==================================================================================
    protected void HAM_ApplyPhotoModeBlock()
    {
        if (!m_bTRN_BlockPhotoMode)
            return;

        SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
        if (!core)
            return;

        SCR_EditorManagerEntity manager = core.GetEditorManager();
        if (!manager)
            return;

        if (manager.IsLimited() && manager.IsOpened() && manager.GetCurrentMode() == EEditorMode.PHOTO)
            manager.Close();
    }

    // ==================================================================================
    // VON INCONSCIENT — application locale du flag vanilla (non répliqué à chaud)
    // ==================================================================================
    protected void HAM_ApplyUnconsciousVON()
    {
        SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
        if (!gamemode)
            return;

        SCR_GameModeHealthSettings healthSettings = gamemode.GetGameModeHealthSettings();
        if (healthSettings)
            healthSettings.SetUnconsciousVONPermitted(!m_bTRN_BlockUnconsciousVON);

        // Rafraîchit le cache statique du contrôleur VON local
        SCR_VONController.UpdateUnconsciousVONPermitted();
    }

    // ==================================================================================
    // PRÉRÉGLAGES — charge le .conf de défauts et l'applique aux champs répliqués.
    // AUTORITÉ SEULEMENT : les clients reçoivent les valeurs par réplication (appliquer
    // localement côté client risquerait d'écraser un état serveur déjà reçu).
    // Ordre au boot : prefab → CE conf → persistance (si save) → toggles GM en direct.
    // ==================================================================================
    protected void HAM_ApplyDefaultsFromConfig()
    {
        if (RplSession.Mode() == RplMode.Client)
            return;

        HAM_ApplyConfigValues(m_sTRN_DefaultsConfig);
        HAM_FreezeSupplyRegen();
    }

    //! Charge un .conf HAM_MilsimDefaults et applique ses 20 valeurs aux champs
    //! répliqués. Ne réplique PAS lui-même (les appelants gèrent BumpMe/SettingChanged).
    protected bool HAM_ApplyConfigValues(ResourceName cfgPath)
    {
        if (cfgPath.IsEmpty())
            return false;

        Resource container = BaseContainerTools.LoadContainer(cfgPath);
        if (!container)
            return false;

        HAM_MilsimDefaults defaults = HAM_MilsimDefaults.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
        if (!defaults)
        {
            Print("[HAM_Milsim] Config de préréglages illisible : " + cfgPath, LogLevel.WARNING);
            return false;
        }

        m_bTRN_HideNameTags          = defaults.m_bHideNameTags;
        m_bTRN_HideUnconsciousIcon   = defaults.m_bHideUnconsciousIcon;
        m_bTRN_HideVONNames          = defaults.m_bHideVONNames;
        m_bTRN_KeepWrecks            = defaults.m_bKeepWrecks;
        m_bTRN_KeepCorpses           = defaults.m_bKeepCorpses;
        m_bTRN_VehicleVirtualization = defaults.m_bVehicleVirtualization;
        m_bTRN_BlockUnconsciousVON   = defaults.m_bBlockUnconsciousVON;
        m_bTRN_HideEnemyProxVONUI    = defaults.m_bHideEnemyProxVONUI;
        m_bTRN_HideAllProxVONUI      = defaults.m_bHideAllProxVONUI;
        m_bTRN_IndestructibleBuildings = defaults.m_bIndestructibleBuildings;
        m_bTRN_BlockPhotoMode        = defaults.m_bBlockPhotoMode;
        m_bTRN_DisableVanillaMarkers = defaults.m_bDisableVanillaMarkers;
        m_bTRN_NoMinimumRank         = defaults.m_bNoMinimumRank;
        m_bTRN_StealthGM             = defaults.m_bStealthGM;
        m_fTRN_NameTagMaxDistance    = defaults.m_fNameTagMaxDistance;
        m_bTRN_NoSupplyRegenHQ       = defaults.m_bNoSupplyRegenHQ;
        m_bTRN_NoSupplyRegenHubs     = defaults.m_bNoSupplyRegenHubs;
        m_bTRN_NoSupplyRegenSources  = defaults.m_bNoSupplyRegenSources;
        m_bTRN_NoSupplyRegenBases    = defaults.m_bNoSupplyRegenBases;
        m_bTRN_NoSupplyRegenDepots   = defaults.m_bNoSupplyRegenDepots;
        m_fTRN_DepotRegenAmount      = defaults.m_fDepotRegenAmount;
        m_fTRN_DepotRegenTickrate    = defaults.m_fDepotRegenTickrate;
        m_bTRN_ForceFirstPersonFoot  = defaults.m_bForceFirstPersonFoot;
        m_bTRN_ForceFirstPersonVehicle = defaults.m_bForceFirstPersonVehicle;
        m_fTRN_WeaponDropChance      = defaults.m_fWeaponDropChance;
        m_bTRN_ShowCompass           = defaults.m_bShowCompass;

        return true;
    }

    // ==================================================================================
    // PRESETS — menu déroulant GM « Preset de réglages »
    // ==================================================================================
    int HAM_GetPresetIndex() { return m_iTRN_PresetIndex; }

    //! (serveur) applique le preset d'index donné du registre : charge son .conf,
    //! écrase les 20 réglages, mémorise l'index et réplique le tout
    void HAM_GMSetPreset(int index)
    {
        HAM_MilsimPresets presets = HAM_MilsimPresets.HAM_Load(m_sTRN_PresetsConfig);
        if (!presets || !presets.m_aPresets)
            return;

        if (index < 0 || index >= presets.m_aPresets.Count())
            return;

        HAM_MilsimPresetEntry entry = presets.m_aPresets[index];
        if (!entry || entry.m_sConfig.IsEmpty())
            return;

        if (!HAM_ApplyConfigValues(entry.m_sConfig))
            return;

        m_iTRN_PresetIndex = index;
        HAM_SettingChanged();
        HAM_FreezeSupplyRegen();
    }

    //! (persistance) restaure l'index affiché SANS ré-appliquer le .conf du preset —
    //! les 20 valeurs sont restaurées individuellement par le serializer
    void HAM_SetPresetIndexRaw(int index)
    {
        m_iTRN_PresetIndex = index;
        Replication.BumpMe();
    }

    // ==================================================================================
    override void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);

        if (SCR_Global.IsEditMode())
            return;

        // Préréglages depuis le .conf (autorité) — AVANT l'application différée, et
        // avant la désérialisation de persistance (AFTER_ENTITY_FINALIZE) qui garde
        // le dernier mot si une save existe.
        HAM_ApplyDefaultsFromConfig();

        // Application différée : laisse le gamemode et ses composants finir leur init
        // (SCR_GameModeHealthSettings, contrôleurs VON). Nettoyé en OnDelete (règle 1.7).
        GetGame().GetCallqueue().CallLater(HAM_ApplyUnconsciousVON, 2000, false);
    }

    override void OnDelete(IEntity owner)
    {
        GetGame().GetCallqueue().Remove(HAM_ApplyUnconsciousVON);

        if (s_pInstance == this)
            s_pInstance = null;

        super.OnDelete(owner);
    }
}
