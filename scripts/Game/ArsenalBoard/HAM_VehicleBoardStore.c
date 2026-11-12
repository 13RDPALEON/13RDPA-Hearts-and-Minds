//------------------------------------------------------------------------------------------------
// Tri + paramétrage des véhicules. Fichier $profile dédié, distinct du tableau objets.
//------------------------------------------------------------------------------------------------

class HAM_VehicleBoardStore
{
	protected static const string FILE = "$profile:13RDPA_HeartsAndMinds/VehicleBoard.json";

	protected static string Dir()
	{
		return HAM_ProfileFiles.ProfilePath("13RDPA_HeartsAndMinds/Trie/Vehicule/");
	}
	protected static const string LOG_PREFIX = "[13RDPA H&M VehicleBoard]";

	protected static bool s_bLoaded;
	protected static ref array<string> s_aHidden;
	protected static ref array<string> s_aCostPrefab;
	protected static ref array<int> s_aCostValue;
	protected static ref array<string> s_aRankPrefab;
	protected static ref array<int> s_aRankValue;
	protected static ref array<string> s_aCargoPrefab;
	protected static ref array<float> s_aCargoValue;
	protected static ref array<SCR_CatalogEntitySpawnerComponent> s_aSpawners;
	protected static ref map<string, string> s_mFile;
	protected static ref map<string, int> s_mDepot;
	protected static ref map<string, int> s_mDirty;
	protected static bool s_bFlushArmed;
	protected static float s_fFlushArmedAt;
	protected static int s_iNextFile;
	protected static bool s_bRefreshQueued;

