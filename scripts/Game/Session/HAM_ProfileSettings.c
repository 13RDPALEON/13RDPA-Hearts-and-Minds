//------------------------------------------------------------------------------------------------
// Applique les blocs des outils portés de Training (milsim, ai, holdBreath, teleport, vehicleDepot,
// health, groupAccess, game, gm) depuis $profile:13RDPA_HeartsAndMinds_Settings.json, en mode H&M seulement.
// Les clés milsim, ai et gm sont celles du profil Conflict 13RDPA (blocs fof, snk_ai et gm).
//------------------------------------------------------------------------------------------------

class HAM_ProfileSettingsApplier
{
	protected static const string LOG_PREFIX = "[13RDPA H&M]";

	protected static bool s_bApplying;
	protected static bool s_bGmEdited;
	protected static float s_fHoldMax = 20;
	protected static float s_fHoldMinStamina = 0;
	protected static bool s_bHoldSounds = true;

	static int s_iHour = 7;
	static int s_iMinute = 0;
	static int s_iSecond = 0;
	static int s_iYear = 1986;
	static int s_iMonth = 11;
	static int s_iDay = 7;
	static bool s_bTimeAdvance = true;
	static float s_fTimeMultiplier = 1;
	static bool s_bWeatherAutomatic = true;
	static int s_iWeatherState = 0;
	static bool s_bWindAutomatic = false;
	static float s_fWindSpeed = 0;
	static int s_iWindDirection = 0;
	static bool s_bGlobalSupplies = true;
	static bool s_bRankSupplyLimit = false;
	static bool s_bMortarBallistics = true;
	static bool s_bAllowRespawn = true;
	static bool s_bPunishTeamKill = true;
	static bool s_bPerceivedDisabled = true;
	static bool s_bDisguiseNone = true;
	static int s_iKillFeed = 0;
	static int s_iKillReceive = 1;
	static int s_iFriendlyFeed = 1;
	static float s_fSpawnCost = 0;

	//------------------------------------------------------------------------------------------------
	static void NoteGmEdit()
	{
		if (s_bApplying)
			return;

		s_bGmEdited = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Second passage : le JSON ne réécrit pas un réglage déjà changé par le maître du jeu.
	static void ApplyLate()
	{
		if (!Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		if (!s_bGmEdited)
		{
			Apply();
			return;
		}

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode)
			return;

		HAM_MilsimSettingsComponent milsim = HAM_MilsimSettingsComponent.Cast(gameMode.FindComponent(HAM_MilsimSettingsComponent));
		if (milsim)
			milsim.HAM_FreezeSupplyRegen();
	}

	//------------------------------------------------------------------------------------------------
	static void Apply()
	{
		if (!Replication.IsServer())
			return;

		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!gameMode || !HAM_Mission.IsActive())
			return;

		s_bApplying = true;

		// Écrit le fichier de réglages s'il manque, avant de le relire ici.
		HAM_Settings.Get();
		HAM_NoLimits.ApplyActiveAiLimit();

		// Tri arsenal et véhicules : leurs propres dossiers dans le profil, indépendants du JSON de réglages.
		HAM_ArsenalBoardStore.Start();
		HAM_VehicleBoardStore.Start();
		HAM_ArsenalBoardBagApply.EnsureListener();

		JsonLoadContext ctx = new JsonLoadContext();
		if (FileIO.FileExists(HAM_Settings.FILE_PATH) && ctx.LoadFromFile(HAM_Settings.FILE_PATH))
		{
			int applied = ApplyHealth(gameMode, ctx);
			applied += ApplyGroupAccess(ctx);
			applied += ApplyTeleport(ctx);
			applied += ApplyVehicleDepot(ctx);
			applied += ApplyMilsim(gameMode, ctx);
			applied += ApplyAI(gameMode, ctx);
			applied += ApplyHoldBreath(ctx);
			applied += ApplyTime(ctx);
			applied += ApplyWeather(ctx);
			applied += ApplyGameRules(gameMode, ctx);
			applied += ApplyGMBudgets(gameMode, ctx);
			Print(string.Format("%1 Réglages appliqués depuis %2 (%3 sections)", LOG_PREFIX, HAM_Settings.FILE_PATH, applied), LogLevel.NORMAL);
		}
		else
		{
			Print(string.Format("%1 Réglages illisibles : %2", LOG_PREFIX, HAM_Settings.FILE_PATH), LogLevel.WARNING);
		}

		HAM_RadioChannels.Start();
		HAM_VehicleDepot.Start();
		s_bApplying = false;
	}

