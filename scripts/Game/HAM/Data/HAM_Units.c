//------------------------------------------------------------------------------------------------
// Pools d'unités, comme btc_type_units dans Hearts and Minds.
// On écrit ici chaque prefab qui peut apparaître. Rien n'est deviné dans les mods.
// Une liste spéciale vide retombe sur la liste générale du même camp ; si elle est vide aussi, rien ne naît.
//------------------------------------------------------------------------------------------------

class HAM_Units
{
	// Clés de faction : camp Conflit et catalogue d'arsenal.
	static const string PLAYER_FACTION = "USSR";
	static const string ENEMY_FACTION = "US";
	static const string CIVILIAN_FACTION = "CIV";

	static const ResourceName GROUP_LIGHT = "{860C1B99A79AE5ED}Prefabs/Groups/BLUFOR/Group_US_LightFireTeam.et";
	static const ResourceName GROUP_MEDICAL = "{EF62027CC75A745A}Prefabs/Groups/BLUFOR/Group_US_MedicalSection.et";
	static const ResourceName GROUP_GL = "{DB29E339E9AB2494}Prefabs/Groups/BLUFOR/Group_US_Team_GL.et";
	static const ResourceName GROUP_RIFLE = "{9BC099F87CA1421D}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et";
	static const ResourceName GROUP_LAT = "{ACB1FCF863E68722}Prefabs/Groups/BLUFOR/Group_US_Team_LAT.et";
	static const ResourceName GROUP_TRANSPORT = "{93645DF4BFAA34BB}Prefabs/Groups/BLUFOR/Group_US_Transport.et";
	static const ResourceName GROUP_PLAYER_CONVOY = "{30ED11AA4F0D41E6}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et";

	static const ResourceName VEHICLE_M1025 = "{3EA6F47D95867115}Prefabs/Vehicles/Wheeled/M998/M1025_armed_M2HB.et";
	static const ResourceName VEHICLE_LAV = "{0FBF8F010F81A4E6}Prefabs/Vehicles/Wheeled/LAV25/LAV25.et";
	static const ResourceName VEHICLE_UH1H = "{21E9A875C0A3C40A}Prefabs/Vehicles/Helicopters/UH1H/UH1H_armed_gunship_HE.et";

	static const ResourceName CQB_MG = "{ADB0F1274E9A0670}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_MG.et";
	static const ResourceName CQB_RIFLEMAN = "{F44F87222B67E26A}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Rifleman.et";
	static const ResourceName CQB_SNIPER = "{C0EE7CD6A89B123B}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Sniper.et";

	static ref array<ResourceName> s_aAreaGroups;
	static ref array<ResourceName> s_aSupplyGroups;
	static ref array<ResourceName> s_aCqbMen;

	// Ennemi : listes générales
	static ref array<ResourceName> s_aEnemyInfantry;
	static ref array<ResourceName> s_aEnemyMotorized;
	static ref array<ResourceName> s_aEnemyArmed;
	static ref array<ResourceName> s_aEnemyCrew;

	// Ennemi : listes spéciales
	static ref array<ResourceName> s_aEnemyGarrison;
	static ref array<ResourceName> s_aEnemyPatrol;
	static ref array<ResourceName> s_aEnemyPatrolVehicles;
	static ref array<ResourceName> s_aEnemyCqb;

	// Ennemi : véhicules autour des zones, tirés selon les pourcentages du bloc vehicles du JSON
	static ref array<ResourceName> s_aEnemyZoneArmor;
	static ref array<ResourceName> s_aEnemyZoneBtr;
	static ref array<ResourceName> s_aEnemyZoneTruck;
	static ref array<ResourceName> s_aEnemyZoneCar;
	static ref array<ResourceName> s_aEnemyCounterInfantry;
	static ref array<ResourceName> s_aEnemyCounterVehicles;
	static ref array<ResourceName> s_aEnemyMission;
	static ref array<ResourceName> s_aEnemyOfficer;

	// Joueur
	static ref array<ResourceName> s_aPlayerInfantry;
	static ref array<ResourceName> s_aPlayerMotorized;
	static ref array<ResourceName> s_aPlayerArmed;
	static ref array<ResourceName> s_aPlayerCrew;
	static ref array<ResourceName> s_aPlayerGarrison;
	static ref array<ResourceName> s_aPlayerConvoyVehicles;
	static ref array<ResourceName> s_aBarracksUnits;

	// Morceaux de chemin des loadouts proposés au déploiement. Vide = tous ceux de la faction.
	static ref array<string> s_aPlayerSpawnSlots;