	static void Start()
	{
		if (s_bLoaded)
			return;

		s_bLoaded = true;
		s_aHidden = {};
		s_aCostPrefab = {};
		s_aCostValue = {};
		s_aRankPrefab = {};
		s_aRankValue = {};
		s_aCargoPrefab = {};
		s_aCargoValue = {};
		if (!s_aSpawners)
			s_aSpawners = {};

		if (!Replication.IsServer())
			return;

		s_mFile = new map<string, string>();
		s_mDepot = new map<string, int>();
		s_mDirty = new map<string, int>();
		s_iNextFile = 1;
		Load();
		EnsureCatalogFiles();
		GetGame().GetCallqueue().CallLater(FlushWatch, 1000, true);
		GetGame().GetCallqueue().CallLater(SelfTest, 6000, false);
		Print(string.Format("%1 chargé — exclus: %2, coûts: %3, ranks: %4, coffres: %5", LOG_PREFIX, s_aHidden.Count(), s_aCostPrefab.Count(), s_aRankPrefab.Count(), s_aCargoPrefab.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static void SelfTest()
	{
		array<string> prefabs = {};
		if (s_aCostPrefab)
			prefabs.InsertAll(s_aCostPrefab);

		if (s_aRankPrefab)
		{
			foreach (string ranked : s_aRankPrefab)
			{
				if (prefabs.Find(ranked) == -1)
					prefabs.Insert(ranked);
			}
		}

		HAM_VehicleBuildTri.LogSelfTest(prefabs);
	}

	//------------------------------------------------------------------------------------------------
	static void RegisterSpawner(SCR_CatalogEntitySpawnerComponent spawner)
	{
		if (!spawner)
			return;

		if (!s_aSpawners)
			s_aSpawners = {};

		if (s_aSpawners.Find(spawner) < 0)
			s_aSpawners.Insert(spawner);
	}

	static void UnregisterSpawner(SCR_CatalogEntitySpawnerComponent spawner)
	{
		if (!s_aSpawners || !spawner)
			return;

		int idx = s_aSpawners.Find(spawner);
		if (idx >= 0)
			s_aSpawners.Remove(idx);
	}

	static void RefreshSpawners()
	{
		if (s_bRefreshQueued)
			return;

		s_bRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(RefreshSpawnersNow, 200, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RefreshSpawnersNow()
	{
		s_bRefreshQueued = false;
		if (s_aSpawners)
		{
			for (int i = s_aSpawners.Count() - 1; i >= 0; i--)
			{
				SCR_CatalogEntitySpawnerComponent spawner = s_aSpawners[i];
				if (spawner)
					spawner.HAM_RefreshCatalog();
				else
					s_aSpawners.Remove(i);
			}
		}

		HAM_VehicleSpawnUI.RefreshIfOpen();
	}

	static bool IsHidden(string prefab)
	{
		Start();
		return IndexOfPath(s_aHidden, prefab) >= 0;
	}

	static bool IsShown(string prefab)
	{
		return !IsHidden(prefab);
	}

	static void SetShown(string prefab, bool shown)
	{
		Start();
		if (prefab.IsEmpty())
			return;

		DropPath(s_aHidden, prefab);
		if (!shown)
			s_aHidden.Insert(prefab);

		if (Replication.IsServer())
		{
			WriteVehicle(prefab);
			BroadcastShown(prefab, shown);
			RefreshSpawners();
		}
	}

	static bool HasCostOverride(string prefab)
	{
		return IndexOfPath(s_aCostPrefab, prefab) >= 0;
	}

	static int GetCostOverride(string prefab, int fallback)
	{
		int idx = IndexOfPath(s_aCostPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aCostValue[idx];
	}

	static int BoardPrice(string prefab)
	{
		int idx = IndexOfPath(s_aCostPrefab, prefab);
		if (idx < 0)
			return 0;

		return s_aCostValue[idx];
	}

	static bool HasRankOverride(string prefab)
	{
		return IndexOfPath(s_aRankPrefab, prefab) >= 0;
	}

	static SCR_ECharacterRank GetRankOverride(string prefab, SCR_ECharacterRank fallback)
	{
		int idx = IndexOfPath(s_aRankPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aRankValue[idx];
	}

	static SCR_ECharacterRank BoardRank(string prefab)
	{
		if (!HasRankOverride(prefab))
			return SCR_ECharacterRank.RENEGADE;

		return GetRankOverride(prefab, SCR_ECharacterRank.RENEGADE);
	}

	static bool HasCapacityOverride(string prefab)
	{
		return IndexOfPath(s_aCargoPrefab, prefab) >= 0;
	}

	static float GetCapacityOverride(string prefab, float fallback)
	{
		int idx = IndexOfPath(s_aCargoPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aCargoValue[idx];
	}

	static void ApplyVehicleParams(string prefab, int cost, int rank, float cargo)
	{
		Start();
		UpsertInt(s_aCostPrefab, s_aCostValue, prefab, cost);
		UpsertInt(s_aRankPrefab, s_aRankValue, prefab, rank);
		if (cargo < 0)
			RemoveFloat(s_aCargoPrefab, s_aCargoValue, prefab);
		else
			UpsertFloat(s_aCargoPrefab, s_aCargoValue, prefab, cargo);
		if (Replication.IsServer())
		{
			WriteVehicle(prefab);
			BroadcastParams();
		}
	}

	static void ClearAllOverrides(string prefab)
	{
		Start();
		RemoveInt(s_aCostPrefab, s_aCostValue, prefab);
		RemoveInt(s_aRankPrefab, s_aRankValue, prefab);
		RemoveFloat(s_aCargoPrefab, s_aCargoValue, prefab);
		if (Replication.IsServer())
		{
			WriteVehicle(prefab);
			BroadcastParams();
		}
	}

	static void SetDepotShown(string prefab, bool shown)
	{
		Start();
		if (prefab.IsEmpty())
			return;

		if (!s_mDepot)
			s_mDepot = new map<string, int>();

		if (shown)
			s_mDepot.Set(prefab, 1);
		else
			s_mDepot.Set(prefab, 0);

		if (Replication.IsServer())
			WriteVehicle(prefab);
	}

	static int DepotFlag(string prefab)
	{
		if (!s_mDepot || prefab.IsEmpty())
			return -1;

		int flag;
		if (!s_mDepot.Find(prefab, flag))
			return -1;

		return flag;
	}

	// applies = true seulement pour la soute du véhicule (pas la boîte à gants, pas un sac dedans).
	static float TrunkDesired(IEntity storageOwner, out bool applies)
	{
		applies = false;
		if (!storageOwner)
			return 0;

		IEntity vehicleEnt;
		IEntity cursor = storageOwner;
		while (cursor)
		{
			if (Vehicle.Cast(cursor))
				vehicleEnt = cursor;

			cursor = cursor.GetParent();
		}

		if (!vehicleEnt || !vehicleEnt.GetPrefabData())
			return 0;

		string prefab = CargoKey(vehicleEnt.GetPrefabData().GetPrefab());
		if (prefab.IsEmpty())
			return 0;

		if (!IsCargoStorage(vehicleEnt, storageOwner))
			return 0;

		applies = true;
		return GetCapacityOverride(prefab, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Une variante sans réglage (M151A2_MERDC) prend la soute du véhicule dont elle hérite (M151A2).
	static string CargoKey(BaseContainer prefab)
	{
		while (prefab)
		{
			string name = prefab.GetResourceName();
			if (!name.IsEmpty() && HasCapacityOverride(name))
				return name;

			prefab = prefab.GetAncestor();
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static string CargoKeyOfPrefab(string prefab)
	{
		if (prefab.IsEmpty())
			return string.Empty;

		if (HasCapacityOverride(prefab))
			return prefab;

		if (HAM_ProfileFiles.IsProfilePath(prefab))
			return string.Empty;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return string.Empty;

		return CargoKey(resource.GetResource().ToBaseContainer());
	}

	static void SyncToPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId < 1)
			return;

		Start();
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (!player)
			return;

		SendHidden(player);
		string costs;
		string ranks;
		string cargos;
		PackParams(costs, ranks, cargos);
		player.HAM_VehicleBoardSyncParams(costs, ranks, cargos);
	}

	static void ApplyHiddenPack(string pack)
	{
		s_bLoaded = true;
		bool reset = true;
		if (pack.IndexOf("*|") == 0)
		{
			reset = false;
			if (pack.Length() > 2)
				pack = pack.Substring(2, pack.Length() - 2);
			else
				pack = string.Empty;
		}
		else if (pack.IndexOf("!|") == 0)
		{
			if (pack.Length() > 2)
				pack = pack.Substring(2, pack.Length() - 2);
			else
				pack = string.Empty;
		}

		if (reset || !s_aHidden)
			s_aHidden = {};

		string rest = pack;
		while (!rest.IsEmpty())
		{
			int cut = rest.IndexOf("|");
			string part = rest;
			if (cut >= 0)
			{
				part = rest.Substring(0, cut);
				rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
			}
			else
			{
				rest = string.Empty;
			}

			part.Trim();
			if (!part.IsEmpty())
				s_aHidden.Insert(part);
		}

		RefreshSpawners();
		HAM_VehicleSpawnUI.RefreshIfOpen();
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyShown(string prefab, bool shown)
	{
		Start();
		if (prefab.IsEmpty())
			return;

		DropPath(s_aHidden, prefab);
		if (!shown)
			s_aHidden.Insert(prefab);

		RefreshSpawners();
		HAM_VehicleSpawnUI.RefreshIfOpen();
	}

	static void ApplyParamsPack(string costs, string ranks, string cargos)
	{
		s_bLoaded = true;
		s_aCostPrefab = {};
		s_aCostValue = {};
		s_aRankPrefab = {};
		s_aRankValue = {};
		s_aCargoPrefab = {};
		s_aCargoValue = {};
		ParseIntPairs(costs, s_aCostPrefab, s_aCostValue);
		ParseIntPairs(ranks, s_aRankPrefab, s_aRankValue);
		ParseFloatPairs(cargos, s_aCargoPrefab, s_aCargoValue);
		HAM_VehicleSpawnUI.RefreshIfOpen();
	}

	protected static bool IsCargoStorage(IEntity vehicleEnt, IEntity storageOwner)
	{
		SCR_UniversalInventoryStorageComponent mine = SCR_UniversalInventoryStorageComponent.Cast(storageOwner.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!mine)
			return false;

		SCR_UniversalInventoryStorageComponent best;
		float bestVol = -1;
		CollectCargo(vehicleEnt, vehicleEnt, best, bestVol);
		return best == mine;
	}

	protected static void CollectCargo(IEntity node, IEntity vehicleEnt, out SCR_UniversalInventoryStorageComponent best, out float bestVol)
	{
		if (!node)
			return;

		bool storedItem = false;
		InventoryItemComponent itemComp = InventoryItemComponent.Cast(node.FindComponent(InventoryItemComponent));
		if (itemComp && itemComp.GetParentSlot())
			storedItem = true;

		if (storedItem)
			return;

		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(node.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (storage)
		{
			float vol = storage.GetMaxVolumeCapacity();
			if (vol > bestVol)
			{
				bestVol = vol;
				best = storage;
			}
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			CollectCargo(child, vehicleEnt, best, bestVol);
			child = child.GetSibling();
		}
	}

	protected static void SendHidden(SCR_PlayerController player)
	{
		if (!player)
			return;

		if (!s_aHidden || s_aHidden.IsEmpty())
		{
			player.HAM_VehicleBoardSyncHidden("!|");
			return;
		}

		string chunk = "!|";
		int count = 0;
		foreach (string prefab : s_aHidden)
		{
			if (prefab.IsEmpty())
				continue;

			if (count >= 20)
			{
				player.HAM_VehicleBoardSyncHidden(chunk);
				chunk = "*|";
				count = 0;
			}

			chunk = chunk + prefab + "|";
			count = count + 1;
		}

		if (count > 0)
			player.HAM_VehicleBoardSyncHidden(chunk);
	}

	protected static void BroadcastShown(string prefab, bool shown)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			SCR_PlayerController player = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
			if (player)
				player.HAM_VehicleBoardSyncShown(prefab, shown);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void BroadcastHidden()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		string pack = PackHidden();
		foreach (int playerId : ids)
		{
			SCR_PlayerController player = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
			if (player)
				player.HAM_VehicleBoardSyncHidden(pack);
		}
	}

	protected static void BroadcastParams()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		string costs;
		string ranks;
		string cargos;
		PackParams(costs, ranks, cargos);
		foreach (int playerId : ids)
		{
			SCR_PlayerController player = SCR_PlayerController.Cast(players.GetPlayerController(playerId));
			if (player)
				player.HAM_VehicleBoardSyncParams(costs, ranks, cargos);
		}
	}

	protected static string PackHidden()
	{
		string outPack = string.Empty;
		if (!s_aHidden)
			return outPack;

		foreach (string prefab : s_aHidden)
			outPack = outPack + prefab + "|";

		return outPack;
	}

	protected static void PackParams(out string costs, out string ranks, out string cargos)
	{
		costs = string.Empty;
		ranks = string.Empty;
		cargos = string.Empty;
		if (s_aCostPrefab)
		{
			for (int i = 0; i < s_aCostPrefab.Count(); i++)
				costs = costs + s_aCostPrefab[i] + "~" + s_aCostValue[i].ToString() + "|";
		}

		if (s_aRankPrefab)
		{
			for (int i = 0; i < s_aRankPrefab.Count(); i++)
				ranks = ranks + s_aRankPrefab[i] + "~" + s_aRankValue[i].ToString() + "|";
		}

		if (s_aCargoPrefab)
		{
			for (int i = 0; i < s_aCargoPrefab.Count(); i++)
				cargos = cargos + s_aCargoPrefab[i] + "~" + s_aCargoValue[i].ToString() + "|";
		}
	}

	protected static void ParseIntPairs(string pack, out array<string> keys, out array<int> values)
	{
		keys = {};
		values = {};
		string rest = pack;
		while (!rest.IsEmpty())
		{
			int cut = rest.IndexOf("|");
			string part = rest;
			if (cut >= 0)
			{
				part = rest.Substring(0, cut);
				rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
			}
			else
			{
				rest = string.Empty;
			}

			int sep = part.IndexOf("~");
			if (sep <= 0)
				continue;

			string key = part.Substring(0, sep);
			string val = part.Substring(sep + 1, part.Length() - sep - 1);
			keys.Insert(key);
			values.Insert(val.ToInt());
		}
	}

	protected static void ParseFloatPairs(string pack, out array<string> keys, out array<float> values)
	{
		keys = {};
		values = {};
		string rest = pack;
		while (!rest.IsEmpty())
		{
			int cut = rest.IndexOf("|");
			string part = rest;
			if (cut >= 0)
			{
				part = rest.Substring(0, cut);
				rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
			}
			else
			{
				rest = string.Empty;
			}

			int sep = part.IndexOf("~");
			if (sep <= 0)
				continue;

			string key = part.Substring(0, sep);
			string val = part.Substring(sep + 1, part.Length() - sep - 1);
			keys.Insert(key);
			values.Insert(val.ToFloat());
		}
	}

	protected static void Load()
	{
		HAM_ProfileFiles.EnsureDir(HAM_ProfileFiles.ProfilePath("13RDPA_HeartsAndMinds/Trie/"));
		HAM_ProfileFiles.EnsureDir(Dir());
		array<string> files = {};
		HAM_ProfileFiles.ListJson(Dir(), files);
		if (!files.IsEmpty())
		{
			foreach (string path : files)
				ReadVehicleFile(path);

			return;
		}

		if (!FileIO.FileExists(FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE))
			return;

		ctx.ReadValue("hidden", s_aHidden);
		ctx.ReadValue("costPrefabs", s_aCostPrefab);
		ctx.ReadValue("costValues", s_aCostValue);
		ctx.ReadValue("rankPrefabs", s_aRankPrefab);
		ctx.ReadValue("rankValues", s_aRankValue);
		ctx.ReadValue("cargoPrefabs", s_aCargoPrefab);
		ctx.ReadValue("cargoValues", s_aCargoValue);
		NormalizeArrays();
		FlushKnown();
	}

	protected static void NormalizeArrays()
	{
		if (!s_aHidden)
			s_aHidden = {};
		if (!s_aCostPrefab)
			s_aCostPrefab = {};
		if (!s_aCostValue)
			s_aCostValue = {};
		if (!s_aRankPrefab)
			s_aRankPrefab = {};
		if (!s_aRankValue)
			s_aRankValue = {};
		if (!s_aCargoPrefab)
			s_aCargoPrefab = {};
		if (!s_aCargoValue)
			s_aCargoValue = {};
	}

	protected static void ReadVehicleFile(string path)
	{
		int nameIndex = HAM_ProfileFiles.IndexInName(path, "Vehicule_");
		if (nameIndex >= s_iNextFile)
			s_iNextFile = nameIndex + 1;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!HAM_ProfileFiles.ReadJson(ctx, path))
			return;

		string prefab;
		if (!ctx.ReadValue("prefab", prefab) || prefab.IsEmpty())
			return;

		int shown = 1;
		int cost = -1;
		int rank = -1;
		float cargo = -1;
		int depot = -1;
		ctx.ReadValue("shown", shown);
		ctx.ReadValue("cost", cost);
		ctx.ReadValue("rank", rank);
		ctx.ReadValue("cargo", cargo);
		ctx.ReadValue("depot", depot);
		if (!s_mFile)
			s_mFile = new map<string, string>();

		s_mFile.Set(prefab, path);
		int index = HAM_ProfileFiles.IndexInName(path, "Vehicule_");
		if (index >= s_iNextFile)
			s_iNextFile = index + 1;

		NormalizeArrays();
		if (shown == 0)
			s_aHidden.Insert(prefab);
		if (cost >= 0)
		{
			s_aCostPrefab.Insert(prefab);
			s_aCostValue.Insert(cost);
		}
		if (rank >= 0)
		{
			s_aRankPrefab.Insert(prefab);
			s_aRankValue.Insert(rank);
		}
		if (cargo >= 0)
		{
			s_aCargoPrefab.Insert(prefab);
			s_aCargoValue.Insert(cargo);
		}
		if (depot >= 0)
		{
			if (!s_mDepot)
				s_mDepot = new map<string, int>();

			s_mDepot.Set(prefab, depot);
			HAM_VehicleDepotFilter.Remember(prefab, depot == 1);
		}
	}

	protected static void FlushKnown()
	{
		map<string, int> seen = new map<string, int>();
		foreach (string prefab : s_aHidden)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteVehicle(prefab);
		}

		foreach (string prefab : s_aCostPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteVehicle(prefab);
		}

		foreach (string prefab : s_aRankPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteVehicle(prefab);
		}

		foreach (string prefab : s_aCargoPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteVehicle(prefab);
		}
	}

	// Un fichier par véhicule du catalogue (shown=1) remplissait la file JSON (1 écriture / 1,5 s).
	// Absent du disque = inclus. Les fichiers déjà écrits restent lus.
	protected static void EnsureCatalogFiles()
	{
		if (!Replication.IsServer())
			return;

		HAM_ProfileFiles.EnsureDir(HAM_ProfileFiles.ProfilePath("13RDPA_HeartsAndMinds/Trie/"));
		HAM_ProfileFiles.EnsureDir(Dir());
	}

	static void FlushPending()
	{
		if (!Replication.IsServer())
			return;

		int guard = 0;
		while (s_mDirty && !s_mDirty.IsEmpty() && guard < 4)
		{
			WriteDirtyBatch(20);
			guard = guard + 1;
		}

		if (s_mDirty && !s_mDirty.IsEmpty())
			ArmFlush();
	}

	protected static void WriteVehicle(string prefab)
	{
		if (!Replication.IsServer() || prefab.IsEmpty())
			return;

		if (!s_mDirty)
			s_mDirty = new map<string, int>();

		s_mDirty.Set(prefab, 1);
		ArmFlush();
	}

	protected static void ArmFlush()
	{
		if (!Replication.IsServer() || s_bFlushArmed)
			return;

		if (!GetGame() || !GetGame().GetCallqueue())
			return;

		s_bFlushArmed = true;
		if (GetGame().GetWorld())
			s_fFlushArmedAt = GetGame().GetWorld().GetWorldTime();

		GetGame().GetCallqueue().CallLater(FlushDirty, 50, false);
	}

	protected static void FlushWatch()
	{
		if (!Replication.IsServer() || !s_mDirty || s_mDirty.IsEmpty())
			return;

		float now = 0;
		if (GetGame() && GetGame().GetWorld())
			now = GetGame().GetWorld().GetWorldTime();

		if (s_bFlushArmed && now - s_fFlushArmedAt < 2000)
			return;

		s_bFlushArmed = false;
		ArmFlush();
	}

	protected static void FlushDirty()
	{
		s_bFlushArmed = false;
		WriteDirtyBatch(20);
		if (s_mDirty && !s_mDirty.IsEmpty())
			ArmFlush();
	}

	protected static void WriteDirtyBatch(int limit)
	{
		if (!s_mDirty || s_mDirty.IsEmpty())
			return;

		array<string> keys = {};
		foreach (string prefab, int flag : s_mDirty)
		{
			if (flag >= 0)
				keys.Insert(prefab);
		}

		int wrote = 0;
		foreach (string prefab : keys)
		{
			if (wrote >= limit)
				break;

			s_mDirty.Remove(prefab);
			WriteVehicleNow(prefab);
			wrote = wrote + 1;
		}
	}

	protected static void WriteVehicleNow(string prefab)
	{
		if (!Replication.IsServer() || prefab.IsEmpty())
			return;

		if (!s_mFile)
			s_mFile = new map<string, string>();

		int shown = 1;
		if (IsHidden(prefab))
			shown = 0;

		int cost = -1;
		int costIdx = IndexOfPath(s_aCostPrefab, prefab);
		if (costIdx >= 0)
			cost = s_aCostValue[costIdx];

		int rank = -1;
		int rankIdx = IndexOfPath(s_aRankPrefab, prefab);
		if (rankIdx >= 0)
			rank = s_aRankValue[rankIdx];

		float cargo = -1;
		int cargoIdx = IndexOfPath(s_aCargoPrefab, prefab);
		if (cargoIdx >= 0)
			cargo = s_aCargoValue[cargoIdx];

		int depot = DepotFlag(prefab);
		string path;
		if (!s_mFile.Find(prefab, path))
		{
			if (shown == 1 && cost < 0 && rank < 0 && cargo < 0 && depot < 0)
				return;

			bool free = false;
			int guard = 0;
			while (guard < 5000)
			{
				path = HAM_ProfileFiles.ItemPath(Dir(), "Vehicule_", s_iNextFile, prefab);
				s_iNextFile = s_iNextFile + 1;
				guard = guard + 1;
				if (!FileIO.FileExists(path))
				{
					free = true;
					break;
				}
			}

			if (!free)
				return;

			s_mFile.Set(prefab, path);
		}

		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("prefab", prefab);
		ctx.WriteValue("shown", shown);
		ctx.WriteValue("cost", cost);
		ctx.WriteValue("rank", rank);
		ctx.WriteValue("cargo", cargo);
		ctx.WriteValue("depot", depot);
		ctx.SaveToFile(path);
	}

	protected static string PathOf(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	protected static int IndexOfPath(array<string> keys, string prefab)
	{
		if (!keys || prefab.IsEmpty())
			return -1;

		int exact = keys.Find(prefab);
		if (exact >= 0)
			return exact;

		string path = PathOf(prefab);
		for (int i = 0; i < keys.Count(); i++)
		{
			if (PathOf(keys[i]) == path)
				return i;
		}

		// Une variante Conflit et son véhicule de base sont le même véhicule au tableau.
		bool variant = IsConflictVariant(prefab);
		string name = VehicleName(prefab);
		for (int j = 0; j < keys.Count(); j++)
		{
			if (IsConflictVariant(keys[j]) != variant && VehicleName(keys[j]) == name)
				return j;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsConflictVariant(string prefab)
	{
		return prefab.Contains("Conflict_Variants/");
	}

	//------------------------------------------------------------------------------------------------
	//! Nom de fichier sans extension ; « _Conflict » retiré pour une variante Conflit.
	protected static string VehicleName(string prefab)
	{
		string path = PathOf(prefab);
		int slash = path.LastIndexOf("/");
		string name = path.Substring(slash + 1, path.Length() - slash - 1);
		name.Replace(".et", "");
		if (IsConflictVariant(prefab))
			name.Replace("_Conflict", "");

		return name;
	}

	protected static void DropPath(array<string> list, string prefab)
	{
		if (!list)
			return;

		string path = PathOf(prefab);
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			if (list[i] == prefab || PathOf(list[i]) == path)
				list.Remove(i);
		}
	}

	protected static void UpsertInt(array<string> keys, array<int> values, string prefab, int value)
	{
		int idx = IndexOfPath(keys, prefab);
		if (idx >= 0)
		{
			keys[idx] = prefab;
			values[idx] = value;
			return;
		}

		keys.Insert(prefab);
		values.Insert(value);
	}

	protected static void UpsertFloat(array<string> keys, array<float> values, string prefab, float value)
	{
		int idx = IndexOfPath(keys, prefab);
		if (idx >= 0)
		{
			keys[idx] = prefab;
			values[idx] = value;
			return;
		}

		keys.Insert(prefab);
		values.Insert(value);
	}

	protected static void RemoveInt(array<string> keys, array<int> values, string prefab)
	{
		int idx = IndexOfPath(keys, prefab);
		if (idx < 0)
			return;

		keys.Remove(idx);
		values.Remove(idx);
	}

	protected static void RemoveFloat(array<string> keys, array<float> values, string prefab)
	{
		int idx = IndexOfPath(keys, prefab);
		if (idx < 0)
			return;

		keys.Remove(idx);
		values.Remove(idx);
	}
}
