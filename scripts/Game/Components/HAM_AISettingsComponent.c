[ComponentEditorProps(category: "13RDPA/IA", description: "Reglages IA globaux pilotables par le GM — a attacher au GameMode")]
class HAM_AISettingsComponentClass : ScriptComponentClass {}

class HAM_AISettingsComponent : ScriptComponent
{

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Multiplicateur de perception de l'IA. 1 = comportement d'origine, au-dessus elle voit de plus loin", params: "0 10 0.1", category: "IA - Perception")]
    protected float m_fTRN_PerceptionFactor;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Multiplicateur de dispersion du tir. 1 = d'origine, au-dessus l'IA vise moins bien", params: "0 10 0.1", category: "IA - Tir")]
    protected float m_fTRN_AimErrorScale;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Multiplicateur du temps de visee avant tir. Au-dessus de 1, l'IA stabilise plus longtemps", params: "0 10 0.1", category: "IA - Tir")]
    protected float m_fTRN_ReactionScale;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Multiplicateur du delai avant ouverture du feu a la detection", params: "0 10 0.1", category: "IA - Tir")]
    protected float m_fTRN_EngageDelayScale;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Reflexe de couvert impose a toutes les IA", params: "", enums: ParamEnumArray.FromEnum(HAM_EAICoverSeekOverride), category: "IA - Combat")]
    protected int m_iTRN_CoverSeekOverride;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Niveau de competence impose a toutes les IA. NONE = ne rien toucher", params: "", enums: ParamEnumArray.FromEnum(EAISkill), category: "IA - Tir")]
    protected int m_iTRN_AISkill;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "L'IA epuise reellement ses chargeurs. Decoche = munitions infinies", category: "IA - Tir")]
    protected bool m_bTRN_ConsumeMagazines;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Style de combat donne aux groupes restes sur Defaut", params: "", enums: ParamEnumArray.FromEnum(HAM_EAICombatStyle), category: "IA - Combat")]
    protected int m_iTRN_CombatStyle;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Formation de deplacement de tous les groupes, chef compris. Selon le style = chaque groupe garde celle de son style de combat. Un groupe ayant son propre reglage le conserve.", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIMoveFormation), category: "IA - Combat")]
    protected int m_iTRN_MoveFormation;

    [Attribute(defvalue: "6", uiwidget: UIWidgets.ComboBox, desc: "Posture donnee aux IA restees sur Suivre le reglage general. AUTOMATIC = l'IA decide seule (comportement d'origine)", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIStanceMode), category: "IA - Combat")]
    protected int m_iTRN_StanceMode;

    [Attribute(defvalue: "2", uiwidget: UIWidgets.ComboBox, desc: "Regard des IA restees sur Suivre le reglage general. HOLD = elles tiennent leur direction et ne pivotent plus vers un bruit lointain. AUTOMATIC = comportement d origine", params: "", enums: ParamEnumArray.FromEnum(HAM_EAILookMode), category: "IA - Combat")]
    protected int m_iTRN_LookMode;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Port de l'arme de toutes les IA, renforts compris. Automatique = comportement d'origine. Engagement = arme sortie seulement face a une cible ennemie. Alerte = arme sortie tant que l'IA reste en alerte. Maintenue = arme sortie apres le premier engagement. Un groupe ou un personnage ayant son propre reglage le conserve.", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIWeaponModeSetting), category: "IA - Combat")]
    protected int m_iTRN_WeaponMode;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Mode de tir de toutes les IA. Au choix de l'IA = comportement d'origine. Les autres valeurs imposent le selecteur de l'arme, quand l'arme possede ce mode : une mitrailleuse reste en rafale. Un groupe ou un personnage ayant son propre reglage le conserve.", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIFireModeSetting), category: "IA - Tir")]
    protected int m_iTRN_FireMode;

    [Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Distance au dela de laquelle un danger ne capte plus l attention de l IA, en metres. -1 = aucune limite. Sans effet sur un tir qui la vise ou qui l a touchee", params: "-1 1000 5", category: "IA - Combat")]
    protected float m_fTRN_AttentionDistance;

    [Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Duree pendant laquelle l IA continue de regarder un contact qu elle ne voit plus, en secondes. 1 = valeur d origine du jeu", params: "1 60 1", category: "IA - Combat")]
    protected float m_fTRN_ContactMemory;

    [Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Distance de manoeuvre des groupes, en metres. -1 = aucune limite", params: "-1 5000 10", category: "IA - Combat")]
    protected float m_fTRN_MaxEngagementDistance;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Probabilite en % qu'un inconscient reste une cible. 0 = jamais acheve", params: "0 100 5", category: "IA - Combat")]
    protected int m_iTRN_FinishOffChance;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Un personnage sans arme cesse d'etre pris pour cible", category: "IA - Combat")]
    protected bool m_bTRN_SpareUnarmed;

    [Attribute(defvalue: "500", uiwidget: UIWidgets.EditBox, desc: "Portee du partage des contacts entre groupes amis, en metres. 0 = desactive", params: "0 5000 50", category: "IA - Perception")]
    protected float m_fTRN_ContactShareRange;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Perception sous le feu. 1 = d'origine (une IA suppressee devient quasi aveugle), ~8 annule cet ecrasement", params: "0 20 0.5", category: "IA - Perception")]
    protected float m_fTRN_ThreatenedPerceptionMul;

    [Attribute(defvalue: "100", uiwidget: UIWidgets.EditBox, desc: "Distance a laquelle l'IA entend un tir SILENCE, en metres. 100 = valeur d'origine, 0 = elle n'entend pas", params: "0 500 10", category: "IA - Perception")]
    protected float m_fTRN_SuppressedAudibleDistance;

    [Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Distance de tir a vue, en metres. -1 = aucune limite. Au-dela, l'IA surveille sans ouvrir le feu", params: "-1 2000 25", category: "IA - Tir")]
    protected float m_fTRN_FireAtWillDistance;

    [Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Distance de riposte, en metres. -1 = aucune limite. Toujours au moins egale au tir a vue", params: "-1 2000 25", category: "IA - Tir")]
    protected float m_fTRN_ReturnFireDistance;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Navigation a pied : rapport de l'IA aux routes", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIFootNavPreset), category: "IA - Navigation")]
    protected int m_iTRN_FootNavPreset;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.ComboBox, desc: "Navigation des vehicules : rapport au bitume, aux pistes et au hors-piste", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIVehicleNavPreset), category: "IA - Navigation")]
    protected int m_iTRN_VehicleNavPreset;

    [Attribute(defvalue: "30", uiwidget: UIWidgets.EditBox, desc: "Vitesse de croisiere des vehicules terrestres conduits par l'IA, en km/h. 0 = vitesse d'origine. Helicopteres non concernes", params: "0 150 5", category: "IA - Vehicules")]
    protected float m_fTRN_VehicleCruiseSpeed;

    [Attribute(defvalue: "3", uiwidget: UIWidgets.ComboBox, desc: "Qui reste a bord. FREE = l'IA decide, ALL = personne ne debarque, CREW = conducteur et tireurs seulement", params: "", enums: ParamEnumArray.FromEnum(HAM_EAIStayInVehicleMode), category: "IA - Vehicules")]
    protected int m_iTRN_StayInVehicleMode;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Les vehicules consomment leur carburant. Decoche = les reservoirs ne se vident plus", category: "IA - Vehicules")]
    protected bool m_bTRN_FuelConsumption;

    [Attribute(defvalue: "8", uiwidget: UIWidgets.EditBox, desc: "Multiplicateur de consommation. 1 = consommation reelle, 8 = valeur d'origine du jeu", params: "0.5 20 0.5", category: "IA - Vehicules")]
    protected float m_fTRN_FuelConsumptionScale;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Les IA lancent des drones de reconnaissance qui signalent les contacts aux groupes amis.", category: "IA - Perception")]
    protected bool m_bTRN_ReconDrones;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Les drones de reconnaissance larguent une grenade sur un contact. Sans effet si les drones IA sont desactives", category: "IA - Perception")]
    protected bool m_bTRN_BombingDrones;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.EditBox, desc: "Nombre maximal de drones de reconnaissance en vol en meme temps", params: "1 6 1", category: "IA - Perception")]
    protected int m_iTRN_ReconDronesMax;

    [Attribute(defvalue: "10", uiwidget: UIWidgets.EditBox, desc: "Delai minimal entre deux drones, en minutes, compte depuis la fin du vol precedent", params: "1 60 1", category: "IA - Perception")]
    protected float m_fTRN_ReconDronesInterval;

    [Attribute(defvalue: "2", uiwidget: UIWidgets.EditBox, desc: "Nombre de groupes amis alertes par contact de drone (les plus proches du contact). Sert aussi au nombre de patrouilles envoyees fouiller", params: "1 5 1", category: "IA - Perception")]
    protected int m_iTRN_ReconDronesAlertGroups;

    [Attribute(desc: "Presets de style de combat (index = HAM_EAICombatStyle : 0 Defaut, 1 Offensif, 2 Urbain, 3 Defensif, 4 Mecanise, 5 CQB defensif, 6 CQB offensif)", params: "conf", category: "IA")]
    protected ref array<ResourceName> m_aTRN_StylePresets;

    protected static HAM_AISettingsComponent s_pInstance;

    static HAM_AISettingsComponent GetInstance()
    {
        if (!s_pInstance)
        {
            BaseGameMode gamemode = GetGame().GetGameMode();
            if (gamemode)
                s_pInstance = HAM_AISettingsComponent.Cast(gamemode.FindComponent(HAM_AISettingsComponent));

            if (s_pInstance && Replication.IsServer())
                GetGame().GetCallqueue().CallLater(HAM_AINVGToggle.Sweep, HAM_AINVGToggle.CHECK_PERIOD_MS, true);
        }
        return s_pInstance;
    }

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Les IA descendent leurs NVG a la nuit tombee et les relevent au jour. Decoche = elles n'y touchent pas", category: "IA - Perception")]
    protected bool m_bTRN_NVGAuto;

    [Attribute(defvalue: "3", uiwidget: UIWidgets.EditBox, desc: "Vitesse de reconnaissance d'une IA dont les NVG sont descendues. Echelle du jeu : 1 = standard, 3 = ce que donnent les jumelles. Au-dela de 3 on sort de la plage prevue par Bohemia", params: "1 5 0.1", category: "IA - Perception")]
    protected float m_fTRN_NVGPerceptionMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.EditBox, desc: "Vitesse de reconnaissance de nuit d'une IA sans NVG. Echelle du jeu : 1 = standard, 0.4 = ce que donne la suppression, 3 = les jumelles", params: "0.2 3 0.1", category: "IA - Perception")]
    protected float m_fTRN_NightPerceptionMul;

    static float HAM_PerceptionFactor()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_PerceptionFactor;
    }

    static float HAM_AimErrorScale()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_AimErrorScale;
    }

    static float HAM_ReactionScale()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_ReactionScale;
    }

    static float HAM_EngageDelayScale()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_EngageDelayScale;
    }

    static int HAM_CoverSeekOverride()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAICoverSeekOverride.FROM_STYLE;
        return inst.m_iTRN_CoverSeekOverride;
    }

    static int HAM_AISkill()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return EAISkill.NONE;
        return inst.m_iTRN_AISkill;
    }

    static bool HAM_ConsumeMagazines()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return true;
        return inst.m_bTRN_ConsumeMagazines;
    }

    static int HAM_CombatStyle()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAICombatStyle.DEFAULT;
        return inst.m_iTRN_CombatStyle;
    }

    static int HAM_MoveFormation()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAIMoveFormation.FROM_GENERAL;
        return inst.m_iTRN_MoveFormation;
    }

    static int HAM_StanceMode()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAIStanceMode.AUTOMATIC;
        return inst.m_iTRN_StanceMode;
    }

    static int HAM_LookMode()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAILookMode.AUTOMATIC;
        return inst.m_iTRN_LookMode;
    }

    static float HAM_AttentionDistance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return -1;
        return inst.m_fTRN_AttentionDistance;
    }

    static float HAM_ContactMemory()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 10;
        return inst.m_fTRN_ContactMemory;
    }

    static float HAM_MaxEngagementDistance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return -1;
        return inst.m_fTRN_MaxEngagementDistance;
    }

    static int HAM_FinishOffChance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_iTRN_FinishOffChance;
    }

    static bool HAM_SpareUnarmed()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_SpareUnarmed;
    }

    static float HAM_ContactShareRange()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_fTRN_ContactShareRange;
    }

    static bool HAM_ReconDronesEnabled()
    {
        if (!HAM_DroneSupport.IsAvailable())
            return false;

        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_ReconDrones;
    }

    static int HAM_ReconDronesMaxCount()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1;
        return inst.m_iTRN_ReconDronesMax;
    }

    static int HAM_ReconDronesAlertCount()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 2;
        return inst.m_iTRN_ReconDronesAlertGroups;
    }

    static float HAM_ReconDronesIntervalMs()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 600000;
        return inst.m_fTRN_ReconDronesInterval * 60000;
    }

    static bool HAM_BombingDronesEnabled()
    {
        if (!HAM_ReconDronesEnabled())
            return false;

        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return false;
        return inst.m_bTRN_BombingDrones;
    }

    static bool HAM_NVGAuto()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return true;
        return inst.m_bTRN_NVGAuto;
    }

    static float HAM_NightPerceptionMul()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_NightPerceptionMul;
    }

    static float HAM_NVGPerceptionMul()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_NVGPerceptionMul;
    }

    static float HAM_ThreatenedPerceptionMul()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 1.0;
        return inst.m_fTRN_ThreatenedPerceptionMul;
    }

    static float HAM_SuppressedAudibleDistance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 100;
        return inst.m_fTRN_SuppressedAudibleDistance;
    }

    static float HAM_FireAtWillDistance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return -1;
        return inst.m_fTRN_FireAtWillDistance;
    }

    static float HAM_ReturnFireDistance()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return -1;
        return inst.m_fTRN_ReturnFireDistance;
    }

    static int HAM_FootNavPreset()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAIFootNavPreset.BALANCED;
        return inst.m_iTRN_FootNavPreset;
    }

    static int HAM_VehicleNavPreset()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAIVehicleNavPreset.ROADS_PREFERRED;
        return inst.m_iTRN_VehicleNavPreset;
    }

    static float HAM_VehicleCruiseSpeed()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return 0;
        return inst.m_fTRN_VehicleCruiseSpeed;
    }

    static int HAM_StayInVehicleMode()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst) return HAM_EAIStayInVehicleMode.FREE;
        return inst.m_iTRN_StayInVehicleMode;
    }

    static float HAM_FuelScale()
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst)
            return SCR_FuelConsumptionComponent.GetGlobalFuelConsumptionScale();

        if (!inst.m_bTRN_FuelConsumption)
            return 0;

        return inst.m_fTRN_FuelConsumptionScale;
    }

    static ResourceName HAM_StylePresetPath(int style)
    {
        HAM_AISettingsComponent inst = GetInstance();
        if (!inst || !inst.m_aTRN_StylePresets)
            return ResourceName.Empty;

        if (style < 0 || style >= inst.m_aTRN_StylePresets.Count())
            return ResourceName.Empty;

        return inst.m_aTRN_StylePresets[style];
    }

    static ResourceName HAM_FootNavConfigFor(int preset)
    {
        switch (preset)
        {
            case HAM_EAIFootNavPreset.ROADS_PREFERRED:
                return "{E7C833E8A6C73BE0}Configs/Navigation/HAM_NavFoot_RoadsPreferred.conf";
            case HAM_EAIFootNavPreset.AVOID_ROADS:
                return "{34C35C56B423792B}Configs/Navigation/HAM_NavFoot_AvoidRoads.conf";
            case HAM_EAIFootNavPreset.NO_ROADS:
                return "{603032C4FD56A454}Configs/Navigation/HAM_NavFoot_NoRoads.conf";
            case HAM_EAIFootNavPreset.TRAILS_PREFERRED:
                return "{99D270699C3DDEE3}Configs/Navigation/HAM_NavFoot_TrailsPreferred.conf";
        }

        return "{AD0CE49E65D528C6}Configs/Navigation/HAM_NavFoot_Balanced.conf";
    }

    static ResourceName HAM_FootNavConfig()
    {
        return HAM_FootNavConfigFor(HAM_FootNavPreset());
    }

    static ResourceName HAM_VehicleNavConfigFor(int preset)
    {
        switch (preset)
        {
            case HAM_EAIVehicleNavPreset.ROADS_ONLY:
                return "{E44B209E274905A2}Configs/Navigation/HAM_NavVehicle_RoadsOnly.conf";
            case HAM_EAIVehicleNavPreset.FREE:
                return "{C31E27F415B761E7}Configs/Navigation/HAM_NavVehicle_Free.conf";
            case HAM_EAIVehicleNavPreset.AVOID_ASPHALT:
                return "{CFC319D3A04C3D74}Configs/Navigation/HAM_NavVehicle_AvoidAsphalt.conf";
            case HAM_EAIVehicleNavPreset.DIRT_PREFERRED:
                return "{2319ED5BFB375114}Configs/Navigation/HAM_NavVehicle_DirtPreferred.conf";
            case HAM_EAIVehicleNavPreset.OFFROAD_ONLY:
                return "{CD849E4E3EC254D4}Configs/Navigation/HAM_NavVehicle_OffRoadOnly.conf";
        }

        return "{F5C7B2C33793333F}Configs/Navigation/HAM_NavVehicle_RoadsPreferred.conf";
    }

    static ResourceName HAM_VehicleNavConfig()
    {
        return HAM_VehicleNavConfigFor(HAM_VehicleNavPreset());
    }

    float HAM_GetPerceptionFactor() { return m_fTRN_PerceptionFactor; }
    float HAM_GetAimErrorScale()    { return m_fTRN_AimErrorScale; }
    float HAM_GetReactionScale()    { return m_fTRN_ReactionScale; }
    float HAM_GetEngageDelayScale() { return m_fTRN_EngageDelayScale; }
    int   HAM_GetAISkill()          { return m_iTRN_AISkill; }
    int   HAM_GetCoverSeekOverride() { return m_iTRN_CoverSeekOverride; }
    bool  HAM_GetConsumeMagazines() { return m_bTRN_ConsumeMagazines; }
    bool  HAM_GetNVGAuto() { return m_bTRN_NVGAuto; }
    float HAM_GetNVGPerceptionMul() { return m_fTRN_NVGPerceptionMul; }
    float HAM_GetNightPerceptionMul() { return m_fTRN_NightPerceptionMul; }
    int   HAM_GetCombatStyle()      { return m_iTRN_CombatStyle; }
    int   HAM_GetMoveFormation()    { return m_iTRN_MoveFormation; }
    int   HAM_GetStanceMode()       { return m_iTRN_StanceMode; }
    int   HAM_GetLookMode()         { return m_iTRN_LookMode; }
    int   HAM_GetWeaponMode()       { return m_iTRN_WeaponMode; }
    int   HAM_GetFireMode()         { return m_iTRN_FireMode; }
    float HAM_GetAttentionDistance() { return m_fTRN_AttentionDistance; }
    float HAM_GetContactMemory()     { return m_fTRN_ContactMemory; }
    float HAM_GetMaxEngagementDistance() { return m_fTRN_MaxEngagementDistance; }
    int   HAM_GetFinishOffChance()       { return m_iTRN_FinishOffChance; }
    bool  HAM_GetSpareUnarmed()          { return m_bTRN_SpareUnarmed; }
    float HAM_GetContactShareRange()     { return m_fTRN_ContactShareRange; }
    bool  HAM_GetReconDrones()           { return m_bTRN_ReconDrones; }
    bool  HAM_GetBombingDrones()         { return m_bTRN_BombingDrones; }
    int   HAM_GetReconDronesMax()        { return m_iTRN_ReconDronesMax; }
    float HAM_GetReconDronesInterval()   { return m_fTRN_ReconDronesInterval; }
    int   HAM_GetReconDronesAlertGroups() { return m_iTRN_ReconDronesAlertGroups; }
    float HAM_GetFireAtWillDistance()    { return m_fTRN_FireAtWillDistance; }
    float HAM_GetReturnFireDistance()    { return m_fTRN_ReturnFireDistance; }
    float HAM_GetThreatenedPerceptionMul()   { return m_fTRN_ThreatenedPerceptionMul; }
    float HAM_GetSuppressedAudibleDistance() { return m_fTRN_SuppressedAudibleDistance; }
    int   HAM_GetFootNavPreset()             { return m_iTRN_FootNavPreset; }
    int   HAM_GetVehicleNavPreset()          { return m_iTRN_VehicleNavPreset; }
    float HAM_GetVehicleCruiseSpeed()        { return m_fTRN_VehicleCruiseSpeed; }
    int   HAM_GetStayInVehicleMode()         { return m_iTRN_StayInVehicleMode; }
    bool  HAM_GetFuelConsumption()           { return m_bTRN_FuelConsumption; }
    float HAM_GetFuelConsumptionScale()      { return m_fTRN_FuelConsumptionScale; }

    void HAM_GMSetThreatenedPerceptionMul(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_ThreatenedPerceptionMul = v;
    }

    void HAM_GMSetSuppressedAudibleDistance(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_SuppressedAudibleDistance = v;
    }

    void HAM_GMSetContactShareRange(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_ContactShareRange = v;
    }

    void HAM_GMSetReconDrones(bool enabled)
    {
        m_bTRN_ReconDrones = enabled && HAM_DroneSupport.IsAvailable();
    }

    void HAM_GMSetBombingDrones(bool enabled)
    {
        m_bTRN_BombingDrones = enabled && HAM_DroneSupport.IsAvailable();
    }

    void HAM_GMSetReconDronesMax(int v)
    {
        m_iTRN_ReconDronesMax = Math.Clamp(v, 1, 6);
    }

    void HAM_GMSetReconDronesInterval(float v)
    {
        m_fTRN_ReconDronesInterval = Math.Clamp(v, 1, 60);
    }

    void HAM_GMSetReconDronesAlertGroups(int v)
    {
        m_iTRN_ReconDronesAlertGroups = Math.Clamp(v, 1, 5);
    }

    void HAM_GMSetFireAtWillDistance(float v)
    {
        if (v <= 0)
            v = -1;

        m_fTRN_FireAtWillDistance = v;
    }

    void HAM_GMSetReturnFireDistance(float v)
    {
        if (v <= 0)
            v = -1;

        m_fTRN_ReturnFireDistance = v;
    }

    void HAM_GMSetSpareUnarmed(bool v)
    {
        m_bTRN_SpareUnarmed = v;

        SCR_CharacterPerceivableComponent.HAM_RefreshAllCharacters();
    }

    void HAM_GMSetFinishOffChance(int v)
    {
        m_iTRN_FinishOffChance = Math.ClampInt(v, 0, 100);
    }

    void HAM_GMSetPerceptionFactor(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_PerceptionFactor = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetAimErrorScale(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_AimErrorScale = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetReactionScale(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_ReactionScale = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetEngageDelayScale(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_EngageDelayScale = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetCoverSeekOverride(int v)
    {
        m_iTRN_CoverSeekOverride = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetAISkill(int v)
    {
        m_iTRN_AISkill = v;
        HAM_SettingChanged();
    }

    void HAM_GMSetConsumeMagazines(bool v)
    {
        m_bTRN_ConsumeMagazines = v;
    }

    void HAM_GMSetNVGAuto(bool v)
    {
        m_bTRN_NVGAuto = v;
    }

    void HAM_GMSetNVGPerceptionMul(float v)
    {
        m_fTRN_NVGPerceptionMul = v;
        HAM_AINVGToggle.RefreshAllPerception();
    }

    void HAM_GMSetNightPerceptionMul(float v)
    {
        m_fTRN_NightPerceptionMul = v;
        HAM_AINVGToggle.RefreshAllPerception();
    }

    void HAM_GMSetCombatStyle(int v)
    {
        m_iTRN_CombatStyle = v;

        SCR_AIGroupUtilityComponent.HAM_ApplyStyleToAllGroups();
    }

    void HAM_GMSetMoveFormation(int v)
    {
        if (v < HAM_EAIMoveFormation.FROM_GENERAL || v > HAM_EAIMoveFormation.STAGGERED_COLUMN)
            v = HAM_EAIMoveFormation.FROM_GENERAL;

        m_iTRN_MoveFormation = v;

        SCR_AIGroupUtilityComponent.HAM_ApplyMoveFormationToAllGroups();
    }

    void HAM_GMSetStanceMode(int v)
    {
        m_iTRN_StanceMode = v;

        SCR_AICombatComponent.HAM_ApplyStanceToAllAgents();
    }

    void HAM_GMSetLookMode(int v)
    {
        m_iTRN_LookMode = v;
    }

    void HAM_GMSetWeaponMode(int v)
    {
        if (v != HAM_EAIWeaponMode.AUTOMATIC && v != HAM_EAIWeaponMode.ENGAGEMENT_ONLY && v != HAM_EAIWeaponMode.ENGAGEMENT_STICKY && v != HAM_EAIWeaponMode.ALERT_ONLY)
            v = HAM_EAIWeaponMode.AUTOMATIC;

        m_iTRN_WeaponMode = v;
        SCR_AICombatComponent.HAM_ApplyWeaponModeToAllAgents();
    }

    void HAM_GMSetFireMode(int v)
    {
        if (v != HAM_EAIFireModeChoice.AUTOMATIC && v != HAM_EAIFireModeChoice.SEMI
            && v != HAM_EAIFireModeChoice.BURST && v != HAM_EAIFireModeChoice.FULL_AUTO)
            v = HAM_EAIFireModeChoice.AUTOMATIC;

        m_iTRN_FireMode = v;
    }

    void HAM_GMSetAttentionDistance(float v)
    {
        if (v <= 0)
            v = -1;

        m_fTRN_AttentionDistance = v;
    }

    void HAM_GMSetContactMemory(float v)
    {
        if (v < 1)
            v = 1;

        m_fTRN_ContactMemory = v;
    }

    void HAM_GMSetMaxEngagementDistance(float v)
    {
        if (v <= 0)
            v = -1;

        m_fTRN_MaxEngagementDistance = v;

        SCR_AIGroupUtilityComponent.HAM_ApplyEngagementDistanceToAllGroups();
    }

    void HAM_GMSetFootNavPreset(int v)
    {
        m_iTRN_FootNavPreset = v;

        SCR_AICombatComponent.HAM_ApplySettingsToAllAgents();
        SCR_AIGroupUtilityComponent.HAM_ApplyNavToAllGroups();
    }

    void HAM_GMSetVehicleNavPreset(int v)
    {
        m_iTRN_VehicleNavPreset = v;

        SCR_AICombatComponent.HAM_ApplySettingsToAllAgents();
    }

    void HAM_GMSetVehicleCruiseSpeed(float v)
    {
        if (v < 0)
            v = 0;

        m_fTRN_VehicleCruiseSpeed = v;

        SCR_AICombatComponent.HAM_ApplySettingsToAllAgents();
    }

    void HAM_GMSetStayInVehicleMode(int v)
    {
        m_iTRN_StayInVehicleMode = v;
    }

    void HAM_GMSetFuelConsumption(bool enabled, int playerID = -1)
    {
        m_bTRN_FuelConsumption = enabled;
        HAM_PushFuelScaleToEngine(playerID);
    }

    void HAM_GMSetFuelConsumptionScale(float v, int playerID = -1)
    {
        if (v < 0)
            v = 0;

        m_fTRN_FuelConsumptionScale = v;
        HAM_PushFuelScaleToEngine(playerID);
    }

    protected void HAM_PushFuelScaleToEngine(int playerID)
    {
        float scale = m_fTRN_FuelConsumptionScale;
        if (!m_bTRN_FuelConsumption)
            scale = 0;

        SCR_FuelConsumptionComponent.SetGlobalFuelConsumptionScale(scale, playerID);
    }

    protected void HAM_SettingChanged()
    {

        SCR_AICombatComponent.HAM_ApplySettingsToAllAgents();
    }

    override void OnDelete(IEntity owner)
    {
        if (s_pInstance == this)
        {
            GetGame().GetCallqueue().Remove(HAM_AINVGToggle.Sweep);
            s_pInstance = null;
        }

        super.OnDelete(owner);
    }
}