	// Civils
	static ref array<ResourceName> s_aCivilians;

	// Engins
	static const ResourceName MINE_AT = "{49FFE8F373F55960}Prefabs/Weapons/Explosives/Mine_M15AT/Mine_M15AT.et";
	static const ResourceName IED_ZONE = "{0B342F48EDE81A44}Prefabs/Ied_Zone.et";
	static const ResourceName MINE_AP = "{E4C9F0A4090CFE4D}Prefabs/Weapons/Explosives/Mine_M14/Mine_M14.et";
	static const ResourceName MINE_FLAG = "{EC13D4E934B2ADD8}Prefabs/Items/Demining/MineFlags/MineFlag_US.et";
	static const ResourceName IED_COVER = "{C915675AA727A922}Prefabs/HAM/Props/IED/HAM_IED_TirePile_01_medium.et";
	// Accessoires d'IED du Conflit 13RDPA, posés à tour de rôle ; la mine M15 est à côté.
	static ref array<ResourceName> s_aIedProps;

	// Objets des missions secondaires
	static const ResourceName MISSION_TOWER = "{6A004A8F0571D456}Prefabs/Structures/Infrastructure/Towers/TransmitterTower_01/TransmitterTower_01_small.et";
	static const ResourceName MISSION_CACHE = "{29F2FAA1F49B7E20}Prefabs/HAM/Missions/HAM_Cache.et";
	static const ResourceName MISSION_CHECKPOINT = "{A870787967D4D18D}PrefabsEditable/Auto/Compositions/Slotted/SlotRoadSmall/E_Checkpoint_S_US_01.et";
	static const ResourceName MISSION_HOSTAGE = "{C1A10001A11C0005}Prefabs/Characters/Factions/CIV/HAM/HAM_CIV_Hostage.et";
	static const ResourceName MISSION_HOSTAGE_ACTION = "{A651D9004CC1D3EB}Prefabs/HAM/Missions/HAM_HostageAction.et";

	// Waypoints
	static const ResourceName WP_DEFEND = "{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et";
	//! Groupe vide du jeu de base : chaque homme de CQB y est mis seul, comme dans le Conflit.
	static const ResourceName CQB_SOLO_GROUP = "{000CD338713F2B5A}Prefabs/AI/Groups/Group_Base.et";
	static const ResourceName WP_DEFEND_SMALL = "{2FCBE5C76E285A7B}Prefabs/AI/Waypoints/AIWaypoint_DefendSmall.et";
	static const ResourceName WP_PATROL = "{22A875E30470BD4F}Prefabs/AI/Waypoints/AIWaypoint_Patrol.et";
	static const ResourceName WP_MOVE = "{750A8D1695BD6998}Prefabs/AI/Waypoints/AIWaypoint_Move.et";
	static const ResourceName WP_SEARCH_DESTROY = "{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et";

	// Prefabs de zone H&M
	static const ResourceName ZONE_SLAVE = "{28B8CDE40D97C895}Prefabs/HAM/Zones/HAM_Slave.et";
	static const ResourceName ZONE_DEPOSIT = "{7178DA7410FD44E1}Prefabs/HAM/Zones/HAM_Deposit.et";
	static const ResourceName SUPPLY_CACHE_US = "{253C2F12F79B289D}Prefabs/HAM/Zones/HAM_SupplyCache.et";
	static const ResourceName INTEL_ITEM = "{BF2618D0E196028F}Prefabs/HAM/Items/HAM_Intel.et";
	static const ResourceName BODY_SEARCH_ACTION = "{5A78F81E0773ED93}Prefabs/HAM/Items/HAM_BodySearchAction.et";
	static const ResourceName CIVILIAN_TALK_ACTION = "{69F1B89B0BC59001}Prefabs/HAM/Items/HAM_CivilianTalkAction.et";
	static const ResourceName CIVILIAN_ASK_ACTION = "{69F1B89B0BC59011}Prefabs/HAM/Items/HAM_CivilianAskAction.et";
	static const ResourceName CIVILIAN_REPORT_ACTION = "{69F1B89B0BC59021}Prefabs/HAM/Items/HAM_CivilianReportAction.et";

