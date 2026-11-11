//------------------------------------------------------------------------------------------------
// Réglages du front, comme param.hpp de Hearts and Minds.
// Toutes les valeurs vivent ici. $profile:13RDPA_HeartsAndMinds_Settings.json n'en expose qu'une partie,
// celle que le plan rend réglable sans recompiler. S'il manque, il est écrit avec ces valeurs, puis relu au boot.
// Les blocs gm, milsim, ai, holdBreath, teleport, vehicleDepot, health, groupAccess et game du même fichier
// sont lus par HAM_ProfileSettingsApplier (outils portés de Training).
//------------------------------------------------------------------------------------------------

class HAM_ZoneTune
{
	int m_iUnits;
	float m_fActivation;
	float m_fDespawn;
	int m_iVehicles;
	int m_iArmor;
	int m_iBtr;
	int m_iTruck;
	int m_iCar;
	//! Nombre de civils seuls. Chacun a son propre groupe.
	int m_iCivilians;
	int m_iBuildings;
	int m_iIeds;
	int m_iMines;
	int m_iPatrolUnits = 4;
}

class HAM_Settings
{
	static const string FILE_PATH = "$profile:13RDPA_HeartsAndMinds_Settings.json";
	protected static ref HAM_Settings s_Instance;

	// front
	bool m_bDebug = false;
	bool m_bSanctionEnabled = true;
	float m_fSanctionGraceSeconds = 20;
	float m_fSaveIntervalMinutes = 5;

	// garnisons
	float m_fActivationDistance = 800;
	float m_fDespawnDistance = 1300;
	ref HAM_ZoneTune m_Master;
	ref HAM_ZoneTune m_Slave;
	ref HAM_ZoneTune m_Fuel;
	ref HAM_ZoneTune m_Radio;
	ref HAM_ZoneTune m_Camp;
	ref HAM_ZoneTune m_Hideout;
	ref HAM_ZoneTune m_City;
	ref HAM_ZoneTune m_Supply;
	ref HAM_ZoneTune m_DepositZone;
	ref HAM_ZoneTune m_PatrolZone;
	ref HAM_ZoneTune m_CqbZone;
	ref HAM_ZoneTune m_IedZone;
	ref HAM_ZoneTune m_MinesZone;
	ref HAM_ZoneTune m_CiviliansZone;
	ref HAM_ZoneTune m_EnemyHqZone;
	int m_iMaxGroupSize = 6;
	int m_iFriendlyMaxUnits = 12;

	// renforts
	bool m_bReinforcementsEnabled = true;
	float m_fReinforcementMinutes = 20;
	float m_fReinforcementChance = 1;
	int m_iReinforcementUnits = 4;

	// patrouilles
	bool m_bPatrolsEnabled = true;
	int m_iPatrolGroupsPerLine = 6;
	int m_iPatrolMaxActive = 6;
	float m_fPatrolIntervalSeconds = 240;
	float m_fPatrolVehicleShare = 0.3;
	int m_iPatrolUnits = 4;

	//! IA actives en même temps sur la carte ; 0 rend la limite du jeu de base (128).
	int m_iActiveAiLimit = 2000;

	// véhicules ennemis autour de chaque point du front
	int m_iVehiclePatrolMax = 4;

	// patrouille d'hélicoptère
	bool m_bHelicoptersEnabled = true;
	float m_fHeliRadius = 300;
	float m_fHeliAltitudeMin = 80;
	float m_fHeliAltitudeMax = 150;

	// CQB
	bool m_bCqbEnabled = true;

	// capture d'un point : joueurs puis reprise ennemie
	float m_fCaptureSeconds = 300;
	float m_fRetakeSeconds = 1200;

	// mines et IED. Les comptes sont dans zoneTypes.
	bool m_bZoneDevicesEnabled = true;
	float m_fMineMinDistance = 50;
	float m_fMineMaxDistance = 150;
	//! Plus étroit : sentier ou trottoir, pas d'IED. Les routes en terre passent.
	float m_fIedRoadMinWidth = 1.82;

	// contre-attaque
	bool m_bCounterEnabled = true;
	float m_fCounterMinutes = 90;
	float m_fCounterChance = 0.35;
	int m_iCounterInfantryGroups = 1;
	int m_iCounterVehicles = 1;
	int m_iCounterGroupUnits = 6;
	float m_fCounterDelayAfterCaptureMinutes = 10;
	float m_fCounterMinFromPlayer = 600;
	float m_fCounterMinFromBase = 800;

	// dépôts
	bool m_bDepositsEnabled = true;
	float m_fDepositWaterMax = 300;
	float m_fDepositFoodMax = 300;
	float m_fDepositAmmoMax = 300;
	float m_fDepositRefillPerMinute = 4;
	float m_fDepositRefillInterval = 60;
	float m_fLoadRadius = 150;
	float m_fVehicleCapacity = 150;

	// tenue d'une cellule
	float m_fWaterThreshold = 50;
	float m_fFoodThreshold = 50;
	float m_fAmmoThreshold = 50;
	int m_iMenThreshold = 4;
	float m_fCycleMinutes = 10;
	float m_fUpkeepWater = 6;
	float m_fUpkeepFood = 6;
	float m_fUpkeepAmmo = 4;
	float m_fGraceMinutes = 60;
	int m_iMenPerCycle = 2;
	float m_fMenCostWater = 10;
	float m_fMenCostFood = 10;
	float m_fMenCostAmmo = 6;
	int m_iMenMax = 40;

	// échange
	float m_fExchangeRatio = 0.8;
	float m_fExchangeStep = 50;

