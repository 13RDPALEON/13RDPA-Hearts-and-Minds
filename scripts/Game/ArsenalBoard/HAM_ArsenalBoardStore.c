//------------------------------------------------------------------------------------------------
// Tri + paramétrage arsenal Training — persistance $profile + helpers staff.
//------------------------------------------------------------------------------------------------

class HAM_ArsenalBoardStore
{
	protected static const string FILE = "$profile:13RDPA_HeartsAndMinds/ArsenalBoard.json";

	protected static string Dir()
	{
		return HAM_ProfileFiles.ProfilePath("13RDPA_HeartsAndMinds/Trie/Item/");
	}
	protected static const string LOG_PREFIX = "[13RDPA H&M ArsenalBoard]";

	protected static bool s_bLoaded;
	protected static bool s_bPauseFilter;
	protected static ref array<string> s_aHidden;
	protected static ref array<string> s_aCostPrefab;
	protected static ref array<int> s_aCostValue;
	protected static ref array<string> s_aRankPrefab;
	protected static ref array<int> s_aRankValue;
	protected static ref array<string> s_aBagPrefab;
	protected static ref array<float> s_aBagValue;
	protected static ref array<SCR_ArsenalComponent> s_aArsenals;
	protected static ref map<string, string> s_mFile;
	protected static ref map<string, int> s_mHidden;
	protected static int s_iNextFile;
	protected static int s_iRefreshIndex;
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
		s_aBagPrefab = {};
		s_aBagValue = {};
		s_aArsenals = {};

		if (!Replication.IsServer())
			return;