	// QG déplacé : point d'apparition, construction, stock de fournitures
	static const ResourceName HQ_SPAWNPOINT = "{E7F4D5562F48DDE4}Prefabs/MP/Spawning/SpawnPoint_Base.et";
	static const ResourceName HQ_BUILDING = "{473DB1C66CAD5088}Prefabs/Props/Military/Compositions/USSR/BuildingService_USSR.et";
	static const ResourceName HQ_SUPPLY_STORAGE = "{98A34AE10104A932}Prefabs/Props/Military/SupplyBox/SupplyStack/SupplyStack_01/SupplyStack_01_V1_storage.et";
	static const ResourceName ARSENAL_BOARD = "{635F701501D22321}Prefabs/HAM/Props/HAM_ArsenalBoard.et";

	protected static bool s_bInit;

	//------------------------------------------------------------------------------------------------
	static void Init()
	{
		if (s_bInit)
			return;

		s_bInit = true;

		s_aEnemyInfantry = {};
		s_aEnemyInfantry.Insert("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et");
		s_aEnemyInfantry.Insert("{FCF7F5DC4F83955C}Prefabs/Groups/BLUFOR/Group_US_LightFireTeam.et");
		s_aEnemyInfantry.Insert("{DDF3799FA1387848}Prefabs/Groups/BLUFOR/Group_US_RifleSquad.et");
		s_aEnemyInfantry.Insert("{958039B857396B7B}Prefabs/Groups/BLUFOR/Group_US_MachineGunTeam.et");
		s_aEnemyInfantry.Insert("{FAEA8B9E1252F56E}Prefabs/Groups/BLUFOR/Group_US_Team_LAT.et");
		s_aEnemyInfantry.Insert("{DE747BC9217D383C}Prefabs/Groups/BLUFOR/Group_US_Team_GL.et");

		s_aEnemyMotorized = {};
		s_aEnemyMotorized.Insert("{F1FBD0972FA5FE09}Prefabs/Vehicles/Wheeled/M923A1/M923A1_transport.et");
		s_aEnemyMotorized.Insert("{B55C6990A6A9411B}Prefabs/Vehicles/Wheeled/M998/M998_covered.et");
		s_aEnemyMotorized.Insert("{4A71F755A4513227}Prefabs/Vehicles/Wheeled/M998/M1025.et");

		s_aEnemyArmed = {};
		s_aEnemyArmed.Insert(VEHICLE_M1025);
		s_aEnemyArmed.Insert(VEHICLE_LAV);

		s_aEnemyCrew = {};
		s_aEnemyCrew.Insert("{E1CB513B8B9B08F4}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Crew.et");

		s_aEnemyGarrison = {};
		s_aEnemyPatrol = {};
		s_aEnemyPatrol.Insert("{84E5BBAB25EA23E5}Prefabs/Groups/BLUFOR/Group_US_FireTeam.et");
		s_aEnemyPatrol.Insert("{3BF36BDEEB33AEC9}Prefabs/Groups/BLUFOR/Group_US_SentryTeam.et");
		s_aEnemyPatrolVehicles = {};
		s_aEnemyPatrolVehicles.Insert(VEHICLE_M1025);
		s_aEnemyPatrolVehicles.Insert(VEHICLE_LAV);
		s_aEnemyCqb = {};
		s_aEnemyCqb.Insert("{FCF7F5DC4F83955C}Prefabs/Groups/BLUFOR/Group_US_LightFireTeam.et");

		s_aIedProps = {};
		s_aIedProps.Insert("{C1FC248CC83CAA83}Prefabs/HAM/Props/IED/HAM_IED_BoatTrailer_01.et");
		s_aIedProps.Insert("{7643391820571D16}Prefabs/HAM/Props/IED/HAM_IED_CarrierFood_01.et");
		s_aIedProps.Insert("{2C9A91ACD09ACF81}Prefabs/HAM/Props/IED/HAM_IED_BrickPileOld_01_Medium.et");
		s_aIedProps.Insert("{81B4C9BD3376160A}Prefabs/HAM/Props/IED/HAM_IED_PlankPile_01_Medium.et");
		s_aIedProps.Insert("{9EE1CD5BD497E788}Prefabs/HAM/Props/IED/HAM_IED_BMP1_wreck.et");
		s_aIedProps.Insert("{B508DD6073D1283B}Prefabs/HAM/Props/IED/HAM_IED_BRDM2_wreck.et");

		s_aEnemyZoneArmor = {};
		s_aEnemyZoneArmor.Insert(VEHICLE_LAV);
		s_aEnemyZoneBtr = {};
		s_aEnemyZoneTruck = {};
		s_aEnemyZoneCar = {};
		s_aEnemyZoneCar.Insert(VEHICLE_M1025);
		s_aEnemyCounterInfantry = {};
		s_aEnemyCounterInfantry.Insert(GROUP_LIGHT);
		s_aEnemyCounterVehicles = {};
		s_aEnemyCounterVehicles.Insert(VEHICLE_M1025);
		s_aEnemyCounterVehicles.Insert(VEHICLE_LAV);

		s_aAreaGroups = {};
		s_aAreaGroups.Insert(GROUP_GL);
		s_aAreaGroups.Insert(GROUP_LAT);
		s_aAreaGroups.Insert(GROUP_MEDICAL);
		s_aAreaGroups.Insert(GROUP_LIGHT);

		s_aSupplyGroups = {};
		s_aSupplyGroups.Insert(GROUP_RIFLE);
		s_aSupplyGroups.Insert(GROUP_LAT);
		s_aSupplyGroups.Insert(GROUP_GL);
		s_aSupplyGroups.Insert(GROUP_MEDICAL);

		s_aCqbMen = {};
		s_aCqbMen.Insert(CQB_MG);
		s_aCqbMen.Insert(CQB_RIFLEMAN);
		s_aCqbMen.Insert(CQB_SNIPER);
		s_aEnemyMission = {};
		s_aEnemyOfficer = {};
		s_aEnemyOfficer.Insert("{DE15FB5FAFC3E63F}Prefabs/Characters/Factions/BLUFOR/US_Army/Character_US_Officer.et");

		s_aPlayerInfantry = {};
		s_aPlayerInfantry.Insert("{30ED11AA4F0D41E5}Prefabs/Groups/OPFOR/Group_USSR_FireGroup.et");
		s_aPlayerInfantry.Insert("{657590C1EC9E27D3}Prefabs/Groups/OPFOR/Group_USSR_LightFireTeam.et");
		s_aPlayerInfantry.Insert("{E552DABF3636C2AD}Prefabs/Groups/OPFOR/Group_USSR_RifleSquad.et");
		s_aPlayerInfantry.Insert("{A2F75E45C66B1C0A}Prefabs/Groups/OPFOR/Group_USSR_MachineGunTeam.et");

		s_aPlayerMotorized = {};
		s_aPlayerMotorized.Insert("{D9B91FAB817A6033}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport_covered.et");
		s_aPlayerMotorized.Insert("{259EE7B78C51B624}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469.et");

		s_aPlayerArmed = {};
		s_aPlayerArmed.Insert("{C012BB3488BEA0C2}Prefabs/Vehicles/Wheeled/BTR70/BTR70.et");
		s_aPlayerArmed.Insert("{0B4DEA8078B78A9B}Prefabs/Vehicles/Wheeled/UAZ469/UAZ469_PKM.et");
		s_aPlayerArmed.Insert("{254289B9C09904AB}Prefabs/Vehicles/Wheeled/BRDM2/BRDM2.et");

		s_aPlayerCrew = {};
		s_aPlayerCrew.Insert("{9FFEF10757E742EB}Prefabs/Characters/Factions/OPFOR/USSR_Army/Character_USSR_Crew.et");

		s_aPlayerGarrison = {};
		s_aPlayerConvoyVehicles = {};
		s_aPlayerConvoyVehicles.Insert("{D9B91FAB817A6033}Prefabs/Vehicles/Wheeled/Ural4320/Ural4320_transport_covered.et");

		s_aBarracksUnits = {};

		// Pour l'instant une seule tenue vanilla : le soldat. Les groupes du monde ne proposent qu'elle aussi.
		s_aPlayerSpawnSlots = {};
		s_aPlayerSpawnSlots.Insert("Campaign_USSR_Player_Rifleman");

		s_aCivilians = {};
		s_aCivilians.Insert("{C1A10001A11C0001}Prefabs/Characters/Factions/CIV/HAM/HAM_CIV_Randomized.et");
		s_aCivilians.Insert("{C1A10001A11C0002}Prefabs/Characters/Factions/CIV/HAM/HAM_CIV_Businessman.et");
		s_aCivilians.Insert("{C1A10001A11C0003}Prefabs/Characters/Factions/CIV/HAM/HAM_CIV_Construction.et");
		s_aCivilians.Insert("{C1A10001A11C0004}Prefabs/Characters/Factions/CIV/HAM/HAM_CIV_Dockworker.et");
	}

