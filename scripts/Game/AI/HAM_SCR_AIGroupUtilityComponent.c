modded class SCR_AIGroupUtilityComponent : SCR_AIBaseUtilityComponent
{
    protected const int HAM_MAX_TEAMS = 8;

    protected const ResourceName HAM_HOLD_WAYPOINT = "{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et";

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Style de combat du groupe : articulation, formation, progression au contact.", enums: ParamEnumArray.FromEnum(HAM_EAICombatStyle), category: "IA")]
    protected HAM_EAICombatStyle m_eTRN_CombatStyle;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Forcer la riposte immediate ou la recherche de couvert, quel que soit le style.", enums: ParamEnumArray.FromEnum(HAM_EAICoverSeekOverride), category: "IA")]
    protected HAM_EAICoverSeekOverride m_eTRN_CoverSeekOverride;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Le groupe n'engage pas d'ennemis reperes hors de la zone (waypoint Defend pose sur place). Ne le cloue PAS sur place : pour cela, Immobiliser. Tant que c'est actif, les ordres de deplacement donnes au groupe ne s'executent pas.", category: "IA")]
    protected bool m_bTRN_HoldPosition;

    [Attribute(defvalue: "15", uiwidget: UIWidgets.Slider, desc: "Rayon du maintien de position, en metres. JAMAIS 0 : le confinement serait casse et l'IA sortirait.", params: "1 100 1", category: "IA")]
    protected float m_fTRN_HoldRadius;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Distance de MANOEUVRE : au-dela, le groupe ne se deplace ni pour attaquer ni pour inspecter un contact. Ne limite PAS le tir (voir tir a vue et riposte). 0 = suit le reglage global ; -1 = illimite pour ce groupe. SANS EFFET si le maintien de position est actif.", params: "-1 2000 25", category: "IA")]
    protected float m_fTRN_MaxEngagementDistance;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Distance de TIR A VUE : au-dela, les soldats du groupe surveillent sans ouvrir le feu. 0 = suit le reglage global ; -1 = illimite pour ce groupe.", params: "-1 2000 25", category: "IA")]
    protected float m_fTRN_FireAtWillDistance;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Distance de RIPOSTE : jusqu'ou les soldats rendent le feu a une cible qui les vise ou leur tire dessus. Toujours au moins egale au tir a vue. 0 = suit le reglage global ; -1 = illimite pour ce groupe.", params: "-1 2000 25", category: "IA")]
    protected float m_fTRN_ReturnFireDistance;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Ce groupe epuise-t-il ses chargeurs ? Suit le global par defaut.", enums: ParamEnumArray.FromEnum(HAM_EAIMagazineConsumption), category: "IA")]
    protected HAM_EAIMagazineConsumption m_eTRN_MagazineConsumption;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Navigation A PIED de ce groupe (attirance pour les routes). FROM_GLOBAL = suit le reglage du scenario.", enums: ParamEnumArray.FromEnum(HAM_EAIFootNavChoice), category: "IA")]
    protected HAM_EAIFootNavChoice m_eTRN_FootNavChoice;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Navigation VEHICULE de ce groupe (attirance pour les routes des vehicules qu'il conduit). FROM_GLOBAL = suit le reglage du scenario.", enums: ParamEnumArray.FromEnum(HAM_EAIVehicleNavChoice), category: "IA")]
    protected HAM_EAIVehicleNavChoice m_eTRN_VehicleNavChoice;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Vitesse des vehicules terrestres conduits par ce groupe, en km/h. 0 = suit le reglage global ; -1 = vitesse d'origine pour ce groupe.", params: "-1 120 1", category: "IA")]
    protected float m_fTRN_VehicleCruiseSpeed;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Le groupe arrose-t-il une position ennemie pour la clouer ? Decoche : il ne tire que sur ce qu'il voit.", category: "IA")]
    protected bool m_bTRN_SuppressiveFire;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Formation de chaque binome. Selon le style par defaut.", enums: ParamEnumArray.FromEnum(HAM_EAIFormationChoice), category: "IA")]
    protected HAM_EAIFormationChoice m_eTRN_FormationChoice;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Formation de deplacement de tout le groupe, chef compris. Prend le pas sur la formation du style de combat. Suivre le reglage general : reglage global, puis style.", enums: ParamEnumArray.FromEnum(HAM_EAIMoveFormation), category: "IA")]
    protected HAM_EAIMoveFormation m_eTRN_MoveFormation;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Espacement de la formation. 0 = selon le style. 1 = espacement d'origine du jeu.", params: "0 4 0.1", category: "IA")]
    protected float m_fTRN_FormationScale;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Distance sous laquelle l'IA cesse d'avancer vers sa cible, en metres. 0 = selon le style ; -1 = calcul d'origine du jeu.", params: "-1 100 1", category: "IA")]
    protected float m_fTRN_ApproachFloor;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.Slider, desc: "Temps a l'arret entre deux bonds au contact, en secondes (cas de reference : a couvert, hors suppression ; 5 s dans le jeu d'origine, les autres situations s'ajustent proportionnellement). 0 = selon le style.", params: "0 30 0.5", category: "IA")]
    protected float m_fTRN_HoldTime_s;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Le groupe exige-t-il un couvert a l'arrivee d'un bond ? Selon le style par defaut.", enums: ParamEnumArray.FromEnum(HAM_EAICoverRequirement), category: "IA")]
    protected HAM_EAICoverRequirement m_eTRN_CoverRequirement;

    protected ref array<int> m_aTRN_TeamHandlers = {};

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Perception donnee aux membres restes neutres. 1 = ne rien imposer.", params: "0 3 0.1", category: "IA")]
    protected float m_fTRN_GroupPerceptionMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Dispersion du tir donnee aux membres restes neutres. 1 = ne rien imposer.", params: "0 4 0.1", category: "IA")]
    protected float m_fTRN_GroupAimErrorMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Temps de visee avant tir donne aux membres restes neutres. 1 = ne rien imposer.", params: "0 4 0.1", category: "IA")]
    protected float m_fTRN_GroupReactionMul;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Delai d'ouverture du feu donne aux membres restes neutres. 1 = ne rien imposer.", params: "0 5 0.1", category: "IA")]
    protected float m_fTRN_GroupEngageDelayMul;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Competence de tir donnee aux membres restes neutres. NONE = ne rien imposer.", enums: ParamEnumArray.FromEnum(EAISkill), category: "IA")]
    protected EAISkill m_eTRN_GroupSkill;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Posture donnee aux membres. FROM_GLOBAL = suit le reglage general du scenario. AUTOMATIC = chaque membre decide seul.", enums: ParamEnumArray.FromEnum(HAM_EAIStanceMode), category: "IA")]
    protected HAM_EAIStanceMode m_eTRN_GroupStanceMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Regard donne aux membres. FROM_GLOBAL = suit le reglage general du scenario. HOLD = ils tiennent leur direction. AUTOMATIC = comportement d origine.", enums: ParamEnumArray.FromEnum(HAM_EAILookMode), category: "IA")]
    protected HAM_EAILookMode m_eTRN_GroupLookMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Port de l'arme des membres du groupe. Suivre le reglage general = le scenario decide. Automatique = comportement d'origine. Engagement = arme sortie seulement face a une cible ennemie. Alerte = arme sortie tant que l'IA reste en alerte. Maintenue = arme sortie apres le premier engagement.", enums: ParamEnumArray.FromEnum(HAM_EAIWeaponModeSetting), category: "IA")]
    protected HAM_EAIWeaponMode m_eTRN_GroupWeaponMode;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Mode de tir des membres du groupe. Suivre le reglage general = le scenario decide. Au choix de l'IA = comportement d'origine. Les autres valeurs imposent le selecteur de l'arme quand l'arme possede ce mode : une mitrailleuse reste en rafale.", enums: ParamEnumArray.FromEnum(HAM_EAIFireModeChoice), category: "IA")]
    protected HAM_EAIFireModeChoice m_eTRN_GroupFireMode;
    protected bool m_bTRN_GroupWeaponEngagementSeen;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, desc: "Duree pendant laquelle les membres continuent de regarder un contact perdu, en secondes. 0 = suit le reglage general du scenario", params: "0 60 1", category: "IA")]
    protected float m_fTRN_GroupContactMemory;

    [Attribute(defvalue: "1.0", uiwidget: UIWidgets.Slider, desc: "Vue des membres SOUS LE FEU. 1 = suit le reglage global ; toute autre valeur l'emporte seule. Le jeu ecrase la vue d'une IA suppressee a un tiers ; vers 8 l'ecrasement est annule. Ne touche pas la mise a couvert.", params: "0 10 0.5", category: "IA")]
    protected float m_fTRN_ThreatenedPerceptionMul;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.ComboBox, desc: "Consigne vehicule des membres. FROM_GLOBAL = suit le reglage general du scenario. ALL = personne ne debarque. CREW = seuls conducteur et tireurs restent, les passagers debarquent. FREE = l'IA decide, meme si le scenario impose une consigne. Les renforts rejoignant le groupe adoptent la consigne.", enums: ParamEnumArray.FromEnum(HAM_EAIStayInVehicleMode), category: "IA")]
    protected HAM_EAIStayInVehicleMode m_eTRN_StayInVehicleMode;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Les vehicules conduits par ce groupe consomment leur carburant. Decoche = leurs reservoirs ne se vident plus, quel que soit le multiplicateur", category: "IA")]
    protected bool m_bTRN_FuelConsumption;

    [Attribute(defvalue: "-1", uiwidget: UIWidgets.EditBox, desc: "Consommation de carburant des vehicules conduits par ce groupe. -1 = suit le scenario, 0 = ne consomme pas, sinon multiplicateur", params: "-1 20 0.5", category: "IA")]
    protected float m_fTRN_FuelScale;

    float HAM_GetGroupPerceptionMul()  { return m_fTRN_GroupPerceptionMul; }
    float HAM_GetGroupAimErrorMul()    { return m_fTRN_GroupAimErrorMul; }
    float HAM_GetGroupReactionMul()    { return m_fTRN_GroupReactionMul; }
    float HAM_GetGroupEngageDelayMul() { return m_fTRN_GroupEngageDelayMul; }
    int   HAM_GetGroupSkill()          { return m_eTRN_GroupSkill; }

    float HAM_GetThreatenedPerceptionMul()
    {
        return SCR_AICombatComponent.HAM_ComposeSetting(m_fTRN_ThreatenedPerceptionMul,
            HAM_AISettingsComponent.HAM_ThreatenedPerceptionMul());
    }

    float HAM_GetOwnThreatenedPerceptionMul()  { return m_fTRN_ThreatenedPerceptionMul; }
    void  HAM_SetThreatenedPerceptionMul(float v) { if (v < 0) v = 0; m_fTRN_ThreatenedPerceptionMul = v; }

    void HAM_SetGroupPerceptionMul(float v)  { if (v < 0) v = 0; m_fTRN_GroupPerceptionMul = v; }
    void HAM_SetGroupAimErrorMul(float v)    { if (v < 0) v = 0; m_fTRN_GroupAimErrorMul = v; }
    void HAM_SetGroupReactionMul(float v)    { if (v < 0) v = 0; m_fTRN_GroupReactionMul = v; }
    void HAM_SetGroupEngageDelayMul(float v) { if (v < 0) v = 0; m_fTRN_GroupEngageDelayMul = v; }
    void HAM_SetGroupSkill(int skill)        { m_eTRN_GroupSkill = skill; }

    int  HAM_GetGroupStayInVehicleMode()         { return m_eTRN_StayInVehicleMode; }
    void HAM_SetGroupStayInVehicleMode(int mode) { m_eTRN_StayInVehicleMode = mode; }

    bool HAM_GetGroupFuelConsumption()         { return m_bTRN_FuelConsumption; }
    void HAM_SetGroupFuelConsumption(bool v)   { m_bTRN_FuelConsumption = v; }

    float HAM_GetGroupFuelScale()          { return m_fTRN_FuelScale; }
    void  HAM_SetGroupFuelScale(float v)
    {
        if (v < -1)
            v = -1;

        m_fTRN_FuelScale = v;
    }

    float HAM_GetGroupFuelScaleEffective()
    {
        if (!m_bTRN_FuelConsumption)
            return 0;

        return m_fTRN_FuelScale;
    }

    int HAM_GetGroupStanceMode() { return m_eTRN_GroupStanceMode; }

    int HAM_GetGroupLookMode() { return m_eTRN_GroupLookMode; }

    int HAM_GetGroupWeaponMode() { return m_eTRN_GroupWeaponMode; }

    int  HAM_GetGroupFireMode()          { return m_eTRN_GroupFireMode; }
    void HAM_SetGroupFireMode(int mode)  { m_eTRN_GroupFireMode = mode; }
    void HAM_SetGroupWeaponMode(int mode)
    {
        m_eTRN_GroupWeaponMode = mode;
        m_bTRN_GroupWeaponEngagementSeen = false;
        HAM_PushGroupWeaponMode();
    }

    bool HAM_HasGroupWeaponEngagementSeen()
    {
        return m_bTRN_GroupWeaponEngagementSeen;
    }

    void HAM_MarkGroupWeaponEngagementSeen()
    {
        if (m_bTRN_GroupWeaponEngagementSeen)
            return;

        m_bTRN_GroupWeaponEngagementSeen = true;
        HAM_PushGroupWeaponMode();
    }

    void HAM_PushGroupWeaponMode()
    {
        if (!m_Owner)
            return;

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
            if (combat)
                combat.HAM_ApplyConfiguredWeaponMode();
        }
    }

    float HAM_GetGroupContactMemory() { return m_fTRN_GroupContactMemory; }

    void HAM_SetGroupContactMemory(float seconds)
    {
        m_fTRN_GroupContactMemory = seconds;

        if (!m_Owner)
            return;

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
            if (combat)
                combat.HAM_SetContactMemory(seconds);
        }
    }

    int HAM_GetGroupLookModeEffective()
    {
        if (m_eTRN_GroupLookMode != HAM_EAILookMode.FROM_GLOBAL)
            return m_eTRN_GroupLookMode;

        return HAM_AICombatStyle.GetLookMode(HAM_GetCombatStyle());
    }

    void HAM_PushGroupLookMode()
    {
        if (!m_Owner)
            return;

        int mode = HAM_GetGroupLookModeEffective();

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
            if (combat)
                combat.HAM_SetLookMode(mode);
        }
    }

    void HAM_SetGroupLookMode(int mode)
    {
        m_eTRN_GroupLookMode = mode;

        HAM_PushGroupLookMode();
    }

    void HAM_SetGroupStanceMode(int mode)
    {
        m_eTRN_GroupStanceMode = mode;

        if (!m_Owner)
            return;

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
            if (combat)
                combat.HAM_SetStanceMode(mode);
        }
    }

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Le groupe part fouiller quand il perd le contact. Decoche : il reste en place. Sans effet en CQB defensif : ce style tient ses positions et ne part jamais fouiller.", category: "IA")]
    protected bool m_bTRN_InvestigateEnabled;

    [Attribute(defvalue: "20", uiwidget: UIWidgets.Slider, desc: "Rayon MINIMAL de la fouille autour du dernier contact, en metres. 20 = valeur d'origine du jeu. La zone s'elargit d'elle-meme si les contacts sont disperses.", params: "20 500 5", category: "IA")]
    protected float m_fTRN_InvestigateRadiusMin;

    [Attribute(defvalue: "120", uiwidget: UIWidgets.Slider, desc: "Duree de la fouille sans information nouvelle, en secondes, tant qu'un ennemi est suppose vivant. 120 = valeur d'origine du jeu.", params: "30 600 10", category: "IA")]
    protected float m_fTRN_InvestigateDuration_s;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.CheckBox, desc: "Le groupe fouille-t-il l'interieur des batiments ? Les points de recherche sont alors recales sur le sol marchable, etages compris. Toujours actif en CQB offensif.", category: "IA")]
    protected bool m_bTRN_BuildingSearch;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "L'equipe designee en couverture tient sa position au lieu de progresser avec l'assaut. Decoche : comportement d'origine du jeu, tout le monde avance.", category: "IA")]
    protected bool m_bTRN_CombatRoles;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Le groupe contourne au lieu de foncer droit sur l'ennemi. Branche le mecanisme de flanquement du moteur, present mais jamais alimente par le jeu.", category: "IA")]
    protected bool m_bTRN_FlankingApproach;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Un soldat qui recharge a decouvert rejoint un abri au lieu de rester plante. Le jeu ne traite que le cas ou il occupe deja un couvert.", category: "IA")]
    protected bool m_bTRN_SeekCoverToReload;

    [Attribute(defvalue: "1", uiwidget: UIWidgets.CheckBox, desc: "Chef embarque = style Mecanise automatique ; le style choisi revient a la descente.", category: "IA")]
    protected bool m_bTRN_AutoMechanized;

    protected AIWaypoint m_HAM_HoldWaypoint;

    bool HAM_GetInvestigateEnabled()
    {
        if (HAM_GetCombatStyle() == HAM_EAICombatStyle.CQB_DEFENSIVE)
            return false;

        return m_bTRN_InvestigateEnabled;
    }

    float HAM_GetInvestigateRadiusMin()  { return m_fTRN_InvestigateRadiusMin; }
    float HAM_GetInvestigateDurationS()  { return m_fTRN_InvestigateDuration_s; }

    bool HAM_GetOwnInvestigateEnabled()   { return m_bTRN_InvestigateEnabled; }
    bool HAM_GetOwnBuildingSearch()       { return m_bTRN_BuildingSearch; }

    bool HAM_GetBuildingSearch()
    {
        if (HAM_GetCombatStyle() == HAM_EAICombatStyle.CQB_OFFENSIVE)
            return true;

        return m_bTRN_BuildingSearch;
    }

    void HAM_SetBuildingSearch(bool v)    { m_bTRN_BuildingSearch = v; }

    bool HAM_GetCombatRoles()          { return m_bTRN_CombatRoles; }
    void HAM_SetCombatRoles(bool v)    { m_bTRN_CombatRoles = v; }

    bool HAM_GetFlankingApproach()        { return m_bTRN_FlankingApproach; }
    void HAM_SetFlankingApproach(bool v)  { m_bTRN_FlankingApproach = v; }

    bool HAM_GetSeekCoverToReload()       { return m_bTRN_SeekCoverToReload; }
    void HAM_SetSeekCoverToReload(bool v) { m_bTRN_SeekCoverToReload = v; }

    bool HAM_GetAutoMechanized()          { return m_bTRN_AutoMechanized; }
    void HAM_SetAutoMechanized(bool v)    { m_bTRN_AutoMechanized = v; }

    protected bool HAM_IsLeaderInVehicle()
    {
        if (!m_Owner)
            return false;

        AIAgent leader = m_Owner.GetLeaderAgent();
        if (!leader)
            return false;

        ChimeraCharacter character = ChimeraCharacter.Cast(leader.GetControlledEntity());
        if (!character)
            return false;

        return character.IsInVehicle();
    }

    void HAM_SetInvestigateEnabled(bool v)       { m_bTRN_InvestigateEnabled = v; }
    void HAM_SetInvestigateRadiusMin(float v)  { if (v < 20) v = 20; m_fTRN_InvestigateRadiusMin = v; }
    void HAM_SetInvestigateDurationS(float v)  { if (v < 30) v = 30; m_fTRN_InvestigateDuration_s = v; }

    bool  HAM_GetHoldPosition() { return m_bTRN_HoldPosition; }
    float HAM_GetHoldRadius()   { return m_fTRN_HoldRadius; }

    void HAM_SetHoldPosition(bool active)
    {
        m_bTRN_HoldPosition = active;
        HAM_RefreshHoldWaypoint();
    }

    void HAM_SetHoldRadius(float radius)
    {
        if (radius < 1.0)
            radius = 1.0;

        m_fTRN_HoldRadius = radius;

        if (m_bTRN_HoldPosition)
            HAM_RefreshHoldWaypoint();
    }

    protected void HAM_RefreshHoldWaypoint()
    {
        HAM_ClearHoldWaypoint();

        if (!m_bTRN_HoldPosition || !m_Owner)
            return;

        Resource waypointPrefab = Resource.Load(HAM_HOLD_WAYPOINT);
        if (!waypointPrefab.IsValid())
            return;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform[3] = m_Owner.GetCenterOfMass();

        AIWaypoint waypoint = AIWaypoint.Cast(GetGame().SpawnEntityPrefabLocal(waypointPrefab, null, params));
        if (!waypoint)
            return;

        waypoint.SetCompletionRadius(m_fTRN_HoldRadius);

        HAM_MakeWaypointPermanent(waypoint);

        m_HAM_HoldWaypoint = waypoint;

        m_Owner.AddWaypointAt(waypoint, 0);
    }

    protected void HAM_MakeWaypointPermanent(notnull AIWaypoint waypoint)
    {
        SCR_TimedWaypoint timed = SCR_TimedWaypoint.Cast(waypoint);
        if (!timed)
            return;

        if (!timed.m_TimedWaypointParameters)
            timed.m_TimedWaypointParameters = new SCR_AITimedWaypointParameters();

        timed.SetHoldingTime(float.MAX);
    }

    protected void HAM_ClearHoldWaypoint()
    {
        if (!m_HAM_HoldWaypoint)
            return;

        if (m_Owner)
            m_Owner.RemoveWaypoint(m_HAM_HoldWaypoint);

        SCR_EntityHelper.DeleteEntityAndChildren(m_HAM_HoldWaypoint);
        m_HAM_HoldWaypoint = null;
    }

    void ~SCR_AIGroupUtilityComponent()
    {
        GetGame().GetCallqueue().Remove(HAM_ApplyPrefabSettings);
        GetGame().GetCallqueue().Remove(HAM_DeliverContactReport);

        if (m_Perception)
            m_Perception.GetOnEnemyDetectedFiltered().Remove(HAM_OnEnemyDetectedShare);

        if (m_Owner)
            m_Owner.GetOnCurrentWaypointChanged().Remove(HAM_OnZoneWaypointChanged);

        HAM_ClearHoldWaypoint();
    }

    protected void HAM_OnEnemyDetectedShare(SCR_AIGroup group, SCR_AITargetInfo target, AIAgent reporter)
    {
        if (HAM_AISettingsComponent.HAM_ContactShareRange() <= 0)
            return;

        if (!target || !target.m_Entity)
            return;

        GetGame().GetCallqueue().CallLater(HAM_DeliverContactReport, Math.RandomInt(1500, 4000), false,
            target.m_Entity, target.m_vWorldPos, target.m_Faction, target.m_fTimestamp);
    }

    void HAM_RelayExternalContact(IEntity enemy, vector pos, Faction enemyFaction)
    {
        PerceptionManager perceptionMan = GetGame().GetPerceptionManager();
        if (!perceptionMan)
            return;

        float timestamp = perceptionMan.GetTime();

        if (m_Perception)
            m_Perception.AddOrUpdateGunshot(enemy, pos, enemyFaction, timestamp, false);

        HAM_DeliverLimitedContactReport(enemy, pos, enemyFaction, timestamp, HAM_AISettingsComponent.HAM_ReconDronesAlertCount());
    }

    protected void HAM_DeliverLimitedContactReport(IEntity enemy, vector pos, Faction enemyFaction, float timestamp, int maxGroups)
    {
        if (!enemy || !m_Owner)
            return;

        float range = HAM_AISettingsComponent.HAM_ContactShareRange();
        Faction myFaction = m_Owner.GetFaction();
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (range <= 0 || !myFaction || !aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        array<SCR_AIGroup> nearest = {};
        array<float> distances = {};

        foreach (AIAgent agent : agents)
        {
            SCR_AIGroup group = SCR_AIGroup.Cast(agent);
            if (!group || group == m_Owner)
                continue;

            Faction otherFaction = group.GetFaction();
            if (!otherFaction || !myFaction.IsFactionFriendly(otherFaction))
                continue;

            float dist = vector.Distance(pos, group.GetCenterOfMass());
            if (dist > range)
                continue;

            int index = distances.Count();
            for (int i = 0; i < distances.Count(); i++)
            {
                if (dist < distances[i])
                {
                    index = i;
                    break;
                }
            }

            nearest.InsertAt(group, index);
            distances.InsertAt(dist, index);
        }

        int delivered = 0;
        foreach (SCR_AIGroup target : nearest)
        {
            if (delivered >= maxGroups)
                break;

            SCR_AIGroupUtilityComponent otherUtility = SCR_AIGroupUtilityComponent.Cast(target.FindComponent(SCR_AIGroupUtilityComponent));
            if (!otherUtility || !otherUtility.m_Perception)
                continue;

            otherUtility.m_Perception.AddOrUpdateGunshot(enemy, pos, enemyFaction, timestamp, false);
            delivered++;
        }
    }

    protected void HAM_DeliverContactReport(IEntity enemy, vector pos, Faction enemyFaction, float timestamp)
    {
        if (!enemy || !m_Owner)
            return;

        float range = HAM_AISettingsComponent.HAM_ContactShareRange();
        if (range <= 0)
            return;

        Faction myFaction = m_Owner.GetFaction();
        if (!myFaction)
            return;

        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            SCR_AIGroup group = SCR_AIGroup.Cast(agent);
            if (!group || group == m_Owner)
                continue;

            Faction otherFaction = group.GetFaction();
            if (!otherFaction || !myFaction.IsFactionFriendly(otherFaction))
                continue;

            if (vector.Distance(pos, group.GetCenterOfMass()) > range)
                continue;

            SCR_AIGroupUtilityComponent otherUtility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (!otherUtility || !otherUtility.m_Perception)
                continue;

            otherUtility.m_Perception.AddOrUpdateGunshot(enemy, pos, enemyFaction, timestamp, false);
        }
    }

    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);

        if (SCR_Global.IsEditMode())
            return;

        GetGame().GetCallqueue().CallLater(HAM_ApplyPrefabSettings, 2000, false);
    }

    protected void HAM_ApplyPrefabSettings()
    {
        if (m_Perception)
            m_Perception.GetOnEnemyDetectedFiltered().Insert(HAM_OnEnemyDetectedShare);

        HAM_ApplyFormationSetting();

        if (HAM_GetCombatStyle() != HAM_EAICombatStyle.DEFAULT)
            HAM_ApplyArticulation();

        HAM_ApplyEngagementDistance();

        if (m_bTRN_HoldPosition)
            HAM_RefreshHoldWaypoint();

        HAM_PropagateCombatDefaults();

        HAM_ApplyNavPreset();

        if (m_Owner)
            m_Owner.GetOnCurrentWaypointChanged().Insert(HAM_OnZoneWaypointChanged);
    }

    void HAM_ApplyNavPreset()
    {
        IEntity owner = GetOwner();
        if (!owner)
            return;

        AIPathfindingComponent pathfinding = AIPathfindingComponent.Cast(
            owner.FindComponent(AIPathfindingComponent));

        if (pathfinding)
            pathfinding.SetAreaCosts(HAM_GetFootNavConfig());
    }

    int HAM_GetFootNavChoice()    { return m_eTRN_FootNavChoice; }
    int HAM_GetVehicleNavChoice() { return m_eTRN_VehicleNavChoice; }

    ResourceName HAM_GetFootNavConfig()
    {
        int own = HAM_AICombatStyle.FootNavChoiceToPreset(m_eTRN_FootNavChoice);
        if (own >= 0)
            return HAM_AISettingsComponent.HAM_FootNavConfigFor(own);

        return HAM_AISettingsComponent.HAM_FootNavConfig();
    }

    ResourceName HAM_GetVehicleNavConfig()
    {
        int own = HAM_AICombatStyle.VehicleNavChoiceToPreset(m_eTRN_VehicleNavChoice);
        if (own >= 0)
            return HAM_AISettingsComponent.HAM_VehicleNavConfigFor(own);

        return HAM_AISettingsComponent.HAM_VehicleNavConfig();
    }

    float HAM_GetVehicleCruiseSpeed()
    {
        if (m_fTRN_VehicleCruiseSpeed != 0)
            return m_fTRN_VehicleCruiseSpeed;

        return HAM_AISettingsComponent.HAM_VehicleCruiseSpeed();
    }

    float HAM_GetOwnVehicleCruiseSpeed() { return m_fTRN_VehicleCruiseSpeed; }

    void HAM_SetVehicleCruiseSpeed(float v)
    {
        m_fTRN_VehicleCruiseSpeed = v;
        HAM_ApplyNavToMembers();
    }

    void HAM_SetFootNavChoice(int choice)
    {
        m_eTRN_FootNavChoice = choice;
        HAM_ApplyNavToMembers();
    }

    void HAM_SetVehicleNavChoice(int choice)
    {
        m_eTRN_VehicleNavChoice = choice;
        HAM_ApplyNavToMembers();
    }

    void HAM_ApplyNavToMembers()
    {
        HAM_ApplyNavPreset();

        if (!m_Owner)
            return;

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(
                controlled.FindComponent(SCR_AICombatComponent));

            if (combat)
                combat.HAM_ApplyNavPreset();
        }
    }

    static void HAM_ApplyNavToAllGroups()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            AIGroup group = AIGroup.Cast(agent);
            if (!group)
                continue;

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (utility)
                utility.HAM_ApplyNavPreset();
        }
    }

    void HAM_PropagateCombatDefaults()
    {
        if (!m_Owner)
            return;

        array<AIAgent> agents = {};
        m_Owner.GetAgents(agents);

        foreach (AIAgent agent : agents)
        {
            if (!agent)
                continue;

            IEntity controlled = agent.GetControlledEntity();
            if (!controlled)
                continue;

            SCR_AICombatComponent combat = SCR_AICombatComponent.Cast(controlled.FindComponent(SCR_AICombatComponent));
            if (!combat)
                continue;

            combat.HAM_AdoptGroupDefaults(
                m_fTRN_GroupPerceptionMul,
                m_fTRN_GroupAimErrorMul,
                m_fTRN_GroupReactionMul,
                m_fTRN_GroupEngageDelayMul,
                m_eTRN_GroupSkill,
                m_eTRN_GroupStanceMode,
                HAM_GetGroupLookModeEffective(),
                m_fTRN_GroupContactMemory);

            combat.HAM_ApplyConfiguredWeaponMode();

            combat.HAM_ApplyNavPreset();
        }
    }

    HAM_EAICombatStyle HAM_GetCombatStyle()
    {
        if (m_bTRN_AutoMechanized && HAM_IsLeaderInVehicle())
            return HAM_EAICombatStyle.MECHANIZED;

        SCR_DefendWaypoint zone = HAM_GetCurrentDefendZone();
        if (zone)
        {
            int zoneStyle = HAM_AICombatStyle.ZoneStyleChoiceToStyle(zone.HAM_GetZoneStyleChoice());
            if (zoneStyle >= 0)
                return zoneStyle;
        }

        if (m_eTRN_CombatStyle != HAM_EAICombatStyle.DEFAULT)
            return m_eTRN_CombatStyle;

        return HAM_AISettingsComponent.HAM_CombatStyle();
    }

    SCR_DefendWaypoint HAM_GetCurrentDefendZone()
    {
        if (!m_Owner)
            return null;

        return SCR_DefendWaypoint.Cast(m_Owner.GetCurrentWaypoint());
    }

    int HAM_GetZoneFireMode()
    {
        SCR_DefendWaypoint zone = HAM_GetCurrentDefendZone();
        if (!zone)
            return HAM_EAIZoneFireMode.FOLLOW;

        return zone.HAM_GetZoneFireMode();
    }

    protected void HAM_OnZoneWaypointChanged(AIWaypoint current, AIWaypoint prev)
    {
        if (!HAM_ZoneImposesStyle(current) && !HAM_ZoneImposesStyle(prev))
            return;

        HAM_ApplyFormationSetting();
        HAM_ApplyArticulation();
    }

    protected static bool HAM_ZoneImposesStyle(AIWaypoint waypoint)
    {
        SCR_DefendWaypoint zone = SCR_DefendWaypoint.Cast(waypoint);
        if (!zone)
            return false;

        return HAM_AICombatStyle.ZoneStyleChoiceToStyle(zone.HAM_GetZoneStyleChoice()) >= 0;
    }

    static void HAM_ReapplyStyleForZone(AIWaypoint zoneWaypoint)
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            AIGroup group = AIGroup.Cast(agent);
            if (!group || group.GetCurrentWaypoint() != zoneWaypoint)
                continue;

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (!utility)
                continue;

            utility.HAM_ApplyFormationSetting();
            utility.HAM_ApplyArticulation();
        }
    }

    int HAM_GetOwnCombatStyle()
    {
        return m_eTRN_CombatStyle;
    }

    SCR_EAIGroupFormation HAM_GetTeamFormation()
    {
        SCR_EAIGroupFormation moveFormation;
        if (HAM_AICombatStyle.MoveFormationToVanilla(HAM_GetMoveFormation(), moveFormation))
            return moveFormation;

        return HAM_AICombatStyle.ResolveFormation(m_eTRN_FormationChoice, HAM_GetCombatStyle());
    }

    int HAM_GetOwnMoveFormation() { return m_eTRN_MoveFormation; }

    int HAM_GetMoveFormation()
    {
        if (m_eTRN_MoveFormation != HAM_EAIMoveFormation.FROM_GENERAL)
            return m_eTRN_MoveFormation;

        return HAM_AISettingsComponent.HAM_MoveFormation();
    }

    void HAM_SetMoveFormation(int formation)
    {
        if (formation < HAM_EAIMoveFormation.FROM_GENERAL || formation > HAM_EAIMoveFormation.STAGGERED_COLUMN)
            formation = HAM_EAIMoveFormation.FROM_GENERAL;

        m_eTRN_MoveFormation = formation;
        HAM_ApplyFormationSetting();
    }

    static void HAM_ApplyMoveFormationToAllGroups()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            AIGroup group = AIGroup.Cast(agent);
            if (!group)
                continue;

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (utility)
                utility.HAM_ApplyFormationSetting();
        }
    }

    float HAM_GetFormationScale()
    {
        if (m_fTRN_FormationScale > 0)
            return m_fTRN_FormationScale;

        return HAM_AICombatStyle.GetMoveFormationScale(HAM_GetCombatStyle());
    }

    float HAM_GetApproachFloor()
    {
        if (m_fTRN_ApproachFloor != 0)
            return m_fTRN_ApproachFloor;

        return HAM_AICombatStyle.GetApproachFloor(HAM_GetCombatStyle());
    }

    float HAM_GetHoldTimeS()
    {
        if (m_fTRN_HoldTime_s > 0)
            return m_fTRN_HoldTime_s;

        return HAM_AICombatStyle.GetHoldTimeS(HAM_GetCombatStyle());
    }

    bool HAM_GetRequiresCover()
    {
        return HAM_AICombatStyle.ResolveRequiresCover(m_eTRN_CoverRequirement, HAM_GetCombatStyle());
    }

    bool HAM_OverridesProgression()
    {
        if (HAM_AICombatStyle.OverridesProgression(HAM_GetCombatStyle()))
            return true;

        return m_fTRN_ApproachFloor != 0
            || m_fTRN_HoldTime_s > 0
            || m_eTRN_CoverRequirement != HAM_EAICoverRequirement.FROM_STYLE;
    }

    int   HAM_GetFormationChoice()   { return m_eTRN_FormationChoice; }
    float HAM_GetOwnFormationScale() { return m_fTRN_FormationScale; }
    float HAM_GetOwnApproachFloor()  { return m_fTRN_ApproachFloor; }
    float HAM_GetOwnHoldTimeS()      { return m_fTRN_HoldTime_s; }
    int   HAM_GetCoverRequirement()  { return m_eTRN_CoverRequirement; }

    void HAM_SetFormationChoice(int choice)
    {
        m_eTRN_FormationChoice = choice;
        HAM_ApplyFormationSetting();
    }

    void HAM_SetFormationScale(float v)  { if (v < 0) v = 0; m_fTRN_FormationScale = v; }
    void HAM_SetApproachFloor(float v)   { if (v < 0) v = -1; m_fTRN_ApproachFloor = v; }
    void HAM_SetHoldTimeS(float v)       { if (v < 0) v = 0; m_fTRN_HoldTime_s = v; }
    void HAM_SetCoverRequirement(int v)  { m_eTRN_CoverRequirement = v; }

    static void HAM_ApplyStyleToAllGroups()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            AIGroup group = AIGroup.Cast(agent);
            if (!group)
                continue;

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (!utility)
                continue;

            if (utility.HAM_GetOwnCombatStyle() != HAM_EAICombatStyle.DEFAULT)
                continue;

            utility.HAM_ApplyFormationSetting();
            utility.HAM_ApplyArticulation();
        }
    }

    int HAM_GetCoverSeekOverride()
    {
        return m_eTRN_CoverSeekOverride;
    }

    void HAM_SetCoverSeekOverride(int v)
    {
        m_eTRN_CoverSeekOverride = v;
    }

    bool HAM_FiresBeforeSeekingCover()
    {
        int coverMode = m_eTRN_CoverSeekOverride;
        if (coverMode == HAM_EAICoverSeekOverride.FROM_STYLE)
            coverMode = HAM_AISettingsComponent.HAM_CoverSeekOverride();

        if (coverMode == HAM_EAICoverSeekOverride.FORCE_FIRE_FIRST)
            return true;

        if (coverMode == HAM_EAICoverSeekOverride.FORCE_SEEK_COVER)
            return false;

        return HAM_AICombatStyle.FiresBeforeSeekingCover(HAM_GetCombatStyle());
    }

    void HAM_SetCombatStyle(HAM_EAICombatStyle style)
    {
        m_eTRN_CombatStyle = style;

        HAM_ApplyFormationSetting();
        HAM_ApplyArticulation();
        HAM_PushGroupLookMode();
    }

    float HAM_GetMaxEngagementDistance()
    {
        if (m_fTRN_MaxEngagementDistance != 0)
            return m_fTRN_MaxEngagementDistance;

        return HAM_AISettingsComponent.HAM_MaxEngagementDistance();
    }

    float HAM_GetOwnMaxEngagementDistance()
    {
        return m_fTRN_MaxEngagementDistance;
    }

    float HAM_GetFireAtWillDistance()
    {
        if (m_fTRN_FireAtWillDistance != 0)
            return m_fTRN_FireAtWillDistance;

        return HAM_AISettingsComponent.HAM_FireAtWillDistance();
    }

    float HAM_GetReturnFireDistance()
    {
        if (m_fTRN_ReturnFireDistance != 0)
            return m_fTRN_ReturnFireDistance;

        return HAM_AISettingsComponent.HAM_ReturnFireDistance();
    }

    float HAM_GetOwnFireAtWillDistance() { return m_fTRN_FireAtWillDistance; }
    float HAM_GetOwnReturnFireDistance() { return m_fTRN_ReturnFireDistance; }

    void HAM_SetFireAtWillDistance(float v) { if (v < 0) v = -1; m_fTRN_FireAtWillDistance = v; }
    void HAM_SetReturnFireDistance(float v) { if (v < 0) v = -1; m_fTRN_ReturnFireDistance = v; }

    void HAM_SetMaxEngagementDistance(float distance)
    {
        if (distance < 0)
            distance = -1;

        m_fTRN_MaxEngagementDistance = distance;
        HAM_ApplyEngagementDistance();
    }

    int HAM_GetMagazineConsumption() { return m_eTRN_MagazineConsumption; }

    void HAM_SetMagazineConsumption(int mode) { m_eTRN_MagazineConsumption = mode; }

    bool HAM_GetSuppressiveFire() { return m_bTRN_SuppressiveFire; }

    void HAM_SetSuppressiveFire(bool enabled) { m_bTRN_SuppressiveFire = enabled; }

    protected override bool ShouldSuppressCluster(SCR_AITargetClusterState clusterState)
    {
        if (!m_bTRN_SuppressiveFire)
            return false;

        if (clusterState)
        {
            float fireDist = HAM_GetFireAtWillDistance();
            float riposteDist = HAM_GetReturnFireDistance();

            if (fireDist > 0 && clusterState.m_fDistMin > fireDist)
            {
                if (clusterState.m_iCountEndangering == 0)
                    return false;

                if (riposteDist > 0 && clusterState.m_fDistMin > Math.Max(fireDist, riposteDist))
                    return false;
            }
        }

        return super.ShouldSuppressCluster(clusterState);
    }

    void HAM_ApplyEngagementDistance()
    {
        float distance = HAM_GetMaxEngagementDistance();

        if (distance <= 0)
            SetMaxAutonomousDistance(AUTONOMOUS_DISTANCE_MAX_DEFAULT);
        else
            SetMaxAutonomousDistance(distance);
    }

    static void HAM_ApplyEngagementDistanceToAllGroups()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (!aiWorld)
            return;

        array<AIAgent> agents = {};
        aiWorld.GetAIAgents(agents);

        foreach (AIAgent agent : agents)
        {
            AIGroup group = AIGroup.Cast(agent);
            if (!group)
                continue;

            SCR_AIGroupUtilityComponent utility = SCR_AIGroupUtilityComponent.Cast(
                group.FindComponent(SCR_AIGroupUtilityComponent));

            if (utility)
                utility.HAM_ApplyEngagementDistance();
        }
    }

    void HAM_ApplyFormationSetting()
    {
        if (!m_SettingsComponent)
            return;

        if (m_eTRN_FormationChoice == HAM_EAIFormationChoice.FROM_STYLE
            && HAM_GetCombatStyle() == HAM_EAICombatStyle.DEFAULT
            && HAM_GetMoveFormation() == HAM_EAIMoveFormation.FROM_GENERAL)
        {
            m_SettingsComponent.RemoveSettingsOfTypeAndOrigin(
                SCR_AIGroupFormationSettingBase, SCR_EAISettingOrigin.EDITOR);
            return;
        }

        HAM_AIGroupFormationSetting setting = HAM_AIGroupFormationSetting.Create(
            SCR_EAISettingOrigin.EDITOR, HAM_GetTeamFormation());

        m_SettingsComponent.AddSetting(setting, false, true);
    }

    void HAM_ApplyArticulation()
    {
        if (!m_Owner || !m_GroupMovementComponent || !m_FireteamMgr)
            return;

        HAM_EAICombatStyle style = HAM_GetCombatStyle();
        if (style == HAM_EAICombatStyle.DEFAULT || style == HAM_EAICombatStyle.MECHANIZED)
        {
            HAM_ReleaseTeamHandlers();
            return;
        }

        if (CanRebalanceFireteams())
            m_FireteamMgr.HAM_EnforceTeamSizes();

        array<SCR_AIGroupFireteam> fireteams = {};
        m_FireteamMgr.HAM_GetFootFireteams(fireteams);

        if (fireteams.Count() <= 1)
        {
            HAM_ReleaseTeamHandlers();
            return;
        }

        HAM_MoveLeaderTeamLast(fireteams);

        int wanted = fireteams.Count() - 1;
        if (wanted > HAM_MAX_TEAMS)
            wanted = HAM_MAX_TEAMS;

        HAM_ResizeTeamHandlers(wanted);

        array<AIAgent> members = {};

        foreach (int team, SCR_AIGroupFireteam fireteam : fireteams)
        {
            int handlerId = AIGroupMovementComponent.DEFAULT_HANDLER_ID;

            if (team > 0)
            {
                if (team - 1 < m_aTRN_TeamHandlers.Count())
                    handlerId = m_aTRN_TeamHandlers[team - 1];
            }

            fireteam.GetMembers(members);

            foreach (int i, AIAgent agent : members)
            {
                if (!agent)
                    continue;

                ChimeraCharacter character = ChimeraCharacter.Cast(agent.GetControlledEntity());
                if (character && character.IsInVehicle())
                    continue;

                int currentHandler = m_GroupMovementComponent.GetAgentMoveHandlerId(agent);
                if (currentHandler == handlerId)
                    continue;

                if (i == 0 && team > 0)
                    m_GroupMovementComponent.SetMoveHandlerLeader(agent, currentHandler, handlerId);
                else
                    m_GroupMovementComponent.MoveAgentToHandler(agent, currentHandler, handlerId);
            }
        }
    }

    protected void HAM_MoveLeaderTeamLast(notnull array<SCR_AIGroupFireteam> fireteams)
    {
        AIAgent leader = m_Owner.GetLeaderAgent();
        if (!leader)
            return;

        int last = fireteams.Count() - 1;

        foreach (int i, SCR_AIGroupFireteam fireteam : fireteams)
        {
            if (!fireteam.HasMember(leader))
                continue;

            if (i < last)
            {
                fireteams.Remove(i);
                fireteams.Insert(fireteam);
            }

            return;
        }
    }

    protected void HAM_ResizeTeamHandlers(int wanted)
    {
        while (m_aTRN_TeamHandlers.Count() > wanted)
        {
            int last = m_aTRN_TeamHandlers.Count() - 1;
            HAM_ReleaseHandler(m_aTRN_TeamHandlers[last]);
            m_aTRN_TeamHandlers.Remove(last);
        }

        if (m_aTRN_TeamHandlers.Count() >= wanted)
            return;

        string formationName = typename.EnumToString(SCR_EAIGroupFormation, HAM_GetTeamFormation());

        while (m_aTRN_TeamHandlers.Count() < wanted)
        {
            int handlerId = m_GroupMovementComponent.CreateGroupMoveHandler(formationName);

            if (handlerId <= 0)
                return;

            m_aTRN_TeamHandlers.Insert(handlerId);
        }
    }

    protected void HAM_ReleaseHandler(int handlerId)
    {
        if (handlerId == AIGroupMovementComponent.DEFAULT_HANDLER_ID)
            return;

        array<AIAgent> agents = {};
        m_GroupMovementComponent.GetAgentsInHandler(agents, handlerId);

        foreach (AIAgent agent : agents)
        {
            if (agent)
                m_GroupMovementComponent.MoveAgentToHandler(agent, handlerId,
                    AIGroupMovementComponent.DEFAULT_HANDLER_ID);
        }

        if (m_GroupMovementComponent)
            m_GroupMovementComponent.RemoveGroupMoveHandler(handlerId);
    }

    protected void HAM_ReleaseTeamHandlers()
    {
        if (!m_aTRN_TeamHandlers)
            return;

        for (int i = m_aTRN_TeamHandlers.Count() - 1; i >= 0; i--)
            HAM_ReleaseHandler(m_aTRN_TeamHandlers[i]);

        m_aTRN_TeamHandlers.Clear();
    }
}