	//------------------------------------------------------------------------------------------------
	//! Réglages que le client applique lui-même : téléport et retenue du souffle.
	static void SyncToPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId < 1)
			return;

		HAM_TeleportSettings.SyncToPlayer(playerId);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (player)
			player.HAM_SyncHoldBreath(s_fHoldMax, s_fHoldMinStamina, s_bHoldSounds);
	}

	//------------------------------------------------------------------------------------------------
	//! SniperOnly reste celui de Configs/HAM_HoldBreath/Settings.conf : il n'est pas dans le JSON.
	static void ApplyHoldBreathValues(float maxHold, float minStamina, bool sounds)
	{
		HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
		if (!config)
			return;

		HAM_HoldBreathConfig.HAM_ApplyRuntime(maxHold, minStamina, sounds, config.SniperOnly);
	}

	//------------------------------------------------------------------------------------------------
	protected static int ApplyHealth(SCR_BaseGameMode gameMode, JsonLoadContext ctx)
	{
		if (!ctx.StartObject("health"))
			return 0;

		SCR_GameModeHealthSettings health = SCR_GameModeHealthSettings.Cast(gameMode.FindComponent(SCR_GameModeHealthSettings));
		if (!health)
		{
			ctx.EndObject();
			Print(string.Format("%1 SCR_GameModeHealthSettings absent", LOG_PREFIX), LogLevel.WARNING);
			return 0;
		}

		bool touched = false;
		float f;

		if (ctx.ReadValue("dotScale", f))
		{
			health.HAM_SetDOTScale(f);
			touched = true;
		}

		if (ctx.ReadValue("regenScale", f))
		{
			health.HAM_SetRegenScale(f);
			touched = true;
		}

		ctx.EndObject();

		if (touched)
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static int ApplyGroupAccess(JsonLoadContext ctx)
	{
		if (HAM_GroupAccessHelper.ApplyJsonGroupAccess(ctx))
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static int ApplyTeleport(JsonLoadContext ctx)
	{
		if (!ctx.StartObject("teleport"))
			return 0;

		bool playerToPlayer = true;
		bool betweenBases = true;
		ctx.ReadValue("playerToPlayer", playerToPlayer);
		ctx.ReadValue("betweenBases", betweenBases);
		ctx.EndObject();

		HAM_TeleportSettings.Apply(playerToPlayer, betweenBases);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	protected static int ApplyVehicleDepot(JsonLoadContext ctx)
	{
		if (HAM_SpawnConfig.Get().ApplyVehicleDepotJson(ctx))
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Plan : nametags, VON, épaves, cadavres, bâtiments, regen des fournitures Conflit, première personne.
	protected static int ApplyMilsim(SCR_BaseGameMode gameMode, JsonLoadContext ctx)
	{
		HAM_MilsimSettingsComponent milsim = HAM_MilsimSettingsComponent.Cast(gameMode.FindComponent(HAM_MilsimSettingsComponent));
		if (!milsim)
		{
			if (ctx.StartObject("milsim"))
				ctx.EndObject();

			Print(string.Format("%1 HAM_MilsimSettingsComponent absent", LOG_PREFIX), LogLevel.WARNING);
			return 0;
		}

		if (!ctx.StartObject("milsim"))
		{
			milsim.HAM_FreezeSupplyRegen();
			return 1;
		}

		bool b;
		float f;

		if (ctx.ReadValue("hideNameTags", b)) milsim.HAM_GMSetHideNameTags(b);
		if (ctx.ReadValue("hideUnconsciousIcon", b)) milsim.HAM_GMSetHideUnconsciousIcon(b);
		if (ctx.ReadValue("nameTagMaxDistance", f)) milsim.HAM_GMSetNameTagMaxDistance(f);
		if (ctx.ReadValue("hideVONNames", b)) milsim.HAM_GMSetHideVONNames(b);
		if (ctx.ReadValue("blockUnconsciousVON", b)) milsim.HAM_GMSetBlockUnconsciousVON(b);
		if (ctx.ReadValue("hideEnemyProxVONUI", b)) milsim.HAM_GMSetHideEnemyProxVONUI(b);
		if (ctx.ReadValue("hideAllProxVONUI", b)) milsim.HAM_GMSetHideAllProxVONUI(b);
		if (ctx.ReadValue("keepWrecks", b)) milsim.HAM_GMSetKeepWrecks(b);
		if (ctx.ReadValue("keepCorpses", b)) milsim.HAM_GMSetKeepCorpses(b);
		if (ctx.ReadValue("indestructibleBuildings", b)) milsim.HAM_GMSetIndestructibleBuildings(b);
		if (ctx.ReadValue("forceFirstPersonFoot", b)) milsim.HAM_GMSetForceFirstPersonFoot(b);
		if (ctx.ReadValue("forceFirstPersonVehicle", b)) milsim.HAM_GMSetForceFirstPersonVehicle(b);
		if (ctx.ReadValue("vehicleVirtualization", b)) milsim.HAM_GMSetVehicleVirtualization(b);
		if (ctx.ReadValue("blockPhotoMode", b)) milsim.HAM_GMSetBlockPhotoMode(b);
		if (ctx.ReadValue("disableVanillaMarkers", b)) milsim.HAM_GMSetDisableVanillaMarkers(b);
		if (ctx.ReadValue("noMinimumRank", b)) milsim.HAM_GMSetNoMinimumRank(b);
		if (ctx.ReadValue("weaponDropChance", f)) milsim.HAM_GMSetWeaponDropChance(f);
		if (ctx.ReadValue("showCompass", b)) milsim.HAM_GMSetShowCompass(b);
		if (ctx.ReadValue("stealthGM", b)) milsim.HAM_GMSetStealthGM(b);

		ctx.EndObject();
		milsim.HAM_FreezeSupplyRegen();
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Plan : perception, skill, distances de tir, style de combat, nuit, carburant.
	protected static int ApplyAI(SCR_BaseGameMode gameMode, JsonLoadContext ctx)
	{
		HAM_AISettingsComponent ai = HAM_AISettingsComponent.Cast(gameMode.FindComponent(HAM_AISettingsComponent));
		if (!ai)
		{
			if (ctx.StartObject("ai"))
				ctx.EndObject();

			Print(string.Format("%1 HAM_AISettingsComponent absent", LOG_PREFIX), LogLevel.WARNING);
			return 0;
		}

		if (!ctx.StartObject("ai"))
			return 0;

		bool b;
		int i;
		float f;
		bool touched = false;

		if (ctx.ReadValue("perceptionFactor", f)) { ai.HAM_GMSetPerceptionFactor(f); touched = true; }
		if (ctx.ReadValue("threatenedPerceptionMul", f)) { ai.HAM_GMSetThreatenedPerceptionMul(f); touched = true; }
		if (ctx.ReadValue("suppressedAudibleDistance", f)) { ai.HAM_GMSetSuppressedAudibleDistance(f); touched = true; }
		if (ctx.ReadValue("aiSkill", i)) { ai.HAM_GMSetAISkill(i); touched = true; }
		if (ctx.ReadValue("aimErrorScale", f)) { ai.HAM_GMSetAimErrorScale(f); touched = true; }
		if (ctx.ReadValue("reactionScale", f)) { ai.HAM_GMSetReactionScale(f); touched = true; }
		if (ctx.ReadValue("engageDelayScale", f)) { ai.HAM_GMSetEngageDelayScale(f); touched = true; }
		if (ctx.ReadValue("maxEngagementDistance", f)) { ai.HAM_GMSetMaxEngagementDistance(f); touched = true; }
		if (ctx.ReadValue("fireAtWillDistance", f)) { ai.HAM_GMSetFireAtWillDistance(f); touched = true; }
		if (ctx.ReadValue("returnFireDistance", f)) { ai.HAM_GMSetReturnFireDistance(f); touched = true; }
		if (ctx.ReadValue("combatStyle", i)) { ai.HAM_GMSetCombatStyle(i); touched = true; }
		if (ctx.ReadValue("nightPerception", f)) { ai.HAM_GMSetNightPerceptionMul(f); touched = true; }
		if (ctx.ReadValue("nvgAuto", b)) { ai.HAM_GMSetNVGAuto(b); touched = true; }
		if (ctx.ReadValue("nvgPerception", f)) { ai.HAM_GMSetNVGPerceptionMul(f); touched = true; }
		if (ctx.ReadValue("fuelConsumption", b)) { ai.HAM_GMSetFuelConsumption(b); touched = true; }
		if (ctx.ReadValue("fuelConsumptionScale", f)) { ai.HAM_GMSetFuelConsumptionScale(f); touched = true; }
		if (ctx.ReadValue("stanceMode", i)) { ai.HAM_GMSetStanceMode(i); touched = true; }
		if (ctx.ReadValue("lookMode", i)) { ai.HAM_GMSetLookMode(i); touched = true; }
		if (ctx.ReadValue("weaponMode", i)) { ai.HAM_GMSetWeaponMode(i); touched = true; }
		if (ctx.ReadValue("fireMode", i)) { ai.HAM_GMSetFireMode(i); touched = true; }
		if (ctx.ReadValue("contactMemory", f)) { ai.HAM_GMSetContactMemory(f); touched = true; }
		if (ctx.ReadValue("attentionDistance", f)) { ai.HAM_GMSetAttentionDistance(f); touched = true; }
		if (ctx.ReadValue("coverSeekOverride", i)) { ai.HAM_GMSetCoverSeekOverride(i); touched = true; }
		if (ctx.ReadValue("contactShareRange", f)) { ai.HAM_GMSetContactShareRange(f); touched = true; }
		if (ctx.ReadValue("finishOffChance", i)) { ai.HAM_GMSetFinishOffChance(i); touched = true; }
		if (ctx.ReadValue("spareUnarmed", b)) { ai.HAM_GMSetSpareUnarmed(b); touched = true; }
		if (ctx.ReadValue("consumeMagazines", b)) { ai.HAM_GMSetConsumeMagazines(b); touched = true; }
		if (ctx.ReadValue("footNavPreset", i)) { ai.HAM_GMSetFootNavPreset(i); touched = true; }
		if (ctx.ReadValue("vehicleNavPreset", i)) { ai.HAM_GMSetVehicleNavPreset(i); touched = true; }
		if (ctx.ReadValue("vehicleCruiseSpeed", f)) { ai.HAM_GMSetVehicleCruiseSpeed(f); touched = true; }
		if (ctx.ReadValue("stayInVehicleMode", i)) { ai.HAM_GMSetStayInVehicleMode(i); touched = true; }

		ctx.EndObject();

		if (touched)
			return 1;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static int ApplyGMBudgets(SCR_BaseGameMode gameMode, JsonLoadContext ctx)
	{
		if (!ctx.StartObject("gm"))
			return 0;

		bool enableBudgets = false;
		ctx.ReadValue("enableBudgets", enableBudgets);
		ctx.EndObject();

		gameMode.HAM_GMBudget_SetBudgetsEnabled(enableBudgets);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	//! Plan : durée, endurance, sons. Appliqué ici puis envoyé à chaque client, qui gère son souffle.
	protected static int ApplyHoldBreath(JsonLoadContext ctx)
	{
		if (!ctx.StartObject("holdBreath"))
			return 0;

		float maxHold = s_fHoldMax;
		float minStamina = s_fHoldMinStamina;
		bool sounds = s_bHoldSounds;
		ctx.ReadValue("maxHoldTime", maxHold);
		ctx.ReadValue("minStamina", minStamina);
		ctx.ReadValue("enableBreathSounds", sounds);
		ctx.EndObject();

		SetHoldBreath(maxHold, minStamina, sounds);
		return 1;
	}

	//------------------------------------------------------------------------------------------------
	static float HoldBreathMax()
	{
		return s_fHoldMax;
	}

	static float HoldBreathMinStamina()
	{
		return s_fHoldMinStamina;
	}

	static bool HoldBreathSounds()
	{
		return s_bHoldSounds;
	}

	//------------------------------------------------------------------------------------------------
	//! Serveur : applique puis envoie à chaque client, qui gère son souffle.
	static void SetHoldBreath(float maxHold, float minStamina, bool sounds)
	{
		s_fHoldMax = maxHold;
		s_fHoldMinStamina = minStamina;
		s_bHoldSounds = sounds;
		ApplyHoldBreathValues(s_fHoldMax, s_fHoldMinStamina, s_bHoldSounds);

		array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
			if (player)
				player.HAM_SyncHoldBreath(s_fHoldMax, s_fHoldMinStamina, s_bHoldSounds);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Respawn, arsenal, fournitures, journal des morts, météo de tir de mortier.
	protected static int ApplyGameRules(SCR_BaseGameMode gameMode, JsonLoadContext ctx)
	{
		if (!ctx.StartObject("game"))
			return 0;

		bool b;
		float f;
		string s;
		if (ctx.ReadValue("allowRespawn", b))
			s_bAllowRespawn = b;

		if (ctx.ReadValue("spawnSupplyCostMultiplier", f))
			s_fSpawnCost = f;

		if (ctx.ReadValue("globalSupplies", b))
			s_bGlobalSupplies = b;

		if (ctx.ReadValue("rankSupplyLimit", b))
			s_bRankSupplyLimit = b;

		if (ctx.ReadValue("showMortarBallistics", b))
			s_bMortarBallistics = b;

		if (ctx.ReadValue("punishTeamKill", b))
			s_bPunishTeamKill = b;

		if (ctx.ReadValue("perceivedFaction", s))
			s_bPerceivedDisabled = s == "DISABLED";

		if (ctx.ReadValue("disguisePunishment", s))
			s_bDisguiseNone = s == "NONE";

		if (ctx.ReadValue("deathNotification", s))
			s_iKillFeed = KillFeedFromText(s);

		if (ctx.ReadValue("notificationSource", s))
			s_iKillReceive = KillReceiveFromText(s);

		if (ctx.ReadValue("friendlyKillFeed", s))
			s_iFriendlyFeed = FriendlyFeedFromText(s);

		if (ctx.ReadValue("arsenalMode", s))
			s_sArsenalMode = s;

		ctx.EndObject();
		ApplyStoredGame(gameMode);
		return 1;
	}

	protected static string s_sArsenalMode = "UNRESTRICTED";

	static int ArsenalModeIndex()
	{
		if (s_sArsenalMode == "CAMPAIGN")
			return 1;

		return 0;
	}

	static void SetArsenalModeIndex(int index)
	{
		if (index == 1)
			s_sArsenalMode = "CAMPAIGN";
		else
			s_sArsenalMode = "UNRESTRICTED";

		ApplyStoredGame(null);
	}

	protected static int ApplyTime(JsonLoadContext ctx)
	{
		if (!ctx.StartObject("time"))
			return 0;

		ctx.ReadValue("hour", s_iHour);
		ctx.ReadValue("minute", s_iMinute);
		ctx.ReadValue("second", s_iSecond);
		ctx.ReadValue("year", s_iYear);
		ctx.ReadValue("month", s_iMonth);
		ctx.ReadValue("day", s_iDay);
		ctx.ReadValue("advance", s_bTimeAdvance);
		ctx.ReadValue("multiplier", s_fTimeMultiplier);
		ctx.EndObject();
		ApplyStoredTime();
		return 1;
	}

	protected static int ApplyWeather(JsonLoadContext ctx)
	{
		if (!ctx.StartObject("weather"))
			return 0;

		string state;
		string direction;
		ctx.ReadValue("automatic", s_bWeatherAutomatic);
		if (ctx.ReadValue("state", state))
			s_iWeatherState = WeatherFromText(state);

		ctx.ReadValue("windAutomatic", s_bWindAutomatic);
		ctx.ReadValue("windSpeed", s_fWindSpeed);
		if (ctx.ReadValue("windDirection", direction))
			s_iWindDirection = WindFromText(direction);

		ctx.EndObject();
		ApplyStoredWeather();
		return 1;
	}

	static void ApplyStoredTime()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return;

		manager.SetDate(s_iYear, s_iMonth, s_iDay);
		manager.SetHoursMinutesSeconds(s_iHour, s_iMinute, s_iSecond);
		if (s_bTimeAdvance && s_fTimeMultiplier > 0)
			manager.SetDayDuration(86400 / s_fTimeMultiplier);
	}

	static void ApplyStoredWeather()
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		if (!manager)
			return;

		string weatherId = "Clear";
		if (s_iWeatherState == 1)
			weatherId = "Cloudy";
		else if (s_iWeatherState == 2)
			weatherId = "Overcast";
		else if (s_iWeatherState == 3)
			weatherId = "Rainy";

		manager.ForceWeatherTo(s_bWeatherAutomatic, weatherId, 0, 0, 0);
		manager.SetCurrentWeatherLooping(s_bWeatherAutomatic);
		if (s_bWindAutomatic)
			return;

		float angle = 0;
		if (s_iWindDirection == 1)
			angle = 90;
		else if (s_iWindDirection == 2)
			angle = 180;
		else if (s_iWindDirection == 3)
			angle = 270;

		manager.DelayedOverrideWindSpeed(s_fWindSpeed, -1);
		manager.DelayedOverrideWindDirection(angle, -1);
		manager.DelayedSetWindOverride(true, -1);
	}

	static void ApplyStoredGame(SCR_BaseGameMode gameMode)
	{
		if (!gameMode)
			gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());

		if (!gameMode)
			return;

		gameMode.SetResourceTypeEnabled(s_bGlobalSupplies, EResourceType.SUPPLIES, -1);

		SCR_RespawnSystemComponent respawn = SCR_RespawnSystemComponent.Cast(gameMode.FindComponent(SCR_RespawnSystemComponent));
		if (respawn)
			respawn.ServerSetEnableRespawn(s_bAllowRespawn);

		SCR_ArsenalManagerComponent arsenalManager;
		if (SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager))
		{
			arsenalManager.SetMilitarySupplyAllocationEnabled(s_bRankSupplyLimit);
			arsenalManager.HAM_SetSpawnSupplyCostMultiplier(s_fSpawnCost);
			if (s_sArsenalMode == "CAMPAIGN")
				arsenalManager.SetArsenalGameModeType_S(SCR_EArsenalGameModeType.CAMPAIGN, -1);
			else
				arsenalManager.SetArsenalGameModeType_S(-1, -1);
		}

		SCR_AdditionalGameModeSettingsComponent additional = SCR_AdditionalGameModeSettingsComponent.Cast(gameMode.FindComponent(SCR_AdditionalGameModeSettingsComponent));
		if (additional)
		{
			additional.SetEnableTeamKillPunishment_S(s_bPunishTeamKill, -1);
			additional.SetProjectileBallisticInfoVisibility_S(s_bMortarBallistics, -1);
		}

		SCR_PerceivedFactionManagerComponent perceived = SCR_PerceivedFactionManagerComponent.Cast(gameMode.FindComponent(SCR_PerceivedFactionManagerComponent));
		if (perceived && s_bPerceivedDisabled)
			perceived.SetCharacterPerceivedFactionOutfitType_S(SCR_EPerceivedFactionOutfitType.DISABLED, -1);

		if (perceived && s_bDisguiseNone)
			perceived.SetPunishmentKillingWhileDisguisedFlags_S(0, -1);

		SCR_NotificationSenderComponent sender = SCR_NotificationSenderComponent.Cast(gameMode.FindComponent(SCR_NotificationSenderComponent));
		if (!sender)
			return;

		EKillFeedType feed = EKillFeedType.DISABLED;
		if (s_iKillFeed == 1)
			feed = EKillFeedType.UNKNOWN_KILLER;
		else if (s_iKillFeed == 2)
			feed = EKillFeedType.FULL;

		sender.SetKillFeedType(feed, -1);

		EKillFeedReceiveType receive = EKillFeedReceiveType.ENEMIES_ONLY;
		if (s_iKillReceive == 0)
			receive = EKillFeedReceiveType.ALL;
		else if (s_iKillReceive == 2)
			receive = EKillFeedReceiveType.ALLIES_ONLY;
		else if (s_iKillReceive == 3)
			receive = EKillFeedReceiveType.SAME_FACTION_ONLY;
		else if (s_iKillReceive == 4)
			receive = EKillFeedReceiveType.GROUP_ONLY;

		sender.SetReceiveKillFeedType(receive, -1);

		SCR_EFriendlyFireKillFeedType friendly = SCR_EFriendlyFireKillFeedType.SHOW_FRIENDLY_KILLS_TO_PLAYERS_INVOLVED;
		if (s_iFriendlyFeed == 0)
			friendly = SCR_EFriendlyFireKillFeedType.DEFAULT_SETTINGS;
		else if (s_iFriendlyFeed == 2)
			friendly = SCR_EFriendlyFireKillFeedType.SHOW_FRIENDLY_KILLS_TO_TEAM;

		sender.SetFriendlyFireKillFeedType(friendly, -1);
	}

	protected static int KillFeedFromText(string text)
	{
		if (text == "FULL")
			return 2;

		if (text == "UNKNOWN_KILLER")
			return 1;

		return 0;
	}

	protected static int KillReceiveFromText(string text)
	{
		if (text == "ALL")
			return 0;

		if (text == "ALLIES_ONLY")
			return 2;

		if (text == "SAME_FACTION_ONLY")
			return 3;

		if (text == "GROUP_ONLY")
			return 4;

		return 1;
	}

	protected static int FriendlyFeedFromText(string text)
	{
		if (text == "DEFAULT" || text == "DEFAULT_SETTINGS")
			return 0;

		if (text == "TEAM")
			return 2;

		return 1;
	}

	protected static int WeatherFromText(string text)
	{
		if (text == "CLOUDY")
			return 1;

		if (text == "OVERCAST")
			return 2;

		if (text == "RAIN" || text == "RAINY")
			return 3;

		return 0;
	}

	protected static int WindFromText(string text)
	{
		if (text == "EAST")
			return 1;

		if (text == "SOUTH")
			return 2;

		if (text == "WEST")
			return 3;

		return 0;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_GameModeHealthSettings
{
	void HAM_SetDOTScale(float scale)
	{
		m_fDOTScale = scale;
	}

	float HAM_GetDOTScale()
	{
		return m_fDOTScale;
	}

	void HAM_SetRegenScale(float scale)
	{
		m_fRegenScale = scale;
	}

	float HAM_GetRegenScale()
	{
		return m_fRegenScale;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_ArsenalManagerComponent
{
	void HAM_SetSpawnSupplyCostMultiplier(float value)
	{
		m_fLoadoutSpawnSupplyCostMultiplier = value;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	void HAM_SyncHoldBreath(float maxHold, float minStamina, bool sounds)
	{
		Rpc(HAM_RpcHoldBreath, maxHold, minStamina, sounds);
		if (Replication.IsServer() && SCR_PlayerController.GetLocalPlayerId() == GetPlayerId())
			HAM_ProfileSettingsApplier.ApplyHoldBreathValues(maxHold, minStamina, sounds);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcHoldBreath(float maxHold, float minStamina, bool sounds)
	{
		HAM_ProfileSettingsApplier.ApplyHoldBreathValues(maxHold, minStamina, sounds);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_BaseGameMode
{
	override void OnGameStart()
	{
		super.OnGameStart();

		if (!Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		GetGame().GetCallqueue().CallLater(HAM_ProfileSettingsApplier.Apply, 3000, false);
		GetGame().GetCallqueue().CallLater(HAM_ProfileSettingsApplier.ApplyLate, 20000, false);
	}

	override void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
	{
		if (Replication.IsServer() && HAM_Mission.IsActive())
		{
			HAM_RadioChannels.Flush(playerId);
			HAM_ParadropComponent.Abandon(playerId);
		}

		super.OnPlayerDisconnected(playerId, cause, timeout);
	}

	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);

		if (!Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		HAM_ArsenalBoardStore.SyncToPlayer(playerId);
		HAM_VehicleBoardStore.SyncToPlayer(playerId);
		GetGame().GetCallqueue().CallLater(HAM_ProfileSettingsApplier.SyncToPlayer, 5000, false, playerId);
	}
}