	//------------------------------------------------------------------------------------------------
	//! Liste spéciale si elle contient quelque chose, sinon liste générale. Chaîne vide si les deux sont vides.
	static ResourceName Pick(array<ResourceName> special, array<ResourceName> general)
	{
		if (special && !special.IsEmpty())
			return special.GetRandomElement();

		if (general && !general.IsEmpty())
			return general.GetRandomElement();

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName EnemyGarrisonGroup()
	{
		return Pick(s_aEnemyGarrison, s_aEnemyInfantry);
	}

	static ResourceName EnemyPatrolGroup()
	{
		return Pick(s_aEnemyPatrol, s_aEnemyInfantry);
	}

	//! Équipe légère US. Le nombre réel est plafonné au nombre de places, au plus 3.
	static ResourceName EnemyVehicleCrewGroup()
	{
		return GROUP_LIGHT;
	}

	//------------------------------------------------------------------------------------------------
	static bool SupplyMen(HAM_MapPoint def)
	{
		if (!def)
			return false;

		if (def.m_eRole == HAM_ERole.DEPOSIT)
			return true;

		if (def.m_eType == HAM_EMasterType.SUPPLY_VANILLA)
			return true;

		if (def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName AreaGroup()
	{
		return Pick(s_aAreaGroups, null);
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName SupplyGroup()
	{
		return Pick(s_aSupplyGroups, null);
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName ZoneFootGroup(HAM_MapPoint def)
	{
		if (SupplyMen(def))
			return SupplyGroup();

		return AreaGroup();
	}

	//------------------------------------------------------------------------------------------------
	static ResourceName CqbMan()
	{
		return Pick(s_aCqbMen, null);
	}

	//------------------------------------------------------------------------------------------------
	//! -1 : ce préfab n'a pas de curseur à écrire.
	static float SkillOf(ResourceName prefab)
	{
		if (prefab == GROUP_LIGHT)
			return 0.85;

		if (prefab == GROUP_GL)
			return 0.65;

		if (prefab == GROUP_LAT)
			return 0.9;

		if (prefab == GROUP_RIFLE)
			return 0.55;

		if (prefab == GROUP_MEDICAL)
			return 0.65;

		if (prefab == GROUP_TRANSPORT)
			return 1;

		return -1;
	}

	static ResourceName EnemyPatrolVehicle()
	{
		return Pick(s_aEnemyPatrolVehicles, s_aEnemyArmed);
	}

	static ResourceName EnemyCqbGroup()
	{
		return Pick(s_aEnemyCqb, s_aEnemyInfantry);
	}

	//! LAV ou M1025, au prorata des deux pourcentages restants.
	static ResourceName EnemyZoneVehicle(int lav, int car)
	{
		if (lav < 0)
			lav = 0;

		if (car < 0)
			car = 0;

		int total = lav + car;
		if (total < 1)
			return VEHICLE_M1025;

		int roll = Math.RandomInt(0, total);
		if (roll < lav)
			return VEHICLE_LAV;

		return VEHICLE_M1025;
	}

	static ResourceName EnemyCounterGroup()
	{
		return Pick(s_aEnemyCounterInfantry, s_aEnemyInfantry);
	}

	static ResourceName EnemyCounterVehicle()
	{
		return Pick(s_aEnemyCounterVehicles, s_aEnemyArmed);
	}

	static ResourceName EnemyMissionGroup()
	{
		return Pick(s_aEnemyMission, s_aEnemyInfantry);
	}

	static ResourceName EnemyOfficer()
	{
		return Pick(s_aEnemyOfficer, null);
	}

	static ResourceName PlayerGarrisonGroup()
	{
		return Pick(s_aPlayerGarrison, s_aPlayerInfantry);
	}

	static ResourceName PlayerConvoyVehicle()
	{
		return Pick(s_aPlayerConvoyVehicles, s_aPlayerMotorized);
	}

	static ResourceName Civilian()
	{
		return Pick(s_aCivilians, null);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlayerFaction(string factionKey)
	{
		return factionKey == PLAYER_FACTION;
	}

	static bool IsEnemyFaction(string factionKey)
	{
		return factionKey == ENEMY_FACTION;
	}

	//------------------------------------------------------------------------------------------------
	//! Le loadout est proposé si la liste est vide ou si son chemin contient un des morceaux.
	static bool IsSpawnSlotAllowed(ResourceName loadoutResource)
	{
		if (!s_aPlayerSpawnSlots || s_aPlayerSpawnSlots.IsEmpty())
			return true;

		foreach (string slot : s_aPlayerSpawnSlots)
		{
			if (!slot.IsEmpty() && loadoutResource.Contains(slot))
				return true;
		}

		return false;
	}
}