	// convoi d'hommes
	int m_iConvoyMenPerVehicle = 6;
	int m_iConvoyMaxVehicles = 3;
	float m_fConvoyStuckMinutes = 20;

	// réputation
	int m_iRepStart = 200;
	int m_iRepMin = 0;
	int m_iRepMax = 1000;
	int m_iRepLocalRevealThreshold = 100;
	int m_iRepCivilianTalkThreshold = 300;
	int m_iRepFrontlinePenalty = 40;
	int m_iRepCivilianKillPenalty = 5;
	int m_iRepCaptureBonus = 25;
	int m_iRepMissionBonus = 20;
	int m_iRepLocalCaptureBonus = 40;
	int m_iRepLocalPerEnemyKill = 2;
	int m_iRepLocalPerIntel = 30;

	// intel
	float m_fIntelBodyChance = 0.15;
	int m_iIntelPerCell = 2;
	float m_fIntelBuildingRadius = 250;

	// civils d'une zone ennemie qui n'est ni un camp ni une base
	int m_iCivilianGroups = 4;
	int m_iCivilianVehicles = 16;

	// missions secondaires
	int m_iMissionRewardSupplies = 400;
	float m_fMissionRewardResource = 80;
	float m_fMissionTimeoutMinutes = 60;
	int m_iMissionGroupUnits = 6;

	//------------------------------------------------------------------------------------------------
	static HAM_Settings Get()
	{
		if (!s_Instance)
		{
			s_Instance = new HAM_Settings();
			s_Instance.Load();
		}

		return s_Instance;
	}

	//------------------------------------------------------------------------------------------------
	static void Reload()
	{
		s_Instance = new HAM_Settings();
		s_Instance.Load();
	}

	//------------------------------------------------------------------------------------------------
	protected HAM_ZoneTune MakeTune(int units, float activation, float despawn, int vehicles, int armor, int btr, int truck, int car, int civilians, int buildings, int ieds, int mines)
	{
		HAM_ZoneTune tune = new HAM_ZoneTune();
		tune.m_iUnits = units;
		tune.m_fActivation = activation;
		tune.m_fDespawn = despawn;
		tune.m_iVehicles = vehicles;
		tune.m_iArmor = armor + btr;
		tune.m_iBtr = 0;
		tune.m_iTruck = 0;
		tune.m_iCar = car;
		tune.m_iCivilians = civilians;
		tune.m_iBuildings = buildings;
		tune.m_iIeds = ieds;
		tune.m_iMines = mines;
		tune.m_iPatrolUnits = 4;
		return tune;
	}

	//------------------------------------------------------------------------------------------------
	protected void EnsureTypeDefaults()
	{
		if (m_Master)
			return;

		m_Master = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Slave = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Fuel = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Radio = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Camp = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Hideout = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_City = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
		m_Supply = MakeTune(24, 800, 1000, 3, 50, 50, 10, 10, 0, 10, 4, 5);
		m_DepositZone = MakeTune(24, 800, 1000, 3, 50, 50, 10, 10, 0, 10, 4, 5);
		m_PatrolZone = MakeTune(4, 800, 1000, 3, 50, 50, 10, 10, 12, 0, 0, 0);
		m_CqbZone = MakeTune(0, 800, 1000, 0, 0, 0, 0, 0, 0, 10, 0, 0);
		m_IedZone = MakeTune(0, 800, 1000, 0, 0, 0, 0, 0, 0, 0, 3, 0);
		m_MinesZone = MakeTune(0, 800, 1000, 0, 0, 0, 0, 0, 0, 0, 0, 8);
		m_CiviliansZone = MakeTune(0, 800, 1000, 0, 0, 0, 0, 0, 12, 0, 0, 0);
		m_EnemyHqZone = MakeTune(42, 800, 1000, 3, 50, 50, 10, 10, 12, 10, 5, 4);
	}

	//------------------------------------------------------------------------------------------------
	HAM_ZoneTune TuneFor(HAM_MapPoint def)
	{
		EnsureTypeDefaults();
		if (!def)
			return m_Slave;

		if (def.m_eRole == HAM_ERole.ENEMY_HQ)
			return m_EnemyHqZone;

		if (def.m_eRole == HAM_ERole.MASTER)
			return m_Master;

		if (def.m_eRole == HAM_ERole.PATROL)
			return m_PatrolZone;

		if (def.m_eRole == HAM_ERole.CQB)
			return m_CqbZone;

		if (def.m_eRole == HAM_ERole.IED)
			return m_IedZone;

		if (def.m_eRole == HAM_ERole.MINES)
			return m_MinesZone;

		if (def.m_eRole == HAM_ERole.CIVILIANS)
			return m_CiviliansZone;

		if (def.m_eRole == HAM_ERole.DEPOSIT || def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
			return m_DepositZone;

		if (def.m_eType == HAM_EMasterType.SUPPLY_VANILLA)
			return m_Supply;

		if (def.m_eType == HAM_EMasterType.FUEL)
			return m_Fuel;

		if (def.m_eType == HAM_EMasterType.RADIO)
			return m_Radio;

		if (def.m_eType == HAM_EMasterType.CAMP)
			return m_Camp;

		if (def.m_eType == HAM_EMasterType.HIDEOUT)
			return m_Hideout;

		if (def.m_eType == HAM_EMasterType.CITY)
			return m_City;

		return m_Slave;
	}

	//------------------------------------------------------------------------------------------------
	HAM_ZoneTune TuneAt(int index)
	{
		EnsureTypeDefaults();
		switch (index)
		{
			case 0: return m_Master;
			case 1: return m_Slave;
			case 2: return m_Fuel;
			case 3: return m_Radio;
			case 4: return m_Camp;
			case 5: return m_Hideout;
			case 6: return m_City;
			case 7: return m_Supply;
			case 8: return m_DepositZone;
			case 9: return m_PatrolZone;
			case 10: return m_CqbZone;
			case 11: return m_IedZone;
			case 12: return m_MinesZone;
			case 13: return m_CiviliansZone;
			case 14: return m_EnemyHqZone;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	int UnitsOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune)
			return 0;

		return tune.m_iUnits;
	}

	//------------------------------------------------------------------------------------------------
	float ActivationOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_fActivation < 0)
			return m_fActivationDistance;

