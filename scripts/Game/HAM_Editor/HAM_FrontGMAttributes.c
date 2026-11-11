//------------------------------------------------------------------------------------------------
// Pages « H&M » du Game Master : chaque réglage de $profile:13RDPA_HeartsAndMinds_Settings.json
// qui n'a pas déjà son attribut vanilla ou milsim/IA. Appliqué tout de suite sur le serveur ;
// le JSON reste la valeur de départ au prochain lancement.
// Les numéros m_iSetting sont ceux de Configs/Editor/AttributeLists/Edit.conf.
//------------------------------------------------------------------------------------------------

class HAM_LiveSettings
{
	//------------------------------------------------------------------------------------------------
	static float GetFloat(int id)
	{
		if (id >= 200 && id < 380)
			return ZoneFloat(id);

		if (id >= 400 && id <= 414)
			return ZonePatrolUnits(id);

		HAM_Settings s = HAM_Settings.Get();
		switch (id)
		{
			case 0: return s.m_fActivationDistance;
			case 1: return s.m_fDespawnDistance;
			case 6: return s.m_fCaptureSeconds;
			case 7: return s.m_fRetakeSeconds;
			case 8: return s.m_fDepositWaterMax;
			case 9: return s.m_fDepositFoodMax;
			case 10: return s.m_fDepositAmmoMax;
			case 11: return s.m_fDepositRefillPerMinute;
			case 81: return s.m_fDepositRefillInterval;
			case 12: return s.m_iRepStart;
			case 13: return s.m_iRepLocalRevealThreshold;
			case 14: return s.m_iRepCivilianTalkThreshold;
			case 15: return s.m_iRepFrontlinePenalty;
			case 16: return s.m_fIntelBodyChance;
			case 17: return s.m_iIntelPerCell;
			case 18: return s.m_iPatrolMaxActive;
			case 19: return s.m_fPatrolIntervalSeconds;
			case 20: return s.m_fPatrolVehicleShare;
			case 66: return s.m_iCivilianVehicles;
			case 67: return HAM_ProfileSettingsApplier.s_iHour;
			case 68: return HAM_ProfileSettingsApplier.s_iMinute;
			case 69: return HAM_ProfileSettingsApplier.s_fTimeMultiplier;
			case 70: return HAM_ProfileSettingsApplier.s_iWeatherState;
			case 71: return HAM_ProfileSettingsApplier.s_fWindSpeed;
			case 72: return HAM_ProfileSettingsApplier.s_iWindDirection;
			case 73: return HAM_ProfileSettingsApplier.s_iKillFeed;
			case 74: return HAM_ProfileSettingsApplier.s_iKillReceive;
			case 75: return HAM_ProfileSettingsApplier.s_iFriendlyFeed;
			case 76: return HAM_ProfileSettingsApplier.s_fSpawnCost;
			case 77: return HAM_ProfileSettingsApplier.s_iYear;
			case 78: return HAM_ProfileSettingsApplier.s_iMonth;
			case 79: return HAM_ProfileSettingsApplier.s_iDay;
			case 29: return s.m_fMineMinDistance;
			case 30: return s.m_fMineMaxDistance;
			case 31: return s.m_fIedRoadMinWidth;
			case 34: return s.m_fReinforcementMinutes;
			case 35: return s.m_fReinforcementChance;
			case 36: return s.m_iReinforcementUnits;
			case 37: return s.m_fCounterMinutes;
			case 38: return s.m_fCounterChance;
			case 39: return s.m_iCounterGroupUnits;
			case 40: return s.m_iCounterInfantryGroups;
			case 41: return s.m_iCounterVehicles;
			case 42: return s.m_fWaterThreshold;
			case 43: return s.m_fFoodThreshold;
			case 44: return s.m_fAmmoThreshold;
			case 45: return s.m_iMenThreshold;
			case 46: return s.m_fUpkeepWater;
			case 47: return s.m_fUpkeepFood;
			case 48: return s.m_fUpkeepAmmo;
			case 49: return s.m_fGraceMinutes;
			case 50: return s.m_iMenPerCycle;
			case 80: return s.m_fCycleMinutes;
			case 51: return s.m_fExchangeRatio;
			case 52: return HAM_ProfileSettingsApplier.HoldBreathMax();
			case 53: return HAM_ProfileSettingsApplier.HoldBreathMinStamina();
			case 54: return HAM_SpawnConfig.Get().DepotRadius();
			case 57: return s.m_iPatrolGroupsPerLine;
			case 60: return s.m_iVehiclePatrolMax;
			case 65: return s.m_iActiveAiLimit;
			case 82: return s.m_iCivilianGroups;
			case 83: return s.m_iMenMax;
			case 84: return s.m_fMenCostWater;
			case 85: return s.m_fMenCostFood;
			case 86: return s.m_fMenCostAmmo;
			case 87: return HealthScale(true);
			case 88: return HealthScale(false);
			case 89: return HAM_ProfileSettingsApplier.s_iSecond;
			case 90: return HAM_ProfileSettingsApplier.ArsenalModeIndex();
			case 91: return s.m_fLoadRadius;
			case 92: return s.m_fVehicleCapacity;
			case 93: return s.m_fCounterDelayAfterCaptureMinutes;
			case 94: return s.m_fCounterMinFromPlayer;
			case 95: return s.m_fCounterMinFromBase;
			case 96: return s.m_fSaveIntervalMinutes;
			case 97: return s.m_fHeliRadius;
			case 98: return s.m_fHeliAltitudeMin;
			case 99: return s.m_fHeliAltitudeMax;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	static void SetFloat(int id, float value)
	{
		HAM_ProfileSettingsApplier.NoteGmEdit();
		if (id >= 200 && id < 380)
		{
			SetZoneFloat(id, value);
			return;
		}

		if (id >= 400 && id <= 414)
		{
			SetZonePatrolUnits(id, value);
			return;
		}

		HAM_Settings s = HAM_Settings.Get();
		int count = Math.Round(value);
		switch (id)
		{
			case 0: s.m_fActivationDistance = value; break;
			case 1: s.m_fDespawnDistance = value; break;
			case 6: s.m_fCaptureSeconds = value; HAM_MasterBases.ApplyCaptureDuration(); break;
			case 7: s.m_fRetakeSeconds = value; break;
			case 8: s.m_fDepositWaterMax = value; break;
			case 9: s.m_fDepositFoodMax = value; break;
			case 10: s.m_fDepositAmmoMax = value; break;
			case 11: s.m_fDepositRefillPerMinute = value; break;
			case 81: s.m_fDepositRefillInterval = value; break;
			case 12: s.m_iRepStart = count; break;
			case 13: s.m_iRepLocalRevealThreshold = count; break;
			case 14: s.m_iRepCivilianTalkThreshold = count; break;
			case 15: s.m_iRepFrontlinePenalty = count; break;
			case 16: s.m_fIntelBodyChance = value; break;
			case 17: s.m_iIntelPerCell = count; break;
			case 18: s.m_iPatrolMaxActive = count; break;
			case 19: s.m_fPatrolIntervalSeconds = value; break;
			case 20: s.m_fPatrolVehicleShare = value; break;
			case 66: s.m_iCivilianVehicles = count; break;
			case 67: HAM_ProfileSettingsApplier.s_iHour = Math.Clamp(count, 0, 23); HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 68: HAM_ProfileSettingsApplier.s_iMinute = Math.Clamp(count, 0, 59); HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 69: HAM_ProfileSettingsApplier.s_fTimeMultiplier = value; HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 70: HAM_ProfileSettingsApplier.s_iWeatherState = Math.Clamp(count, 0, 3); HAM_ProfileSettingsApplier.ApplyStoredWeather(); break;
			case 71: HAM_ProfileSettingsApplier.s_fWindSpeed = value; HAM_ProfileSettingsApplier.ApplyStoredWeather(); break;
			case 72: HAM_ProfileSettingsApplier.s_iWindDirection = Math.Clamp(count, 0, 3); HAM_ProfileSettingsApplier.ApplyStoredWeather(); break;
			case 73: HAM_ProfileSettingsApplier.s_iKillFeed = Math.Clamp(count, 0, 2); HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 74: HAM_ProfileSettingsApplier.s_iKillReceive = Math.Clamp(count, 0, 4); HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 75: HAM_ProfileSettingsApplier.s_iFriendlyFeed = Math.Clamp(count, 0, 2); HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 76: HAM_ProfileSettingsApplier.s_fSpawnCost = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 77: HAM_ProfileSettingsApplier.s_iYear = count; HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 78: HAM_ProfileSettingsApplier.s_iMonth = Math.Clamp(count, 1, 12); HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 79: HAM_ProfileSettingsApplier.s_iDay = Math.Clamp(count, 1, 31); HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 29: s.m_fMineMinDistance = value; break;
			case 30: s.m_fMineMaxDistance = value; break;
			case 31: s.m_fIedRoadMinWidth = value; break;
			case 34: s.m_fReinforcementMinutes = value; break;
			case 35: s.m_fReinforcementChance = value; break;
			case 36: s.m_iReinforcementUnits = count; break;
			case 37: s.m_fCounterMinutes = value; break;
			case 38: s.m_fCounterChance = value; break;
			case 39: s.m_iCounterGroupUnits = count; break;
			case 40: s.m_iCounterInfantryGroups = count; break;
			case 41: s.m_iCounterVehicles = count; break;
			case 42: s.m_fWaterThreshold = value; break;
			case 43: s.m_fFoodThreshold = value; break;
			case 44: s.m_fAmmoThreshold = value; break;
			case 45: s.m_iMenThreshold = count; break;
			case 46: s.m_fUpkeepWater = value; break;
			case 47: s.m_fUpkeepFood = value; break;
			case 48: s.m_fUpkeepAmmo = value; break;
			case 49: s.m_fGraceMinutes = value; break;
			case 50: s.m_iMenPerCycle = count; break;
			case 80: s.m_fCycleMinutes = value; break;
			case 51: s.m_fExchangeRatio = value; break;
			case 52: HAM_ProfileSettingsApplier.SetHoldBreath(value, HAM_ProfileSettingsApplier.HoldBreathMinStamina(), HAM_ProfileSettingsApplier.HoldBreathSounds()); break;
			case 53: HAM_ProfileSettingsApplier.SetHoldBreath(HAM_ProfileSettingsApplier.HoldBreathMax(), value, HAM_ProfileSettingsApplier.HoldBreathSounds()); break;
			case 54: HAM_SpawnConfig.Get().SetDepotRadius(value); break;
			case 57: s.m_iPatrolGroupsPerLine = count; break;
			case 60: s.m_iVehiclePatrolMax = count; break;
			case 65: s.m_iActiveAiLimit = count; break;
			case 82: s.m_iCivilianGroups = count; break;
			case 83: s.m_iMenMax = count; break;
			case 84: s.m_fMenCostWater = value; break;
			case 85: s.m_fMenCostFood = value; break;
			case 86: s.m_fMenCostAmmo = value; break;
			case 87: SetHealthScale(true, value); break;
			case 88: SetHealthScale(false, value); break;
			case 89: HAM_ProfileSettingsApplier.s_iSecond = Math.Clamp(count, 0, 59); HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 90: HAM_ProfileSettingsApplier.SetArsenalModeIndex(count); break;
			case 91: s.m_fLoadRadius = value; break;
			case 92: s.m_fVehicleCapacity = value; break;
			case 93: s.m_fCounterDelayAfterCaptureMinutes = value; break;
			case 94: s.m_fCounterMinFromPlayer = value; break;
			case 95: s.m_fCounterMinFromBase = value; break;
			case 96: s.m_fSaveIntervalMinutes = value; break;
			case 97: s.m_fHeliRadius = value; break;
			case 98: s.m_fHeliAltitudeMin = value; break;
			case 99: s.m_fHeliAltitudeMax = value; break;
		}

		s.Sanitize();
		if (id == 65)
			HAM_NoLimits.ApplyActiveAiLimit();
	}

	//------------------------------------------------------------------------------------------------
	protected static float ZoneFloat(int id)
	{
		int offset = id - 200;
		int typeIndex = offset / 12;
		int field = offset - typeIndex * 12;
		HAM_ZoneTune tune = HAM_Settings.Get().TuneAt(typeIndex);
		if (!tune)
			return 0;

		switch (field)
		{
			case 0: return tune.m_iUnits;
			case 1: return tune.m_fActivation;
			case 2: return tune.m_fDespawn;
			case 3: return tune.m_iVehicles;
			case 4: return tune.m_iArmor;
			case 5: return tune.m_iBtr;
			case 6: return tune.m_iTruck;
			case 7: return tune.m_iCar;
			case 8: return tune.m_iCivilians;
			case 9: return tune.m_iBuildings;
			case 10: return tune.m_iIeds;
			case 11: return tune.m_iMines;
		}

		return 0;
	}

	protected static void SetZoneFloat(int id, float value)
	{
		int offset = id - 200;
		int typeIndex = offset / 12;
		int field = offset - typeIndex * 12;
		HAM_ZoneTune tune = HAM_Settings.Get().TuneAt(typeIndex);
		if (!tune)
			return;

		int count = Math.Round(value);
		switch (field)
		{
			case 0: tune.m_iUnits = Math.Max(0, count); break;
			case 1: tune.m_fActivation = Math.Max(0, value); break;
			case 2: tune.m_fDespawn = Math.Max(0, value); break;
			case 3: tune.m_iVehicles = Math.Clamp(count, 0, 20); break;
			case 4: tune.m_iArmor = Math.Clamp(count, 0, 100); break;
			case 5: tune.m_iBtr = Math.Clamp(count, 0, 100); break;
			case 6: tune.m_iTruck = Math.Clamp(count, 0, 100); break;
			case 7: tune.m_iCar = Math.Clamp(count, 0, 100); break;
			case 8: tune.m_iCivilians = Math.Clamp(count, 0, 40); break;
			case 9: tune.m_iBuildings = Math.Max(0, count); break;
			case 10: tune.m_iIeds = Math.Clamp(count, 0, 20); break;
			case 11: tune.m_iMines = Math.Clamp(count, 0, 30); break;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static float ZonePatrolUnits(int id)
	{
		HAM_ZoneTune tune = HAM_Settings.Get().TuneAt(id - 400);
		if (!tune)
			return 4;

		return tune.m_iPatrolUnits;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetZonePatrolUnits(int id, float value)
	{
		HAM_ZoneTune tune = HAM_Settings.Get().TuneAt(id - 400);
		if (!tune)
			return;

		int count = Math.Round(value);
		if (count < 0)
			count = 0;

		tune.m_iPatrolUnits = count;
	}

	protected static float HealthScale(bool dot)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return 0;

		SCR_GameModeHealthSettings health = SCR_GameModeHealthSettings.Cast(gameMode.FindComponent(SCR_GameModeHealthSettings));
		if (!health)
			return 0;

		if (dot)
			return health.HAM_GetDOTScale();

		return health.HAM_GetRegenScale();
	}

	protected static void SetHealthScale(bool dot, float value)
	{
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return;

		SCR_GameModeHealthSettings health = SCR_GameModeHealthSettings.Cast(gameMode.FindComponent(SCR_GameModeHealthSettings));
		if (!health)
			return;

		if (dot)
			health.HAM_SetDOTScale(value);
		else
			health.HAM_SetRegenScale(value);
	}

	static bool GetBool(int id)
	{
		HAM_Settings s = HAM_Settings.Get();
		switch (id)
		{
			case 0: return s.m_bDebug;
			case 1: return s.m_bDepositsEnabled;
			case 2: return s.m_bPatrolsEnabled;
			case 3: return s.m_bCqbEnabled;
			case 4: return s.m_bZoneDevicesEnabled;
			case 5: return s.m_bReinforcementsEnabled;
			case 6: return s.m_bCounterEnabled;
			case 7: return HAM_TeleportSettings.PlayerToPlayerEnabled();
			case 8: return HAM_TeleportSettings.BetweenBasesEnabled();
			case 9: return HAM_ProfileSettingsApplier.HoldBreathSounds();
			case 10: return HAM_ProfileSettingsApplier.s_bTimeAdvance;
			case 11: return HAM_ProfileSettingsApplier.s_bWeatherAutomatic;
			case 12: return HAM_ProfileSettingsApplier.s_bWindAutomatic;
			case 13: return HAM_ProfileSettingsApplier.s_bGlobalSupplies;
			case 14: return HAM_ProfileSettingsApplier.s_bRankSupplyLimit;
			case 15: return HAM_ProfileSettingsApplier.s_bMortarBallistics;
			case 16: return HAM_ProfileSettingsApplier.s_bAllowRespawn;
			case 17: return HAM_ProfileSettingsApplier.s_bPunishTeamKill;
			case 18: return HAM_ProfileSettingsApplier.s_bPerceivedDisabled;
			case 19: return HAM_ProfileSettingsApplier.s_bDisguiseNone;
			case 20: return s.m_bHelicoptersEnabled;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void SetBool(int id, bool value)
	{
		HAM_ProfileSettingsApplier.NoteGmEdit();
		HAM_Settings s = HAM_Settings.Get();
		switch (id)
		{
			case 0: s.m_bDebug = value; break;
			case 1: s.m_bDepositsEnabled = value; break;
			case 2: s.m_bPatrolsEnabled = value; break;
			case 3: s.m_bCqbEnabled = value; break;
			case 4: s.m_bZoneDevicesEnabled = value; break;
			case 5: s.m_bReinforcementsEnabled = value; break;
			case 6: s.m_bCounterEnabled = value; break;
			case 7: HAM_TeleportSettings.RequestApply(value, HAM_TeleportSettings.BetweenBasesEnabled()); break;
			case 8: HAM_TeleportSettings.RequestApply(HAM_TeleportSettings.PlayerToPlayerEnabled(), value); break;
			case 9: HAM_ProfileSettingsApplier.SetHoldBreath(HAM_ProfileSettingsApplier.HoldBreathMax(), HAM_ProfileSettingsApplier.HoldBreathMinStamina(), value); break;
			case 10: HAM_ProfileSettingsApplier.s_bTimeAdvance = value; HAM_ProfileSettingsApplier.ApplyStoredTime(); break;
			case 11: HAM_ProfileSettingsApplier.s_bWeatherAutomatic = value; HAM_ProfileSettingsApplier.ApplyStoredWeather(); break;
			case 12: HAM_ProfileSettingsApplier.s_bWindAutomatic = value; HAM_ProfileSettingsApplier.ApplyStoredWeather(); break;
			case 13: HAM_ProfileSettingsApplier.s_bGlobalSupplies = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 14: HAM_ProfileSettingsApplier.s_bRankSupplyLimit = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 15: HAM_ProfileSettingsApplier.s_bMortarBallistics = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 16: HAM_ProfileSettingsApplier.s_bAllowRespawn = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 17: HAM_ProfileSettingsApplier.s_bPunishTeamKill = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 18: HAM_ProfileSettingsApplier.s_bPerceivedDisabled = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 19: HAM_ProfileSettingsApplier.s_bDisguiseNone = value; HAM_ProfileSettingsApplier.ApplyStoredGame(null); break;
			case 20: s.m_bHelicoptersEnabled = value; break;
		}

		HAM_Mission.MarkClientsDirty();
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_FrontFloatAttribute : SCR_BaseEditorAttribute
{
	[Attribute("0")]
	protected int m_iSetting;

	[Attribute()]
	protected ref SCR_EditorAttributeBaseValues m_BaseValues;

	override int GetEntries(notnull array<ref SCR_BaseEditorAttributeEntry> outEntries)
	{
		if (m_BaseValues)
			outEntries.Insert(new SCR_BaseEditorAttributeEntrySlider(m_BaseValues));

		return outEntries.Count();
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item) || !HAM_Mission.IsActive())
			return null;

		return SCR_BaseEditorAttributeVar.CreateFloat(HAM_LiveSettings.GetFloat(m_iSetting));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !SCR_BaseGameMode.Cast(item))
			return;

		HAM_LiveSettings.SetFloat(m_iSetting, var.GetFloat());
	}
}

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_FrontBoolAttribute : SCR_BaseEditorAttribute
{
	[Attribute("0")]
	protected int m_iSetting;

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		if (!SCR_BaseGameMode.Cast(item) || !HAM_Mission.IsActive())
			return null;

		return SCR_BaseEditorAttributeVar.CreateBool(HAM_LiveSettings.GetBool(m_iSetting));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var || !SCR_BaseGameMode.Cast(item))
			return;

		HAM_LiveSettings.SetBool(m_iSetting, var.GetBool());
	}
}

// Une classe par réglage : le gestionnaire d'attributs refuse deux attributs du même type.
[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Debug : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositsEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PatrolsEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HelicoptersEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HeliRadius : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HeliAltMin : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HeliAltMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CqbEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZoneDevicesEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ReinforcementsEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterEnabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_TeleportPlayers : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_TeleportBases : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HoldBreathSounds : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_TimeAdvance : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WeatherAutomatic : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WindAutomatic : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_GlobalSupplies : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RankSupplyLimit : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MortarBallistics : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_AllowRespawn : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PunishTeamKill : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PerceivedDisabled : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DisguiseNone : HAM_FrontBoolAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Hour : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Minute : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_TimeMultiplier : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Year : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Month : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Day : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WeatherState : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WindSpeed : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WindDirection : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_KillFeed : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_KillReceive : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_FriendlyFeed : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_SpawnCost : HAM_FrontFloatAttribute {}


[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ActivationDistance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DespawnDistance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CaptureSeconds : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RetakeSeconds : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositWaterMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositFoodMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositAmmoMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositRefill : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepositRefillInterval : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RepStart : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RepLocalReveal : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RepCivilianTalk : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_RepFrontlinePenalty : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_IntelBodyChance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_IntelPerCell : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PatrolMaxActive : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PatrolInterval : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PatrolVehicleShare : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CivilianVehicles : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MineMinDistance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MineMaxDistance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_IedRoadMinWidth : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ReinforcementMinutes : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ReinforcementChance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ReinforcementUnits : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterMinutes : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterChance : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterGroupUnits : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterGroups : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterVehicles : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_WaterThreshold : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_FoodThreshold : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_AmmoThreshold : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenThreshold : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_UpkeepWater : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_UpkeepFood : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_UpkeepAmmo : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_GraceMinutes : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenPerCycle : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CycleMinutes : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ExchangeRatio : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HoldBreathMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HoldBreathMinStamina : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_DepotRadius : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_PatrolGroupsPerLine : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_VehiclePatrolMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ActiveAiLimit : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_00_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_01_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_02_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_03_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_04_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_05_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_06_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_07_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_08_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_09_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_10_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_11_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_12_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_13_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Zone_14_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_00 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_01 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_02 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_03 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_04 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_05 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_06 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_07 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_08 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_09 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_10 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_11 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_12 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_13 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ZonePatrol_14 : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CivilianMaxGroups : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenMax : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenCostWater : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenCostFood : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_MenCostAmmo : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HealthDot : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_HealthRegen : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_Second : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_ArsenalMode : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_LoadRadius : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_VehicleCapacity : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterDelay : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterMinPlayer : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_CounterMinBase : HAM_FrontFloatAttribute {}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GM_SaveInterval : HAM_FrontFloatAttribute {}