		s_mFile = new map<string, string>();
		s_iNextFile = 1;
		Load();
		GetGame().GetCallqueue().CallLater(EnsureCatalogFiles, 4000, false);
		Print(string.Format("%1 chargé — exclus: %2, coûts: %3, ranks: %4, sacs: %5", LOG_PREFIX, s_aHidden.Count(), s_aCostPrefab.Count(), s_aRankPrefab.Count(), s_aBagPrefab.Count()), LogLevel.NORMAL);
		GetGame().GetCallqueue().CallLater(RefreshAllArsenals, 1500, false);
		HAM_ArsenalPrices.PushAll();
		GetGame().GetCallqueue().CallLater(HAM_ArsenalPrices.PushAll, 4000, false);
	}

	static bool IsStaff(int playerId)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager || playerId <= 0)
			return false;

		if (manager.HasPlayerRole(playerId, EPlayerRole.ADMINISTRATOR))
			return true;

		if (manager.HasPlayerRole(playerId, EPlayerRole.SESSION_ADMINISTRATOR))
			return true;

		return manager.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER);
	}

	static void RegisterArsenal(SCR_ArsenalComponent arsenal)
	{
		if (!arsenal)
			return;

		if (!s_aArsenals)
			s_aArsenals = {};

		if (s_aArsenals.Find(arsenal) < 0)
			s_aArsenals.Insert(arsenal);
	}

	static void UnregisterArsenal(SCR_ArsenalComponent arsenal)
	{
		if (!s_aArsenals || !arsenal)
			return;

		int idx = s_aArsenals.Find(arsenal);
		if (idx >= 0)
			s_aArsenals.Remove(idx);
	}

	static void RefreshAllArsenals()
	{
		HAM_ArsenalPreview.ForgetList();
		if (s_bRefreshQueued)
			return;

		s_bRefreshQueued = true;
		GetGame().GetCallqueue().CallLater(BeginArsenalRefresh, 200, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Une caisse à la fois. Tout rafraîchir dans le clic dépassait le temps du script :
	//! le tri s'arrêtait, et la caisse suivante (médicale comprise) ne répondait plus.
	protected static void BeginArsenalRefresh()
	{
		s_bRefreshQueued = false;
		if (!s_aArsenals || s_aArsenals.IsEmpty())
			return;

		s_iRefreshIndex = s_aArsenals.Count() - 1;
		RefreshNextArsenal();
	}

	//------------------------------------------------------------------------------------------------
	protected static void RefreshNextArsenal()
	{
		if (!s_aArsenals || s_iRefreshIndex < 0)
			return;

		int index = s_iRefreshIndex;
		s_iRefreshIndex = s_iRefreshIndex - 1;
		if (index >= s_aArsenals.Count())
		{
			RefreshNextArsenal();
			return;
		}

		SCR_ArsenalComponent arsenal = s_aArsenals[index];
		if (!arsenal || !arsenal.GetOwner())
			s_aArsenals.Remove(index);
		else
			arsenal.RefreshArsenal(false);

		if (s_iRefreshIndex >= 0)
			GetGame().GetCallqueue().CallLater(RefreshNextArsenal, 150, false);
	}

	static bool IsHidden(string prefab)
	{
		Start();
		if (!s_mHidden || prefab.IsEmpty())
			return false;

		return s_mHidden.Contains(PrefabPath(prefab));
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

		DropString(s_aHidden, prefab);
		if (!shown)
			s_aHidden.Insert(prefab);

		RebuildHiddenMap();
		if (Replication.IsServer())
		{
			WriteItem(prefab);
			RefreshAllArsenals();
			BroadcastShown(prefab, shown);
		}
	}

	static bool HasCostOverride(string prefab)
	{
		return IndexOf(s_aCostPrefab, prefab) >= 0;
	}

	static int GetCostOverride(string prefab, int fallback)
	{
		int idx = IndexOf(s_aCostPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aCostValue[idx];
	}

	// Un seul argument : depuis la classe d'inventaire moddée, un 2e argument int est remplacé par le composant.
	static int BoardPrice(string prefab)
	{
		int idx = IndexOf(s_aCostPrefab, prefab);
		if (idx < 0)
			return 0;

		return s_aCostValue[idx];
	}

	static bool HasRankOverride(string prefab)
	{
		return IndexOf(s_aRankPrefab, prefab) >= 0;
	}

	static SCR_ECharacterRank GetRankOverride(string prefab, SCR_ECharacterRank fallback)
	{
		int idx = IndexOf(s_aRankPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aRankValue[idx];
	}

	// Un seul argument : même contrainte que BoardPrice depuis la classe d'inventaire moddée.
	static SCR_ECharacterRank BoardRank(string prefab)
	{
		if (!HasRankOverride(prefab))
			return SCR_ECharacterRank.RENEGADE;

		return GetRankOverride(prefab, SCR_ECharacterRank.RENEGADE);
	}

	static bool HasBagOverride(string prefab)
	{
		return IndexOf(s_aBagPrefab, prefab) >= 0;
	}

	static float GetBagOverride(string prefab, float fallback)
	{
		int idx = IndexOf(s_aBagPrefab, prefab);
		if (idx < 0)
			return fallback;

		return s_aBagValue[idx];
	}

	static void ClearAllOverrides(string prefab)
	{
		Start();
		RemovePair(s_aCostPrefab, s_aCostValue, prefab);
		RemovePair(s_aRankPrefab, s_aRankValue, prefab);
		RemoveFloatPair(s_aBagPrefab, s_aBagValue, prefab);
		if (Replication.IsServer())
		{
			WriteItem(prefab);
			BroadcastParams();
			RefreshAllArsenals();
			HAM_ArsenalPrices.PushOne(prefab);
		}
	}

	static void ApplyItemParams(string prefab, int cost, int rank, float bagVolume, bool setBag)
	{
		Start();
		UpsertInt(s_aCostPrefab, s_aCostValue, prefab, cost);
		UpsertInt(s_aRankPrefab, s_aRankValue, prefab, rank);
		if (setBag)
			UpsertFloat(s_aBagPrefab, s_aBagValue, prefab, bagVolume);
		else
			RemoveFloatPair(s_aBagPrefab, s_aBagValue, prefab);

		if (Replication.IsServer())
		{
			WriteItem(prefab);
			BroadcastParams();
			RefreshAllArsenals();
			RefreshCarriedBags();
			HAM_ArsenalPrices.PushOne(prefab);
		}
	}

	static bool IsBackpackItem(SCR_ArsenalItem item)
	{
		if (!item)
			return false;

		return HAM_ArsenalCatalog.IsCarrierPrefab(item.GetItemResourceName());
	}

	static void ApplyBagVolumeToEntity(IEntity ent)
	{
		if (!ent || !Replication.IsServer())
			return;

		EntityPrefabData data = ent.GetPrefabData();
		if (!data)
			return;

		float bonus = BagBonusFor(data.GetPrefabName());
		if (bonus <= 0)
			return;

		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(ent.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (!storage)
			return;

		storage.HAM_SetDesiredVolume(bonus);
	}

	static float DesiredBagVolumeForEntity(IEntity ent)
	{
		IEntity cursor = ent;
		while (cursor)
		{
			EntityPrefabData data = cursor.GetPrefabData();
			if (data)
			{
				float desired = BagBonusFor(data.GetPrefabName());
				if (desired > 0)
					return desired;
			}

			cursor = cursor.GetParent();
		}

		return 0;
	}

	static void RefreshCarriedBags()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			IEntity player = players.GetPlayerControlledEntity(playerId);
			RefreshBagsOn(player);
		}
	}

	protected static void RefreshBagsOn(IEntity ent)
	{
		if (!ent)
			return;

		SCR_UniversalInventoryStorageComponent storage = SCR_UniversalInventoryStorageComponent.Cast(ent.FindComponent(SCR_UniversalInventoryStorageComponent));
		if (storage)
			storage.HAM_ApplySavedVolume();

		IEntity child = ent.GetChildren();
		while (child)
		{
			RefreshBagsOn(child);
			child = child.GetSibling();
		}
	}

	protected static float BagBonusFor(string prefab)
	{
		if (HasBagOverride(prefab))
			return GetBagOverride(prefab, 0);

		string path = PrefabPath(prefab);
		if (!s_aBagPrefab)
			return 0;

		for (int i = 0; i < s_aBagPrefab.Count(); i++)
		{
			if (PrefabPath(s_aBagPrefab[i]) == path)
				return s_aBagValue[i];
		}

		return 0;
	}

	protected static string PrefabPath(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	static void SetFilterPaused(bool paused)
	{
		s_bPauseFilter = paused;
	}

	static void FilterList(inout array<SCR_ArsenalItem> items)
	{
		if (s_bPauseFilter)
			return;

		Start();
		if (!items || !s_aHidden || s_aHidden.IsEmpty())
			return;

		for (int i = items.Count() - 1; i >= 0; i--)
		{
			SCR_ArsenalItem item = items[i];
			if (!item)
			{
				items.Remove(i);
				continue;
			}

			string prefab = item.GetItemResourceName();
			if (prefab.Contains("/PersonalBelongings/") || IsHidden(prefab))
				items.Remove(i);
		}
	}

	static void ApplyShown(string prefab, bool shown)
	{
		Start();
		if (prefab.IsEmpty())
			return;

		DropString(s_aHidden, prefab);
		if (!shown)
			s_aHidden.Insert(prefab);

		RebuildHiddenMap();
		RefreshAllArsenals();
	}

	//------------------------------------------------------------------------------------------------
	protected static void RebuildHiddenMap()
	{
		s_mHidden = new map<string, int>();
		if (!s_aHidden)
			return;

		foreach (string prefab : s_aHidden)
		{
			if (!prefab.IsEmpty())
				s_mHidden.Set(PrefabPath(prefab), 1);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void BroadcastShown(string prefab, bool shown)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			PlayerController controller = players.GetPlayerController(playerId);
			SCR_PlayerController player = SCR_PlayerController.Cast(controller);
			if (player)
				player.HAM_ArsenalBoardSyncShown(prefab, shown);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void BroadcastFilter()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		string pack = PackHidden();
		foreach (int playerId : ids)
		{
			PlayerController controller = players.GetPlayerController(playerId);
			SCR_PlayerController player = SCR_PlayerController.Cast(controller);
			if (player)
				player.HAM_ArsenalBoardSyncHidden(pack);
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
		string bags;
		PackParams(costs, ranks, bags);
		foreach (int playerId : ids)
		{
			PlayerController controller = players.GetPlayerController(playerId);
			SCR_PlayerController player = SCR_PlayerController.Cast(controller);
			if (player)
				player.HAM_ArsenalBoardSyncParams(costs, ranks, bags);
		}
	}

	static void SyncToPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId < 1)
			return;

		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (!player)
			return;

		player.HAM_ArsenalBoardSyncHidden(PackHidden());
		string costs;
		string ranks;
		string bags;
		PackParams(costs, ranks, bags);
		player.HAM_ArsenalBoardSyncParams(costs, ranks, bags);
	}

	static void ApplyHiddenPack(string pack)
	{
		s_bLoaded = true;
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

		RebuildHiddenMap();
		RefreshAllArsenals();
	}

	static void ApplyParamsPack(string costs, string ranks, string bags)
	{
		s_bLoaded = true;
		s_aCostPrefab = {};
		s_aCostValue = {};
		s_aRankPrefab = {};
		s_aRankValue = {};
		s_aBagPrefab = {};
		s_aBagValue = {};
		ParseIntPairs(costs, s_aCostPrefab, s_aCostValue);
		ParseIntPairs(ranks, s_aRankPrefab, s_aRankValue);
		ParseFloatPairs(bags, s_aBagPrefab, s_aBagValue);
		HAM_ArsenalPrices.PushAll();
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

	protected static void PackParams(out string costs, out string ranks, out string bags)
	{
		costs = string.Empty;
		ranks = string.Empty;
		bags = string.Empty;
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

		if (s_aBagPrefab)
		{
			for (int i = 0; i < s_aBagPrefab.Count(); i++)
				bags = bags + s_aBagPrefab[i] + "~" + s_aBagValue[i].ToString() + "|";
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
				ReadItemFile(path);

			RebuildHiddenMap();
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
		ctx.ReadValue("bagPrefabs", s_aBagPrefab);
		ctx.ReadValue("bagValues", s_aBagValue);
		NormalizeArrays();
		RebuildHiddenMap();
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
		if (!s_aBagPrefab)
			s_aBagPrefab = {};
		if (!s_aBagValue)
			s_aBagValue = {};
	}

	protected static void ReadItemFile(string path)
	{
		JsonLoadContext ctx = new JsonLoadContext();
		if (!HAM_ProfileFiles.ReadJson(ctx, path))
			return;

		string prefab;
		if (!ctx.ReadValue("prefab", prefab) || prefab.IsEmpty())
			return;

		int shown = 1;
		int cost = -1;
		int rank = -1;
		float bag = -1;
		ctx.ReadValue("shown", shown);
		ctx.ReadValue("cost", cost);
		ctx.ReadValue("rank", rank);
		ctx.ReadValue("bag", bag);
		if (!s_mFile)
			s_mFile = new map<string, string>();

		s_mFile.Set(prefab, path);
		int index = HAM_ProfileFiles.IndexInName(path, "Item_");
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
		if (bag >= 0)
		{
			s_aBagPrefab.Insert(prefab);
			s_aBagValue.Insert(bag);
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
			WriteItem(prefab);
		}

		foreach (string prefab : s_aCostPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteItem(prefab);
		}

		foreach (string prefab : s_aRankPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteItem(prefab);
		}

		foreach (string prefab : s_aBagPrefab)
		{
			if (seen.Contains(prefab))
				continue;

			seen.Set(prefab, 1);
			WriteItem(prefab);
		}
	}

	protected static void EnsureCatalogFiles()
	{
		if (!Replication.IsServer())
			return;

		array<string> prefabs = {};
		HAM_ArsenalCatalog.CollectPrefabs(prefabs);
		foreach (string prefab : prefabs)
		{
			if (prefab.IsEmpty())
				continue;

			if (s_mFile && s_mFile.Contains(prefab))
				continue;

			WriteItem(prefab);
		}
	}

	protected static void WriteItem(string prefab)
	{
		if (!Replication.IsServer() || prefab.IsEmpty())
			return;

		if (!s_mFile)
			s_mFile = new map<string, string>();

		string path;
		if (!s_mFile.Find(prefab, path))
		{
			path = HAM_ProfileFiles.ItemPath(Dir(), "Item_", s_iNextFile, prefab);
			s_iNextFile = s_iNextFile + 1;
			s_mFile.Set(prefab, path);
		}

		int shown = 1;
		if (IndexOf(s_aHidden, prefab) >= 0)
			shown = 0;

		int cost = -1;
		int costIdx = IndexOf(s_aCostPrefab, prefab);
		if (costIdx >= 0)
			cost = s_aCostValue[costIdx];

		int rank = -1;
		int rankIdx = IndexOf(s_aRankPrefab, prefab);
		if (rankIdx >= 0)
			rank = s_aRankValue[rankIdx];

		float bag = -1;
		int bagIdx = IndexOf(s_aBagPrefab, prefab);
		if (bagIdx >= 0)
			bag = s_aBagValue[bagIdx];

		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("prefab", prefab);
		ctx.WriteValue("shown", shown);
		ctx.WriteValue("cost", cost);
		ctx.WriteValue("rank", rank);
		ctx.WriteValue("bag", bag);
		HAM_JsonQueue.Add(ctx, path);
	}

	protected static int IndexOf(array<string> keys, string key)
	{
		if (!keys || key.IsEmpty())
			return -1;

		int exact = keys.Find(key);
		if (exact >= 0)
			return exact;

		string path = PrefabPath(key);
		for (int i = 0; i < keys.Count(); i++)
		{
			if (PrefabPath(keys[i]) == path)
				return i;
		}

		return -1;
	}

	protected static void DropString(array<string> list, string value)
	{
		if (!list || value.IsEmpty())
			return;

		string path = PrefabPath(value);
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			if (list[i] == value || PrefabPath(list[i]) == path)
				list.Remove(i);
		}
	}

	protected static void UpsertInt(out array<string> keys, out array<int> values, string key, int value)
	{
		if (!keys)
			keys = {};
		if (!values)
			values = {};

		int idx = IndexOf(keys, key);
		if (idx >= 0)
		{
			values[idx] = value;
			return;
		}

		keys.Insert(key);
		values.Insert(value);
	}

	protected static void UpsertFloat(out array<string> keys, out array<float> values, string key, float value)
	{
		if (!keys)
			keys = {};
		if (!values)
			values = {};

		int idx = IndexOf(keys, key);
		if (idx >= 0)
		{
			values[idx] = value;
			return;
		}

		keys.Insert(key);
		values.Insert(value);
	}

	protected static void RemovePair(out array<string> keys, out array<int> values, string key)
	{
		if (!keys || !values || key.IsEmpty())
			return;

		string path = PrefabPath(key);
		for (int i = keys.Count() - 1; i >= 0; i--)
		{
			if (keys[i] != key && PrefabPath(keys[i]) != path)
				continue;

			keys.Remove(i);
			if (i < values.Count())
				values.Remove(i);
		}
	}

	protected static void RemoveFloatPair(out array<string> keys, out array<float> values, string key)
	{
		if (!keys || !values || key.IsEmpty())
			return;

		string path = PrefabPath(key);
		for (int i = keys.Count() - 1; i >= 0; i--)
		{
			if (keys[i] != key && PrefabPath(keys[i]) != path)
				continue;

			keys.Remove(i);
			if (i < values.Count())
				values.Remove(i);
		}
	}
}