		return tune.m_fActivation;
	}

	//------------------------------------------------------------------------------------------------
	float DespawnOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_fDespawn < 0)
			return m_fDespawnDistance;

		return tune.m_fDespawn;
	}

	//------------------------------------------------------------------------------------------------
	int VehiclesOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_iVehicles < 0)
			return 0;

		return tune.m_iVehicles;
	}

	//------------------------------------------------------------------------------------------------
	int CiviliansOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_iCivilians < 0)
			return 0;

		return tune.m_iCivilians;
	}

	//------------------------------------------------------------------------------------------------
	int BuildingsOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_iBuildings < 0)
			return 0;

		return tune.m_iBuildings;
	}

	//------------------------------------------------------------------------------------------------
	int IedsOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune)
			return 0;

		return tune.m_iIeds;
	}

	//------------------------------------------------------------------------------------------------
	int MinesOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune)
			return 0;

		return tune.m_iMines;
	}

	//------------------------------------------------------------------------------------------------
	//! Nombre de groupes entiers : garnison et patrouilles à pied de ce type.
	int PatrolUnitsOf(HAM_MapPoint def)
	{
		HAM_ZoneTune tune = TuneFor(def);
		if (!tune || tune.m_iPatrolUnits < 0)
			return m_iPatrolUnits;

		return tune.m_iPatrolUnits;
	}

	//------------------------------------------------------------------------------------------------
	protected void ReadTune(JsonLoadContext ctx, string key, HAM_ZoneTune tune)
	{
		if (!tune || !ctx.StartObject(key))
			return;

		tune.m_iUnits = RI(ctx, "units", tune.m_iUnits);
		tune.m_fActivation = RF(ctx, "activationDistance", tune.m_fActivation);
		tune.m_fDespawn = RF(ctx, "despawnDistance", tune.m_fDespawn);
		tune.m_iVehicles = RI(ctx, "vehicles", tune.m_iVehicles);
		int lav;
		if (ctx.ReadValue("percentLAV", lav))
			tune.m_iArmor = lav;
		else
			tune.m_iArmor = RI(ctx, "percentArmor", tune.m_iArmor) + RI(ctx, "percentBtr", 0);

		tune.m_iTruck = 0;
		int car;
		if (ctx.ReadValue("percentM1025", car))
			tune.m_iCar = car;
		else
			tune.m_iCar = RI(ctx, "percentCar", tune.m_iCar);
		tune.m_iCivilians = RI(ctx, "civilians", tune.m_iCivilians);
		tune.m_iBuildings = RI(ctx, "buildings", tune.m_iBuildings);
		tune.m_iIeds = RI(ctx, "ieds", tune.m_iIeds);
		tune.m_iMines = RI(ctx, "mines", tune.m_iMines);
		tune.m_iPatrolUnits = RI(ctx, "patrolUnits", tune.m_iPatrolUnits);
		ctx.EndObject();
	}

	//------------------------------------------------------------------------------------------------
	protected void WriteTune(notnull array<string> lines, string key, HAM_ZoneTune tune, bool last, bool vehicles)
	{
		lines.Insert("    \"" + key + "\": {");
		lines.Insert("      \"activationDistance\": " + N(tune.m_fActivation) + ",");
		lines.Insert("      \"despawnDistance\": " + N(tune.m_fDespawn) + ",");
		if (vehicles)
		{
			lines.Insert("      \"vehicles\": " + tune.m_iVehicles.ToString() + ",");
			lines.Insert("      \"percentLAV\": " + tune.m_iArmor.ToString() + ",");
			lines.Insert("      \"percentM1025\": " + tune.m_iCar.ToString() + ",");
		}

		lines.Insert("      \"civilians\": " + tune.m_iCivilians.ToString() + ",");
		lines.Insert("      \"buildings\": " + tune.m_iBuildings.ToString() + ",");
		lines.Insert("      \"ieds\": " + tune.m_iIeds.ToString() + ",");
		lines.Insert("      \"mines\": " + tune.m_iMines.ToString() + ",");
		lines.Insert("      \"patrolUnits\": " + tune.m_iPatrolUnits.ToString());
		if (last)
			lines.Insert("    }");
		else
			lines.Insert("    },");
	}

	//------------------------------------------------------------------------------------------------
	protected void Load()
	{
		if (!Replication.IsServer())
			return;

		if (!FileIO.FileExists(FILE_PATH))
			WriteDefaults();

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE_PATH))
		{
			Print("[13RDPA H&M] Réglages illisibles, valeurs du script utilisées : " + FILE_PATH, LogLevel.WARNING);
			return;
		}

		EnsureTypeDefaults();
		m_bDebug = RB(ctx, "debug", m_bDebug);
		if (ctx.StartObject("save"))
		{
			m_fSaveIntervalMinutes = RF(ctx, "intervalMinutes", m_fSaveIntervalMinutes);
			ctx.EndObject();
		}

		if (ctx.StartObject("garrison"))
		{
			m_fActivationDistance = RF(ctx, "activationDistance", m_fActivationDistance);
			m_fDespawnDistance = RF(ctx, "despawnDistance", m_fDespawnDistance);
			ctx.EndObject();
		}

		if (ctx.StartObject("zoneTypes"))
		{
			ReadTune(ctx, "master", m_Master);
			ReadTune(ctx, "slave", m_Slave);
			ReadTune(ctx, "fuel", m_Fuel);
			ReadTune(ctx, "radio", m_Radio);
			ReadTune(ctx, "camp", m_Camp);
			ReadTune(ctx, "hideout", m_Hideout);
			ReadTune(ctx, "city", m_City);
			ReadTune(ctx, "supply", m_Supply);
			ReadTune(ctx, "deposit", m_DepositZone);
			ReadTune(ctx, "patrol", m_PatrolZone);
			ReadTune(ctx, "cqb", m_CqbZone);
			ReadTune(ctx, "ied", m_IedZone);
			ReadTune(ctx, "mines", m_MinesZone);
			ReadTune(ctx, "civilians", m_CiviliansZone);
			ReadTune(ctx, "enemyHq", m_EnemyHqZone);
			ctx.EndObject();
		}

		if (ctx.StartObject("capture"))
		{
			m_fCaptureSeconds = RF(ctx, "playerSeconds", m_fCaptureSeconds);
			m_fRetakeSeconds = RF(ctx, "enemySeconds", m_fRetakeSeconds);
			ctx.EndObject();
		}

		if (ctx.StartObject("deposits"))
		{
			m_bDepositsEnabled = RB(ctx, "enabled", m_bDepositsEnabled);
			m_fDepositWaterMax = RF(ctx, "waterMax", m_fDepositWaterMax);
			m_fDepositFoodMax = RF(ctx, "foodMax", m_fDepositFoodMax);
			m_fDepositAmmoMax = RF(ctx, "ammoMax", m_fDepositAmmoMax);
			m_fDepositRefillPerMinute = RF(ctx, "refillPerMinute", m_fDepositRefillPerMinute);
			m_fDepositRefillInterval = RF(ctx, "refillIntervalSeconds", m_fDepositRefillInterval);
			m_fLoadRadius = RF(ctx, "loadRadius", m_fLoadRadius);
			m_fVehicleCapacity = RF(ctx, "vehicleCapacity", m_fVehicleCapacity);
			ctx.EndObject();
		}

		if (ctx.StartObject("reputation"))
		{
			m_iRepStart = RI(ctx, "start", m_iRepStart);
			m_iRepLocalRevealThreshold = RI(ctx, "localRevealThreshold", m_iRepLocalRevealThreshold);
			m_iRepCivilianTalkThreshold = RI(ctx, "civilianTalkThreshold", m_iRepCivilianTalkThreshold);
			m_iRepFrontlinePenalty = RI(ctx, "frontlinePenalty", m_iRepFrontlinePenalty);
			ctx.EndObject();
		}

		if (ctx.StartObject("intel"))
		{
			m_fIntelBodyChance = RF(ctx, "bodyChance", m_fIntelBodyChance);
			m_iIntelPerCell = RI(ctx, "buildingIntelPerCell", m_iIntelPerCell);
			ctx.EndObject();
		}

		if (ctx.StartObject("patrols"))
		{
			m_bPatrolsEnabled = RB(ctx, "enabled", m_bPatrolsEnabled);
			m_iPatrolGroupsPerLine = RI(ctx, "maxGroups", m_iPatrolGroupsPerLine);
			m_iPatrolMaxActive = RI(ctx, "maxActive", m_iPatrolMaxActive);
			m_fPatrolIntervalSeconds = RF(ctx, "intervalSeconds", m_fPatrolIntervalSeconds);
			m_fPatrolVehicleShare = RF(ctx, "vehicleShare", m_fPatrolVehicleShare);
			ctx.EndObject();
		}

		if (ctx.StartObject("ai"))
		{
			m_iActiveAiLimit = RI(ctx, "activeLimit", m_iActiveAiLimit);
			ctx.EndObject();
		}

		if (ctx.StartObject("vehicles"))
		{
			m_iVehiclePatrolMax = RI(ctx, "patrolMax", m_iVehiclePatrolMax);
			ctx.EndObject();
		}

		if (ctx.StartObject("helicopters"))
		{
			m_bHelicoptersEnabled = RB(ctx, "enabled", m_bHelicoptersEnabled);
			m_fHeliRadius = RF(ctx, "radius", m_fHeliRadius);
			m_fHeliAltitudeMin = RF(ctx, "altitudeMin", m_fHeliAltitudeMin);
			m_fHeliAltitudeMax = RF(ctx, "altitudeMax", m_fHeliAltitudeMax);
			ctx.EndObject();
		}

		if (ctx.StartObject("cqb"))
		{
			m_bCqbEnabled = RB(ctx, "enabled", m_bCqbEnabled);
			ctx.EndObject();
		}

		if (ctx.StartObject("civilians"))
		{
			m_iCivilianGroups = RI(ctx, "maxGroups", m_iCivilianGroups);
			m_iCivilianVehicles = RI(ctx, "patrolVehicles", m_iCivilianVehicles);
			ctx.EndObject();
		}

		if (ctx.StartObject("mines"))
		{
			m_bZoneDevicesEnabled = RB(ctx, "zonesEnabled", m_bZoneDevicesEnabled);
			m_fMineMinDistance = RF(ctx, "mineMinDistance", m_fMineMinDistance);
			m_fMineMaxDistance = RF(ctx, "mineMaxDistance", m_fMineMaxDistance);
			m_fIedRoadMinWidth = RF(ctx, "iedRoadMinWidth", m_fIedRoadMinWidth);
			ctx.EndObject();
		}

		if (ctx.StartObject("reinforcements"))
		{
			m_bReinforcementsEnabled = RB(ctx, "enabled", m_bReinforcementsEnabled);
			m_fReinforcementMinutes = RF(ctx, "intervalMinutes", m_fReinforcementMinutes);
			m_fReinforcementChance = RF(ctx, "chance", m_fReinforcementChance);
			m_iReinforcementUnits = RI(ctx, "groupUnits", m_iReinforcementUnits);
			ctx.EndObject();
		}

		if (ctx.StartObject("counterAttack"))
		{
			m_bCounterEnabled = RB(ctx, "enabled", m_bCounterEnabled);
			m_fCounterMinutes = RF(ctx, "intervalMinutes", m_fCounterMinutes);
			m_fCounterChance = RF(ctx, "chance", m_fCounterChance);
			m_iCounterGroupUnits = RI(ctx, "groupUnits", m_iCounterGroupUnits);
			m_iCounterInfantryGroups = RI(ctx, "groups", m_iCounterInfantryGroups);
			m_iCounterVehicles = RI(ctx, "vehicles", m_iCounterVehicles);
			m_fCounterDelayAfterCaptureMinutes = RF(ctx, "delayAfterCaptureMinutes", m_fCounterDelayAfterCaptureMinutes);
			m_fCounterMinFromPlayer = RF(ctx, "minFromPlayer", m_fCounterMinFromPlayer);
			m_fCounterMinFromBase = RF(ctx, "minFromBase", m_fCounterMinFromBase);
			ctx.EndObject();
		}

		if (ctx.StartObject("hold"))
		{
			m_fWaterThreshold = RF(ctx, "waterThreshold", m_fWaterThreshold);
			m_fFoodThreshold = RF(ctx, "foodThreshold", m_fFoodThreshold);
			m_fAmmoThreshold = RF(ctx, "ammoThreshold", m_fAmmoThreshold);
			m_iMenThreshold = RI(ctx, "menThreshold", m_iMenThreshold);
			m_fUpkeepWater = RF(ctx, "upkeepWater", m_fUpkeepWater);
			m_fUpkeepFood = RF(ctx, "upkeepFood", m_fUpkeepFood);
			m_fUpkeepAmmo = RF(ctx, "upkeepAmmo", m_fUpkeepAmmo);
			m_fGraceMinutes = RF(ctx, "graceMinutes", m_fGraceMinutes);
			m_fCycleMinutes = RF(ctx, "cycleMinutes", m_fCycleMinutes);
			m_iMenPerCycle = RI(ctx, "menPerCycle", m_iMenPerCycle);
			m_iMenMax = RI(ctx, "menMax", m_iMenMax);
			m_fMenCostWater = RF(ctx, "menCostWater", m_fMenCostWater);
			m_fMenCostFood = RF(ctx, "menCostFood", m_fMenCostFood);
			m_fMenCostAmmo = RF(ctx, "menCostAmmo", m_fMenCostAmmo);
			ctx.EndObject();
		}

		if (ctx.StartObject("exchange"))
		{
			m_fExchangeRatio = RF(ctx, "ratio", m_fExchangeRatio);
			ctx.EndObject();
		}

		Sanitize();
		Print("[13RDPA H&M] Réglages chargés : " + FILE_PATH, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	void Sanitize()
	{
		m_fActivationDistance = Math.Max(100, m_fActivationDistance);
		if (m_fDespawnDistance < m_fActivationDistance + 100)
			m_fDespawnDistance = m_fActivationDistance + 100;

		m_iPatrolGroupsPerLine = Math.Max(0, m_iPatrolGroupsPerLine);
		m_iActiveAiLimit = Math.Clamp(m_iActiveAiLimit, 0, 100000);
		m_iVehiclePatrolMax = Math.Clamp(m_iVehiclePatrolMax, 0, 20);
		m_iCivilianGroups = Math.Max(0, m_iCivilianGroups);
		m_iCivilianVehicles = Math.Clamp(m_iCivilianVehicles, 0, 32);
		m_iCounterInfantryGroups = Math.Max(0, m_iCounterInfantryGroups);
		m_iCounterVehicles = Math.Max(0, m_iCounterVehicles);
		m_fCaptureSeconds = Math.Max(5, m_fCaptureSeconds);
		m_fRetakeSeconds = Math.Max(5, m_fRetakeSeconds);

		if (m_iMaxGroupSize < 2)
			m_iMaxGroupSize = 2;

		if (m_fCycleMinutes < 1)
			m_fCycleMinutes = 1;

		if (m_fSaveIntervalMinutes < 1)
			m_fSaveIntervalMinutes = 1;

		if (m_fExchangeRatio <= 0)
			m_fExchangeRatio = 0.1;

		if (m_fExchangeStep < 1)
			m_fExchangeStep = 1;

		if (m_iConvoyMenPerVehicle < 1)
			m_iConvoyMenPerVehicle = 1;

		if (m_fLoadRadius < 10)
			m_fLoadRadius = 10;

		if (m_fVehicleCapacity < 1)
			m_fVehicleCapacity = 1;

		if (m_fCounterDelayAfterCaptureMinutes < 0)
			m_fCounterDelayAfterCaptureMinutes = 0;

		if (m_fCounterMinFromPlayer < 0)
			m_fCounterMinFromPlayer = 0;

		if (m_fCounterMinFromBase < 0)
			m_fCounterMinFromBase = 0;

		if (m_iRepMax <= m_iRepMin)
			m_iRepMax = m_iRepMin + 1;

		m_iRepStart = Math.Clamp(m_iRepStart, m_iRepMin, m_iRepMax);

		m_fMineMinDistance = Math.Max(0, m_fMineMinDistance);
		if (m_fMineMaxDistance < m_fMineMinDistance + 10)
			m_fMineMaxDistance = m_fMineMinDistance + 10;

		m_fIedRoadMinWidth = Math.Max(0, m_fIedRoadMinWidth);
		if (m_fHeliRadius < 50)
			m_fHeliRadius = 50;

		if (m_fHeliAltitudeMin < 10)
			m_fHeliAltitudeMin = 10;

		if (m_fHeliAltitudeMax < m_fHeliAltitudeMin)
			m_fHeliAltitudeMax = m_fHeliAltitudeMin;

		for (int typeIndex = 0; typeIndex < 15; typeIndex++)
		{
			HAM_ZoneTune tune = TuneAt(typeIndex);
			if (!tune)
				continue;

			if (tune.m_iPatrolUnits < 0)
				tune.m_iPatrolUnits = m_iPatrolUnits;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static float RF(JsonLoadContext ctx, string key, float fallback)
	{
		float value;
		if (ctx.ReadValue(key, value))
			return value;

		return fallback;
	}

	protected static int RI(JsonLoadContext ctx, string key, int fallback)
	{
		int value;
		if (ctx.ReadValue(key, value))
			return value;

		return fallback;
	}

	protected static bool RB(JsonLoadContext ctx, string key, bool fallback)
	{
		bool value;
		if (ctx.ReadValue(key, value))
			return value;

		return fallback;
	}

	//------------------------------------------------------------------------------------------------
	protected static string B(bool value)
	{
		if (value)
			return "true";

		return "false";
	}

	protected static string N(float value)
	{
		return value.ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! Écrit le fichier avec les seuls réglages prévus par le plan. On l'édite ensuite sans recompiler.
	protected void WriteDefaults()
	{
		array<string> lines = {};
		EnsureTypeDefaults();
		lines.Insert("{");
		lines.Insert("  \"debug\": " + B(m_bDebug) + ",");
		lines.Insert("  \"save\": {");
		lines.Insert("    \"intervalMinutes\": " + N(m_fSaveIntervalMinutes));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"garrison\": {");
		lines.Insert("    \"activationDistance\": " + N(m_fActivationDistance) + ",");
		lines.Insert("    \"despawnDistance\": " + N(m_fDespawnDistance));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"zoneTypes\": {");
		WriteTune(lines, "master", m_Master, false, true);
		WriteTune(lines, "slave", m_Slave, false, true);
		WriteTune(lines, "fuel", m_Fuel, false, true);
		WriteTune(lines, "radio", m_Radio, false, true);
		WriteTune(lines, "camp", m_Camp, false, true);
		WriteTune(lines, "hideout", m_Hideout, false, true);
		WriteTune(lines, "city", m_City, false, true);
		WriteTune(lines, "supply", m_Supply, false, true);
		WriteTune(lines, "deposit", m_DepositZone, false, true);
		WriteTune(lines, "patrol", m_PatrolZone, false, false);
		WriteTune(lines, "cqb", m_CqbZone, false, false);
		WriteTune(lines, "ied", m_IedZone, false, false);
		WriteTune(lines, "mines", m_MinesZone, false, false);
		WriteTune(lines, "civilians", m_CiviliansZone, false, false);
		WriteTune(lines, "enemyHq", m_EnemyHqZone, true, true);
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"capture\": {");
		lines.Insert("    \"playerSeconds\": " + N(m_fCaptureSeconds) + ",");
		lines.Insert("    \"enemySeconds\": " + N(m_fRetakeSeconds));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"deposits\": {");
		lines.Insert("    \"enabled\": " + B(m_bDepositsEnabled) + ",");
		lines.Insert("    \"waterMax\": " + N(m_fDepositWaterMax) + ",");
		lines.Insert("    \"foodMax\": " + N(m_fDepositFoodMax) + ",");
		lines.Insert("    \"ammoMax\": " + N(m_fDepositAmmoMax) + ",");
		lines.Insert("    \"refillPerMinute\": " + N(m_fDepositRefillPerMinute) + ",");
		lines.Insert("    \"refillIntervalSeconds\": " + N(m_fDepositRefillInterval) + ",");
		lines.Insert("    \"loadRadius\": " + N(m_fLoadRadius) + ",");
		lines.Insert("    \"vehicleCapacity\": " + N(m_fVehicleCapacity));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"reputation\": {");
		lines.Insert("    \"start\": " + m_iRepStart.ToString() + ",");
		lines.Insert("    \"localRevealThreshold\": " + m_iRepLocalRevealThreshold.ToString() + ",");
		lines.Insert("    \"civilianTalkThreshold\": " + m_iRepCivilianTalkThreshold.ToString() + ",");
		lines.Insert("    \"frontlinePenalty\": " + m_iRepFrontlinePenalty.ToString());
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"intel\": {");
		lines.Insert("    \"bodyChance\": " + N(m_fIntelBodyChance) + ",");
		lines.Insert("    \"buildingIntelPerCell\": " + m_iIntelPerCell.ToString());
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"patrols\": {");
		lines.Insert("    \"enabled\": " + B(m_bPatrolsEnabled) + ",");
		lines.Insert("    \"maxGroups\": " + m_iPatrolGroupsPerLine.ToString() + ",");
		lines.Insert("    \"maxActive\": " + m_iPatrolMaxActive.ToString() + ",");
		lines.Insert("    \"intervalSeconds\": " + N(m_fPatrolIntervalSeconds) + ",");
		lines.Insert("    \"vehicleShare\": " + N(m_fPatrolVehicleShare));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"vehicles\": {");
		lines.Insert("    \"patrolMax\": " + m_iVehiclePatrolMax.ToString());
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"helicopters\": {");
		lines.Insert("    \"enabled\": " + B(m_bHelicoptersEnabled) + ",");
		lines.Insert("    \"radius\": " + N(m_fHeliRadius) + ",");
		lines.Insert("    \"altitudeMin\": " + N(m_fHeliAltitudeMin) + ",");
		lines.Insert("    \"altitudeMax\": " + N(m_fHeliAltitudeMax));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"cqb\": {");
		lines.Insert("    \"enabled\": " + B(m_bCqbEnabled));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"civilians\": {");
		lines.Insert("    \"maxGroups\": " + m_iCivilianGroups.ToString() + ",");
		lines.Insert("    \"patrolVehicles\": " + m_iCivilianVehicles.ToString());
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"mines\": {");
		lines.Insert("    \"zonesEnabled\": " + B(m_bZoneDevicesEnabled) + ",");
		lines.Insert("    \"mineMinDistance\": " + N(m_fMineMinDistance) + ",");
		lines.Insert("    \"mineMaxDistance\": " + N(m_fMineMaxDistance) + ",");
		lines.Insert("    \"iedRoadMinWidth\": " + N(m_fIedRoadMinWidth));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"reinforcements\": {");
		lines.Insert("    \"enabled\": " + B(m_bReinforcementsEnabled) + ",");
		lines.Insert("    \"intervalMinutes\": " + N(m_fReinforcementMinutes) + ",");
		lines.Insert("    \"chance\": " + N(m_fReinforcementChance) + ",");
		lines.Insert("    \"groupUnits\": " + m_iReinforcementUnits.ToString());
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"counterAttack\": {");
		lines.Insert("    \"enabled\": " + B(m_bCounterEnabled) + ",");
		lines.Insert("    \"intervalMinutes\": " + N(m_fCounterMinutes) + ",");
		lines.Insert("    \"chance\": " + N(m_fCounterChance) + ",");
		lines.Insert("    \"groupUnits\": " + m_iCounterGroupUnits.ToString() + ",");
		lines.Insert("    \"groups\": " + m_iCounterInfantryGroups.ToString() + ",");
		lines.Insert("    \"vehicles\": " + m_iCounterVehicles.ToString() + ",");
		lines.Insert("    \"delayAfterCaptureMinutes\": " + N(m_fCounterDelayAfterCaptureMinutes) + ",");
		lines.Insert("    \"minFromPlayer\": " + N(m_fCounterMinFromPlayer) + ",");
		lines.Insert("    \"minFromBase\": " + N(m_fCounterMinFromBase));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"hold\": {");
		lines.Insert("    \"waterThreshold\": " + N(m_fWaterThreshold) + ",");
		lines.Insert("    \"foodThreshold\": " + N(m_fFoodThreshold) + ",");
		lines.Insert("    \"ammoThreshold\": " + N(m_fAmmoThreshold) + ",");
		lines.Insert("    \"menThreshold\": " + m_iMenThreshold.ToString() + ",");
		lines.Insert("    \"upkeepWater\": " + N(m_fUpkeepWater) + ",");
		lines.Insert("    \"upkeepFood\": " + N(m_fUpkeepFood) + ",");
		lines.Insert("    \"upkeepAmmo\": " + N(m_fUpkeepAmmo) + ",");
		lines.Insert("    \"graceMinutes\": " + N(m_fGraceMinutes) + ",");
		lines.Insert("    \"cycleMinutes\": " + N(m_fCycleMinutes) + ",");
		lines.Insert("    \"menPerCycle\": " + m_iMenPerCycle.ToString() + ",");
		lines.Insert("    \"menMax\": " + m_iMenMax.ToString() + ",");
		lines.Insert("    \"menCostWater\": " + N(m_fMenCostWater) + ",");
		lines.Insert("    \"menCostFood\": " + N(m_fMenCostFood) + ",");
		lines.Insert("    \"menCostAmmo\": " + N(m_fMenCostAmmo));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"exchange\": {");
		lines.Insert("    \"ratio\": " + N(m_fExchangeRatio));
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"gm\": {");
		lines.Insert("    \"enableBudgets\": false");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"milsim\": {");
		lines.Insert("    \"hideNameTags\": false,");
		lines.Insert("    \"hideUnconsciousIcon\": false,");
		lines.Insert("    \"hideVONNames\": false,");
		lines.Insert("    \"blockUnconsciousVON\": true,");
		lines.Insert("    \"hideEnemyProxVONUI\": true,");
		lines.Insert("    \"hideAllProxVONUI\": false,");
		lines.Insert("    \"keepWrecks\": true,");
		lines.Insert("    \"keepCorpses\": true,");
		lines.Insert("    \"vehicleVirtualization\": false,");
		lines.Insert("    \"indestructibleBuildings\": true,");
		lines.Insert("    \"blockPhotoMode\": true,");
		lines.Insert("    \"disableVanillaMarkers\": false,");
		lines.Insert("    \"noMinimumRank\": true,");
		lines.Insert("    \"forceFirstPersonFoot\": false,");
		lines.Insert("    \"forceFirstPersonVehicle\": false,");
		lines.Insert("    \"nameTagMaxDistance\": 20,");
		lines.Insert("    \"weaponDropChance\": 0,");
		lines.Insert("    \"showCompass\": false,");
		lines.Insert("    \"stealthGM\": false");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"ai\": {");
		lines.Insert("    \"activeLimit\": " + m_iActiveAiLimit.ToString() + ",");
		lines.Insert("    \"perceptionFactor\": 2,");
		lines.Insert("    \"threatenedPerceptionMul\": 8,");
		lines.Insert("    \"suppressedAudibleDistance\": 0,");
		lines.Insert("    \"aiSkill\": 2,");
		lines.Insert("    \"aimErrorScale\": 0.3,");
		lines.Insert("    \"reactionScale\": 0.3,");
		lines.Insert("    \"engageDelayScale\": 0.3,");
		lines.Insert("    \"combatStyle\": 0,");
		lines.Insert("    \"stanceMode\": 1,");
		lines.Insert("    \"lookMode\": 2,");
		lines.Insert("    \"weaponMode\": 1,");
		lines.Insert("    \"fireMode\": 2,");
		lines.Insert("    \"contactMemory\": 5,");
		lines.Insert("    \"attentionDistance\": 100,");
		lines.Insert("    \"coverSeekOverride\": 2,");
		lines.Insert("    \"maxEngagementDistance\": 150,");
		lines.Insert("    \"fireAtWillDistance\": 150,");
		lines.Insert("    \"returnFireDistance\": 300,");
		lines.Insert("    \"contactShareRange\": 800,");
		lines.Insert("    \"finishOffChance\": 30,");
		lines.Insert("    \"spareUnarmed\": false,");
		lines.Insert("    \"consumeMagazines\": true,");
		lines.Insert("    \"nvgAuto\": true,");
		lines.Insert("    \"nvgPerception\": 3,");
		lines.Insert("    \"nightPerception\": 3,");
		lines.Insert("    \"footNavPreset\": 1,");
		lines.Insert("    \"vehicleNavPreset\": 1,");
		lines.Insert("    \"vehicleCruiseSpeed\": 20,");
		lines.Insert("    \"stayInVehicleMode\": 2,");
		lines.Insert("    \"fuelConsumption\": true,");
		lines.Insert("    \"fuelConsumptionScale\": 0");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"holdBreath\": {");
		lines.Insert("    \"maxHoldTime\": 20.0,");
		lines.Insert("    \"minStamina\": 0.0,");
		lines.Insert("    \"enableBreathSounds\": true");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"teleport\": {");
		lines.Insert("    \"playerToPlayer\": true,");
		lines.Insert("    \"betweenBases\": true");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"vehicleDepot\": {");
		lines.Insert("    \"radiusMeters\": 200");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"health\": {");
		lines.Insert("    \"dotScale\": 0.25,");
		lines.Insert("    \"regenScale\": 2.5");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"groupAccess\": {");
		lines.Insert("    \"whitelistIds\": [");
		lines.Insert("      \"76561198264674074\",");
		lines.Insert("      \"9785b4cd-e324-4863-9c51-f527b210d3d9\"");
		lines.Insert("    ]");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"time\": {");
		lines.Insert("    \"hour\": 7,");
		lines.Insert("    \"minute\": 0,");
		lines.Insert("    \"second\": 0,");
		lines.Insert("    \"year\": 1986,");
		lines.Insert("    \"month\": 11,");
		lines.Insert("    \"day\": 7,");
		lines.Insert("    \"advance\": true,");
		lines.Insert("    \"multiplier\": 1.0");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"weather\": {");
		lines.Insert("    \"automatic\": true,");
		lines.Insert("    \"state\": \"CLEAR\",");
		lines.Insert("    \"windAutomatic\": false,");
		lines.Insert("    \"windSpeed\": 0,");
		lines.Insert("    \"windDirection\": \"NORTH\"");
		lines.Insert("  },");
		lines.Insert("");
		lines.Insert("  \"game\": {");
		lines.Insert("    \"globalSupplies\": true,");
		lines.Insert("    \"allowRespawn\": true,");
		lines.Insert("    \"spawnSupplyCostMultiplier\": 0.0,");
		lines.Insert("    \"deathNotification\": \"DISABLED\",");
		lines.Insert("    \"notificationSource\": \"ENEMIES_ONLY\",");
		lines.Insert("    \"friendlyKillFeed\": \"INVOLVED\",");
		lines.Insert("    \"arsenalMode\": \"UNRESTRICTED\",");
		lines.Insert("    \"rankSupplyLimit\": false,");
		lines.Insert("    \"punishTeamKill\": true,");
		lines.Insert("    \"perceivedFaction\": \"DISABLED\",");
		lines.Insert("    \"disguisePunishment\": \"NONE\",");
		lines.Insert("    \"showMortarBallistics\": true");
		lines.Insert("  }");
		lines.Insert("}");

		// Écrit tout de suite : Load() relit ce fichier dans la même fonction.
		FileHandle file = FileIO.OpenFile(FILE_PATH, FileMode.WRITE);
		if (!file)
		{
			Print("[13RDPA H&M] Impossible d'écrire " + FILE_PATH, LogLevel.ERROR);
			return;
		}

		foreach (string line : lines)
		{
			file.WriteLine(line);
		}

		file.Close();
		Print("[13RDPA H&M] Réglages par défaut écrits : " + FILE_PATH, LogLevel.NORMAL);
	}
}
