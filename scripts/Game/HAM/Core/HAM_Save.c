//------------------------------------------------------------------------------------------------
// Sauvegarde du front : $profile:13RDPA_HeartsAndMinds_Save.json.
// Maîtres et esclaves, stocks, dépôts, réputation, fournitures vanilla, caches de planque,
// IED encore en place (pas les mines AP), constructions posées par les joueurs.
// Les patrouilles et le CQB renaissent à l'approche, sauf si la garnison a été nettoyée.
//------------------------------------------------------------------------------------------------

class HAM_BuiltRecord
{
	string m_sPrefab;
	vector m_vPos;
	float m_fYaw;
	IEntity m_Entity;
}

void HAM_OnBuiltMoved(SCR_EditableEntityComponent editable, vector previous[4])
{
	if (!editable)
		return;

	HAM_Save.FollowMove(editable.GetOwner());
}

class HAM_Save
{
	static const string FILE_PATH = "$profile:13RDPA_HeartsAndMinds_Save.json";
	static const string BACKUP_ROOT = "$profile:SaveHM";
	static const int BACKUP_COUNT = 5;
	static const int VERSION = 2;
	protected static string s_sSavedLine;
	protected static bool s_bWatchMoves;
	protected static string s_FindBuiltPrefab;
	protected static IEntity s_FoundBuilt;
	protected static bool s_bSystemPlace;
	protected static bool s_bRestoringBuilt;
	protected static ref array<ref HAM_BuiltRecord> s_aBuilt;
	protected static ref array<string> s_aCacheNames;
	protected static ref array<int> s_aCacheAlive;
	protected static ref array<vector> s_aCachePos;
	protected static ref array<string> s_aCacheTasks;
	protected static ref array<string> s_aSupplyNames;
	protected static ref array<float> s_aSupplyValues;
	protected static ref array<string> s_aSupplyArmed;
	protected static ref array<string> s_aSupplyStable;
	protected static ref array<string> s_aDepositNames;
	protected static ref array<int> s_aDepositRefillWater;
	protected static ref array<int> s_aDepositRefillFood;
	protected static ref array<int> s_aDepositRefillAmmo;
	protected static ref array<float> s_aDepositWater;
	protected static ref array<float> s_aDepositFood;
	protected static ref array<float> s_aDepositAmmo;
	protected static ref array<int> s_aDepositMen;
	protected static bool s_bDepositsApplied;
	protected static bool s_bBaseSupplyApplied;
	protected static bool s_bBaseStable;
	protected static ref array<string> s_aCampNames;
	protected static ref array<float> s_aCampValues;
	protected static bool s_bCampStable;
	protected static bool s_bSaveRun;
	protected static bool s_bSaveQueued;
	protected static int s_iSavePhase;
	protected static ref array<vector> s_aBasePos;
	protected static ref array<float> s_aBaseValue;
	protected static ref array<float> s_aBaseMax;
	protected static ref array<string> s_aBasePrefab;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_bSystemPlace = false;
		s_bRestoringBuilt = false;
		s_aBuilt = null;
		s_aCacheNames = null;
		s_aCacheAlive = null;
		s_aCachePos = null;
		s_aCacheTasks = null;
		s_aSupplyNames = null;
		s_aSupplyValues = null;
		s_aSupplyArmed = null;
		s_aSupplyStable = null;
		s_aDepositNames = null;
		s_aDepositRefillWater = null;
		s_aDepositRefillFood = null;
		s_aDepositRefillAmmo = null;
		s_aDepositWater = null;
		s_aDepositFood = null;
		s_aDepositAmmo = null;
		s_aDepositMen = null;
		s_bDepositsApplied = false;
		s_bBaseSupplyApplied = false;
		s_bBaseStable = false;
		s_aCampNames = null;
		s_aCampValues = null;
		s_bCampStable = false;
		s_aBasePos = null;
		s_aBaseValue = null;
		s_aBaseMax = null;
		s_aBasePrefab = null;
	}

	//------------------------------------------------------------------------------------------------
	//! Pendant la partie, un enregistrement puis 1,5 s, pour ne pas tout lire dans la même frame.
	//! À la fermeture, tout est écrit tout de suite : la partie s'arrête.
	static void Request(bool immediate)
	{
		if (!Replication.IsServer() || !HAM_Mission.s_aCells)
			return;

		if (immediate)
		{
			HAM_Mission.TrackVanillaSupplies();
			if (s_bBaseStable)
				CaptureBaseSupplies();

			if (s_bCampStable)
				CaptureCampaignSupplies();

			HAM_VehiclePersist.SnapshotAll();
			Save();
			HAM_Mission.NoteSaved();
			return;
		}

		if (s_bSaveRun)
		{
			s_bSaveQueued = true;
			return;
		}

		s_bSaveRun = true;
		s_iSavePhase = 0;
		HAM_VehiclePersist.BeginSnapshot();
		SavePhase();
	}

	//------------------------------------------------------------------------------------------------
	protected static void SavePhase()
	{
		if (s_iSavePhase == 0)
			HAM_Mission.TrackVanillaSupplies();
		else if (s_iSavePhase == 1)
		{
			if (s_bBaseStable)
				CaptureBaseSupplies();
		}
		else if (s_iSavePhase == 2)
		{
			if (s_bCampStable)
				CaptureCampaignSupplies();
		}
		else if (HAM_VehiclePersist.SnapshotOne())
		{
			GetGame().GetCallqueue().CallLater(SavePhase, 1500, false);
			return;
		}
		else
		{
			Save();
			HAM_Mission.NoteSaved();
			s_bSaveRun = false;
			if (s_bSaveQueued)
			{
				s_bSaveQueued = false;
				Request(false);
			}

			return;
		}

		s_iSavePhase++;
		GetGame().GetCallqueue().CallLater(SavePhase, 1500, false);
	}

	//------------------------------------------------------------------------------------------------
	static void Save()
	{
		if (!Replication.IsServer() || !HAM_Mission.s_aCells)
			return;

		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("version", VERSION);
		ctx.WriteValue("map", HAM_Mission.s_Map.m_sKey);
		ctx.WriteValue("globalRep", HAM_Mission.s_fGlobalRep);
		ctx.WriteValue("designatedHq", HAM_Mission.s_sDesignatedHq);
		ctx.WriteValue("triSeeded", HAM_Mission.s_bTriSeeded);

		ctx.StartObject("hq");
		WriteStock(ctx, HAM_Mission.s_HqStock);
		ctx.EndObject();

		array<HAM_Cell> cells = {};
		foreach (HAM_Cell listed : HAM_Mission.s_aCells)
		{
			cells.Insert(listed);
		}

		ctx.WriteValue("cellCount", cells.Count());
		for (int c = 0; c < cells.Count(); c++)
		{
			HAM_Cell cell = cells[c];
			ctx.StartObject("cell" + c.ToString());
			ctx.WriteValue("name", cell.m_sName);
			ctx.WriteValue("revealed", cell.m_bRevealed);
			ctx.WriteValue("localRep", cell.m_iLocalRep);
			ctx.WriteValue("graceLeft", cell.m_fGraceLeft);
			ctx.WriteValue("cycleLeft", cell.m_fCycleLeft);
			// Le temps du monde repart à zéro au chargement : on garde le temps déjà écoulé depuis la prise.
			float captureElapsed = -1;
			if (cell.m_fCapturedAt >= 0)
				captureElapsed = HAM_Util.Now() - cell.m_fCapturedAt;

			ctx.WriteValue("captureElapsed", captureElapsed);
			WriteStock(ctx, cell.m_Stock);
			ctx.EndObject();
		}

		ctx.WriteValue("pointCount", HAM_Mission.s_aPoints.Count());
		for (int p = 0; p < HAM_Mission.s_aPoints.Count(); p++)
		{
			HAM_Point point = HAM_Mission.s_aPoints[p];
			int left = point.m_iEnemyLeft;
			if (point.m_bActive)
				left = HAM_Util.AliveCountAll(point.m_aGroups);

			ctx.StartObject("point" + p.ToString());
			ctx.WriteValue("name", point.m_Def.m_sName);
			ctx.WriteValue("player", point.m_bPlayer);
			ctx.WriteValue("enemyLeft", left);
			float pointElapsed = -1;
			if (point.m_fCapturedAt >= 0)
				pointElapsed = HAM_Util.Now() - point.m_fCapturedAt;

			ctx.WriteValue("captureElapsed", pointElapsed);
			bool wiped = point.m_bGarrisonSpawned && left == 0;
			ctx.WriteValue("wiped", wiped);
			bool hideout = point.m_Def.m_eType == HAM_EMasterType.HIDEOUT;
			ctx.WriteValue("hideout", hideout);
			if (hideout)
			{
				bool cacheAlive = point.m_Cache && !HAM_Util.IsDead(point.m_Cache);
				ctx.WriteValue("cacheAlive", cacheAlive);
				vector cachePos = point.Pos();
				if (point.m_Cache)
					cachePos = point.m_Cache.GetOrigin();

				ctx.WriteValue("cacheX", cachePos[0]);
				ctx.WriteValue("cacheY", cachePos[1]);
				ctx.WriteValue("cacheZ", cachePos[2]);
				// La tâche de cache est temporaire : on ne l'écrit plus. Une vieille clé cacheTask est ignorée à la lecture.
			}

			ctx.EndObject();
		}

		array<string> supplyNames = {};
		array<int> supplyWrites = {};
		foreach (HAM_Point supplyPoint : HAM_Mission.s_aPoints)
		{
			if (!supplyPoint || !supplyPoint.m_Def || supplyPoint.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA)
				continue;

			int supplyValue;
			int supplyMax;
			HAM_Mission.VanillaSupplyStock(supplyPoint.m_Prop, supplyValue, supplyMax);
			float kept;
			bool haveKept = SupplyAmount(supplyPoint.m_Def.m_sName, kept);
			if (haveKept)
				supplyValue = Math.Round(kept);
			else if (supplyMax <= 0)
				continue;

			supplyNames.Insert(supplyPoint.m_Def.m_sName);
			supplyWrites.Insert(supplyValue);
		}

		if (s_bBaseStable)
			CaptureBaseSupplies();

		if (s_bCampStable)
			CaptureCampaignSupplies();

		int baseSupplyCount = 0;
		if (s_aBasePos)
			baseSupplyCount = s_aBasePos.Count();

		ctx.WriteValue("baseSupplyCount", baseSupplyCount);
		for (int baseIndex = 0; baseIndex < baseSupplyCount; baseIndex++)
		{
			ctx.StartObject("baseSupply" + baseIndex.ToString());
			ctx.WriteValue("prefab", s_aBasePrefab[baseIndex]);
			ctx.WriteValue("x", s_aBasePos[baseIndex][0]);
			ctx.WriteValue("y", s_aBasePos[baseIndex][1]);
			ctx.WriteValue("z", s_aBasePos[baseIndex][2]);
			ctx.WriteValue("value", s_aBaseValue[baseIndex]);
			ctx.WriteValue("max", s_aBaseMax[baseIndex]);
			ctx.EndObject();
		}

		int campCount = 0;
		if (s_aCampNames)
			campCount = s_aCampNames.Count();

		ctx.WriteValue("campSupplyCount", campCount);
		for (int campIndex = 0; campIndex < campCount; campIndex++)
		{
			ctx.StartObject("campSupply" + campIndex.ToString());
			ctx.WriteValue("name", s_aCampNames[campIndex]);
			ctx.WriteValue("supplies", s_aCampValues[campIndex]);
			ctx.EndObject();
		}

		ctx.WriteValue("supplyCount", supplyNames.Count());
		for (int s = 0; s < supplyNames.Count(); s++)
		{
			ctx.StartObject("supply" + s.ToString());
			ctx.WriteValue("name", supplyNames[s]);
			ctx.WriteValue("supplies", supplyWrites[s]);
			ctx.EndObject();
		}

		if (!s_aBuilt)
			s_aBuilt = {};

		ctx.WriteValue("builtCount", s_aBuilt.Count());
		for (int b = 0; b < s_aBuilt.Count(); b++)
		{
			HAM_BuiltRecord built = s_aBuilt[b];
			ctx.StartObject("built" + b.ToString());
			ctx.WriteValue("prefab", built.m_sPrefab);
			ctx.WriteValue("x", built.m_vPos[0]);
			ctx.WriteValue("y", built.m_vPos[1]);
			ctx.WriteValue("z", built.m_vPos[2]);
			ctx.WriteValue("yaw", built.m_fYaw);
			ctx.EndObject();
		}

		array<string> clearedLines = {};
		if (HAM_Mission.s_aLines)
		{
			foreach (HAM_Line clearedLine : HAM_Mission.s_aLines)
			{
				if (clearedLine && clearedLine.m_Def && HAM_Garrison.LineStaysClear(clearedLine))
					clearedLines.Insert(clearedLine.m_Def.m_sName);
			}
		}

		ctx.WriteValue("clearedLineCount", clearedLines.Count());
		for (int clearedIndex = 0; clearedIndex < clearedLines.Count(); clearedIndex++)
		{
			ctx.StartObject("clearedLine" + clearedIndex.ToString());
			ctx.WriteValue("name", clearedLines[clearedIndex]);
			ctx.EndObject();
		}

		HAM_Devices.WriteSave(ctx);

		ctx.WriteValue("depositCount", HAM_Mission.s_aDeposits.Count());
		for (int d = 0; d < HAM_Mission.s_aDeposits.Count(); d++)
		{
			HAM_Deposit deposit = HAM_Mission.s_aDeposits[d];
			bool snapWaterOn;
			bool snapFoodOn;
			bool snapAmmoOn;
			float snapWater;
			float snapFood;
			float snapAmmo;
			int snapMen;
			bool useSnap = !DepositsApplied() && DepositSnapshot(deposit.m_Def.m_sName, snapWaterOn, snapFoodOn, snapAmmoOn, snapWater, snapFood, snapAmmo, snapMen);
			ctx.StartObject("deposit" + d.ToString());
			ctx.WriteValue("name", deposit.m_Def.m_sName);
			if (useSnap)
			{
				ctx.WriteValue("refillWater", snapWaterOn);
				ctx.WriteValue("refillFood", snapFoodOn);
				ctx.WriteValue("refillAmmo", snapAmmoOn);
				ctx.WriteValue("water", snapWater);
				ctx.WriteValue("food", snapFood);
				ctx.WriteValue("ammo", snapAmmo);
				ctx.WriteValue("men", snapMen);
			}
			else
			{
				ctx.WriteValue("refillWater", deposit.m_bRefillWater);
				ctx.WriteValue("refillFood", deposit.m_bRefillFood);
				ctx.WriteValue("refillAmmo", deposit.m_bRefillAmmo);
				WriteStock(ctx, deposit.m_Stock);
			}

			ctx.EndObject();
		}

		array<ref HAM_Convoy> convoys = HAM_Convoys.GetConvoys();
		ctx.WriteValue("convoyCount", convoys.Count());
		for (int v = 0; v < convoys.Count(); v++)
		{
			HAM_Convoy convoy = convoys[v];
			ctx.StartObject("convoy" + v.ToString());
			ctx.WriteValue("to", convoy.m_sTo);
			ctx.WriteValue("men", convoy.m_iMen);
			ctx.EndObject();
		}

		HAM_VehiclePersist.WriteSave(ctx);
		HAM_Logistics.WriteSave(ctx);

		s_sSavedLine = string.Format("[13RDPA H&M] Ressources sauvées : dépôts %1, vanilla %2, compteurs de base %3, conteneurs %4", HAM_Mission.s_aDeposits.Count(), supplyNames.Count(), campCount, baseSupplyCount);
		HAM_JsonQueue.AddFrontSave(ctx, FILE_PATH);
	}

	//------------------------------------------------------------------------------------------------
	//! Appelé quand le fichier du front a vraiment été écrit, pas quand il a été mis en file.
	static void OnJsonSaved()
	{
		if (!s_sSavedLine.IsEmpty())
			Print(s_sSavedLine, LogLevel.NORMAL);

		KeepBackup();
	}

	//------------------------------------------------------------------------------------------------
	//! Cinq copies complètes. save1 est la plus récente. À la sixième, seule save5 disparaît.
	protected static void KeepBackup()
	{
		FileIO.MakeDirectory(BACKUP_ROOT);
		int slot = BACKUP_COUNT - 1;
		while (slot >= 1)
		{
			string fromPath = BackupFile(slot);
			string toPath = BackupFile(slot + 1);
			if (FileIO.FileExists(fromPath))
			{
				FileIO.MakeDirectory(BackupFolder(slot + 1));
				if (FileIO.FileExists(toPath))
					FileIO.DeleteFile(toPath);

				FileIO.CopyFile(fromPath, toPath);
			}

			slot = slot - 1;
		}

		string newest = BackupFile(1);
		FileIO.MakeDirectory(BackupFolder(1));
		if (FileIO.FileExists(newest))
			FileIO.DeleteFile(newest);

		if (!FileIO.CopyFile(FILE_PATH, newest))
			Print("[13RDPA H&M] Copie de secours impossible : " + newest, LogLevel.WARNING);
		else
			Print("[13RDPA H&M] Copie de secours : " + newest, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static string BackupFolder(int slot)
	{
		return BACKUP_ROOT + "/save" + slot.ToString();
	}

	//------------------------------------------------------------------------------------------------
	protected static string BackupFile(int slot)
	{
		return BackupFolder(slot) + "/13RDPA_HeartsAndMinds_Save.json";
	}

	//------------------------------------------------------------------------------------------------
	protected static void WriteStock(JsonSaveContext ctx, HAM_Stock stock)
	{
		ctx.WriteValue("water", stock.m_fWater);
		ctx.WriteValue("food", stock.m_fFood);
		ctx.WriteValue("ammo", stock.m_fAmmo);
		ctx.WriteValue("men", stock.m_iMen);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReadStock(JsonLoadContext ctx, HAM_Stock stock)
	{
		float value;
		if (ctx.ReadValue("water", value))
			stock.m_fWater = value;

		if (ctx.ReadValue("food", value))
			stock.m_fFood = value;

		if (ctx.ReadValue("ammo", value))
			stock.m_fAmmo = value;

		int men;
		if (ctx.ReadValue("men", men))
			stock.m_iMen = men;
	}

	//------------------------------------------------------------------------------------------------
	//! Réapplique la sauvegarde sur l'état bâti depuis le script de carte. Les noms absents sont ignorés.
	static void Load()
	{
		s_aCacheNames = {};
		s_aCacheAlive = {};
		s_aCachePos = {};
		s_aCacheTasks = {};
		s_aSupplyNames = {};
		s_aSupplyValues = {};
		s_aSupplyArmed = {};
		s_aSupplyStable = {};
		s_aDepositNames = {};
		s_aDepositRefillWater = {};
		s_aDepositRefillFood = {};
		s_aDepositRefillAmmo = {};
		s_aDepositWater = {};
		s_aDepositFood = {};
		s_aDepositAmmo = {};
		s_aDepositMen = {};
		s_bDepositsApplied = false;
		s_bBaseSupplyApplied = false;
		s_bBaseStable = false;
		s_aCampNames = {};
		s_aCampValues = {};
		s_bCampStable = false;
		s_aBasePos = {};
		s_aBaseValue = {};
		s_aBaseMax = {};
		s_aBasePrefab = {};
		s_aBuilt = {};

		if (!FileIO.FileExists(FILE_PATH))
		{
			s_bBaseSupplyApplied = true;
			s_bBaseStable = true;
			s_bCampStable = true;
			Print("[13RDPA H&M] Pas de sauvegarde du front : nouvelle partie.", LogLevel.NORMAL);
			return;
		}

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE_PATH))
		{
			s_bBaseSupplyApplied = true;
			s_bBaseStable = true;
			s_bCampStable = true;
			Print("[13RDPA H&M] Sauvegarde illisible, nouvelle partie : " + FILE_PATH, LogLevel.WARNING);
			return;
		}

		string mapKey;
		ctx.ReadValue("map", mapKey);
		if (!mapKey.IsEmpty() && mapKey != HAM_Mission.s_Map.m_sKey)
		{
			s_bBaseSupplyApplied = true;
			s_bBaseStable = true;
			s_bCampStable = true;
			Print(string.Format("[13RDPA H&M] Sauvegarde d'une autre carte (%1), ignorée.", mapKey), LogLevel.WARNING);
			return;
		}

		float globalRep;
		if (ctx.ReadValue("globalRep", globalRep))
			HAM_Mission.s_fGlobalRep = globalRep;
		else
		{
			int globalRepWhole;
			if (ctx.ReadValue("globalRep", globalRepWhole))
				HAM_Mission.s_fGlobalRep = globalRepWhole;
		}

		string designated;
		if (ctx.ReadValue("designatedHq", designated) && !designated.IsEmpty())
			HAM_Mission.s_sDesignatedHq = designated;

		bool triSeeded;
		if (ctx.ReadValue("triSeeded", triSeeded))
			HAM_Mission.s_bTriSeeded = triSeeded;

		if (ctx.StartObject("hq"))
		{
			ReadStock(ctx, HAM_Mission.s_HqStock);
			ctx.EndObject();
		}

		int cellCount;
		ctx.ReadValue("cellCount", cellCount);
		for (int c = 0; c < cellCount; c++)
		{
			if (!ctx.StartObject("cell" + c.ToString()))
				continue;

			string cellName;
			ctx.ReadValue("name", cellName);
			HAM_Cell cell = HAM_Mission.FindCell(cellName);
			if (cell)
			{
				bool revealed;
				if (ctx.ReadValue("revealed", revealed))
					cell.m_bRevealed = revealed;

				int localRep;
				if (ctx.ReadValue("localRep", localRep))
					cell.m_iLocalRep = localRep;

				float graceLeft;
				if (ctx.ReadValue("graceLeft", graceLeft))
					cell.m_fGraceLeft = graceLeft;

				float cycleLeft;
				if (ctx.ReadValue("cycleLeft", cycleLeft))
					cell.m_fCycleLeft = cycleLeft;

				float captureElapsed;
				if (ctx.ReadValue("captureElapsed", captureElapsed) && captureElapsed >= 0)
					cell.m_fCapturedAt = HAM_Util.Now() - captureElapsed;

				ReadStock(ctx, cell.m_Stock);
			}

			ctx.EndObject();
		}

		int pointCount;
		ctx.ReadValue("pointCount", pointCount);
		for (int p = 0; p < pointCount; p++)
		{
			if (!ctx.StartObject("point" + p.ToString()))
				continue;

			string pointName;
			ctx.ReadValue("name", pointName);
			HAM_Point point = HAM_Mission.FindPoint(pointName);
			if (point)
			{
				bool player;
				if (ctx.ReadValue("player", player))
					point.m_bPlayer = player;

				int left;
				if (ctx.ReadValue("enemyLeft", left))
					point.m_iEnemyLeft = left;

				float pointElapsed;
				if (ctx.ReadValue("captureElapsed", pointElapsed) && pointElapsed >= 0)
					point.m_fCapturedAt = HAM_Util.Now() - pointElapsed;

				bool wiped;
				if (ctx.ReadValue("wiped", wiped) && wiped)
				{
					point.m_bGarrisonSpawned = true;
					point.m_iEnemyLeft = 0;
					point.m_bZoneSpawned = true;
					point.m_bCqbPlaced = true;
					if (point.IsMaster() && point.m_Cell)
						point.m_Cell.m_bArmorPatrol = true;
				}

				bool hideout;
				if (ctx.ReadValue("hideout", hideout) && hideout)
				{
					bool cacheAlive;
					ctx.ReadValue("cacheAlive", cacheAlive);
					float cacheX;
					float cacheY;
					float cacheZ;
					vector cachePos = point.Pos();
					if (ctx.ReadValue("cacheX", cacheX) && ctx.ReadValue("cacheY", cacheY) && ctx.ReadValue("cacheZ", cacheZ))
						cachePos = Vector(cacheX, cacheY, cacheZ);

					RememberCache(pointName, cacheAlive, cachePos);
					// cacheTask des anciennes sauvegardes ne doit pas rouvrir la tâche.
				}
			}

			ctx.EndObject();
		}

		int supplyCount;
		ctx.ReadValue("supplyCount", supplyCount);
		for (int s = 0; s < supplyCount; s++)
		{
			if (!ctx.StartObject("supply" + s.ToString()))
				continue;

			string supplyName;
			float supplyValue;
			ctx.ReadValue("name", supplyName);
			ctx.ReadValue("supplies", supplyValue);
			ctx.EndObject();
			if (supplyName.IsEmpty())
				continue;

			s_aSupplyNames.Insert(supplyName);
			s_aSupplyValues.Insert(supplyValue);
		}

		int baseSupplyCount;
		ctx.ReadValue("baseSupplyCount", baseSupplyCount);
		for (int baseIndex = 0; baseIndex < baseSupplyCount; baseIndex++)
		{
			if (!ctx.StartObject("baseSupply" + baseIndex.ToString()))
				continue;

			string basePrefab;
			float baseX;
			float baseY;
			float baseZ;
			float baseValue;
			float baseMax;
			ctx.ReadValue("prefab", basePrefab);
			ctx.ReadValue("x", baseX);
			ctx.ReadValue("y", baseY);
			ctx.ReadValue("z", baseZ);
			ctx.ReadValue("value", baseValue);
			ctx.ReadValue("max", baseMax);
			ctx.EndObject();
			s_aBasePrefab.Insert(basePrefab);
			s_aBasePos.Insert(Vector(baseX, baseY, baseZ));
			s_aBaseValue.Insert(baseValue);
			s_aBaseMax.Insert(baseMax);
		}

		int campSupplyCount;
		ctx.ReadValue("campSupplyCount", campSupplyCount);
		for (int campIndex = 0; campIndex < campSupplyCount; campIndex++)
		{
			if (!ctx.StartObject("campSupply" + campIndex.ToString()))
				continue;

			string campName;
			float campValue;
			ctx.ReadValue("name", campName);
			ctx.ReadValue("supplies", campValue);
			ctx.EndObject();
			if (campName.IsEmpty())
				continue;

			s_aCampNames.Insert(campName);
			s_aCampValues.Insert(campValue);
		}

		int builtCount;
		ctx.ReadValue("builtCount", builtCount);
		for (int b = 0; b < builtCount; b++)
		{
			if (!ctx.StartObject("built" + b.ToString()))
				continue;

			HAM_BuiltRecord built = new HAM_BuiltRecord();
			ctx.ReadValue("prefab", built.m_sPrefab);
			float x;
			float y;
			float z;
			float yaw;
			ctx.ReadValue("x", x);
			ctx.ReadValue("y", y);
			ctx.ReadValue("z", z);
			ctx.ReadValue("yaw", yaw);
			built.m_vPos = Vector(x, y, z);
			built.m_fYaw = yaw;
			ctx.EndObject();
			if (!built.m_sPrefab.IsEmpty())
				s_aBuilt.Insert(built);
		}

		int clearedLineCount;
		ctx.ReadValue("clearedLineCount", clearedLineCount);
		for (int clearedIndex = 0; clearedIndex < clearedLineCount; clearedIndex++)
		{
			if (!ctx.StartObject("clearedLine" + clearedIndex.ToString()))
				continue;

			string clearedName;
			ctx.ReadValue("name", clearedName);
			ctx.EndObject();
			if (!HAM_Mission.s_aLines || clearedName.IsEmpty())
				continue;

			foreach (HAM_Line clearedLine : HAM_Mission.s_aLines)
			{
				if (clearedLine && clearedLine.m_Def && clearedLine.m_Def.m_sName == clearedName && clearedLine.m_Def.m_eRole != HAM_ERole.MINES)
					clearedLine.m_bSpawnedOnce = true;
			}
		}

		HAM_Devices.ReadSave(ctx);

		int depositCount;
		ctx.ReadValue("depositCount", depositCount);
		for (int d = 0; d < depositCount; d++)
		{
			if (!ctx.StartObject("deposit" + d.ToString()))
				continue;

			string depositName;
			ctx.ReadValue("name", depositName);
			bool refillWater;
			bool refillFood;
			bool refillAmmo;
			if (!ctx.ReadValue("refillWater", refillWater))
				refillWater = false;

			if (!ctx.ReadValue("refillFood", refillFood))
				refillFood = false;

			if (!ctx.ReadValue("refillAmmo", refillAmmo))
				refillAmmo = false;

			HAM_Stock loaded = new HAM_Stock();
			ReadStock(ctx, loaded);
			RememberDeposit(depositName, refillWater, refillFood, refillAmmo, loaded);
			HAM_Deposit deposit = HAM_Mission.FindDeposit(depositName);
			if (deposit)
				CopyDeposit(deposit, refillWater, refillFood, refillAmmo, loaded);

			ctx.EndObject();
		}

		ApplyLoadedDeposits();

		foreach (HAM_Cell check : HAM_Mission.s_aCells)
		{
			if (check.IsMasterPlayer())
				continue;

			check.m_fGraceLeft = -1;
		}

		int convoyCount;
		ctx.ReadValue("convoyCount", convoyCount);
		for (int v = 0; v < convoyCount; v++)
		{
			if (!ctx.StartObject("convoy" + v.ToString()))
				continue;

			string target;
			int men;
			ctx.ReadValue("to", target);
			ctx.ReadValue("men", men);
			ctx.EndObject();
			GetGame().GetCallqueue().CallLater(HAM_Convoys.CreditPending, 15000, false, target, men);
		}

		HAM_VehiclePersist.ReadSave(ctx);
		HAM_Logistics.ReadSave(ctx);
		float hqWater = 0;
		float hqFood = 0;
		float hqAmmo = 0;
		int hqMen = 0;
		if (HAM_Mission.s_HqStock)
		{
			hqWater = HAM_Mission.s_HqStock.m_fWater;
			hqFood = HAM_Mission.s_HqStock.m_fFood;
			hqAmmo = HAM_Mission.s_HqStock.m_fAmmo;
			hqMen = HAM_Mission.s_HqStock.m_iMen;
		}

		Print(string.Format("[13RDPA H&M] Sauvegarde du front rechargée : %1 — QG eau %2, vivres %3, munitions %4, hommes %5", FILE_PATH, Math.Round(hqWater), Math.Round(hqFood), Math.Round(hqAmmo), hqMen), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	static bool CachePlan(string name, out bool alive, out vector pos)
	{
		alive = false;
		pos = "0 0 0";
		if (!s_aCacheNames)
			return false;

		int index = s_aCacheNames.Find(name);
		if (index < 0)
			return false;

		alive = s_aCacheAlive[index] == 1;
		pos = s_aCachePos[index];
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool CacheTaskOpen(string name)
	{
		return s_aCacheTasks && s_aCacheTasks.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	static bool SupplyAmount(string name, out float value)
	{
		value = 0;
		if (name.IsEmpty() || !s_aSupplyNames)
			return false;

		int index = s_aSupplyNames.Find(name);
		if (index < 0)
			return false;

		value = s_aSupplyValues[index];
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool SupplyArmed(string name)
	{
		return s_aSupplyArmed && s_aSupplyArmed.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	static bool SupplyStable(string name)
	{
		return s_aSupplyStable && s_aSupplyStable.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	static void MarkSupplyStable(string name)
	{
		if (name.IsEmpty())
			return;

		if (!s_aSupplyStable)
			s_aSupplyStable = {};

		if (!s_aSupplyStable.Contains(name))
			s_aSupplyStable.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	static void MarkSupplyArmed(string name)
	{
		if (name.IsEmpty())
			return;

		if (!s_aSupplyArmed)
			s_aSupplyArmed = {};

		if (!s_aSupplyArmed.Contains(name))
			s_aSupplyArmed.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	static void RememberSupply(string name, float value)
	{
		if (name.IsEmpty())
			return;

		if (!s_aSupplyNames)
			s_aSupplyNames = {};

		if (!s_aSupplyValues)
			s_aSupplyValues = {};

		int index = s_aSupplyNames.Find(name);
		if (index < 0)
		{
			s_aSupplyNames.Insert(name);
			s_aSupplyValues.Insert(value);
			return;
		}

		s_aSupplyValues[index] = value;
	}

	//------------------------------------------------------------------------------------------------
	static bool DepositsApplied()
	{
		return s_bDepositsApplied;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberDeposit(string name, bool refillWater, bool refillFood, bool refillAmmo, HAM_Stock stock)
	{
		if (name.IsEmpty() || !stock)
			return;

		if (!s_aDepositNames)
			s_aDepositNames = {};

		if (!s_aDepositRefillWater)
			s_aDepositRefillWater = {};

		if (!s_aDepositRefillFood)
			s_aDepositRefillFood = {};

		if (!s_aDepositRefillAmmo)
			s_aDepositRefillAmmo = {};

		if (!s_aDepositWater)
			s_aDepositWater = {};

		if (!s_aDepositFood)
			s_aDepositFood = {};

		if (!s_aDepositAmmo)
			s_aDepositAmmo = {};

		if (!s_aDepositMen)
			s_aDepositMen = {};

		int waterOn = 0;
		if (refillWater)
			waterOn = 1;

		int foodOn = 0;
		if (refillFood)
			foodOn = 1;

		int ammoOn = 0;
		if (refillAmmo)
			ammoOn = 1;

		int index = s_aDepositNames.Find(name);
		if (index < 0)
		{
			s_aDepositNames.Insert(name);
			s_aDepositRefillWater.Insert(waterOn);
			s_aDepositRefillFood.Insert(foodOn);
			s_aDepositRefillAmmo.Insert(ammoOn);
			s_aDepositWater.Insert(stock.m_fWater);
			s_aDepositFood.Insert(stock.m_fFood);
			s_aDepositAmmo.Insert(stock.m_fAmmo);
			s_aDepositMen.Insert(stock.m_iMen);
			return;
		}

		s_aDepositRefillWater[index] = waterOn;
		s_aDepositRefillFood[index] = foodOn;
		s_aDepositRefillAmmo[index] = ammoOn;
		s_aDepositWater[index] = stock.m_fWater;
		s_aDepositFood[index] = stock.m_fFood;
		s_aDepositAmmo[index] = stock.m_fAmmo;
		s_aDepositMen[index] = stock.m_iMen;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CopyDeposit(HAM_Deposit deposit, bool refillWater, bool refillFood, bool refillAmmo, HAM_Stock stock)
	{
		if (!deposit || !deposit.m_Stock || !stock)
			return;

		deposit.m_bRefillWater = refillWater;
		deposit.m_bRefillFood = refillFood;
		deposit.m_bRefillAmmo = refillAmmo;
		deposit.m_Stock.m_fWater = stock.m_fWater;
		deposit.m_Stock.m_fFood = stock.m_fFood;
		deposit.m_Stock.m_fAmmo = stock.m_fAmmo;
		deposit.m_Stock.m_iMen = stock.m_iMen;
	}

	//------------------------------------------------------------------------------------------------
	static bool DepositSnapshot(string name, out bool refillWater, out bool refillFood, out bool refillAmmo, out float water, out float food, out float ammo, out int men)
	{
		refillWater = false;
		refillFood = false;
		refillAmmo = false;
		water = 0;
		food = 0;
		ammo = 0;
		men = 0;
		if (name.IsEmpty() || !s_aDepositNames)
			return false;

		int index = s_aDepositNames.Find(name);
		if (index < 0)
			return false;

		refillWater = s_aDepositRefillWater[index] == 1;
		refillFood = s_aDepositRefillFood[index] == 1;
		refillAmmo = s_aDepositRefillAmmo[index] == 1;
		water = s_aDepositWater[index];
		food = s_aDepositFood[index];
		ammo = s_aDepositAmmo[index];
		men = s_aDepositMen[index];
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Repose eau, nourriture, munitions et le remplissage lus dans la sauvegarde.
	//! Tant qu'un dépôt nommé n'existe pas, la sauvegarde suivante réécrit ces valeurs, pas le stock neuf.
	static void ApplyLoadedDeposits()
	{
		if (s_bDepositsApplied || !s_aDepositNames)
			return;

		if (!HAM_Mission.s_aDeposits)
			return;

		if (s_aDepositNames.IsEmpty())
		{
			s_bDepositsApplied = true;
			return;
		}

		bool complete = true;
		for (int i = 0; i < s_aDepositNames.Count(); i++)
		{
			HAM_Deposit deposit = HAM_Mission.FindDeposit(s_aDepositNames[i]);
			if (!deposit || !deposit.m_Stock)
			{
				complete = false;
				continue;
			}

			deposit.m_bRefillWater = s_aDepositRefillWater[i] == 1;
			deposit.m_bRefillFood = s_aDepositRefillFood[i] == 1;
			deposit.m_bRefillAmmo = s_aDepositRefillAmmo[i] == 1;
			deposit.m_Stock.m_fWater = s_aDepositWater[i];
			deposit.m_Stock.m_fFood = s_aDepositFood[i];
			deposit.m_Stock.m_fAmmo = s_aDepositAmmo[i];
			deposit.m_Stock.m_iMen = s_aDepositMen[i];
		}

		if (!complete)
			return;

		s_bDepositsApplied = true;
		for (int n = 0; n < s_aDepositNames.Count(); n++)
		{
			string refill = "non";
			if (s_aDepositRefillWater[n] == 1 || s_aDepositRefillFood[n] == 1 || s_aDepositRefillAmmo[n] == 1)
				refill = "oui";

			HAM_Util.Info(string.Format("Dépôt perso %1 rechargé : eau %2, vivres %3, munitions %4, remplissage %5", s_aDepositNames[n], Math.Round(s_aDepositWater[n]), Math.Round(s_aDepositFood[n]), Math.Round(s_aDepositAmmo[n]), refill));
		}
	}

	//------------------------------------------------------------------------------------------------
	static void BeginSystemPlace()
	{
		s_bSystemPlace = true;
	}

	//------------------------------------------------------------------------------------------------
	static void EndSystemPlace()
	{
		GetGame().GetCallqueue().CallLater(EndSystemPlaceFlag, 1500, false);
	}

	//------------------------------------------------------------------------------------------------
	static void EndSystemPlaceFlag()
	{
		s_bSystemPlace = false;
	}

	//------------------------------------------------------------------------------------------------
	static bool SystemPlacing()
	{
		return s_bSystemPlace;
	}

	//------------------------------------------------------------------------------------------------
	static void SpawnBuilt()
	{
		if (!s_aBuilt)
			return;

		s_bRestoringBuilt = true;
		for (int builtIndex = s_aBuilt.Count() - 1; builtIndex >= 0; builtIndex--)
		{
			HAM_BuiltRecord listed = s_aBuilt[builtIndex];
			if (listed && listed.m_sPrefab.Contains("HAM_ArsenalBoard"))
				s_aBuilt.Remove(builtIndex);
		}

		int placed = 0;
		foreach (HAM_BuiltRecord built : s_aBuilt)
		{
			if (!built || built.m_sPrefab.IsEmpty())
				continue;

			if (HAM_MasterBases.SystemPrefabNear(built.m_sPrefab, built.m_vPos, 20))
				continue;

			if (built.m_Entity && !HAM_Util.IsDead(built.m_Entity))
				continue;

			IEntity already = BuiltEntityNear(built.m_sPrefab, built.m_vPos, 4);
			if (!already)
				already = LoneBuiltEntity(built.m_sPrefab);

			if (already)
			{
				ApplyBuiltTransform(built, already);
				HAM_Util.Forget(already);
				continue;
			}

			IEntity spawned = HAM_Util.Spawn(built.m_sPrefab, built.m_vPos, built.m_fYaw);
			if (!spawned)
				continue;

			built.m_Entity = spawned;
			HAM_Util.Forget(spawned);
			placed++;
			SCR_EditorLinkComponent link = SCR_EditorLinkComponent.Cast(spawned.FindComponent(SCR_EditorLinkComponent));
			if (link && !link.IsSpawned())
				link.SpawnComposition();

			string factionKey = string.Empty;
			HAM_Point host = HAM_Mission.NearestPoint(spawned.GetOrigin());
			if (host && vector.DistanceXZ(host.Pos(), spawned.GetOrigin()) < 220)
			{
				if (host.m_bPlayer)
					factionKey = HAM_Units.PLAYER_FACTION;
				else
					factionKey = HAM_Units.ENEMY_FACTION;
			}

			HAM_MasterBases.OpenBuilt(spawned, factionKey);
		}

		HAM_Util.Info(string.Format("Constructions reposées : %1", placed));
		GetGame().GetCallqueue().CallLater(EndRestoringBuilt, 8000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void EndRestoringBuilt()
	{
		s_bRestoringBuilt = false;
	}

	//------------------------------------------------------------------------------------------------
	static void NoteBuilt(IEntity ent)
	{
		if (s_bSystemPlace || s_bRestoringBuilt || !ent || !ent.GetPrefabData())
			return;

		GetGame().GetCallqueue().CallLater(NoteBuiltReady, 400, false, ent);
	}

	//------------------------------------------------------------------------------------------------
	//! Une pièce liée au prefab parent est déjà dans ce prefab. Une pièce posée sur le QG
	//! ou un maître (parent système, pas dans la liste) doit être notée.
	protected static void NoteBuiltReady(IEntity ent)
	{
		if (s_bSystemPlace || s_bRestoringBuilt || !ent || !ent.GetPrefabData())
			return;

		if (HAM_MasterBases.IsSystemProp(ent))
			return;

		IEntity parent = ent.GetParent();
		if (parent)
		{
			// Une pièce du prefab parent est déjà dans ce prefab. Une pièce posée sur la
			// composition système du maître (absente de la liste) doit être notée.
			if (BuiltAlreadyThere(PrefabOf(parent), WorldPos(parent)))
				return;
		}

		string prefab = ent.GetPrefabData().GetPrefabName();
		vector here = WorldPos(ent);
		if (prefab.IsEmpty())
			return;

		// Le tableau est reposé par la mission. Le laisser dans le JSON le dédouble.
		if (prefab.Contains("HAM_ArsenalBoard"))
		{
			HAM_Util.Forget(ent);
			return;
		}

		HAM_BuiltRecord linked = BuiltRecordOf(ent);
		if (linked)
		{
			ApplyBuiltTransform(linked, ent);
			HAM_Util.Forget(ent);
			return;
		}

		HAM_BuiltRecord near = BuiltRecordNear(prefab, here, 4);
		if (!near)
			near = LoneBuiltRecord(prefab);

		if (near)
		{
			ApplyBuiltTransform(near, ent);
			HAM_Util.Forget(ent);
			return;
		}

		if (!s_aBuilt)
			s_aBuilt = {};

		HAM_BuiltRecord built = new HAM_BuiltRecord();
		built.m_sPrefab = prefab;
		ApplyBuiltTransform(built, ent);
		s_aBuilt.Insert(built);
		HAM_Util.Forget(ent);
		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	protected static string PrefabOf(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return string.Empty;

		return ent.GetPrefabData().GetPrefabName();
	}

	//------------------------------------------------------------------------------------------------
	static void ForgetBuilt(IEntity ent)
	{
		if (s_bSystemPlace || s_bRestoringBuilt || !ent || !s_aBuilt)
			return;

		if (HAM_MasterBases.IsSystemProp(ent))
			return;

		string prefab = PrefabOf(ent);
		if (prefab.IsEmpty())
			return;

		for (int i = s_aBuilt.Count() - 1; i >= 0; i--)
		{
			HAM_BuiltRecord linked = s_aBuilt[i];
			if (!linked || linked.m_Entity != ent)
				continue;

			s_aBuilt.Remove(i);
			HAM_Mission.MarkDirty();
			return;
		}

		vector pos = WorldPos(ent);
		for (int i = s_aBuilt.Count() - 1; i >= 0; i--)
		{
			HAM_BuiltRecord built = s_aBuilt[i];
			if (!built || built.m_sPrefab != prefab)
				continue;

			if (vector.DistanceSqXZ(built.m_vPos, pos) > 16)
				continue;

			s_aBuilt.Remove(i);
			HAM_Mission.MarkDirty();
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool BuiltAlreadyThere(string prefab, vector pos)
	{
		return BuiltRecordNear(prefab, pos, 4) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Le Game Master déplace le prefab. Sans ça, la sauvegarde garde l'ancienne pose
	//! et le jeu repose aussi celle d'après : au redémarrage il y en a deux.
	static void WatchMoves()
	{
		if (s_bWatchMoves || !Replication.IsServer())
			return;

		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!core)
			return;

		core.Event_OnEntityTransformChangedServer.Insert(HAM_OnBuiltMoved);
		s_bWatchMoves = true;
	}

	//------------------------------------------------------------------------------------------------
	static void FollowMove(IEntity ent)
	{
		if (s_bSystemPlace || s_bRestoringBuilt || !ent || !s_aBuilt)
			return;

		HAM_BuiltRecord built = BuiltRecordOf(ent);
		if (!built)
			return;

		ApplyBuiltTransform(built, ent);
		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyBuiltTransform(HAM_BuiltRecord built, IEntity ent)
	{
		if (!built || !ent)
			return;

		built.m_Entity = ent;
		built.m_vPos = WorldPos(ent);
		vector transform[4];
		ent.GetWorldTransform(transform);
		vector angles = Math3D.MatrixToAngles(transform);
		built.m_fYaw = angles[0];
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_BuiltRecord BuiltRecordOf(IEntity ent)
	{
		if (!ent || !s_aBuilt)
			return null;

		foreach (HAM_BuiltRecord built : s_aBuilt)
		{
			if (built && built.m_Entity == ent)
				return built;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_BuiltRecord BuiltRecordNear(string prefab, vector pos, float radius)
	{
		if (!s_aBuilt || prefab.IsEmpty())
			return null;

		float limit = radius * radius;
		foreach (HAM_BuiltRecord built : s_aBuilt)
		{
			if (!built || built.m_sPrefab != prefab)
				continue;

			if (built.m_Entity && !HAM_Util.IsDead(built.m_Entity))
				continue;

			if (vector.DistanceSqXZ(built.m_vPos, pos) <= limit)
				return built;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul exemplaire de ce prefab est encore sans entité : c'est celui qui a été déplacé.
	protected static HAM_BuiltRecord LoneBuiltRecord(string prefab)
	{
		if (!s_aBuilt || prefab.IsEmpty())
			return null;

		HAM_BuiltRecord found;
		foreach (HAM_BuiltRecord built : s_aBuilt)
		{
			if (!built || built.m_sPrefab != prefab)
				continue;

			if (built.m_Entity && !HAM_Util.IsDead(built.m_Entity))
				continue;

			if (found)
				return null;

			found = built;
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity BuiltEntityNear(string prefab, vector pos, float radius)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		s_FindBuiltPrefab = prefab;
		s_FoundBuilt = null;
		world.QueryEntitiesBySphere(pos, radius, SeeBuiltEntity);
		return s_FoundBuilt;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeBuiltEntity(IEntity ent)
	{
		if (!SameBuiltPrefab(ent, s_FindBuiltPrefab))
			return true;

		if (BuiltRecordOf(ent))
			return true;

		s_FoundBuilt = ent;
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity LoneBuiltEntity(string prefab)
	{
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		if (!core)
			return null;

		set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
		core.GetAllEntities(entities);
		IEntity found;
		foreach (SCR_EditableEntityComponent editable : entities)
		{
			if (!editable)
				continue;

			IEntity ent = editable.GetOwner();
			if (!SameBuiltPrefab(ent, prefab) || BuiltRecordOf(ent))
				continue;

			if (found)
				return null;

			found = ent;
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SameBuiltPrefab(IEntity ent, string prefab)
	{
		if (!ent || prefab.IsEmpty() || !ent.GetPrefabData())
			return false;

		return ent.GetPrefabData().GetPrefabName() == prefab;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberCache(string name, bool alive, vector pos)
	{
		if (!s_aCacheNames)
			s_aCacheNames = {};

		if (!s_aCacheAlive)
			s_aCacheAlive = {};

		if (!s_aCachePos)
			s_aCachePos = {};

		s_aCacheNames.Insert(name);
		int flag = 0;
		if (alive)
			flag = 1;

		s_aCacheAlive.Insert(flag);
		s_aCachePos.Insert(pos);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberCacheTask(string name)
	{
		if (!s_aCacheTasks)
			s_aCacheTasks = {};

		if (!s_aCacheTasks.Contains(name))
			s_aCacheTasks.Insert(name);
	}

	//------------------------------------------------------------------------------------------------
	protected static vector s_vSupplySeek;
	protected static string s_sSupplySeekPrefab;
	protected static float s_fSupplySeekBest;
	protected static SCR_ResourceContainer s_SupplySeekFound;
	protected static ref array<SCR_ResourceContainer> s_aSupplyClaimed;

	//------------------------------------------------------------------------------------------------
	static void ApplyBaseSupplies(int tries)
	{
		if (s_bBaseSupplyApplied)
			return;

		if (!s_aBasePos || s_aBasePos.IsEmpty())
		{
			s_bBaseSupplyApplied = true;
			s_bBaseStable = true;
			return;
		}

		if (!s_aSupplyClaimed)
			s_aSupplyClaimed = {};

		s_aSupplyClaimed.Clear();
		int restored = 0;
		bool complete = true;
		for (int i = 0; i < s_aBasePos.Count(); i++)
		{
			SCR_ResourceContainer container = FindBaseContainer(s_aBasePos[i], s_aBasePrefab[i]);
			if (!container)
			{
				complete = false;
				continue;
			}

			float maxValue = s_aBaseMax[i];
			if (maxValue > container.GetMaxResourceValue())
				container.SetMaxResourceValue(maxValue, true);

			container.SetResourceValue(s_aBaseValue[i], true);
			s_aSupplyClaimed.Insert(container);
			restored++;
		}

		if (!complete && tries < 8)
		{
			GetGame().GetCallqueue().CallLater(ApplyBaseSupplies, 2000, false, tries + 1);
			return;
		}

		s_bBaseSupplyApplied = true;
		s_aSupplyClaimed.Clear();
		if (restored > 0)
			HAM_Util.Info(string.Format("Stocks vanilla du maître restaurés : %1 conteneurs", restored));

		if (complete)
		{
			GetGame().GetCallqueue().CallLater(ReassertBaseSupplies, 6000, false);
			GetGame().GetCallqueue().CallLater(ReassertBaseSupplies, 14000, false);
			GetGame().GetCallqueue().CallLater(MarkBaseStable, 20000, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void MarkBaseStable()
	{
		s_bBaseStable = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Le prefab remet parfois le plafond après la première pose. On réécrit la valeur sauvée.
	protected static void ReassertBaseSupplies()
	{
		if (!s_aBasePos)
			return;

		for (int i = 0; i < s_aBasePos.Count(); i++)
		{
			SCR_ResourceContainer container = FindBaseContainer(s_aBasePos[i], s_aBasePrefab[i]);
			if (!container)
				continue;

			float maxValue = container.GetMaxResourceValue();
			float saved = s_aBaseValue[i];
			float live = container.GetResourceValue();
			if (maxValue <= 0.01 || saved + 50 >= maxValue)
				continue;

			if (live + 1 < maxValue)
				continue;

			container.SetResourceValue(saved, true);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeBaseContainer(IEntity ent)
	{
		if (!ent)
			return true;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(ent.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return true;

		SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
		if (!container)
			return true;

		if (s_aSupplyClaimed && s_aSupplyClaimed.Find(container) >= 0)
			return true;

		if (!SamePrefabPath(PrefabOf(ent), s_sSupplySeekPrefab))
			return true;

		float dist = vector.DistanceSq(WorldPos(ent), s_vSupplySeek);
		if (s_SupplySeekFound && dist >= s_fSupplySeekBest)
			return true;

		s_fSupplySeekBest = dist;
		s_SupplySeekFound = container;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ResourceContainer FindBaseContainer(vector pos, string prefab)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		s_vSupplySeek = pos;
		s_sSupplySeekPrefab = prefab;
		s_fSupplySeekBest = 0;
		s_SupplySeekFound = null;
		world.QueryEntitiesBySphere(pos, 30, SeeBaseContainer);
		return s_SupplySeekFound;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SamePrefabPath(string a, string b)
	{
		if (a == b)
			return true;

		return PrefabPath(a) == PrefabPath(b);
	}

	//------------------------------------------------------------------------------------------------
	protected static string PrefabPath(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	//------------------------------------------------------------------------------------------------
	protected static vector WorldPos(IEntity ent)
	{
		if (!ent)
			return "0 0 0";

		vector transform[4];
		ent.GetWorldTransform(transform);
		return transform[3];
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureBaseSupplies()
	{
		s_aBasePos = {};
		s_aBaseValue = {};
		s_aBaseMax = {};
		s_aBasePrefab = {};
		if (HAM_Mission.s_HqBase && HAM_Mission.s_HqBase.GetOwner())
			CollectBaseSupplies(HAM_Mission.s_HqBase.GetOwner());

		SCR_CampaignMilitaryBaseComponent designated = HAM_Mission.DesignatedHqBase();
		if (designated && designated.GetOwner() && designated != HAM_Mission.s_HqBase)
			CollectBaseSupplies(designated.GetOwner());

		if (!HAM_Mission.s_aCells)
			return;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || !cell.m_Master || !cell.m_Master.m_bPlayer)
				continue;

			SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), 160);
			if (base)
				CollectBaseSupplies(base.GetOwner());

			CollectPropSupplies(cell.m_Master.m_aTypeProps);
			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (!slave || !slave.m_bPlayer)
					continue;

				if (slave.m_Def.m_eType == HAM_EMasterType.SUPPLY_VANILLA || slave.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
					continue;

				CollectPropSupplies(slave.m_aTypeProps);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectPropSupplies(array<IEntity> props)
	{
		if (!props)
			return;

		foreach (IEntity prop : props)
			CollectBaseSupplies(prop);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsVanillaDepotNode(IEntity node)
	{
		if (!node || !HAM_Mission.s_aPoints)
			return false;

		IEntity root = node.GetRootParent();
		if (!root)
			root = node;

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (!point || !point.m_Def || point.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA)
				continue;

			if (point.m_Prop == node || point.m_Prop == root)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectBaseSupplies(IEntity node)
	{
		if (!node || IsVanillaDepotNode(node))
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container && !AlreadyCaptured(node))
			{
				s_aBasePrefab.Insert(PrefabOf(node));
				s_aBasePos.Insert(WorldPos(node));
				s_aBaseValue.Insert(container.GetResourceValue());
				s_aBaseMax.Insert(container.GetMaxResourceValue());
			}
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			CollectBaseSupplies(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool AlreadyCaptured(IEntity owner)
	{
		if (!owner || !s_aBasePos)
			return false;

		vector pos = WorldPos(owner);
		for (int i = 0; i < s_aBasePos.Count(); i++)
		{
			if (vector.DistanceSq(s_aBasePos[i], pos) > 0.25)
				continue;

			if (SamePrefabPath(s_aBasePrefab[i], PrefabOf(owner)))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberCamp(string name, float value)
	{
		if (name.IsEmpty())
			return;

		if (!s_aCampNames)
			s_aCampNames = {};

		if (!s_aCampValues)
			s_aCampValues = {};

		int index = s_aCampNames.Find(name);
		if (index < 0)
		{
			s_aCampNames.Insert(name);
			s_aCampValues.Insert(value);
			return;
		}

		s_aCampValues[index] = value;
	}

	//------------------------------------------------------------------------------------------------
	//! Compteur vanilla des bases : QG de départ, QG désigné, chaque maître tenu.
	protected static void CaptureCampaignSupplies()
	{
		s_aCampNames = {};
		s_aCampValues = {};
		if (HAM_Mission.s_HqBase)
			RememberCamp("QG", HAM_Mission.s_HqBase.GetSupplies());

		SCR_CampaignMilitaryBaseComponent designated = HAM_Mission.DesignatedHqBase();
		if (designated && designated != HAM_Mission.s_HqBase)
			RememberCamp("qg:" + HAM_Mission.s_sDesignatedHq, designated.GetSupplies());

		if (!HAM_Mission.s_aCells)
			return;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || !cell.m_Master || !cell.m_Master.m_bPlayer)
				continue;

			SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), 160);
			if (!base || base == HAM_Mission.s_HqBase)
				continue;

			RememberCamp("base:" + cell.m_sName, base.GetSupplies());
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyCampaignSupplies(int tries)
	{
		if (s_bCampStable)
			return;

		if (!s_aCampNames || s_aCampNames.IsEmpty())
		{
			s_bCampStable = true;
			return;
		}

		bool complete = true;
		for (int i = 0; i < s_aCampNames.Count(); i++)
		{
			SCR_CampaignMilitaryBaseComponent base = CampBase(s_aCampNames[i]);
			if (!base)
			{
				complete = false;
				continue;
			}

			float delta = s_aCampValues[i] - base.GetSupplies();
			if (delta > 1 || delta < -1)
				base.AddSupplies(delta);
		}

		if (!complete && tries < 8)
		{
			GetGame().GetCallqueue().CallLater(ApplyCampaignSupplies, 2000, false, tries + 1);
			return;
		}

		if (complete)
			GetGame().GetCallqueue().CallLater(MarkCampStable, 20000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void MarkCampStable()
	{
		s_bCampStable = true;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_CampaignMilitaryBaseComponent CampBase(string name)
	{
		if (name == "QG")
			return HAM_Mission.s_HqBase;

		if (name.IndexOf("qg:") == 0)
			return HAM_Mission.DesignatedHqBase();

		if (name.IndexOf("base:") != 0)
			return null;

		string cellName = name.Substring(5, name.Length() - 5);
		HAM_Cell cell = HAM_Mission.FindCell(cellName);
		if (!cell)
			return null;

		return HAM_Mission.FindBaseNear(cell.Pos(), 160);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingManagerComponent
{
	override void RegisterComposition(notnull SCR_CampaignBuildingCompositionComponent composition)
	{
		super.RegisterComposition(composition);
		if (composition)
		{
			IEntity owner = composition.GetOwner();
			HAM_Save.NoteBuilt(owner);
			HAM_Hospital.Note(owner);
		}
	}

	override void UnregisterComposition(notnull SCR_CampaignBuildingCompositionComponent composition)
	{
		super.UnregisterComposition(composition);
		if (composition)
		{
			IEntity owner = composition.GetOwner();
			HAM_Save.ForgetBuilt(owner);
			HAM_Hospital.Forget(owner);
		}
	}
}
