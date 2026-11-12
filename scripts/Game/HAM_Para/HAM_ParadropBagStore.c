//------------------------------------------------------------------------------------------------
// Sac de paradrop : un JSON par joueur (même principe que les mannequins / dépôt).
// $profile:HMSAC/<id>.json
// Écrit au décollage, lu à l'atterrissage, effacé après restore, wipe au boot serveur.
//------------------------------------------------------------------------------------------------

void HAM_ParadropBagFillLate(int playerId)
{
	HAM_ParadropBagStore.FillLate(playerId);
}

class HAM_ParadropBagStore
{
	protected static ref map<int, string> s_mPendingPack;
	protected static const string ROOT = "$profile:HMSAC/";
	protected static ref array<string> s_aWipeRaw;

	static string PathFor(int playerId)
	{
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		return ROOT + HAM_ProfileFiles.SafeId(identity) + ".json";
	}

	static bool Has(int playerId)
	{
		return FileIO.FileExists(PathFor(playerId));
	}

	static void Delete(int playerId)
	{
		string path = PathFor(playerId);
		if (FileIO.FileExists(path))
			FileIO.DeleteFile(path);

		string legacy = ROOT + "Joueur_" + HAM_ProfileFiles.SafeId(HAM_GroupAccessHelper.GetPlayerIdentityString(playerId)) + "/backpack.json";
		if (FileIO.FileExists(legacy))
			FileIO.DeleteFile(legacy);

		if (s_mPendingPack && s_mPendingPack.Contains(playerId))
			s_mPendingPack.Remove(playerId);
	}

	//! Au boot serveur : aucun JSON sac ne doit survivre d'une session précédente.
	static void WipeOnBoot()
	{
		if (!Replication.IsServer())
			return;

		FileIO.MakeDirectory(ROOT);

		int deleted = 0;
		array<string> paths = {};
		CollectWipePaths(paths);
		foreach (string path : paths)
		{
			if (path.IsEmpty() || !FileIO.FileExists(path))
				continue;

			if (FileIO.DeleteFile(path))
				deleted++;
		}

		Print(string.Format("[HAM Paradrop] JSON sacs effacés au démarrage: %1", deleted), LogLevel.NORMAL);
	}

	protected static void CollectWipePaths(notnull array<string> paths)
	{
		array<string> top = {};
		HAM_ProfileFiles.ListJson(ROOT, top);
		foreach (string path : top)
		{
			if (!path.IsEmpty() && paths.Find(path) < 0)
				paths.Insert(path);
		}

		if (!s_aWipeRaw)
			s_aWipeRaw = {};

		s_aWipeRaw.Clear();
		FileIO.FindFiles(OnWipeFound, ROOT, ".json");
		foreach (string name : s_aWipeRaw)
		{
			if (name.IsEmpty())
				continue;

			string path = name;
			if (name.IndexOf(ROOT) != 0 && !name.Contains(":"))
				path = ROOT + name;

			if (paths.Find(path) < 0)
				paths.Insert(path);

			int slash = name.LastIndexOf("/");
			int back = name.LastIndexOf("\\");
			int cut = slash;
			if (back > cut)
				cut = back;

			if (cut <= 0)
				continue;

			string folder = name.Substring(0, cut + 1);
			if (folder.IndexOf(ROOT) != 0 && !folder.Contains(":"))
				folder = ROOT + folder;

			array<string> nested = {};
			HAM_ProfileFiles.ListJson(folder, nested);
			foreach (string nestedPath : nested)
			{
				if (!nestedPath.IsEmpty() && paths.Find(nestedPath) < 0)
					paths.Insert(nestedPath);
			}
		}
	}

	static void OnWipeFound(string fileName, FileAttribute attributes = 0, string filesystem = string.Empty)
	{
		if (!s_aWipeRaw || fileName.IsEmpty())
			return;

		s_aWipeRaw.Insert(fileName);
	}

	//! Photo du personnage (loadout mannequin) + prefab/pack du sac.
	static bool SaveFromCharacter(int playerId, notnull IEntity body, IEntity backpack)
	{
		if (!Replication.IsServer() || !backpack)
			return false;

		string prefab = PrefabOf(backpack);
		if (prefab.IsEmpty())
			return false;

		string factionKey = FactionKeyOf(body);
		JsonSaveContext arsenal = new JsonSaveContext();
		arsenal.WriteValue(SCR_PlayerArsenalLoadout.ARSENALLOADOUT_FACTION_KEY, factionKey);
		if (!SCR_PlayerArsenalLoadout.ReadLoadoutString(body, arsenal))
			return false;

		string pack = PackOf(body, backpack);
		JsonSaveContext file = new JsonSaveContext();
		file.WriteValue("prefab", prefab);
		file.WriteValue("pack", pack);
		file.WriteValue("loadout", arsenal.SaveToString());

		FileIO.MakeDirectory(ROOT);
		string path = PathFor(playerId);
		if (!file.SaveToFile(path))
		{
			Print(string.Format("[HAM Paradrop] échec écriture sac %1", path), LogLevel.ERROR);
			return false;
		}

		Print(string.Format("[HAM Paradrop] sac enregistré %1 (pack=%2)", path, pack.Length()), LogLevel.NORMAL);
		return true;
	}

	//! Remet le sac + contenu, puis efface le JSON.
	static bool RestoreToCharacter(int playerId, notnull IEntity body)
	{
		if (!Replication.IsServer())
			return false;

		string path = PathFor(playerId);
		if (!FileIO.FileExists(path))
			return false;

		JsonLoadContext file = new JsonLoadContext();
		if (!file.LoadFromFile(path))
			return false;

		string prefab;
		string pack;
		string loadout;
		file.ReadValue("prefab", prefab);
		file.ReadValue("pack", pack);
		file.ReadValue("loadout", loadout);

		ClearBackpackSlot(body);

		bool ok = false;
		if (!prefab.IsEmpty())
			ok = EquipSpawnedBag(playerId, body, prefab, pack);

		if (!ok && !loadout.IsEmpty())
			ok = ApplyMannequinLoadout(body, loadout);

		if (ok)
		{
			// N'efface le JSON qu'une fois le contenu bien remis (ou pack vide d'origine).
			IEntity worn = WornBag(body);
			if (pack.IsEmpty() || CountInBag(worn) > 0)
			{
				Delete(playerId);
				Print(string.Format("[HAM Paradrop] sac restauré, JSON effacé (%1)", path), LogLevel.NORMAL);
			}
			else
			{
				RememberPack(playerId, pack);
				GetGame().GetCallqueue().CallLater(HAM_ParadropBagFillLate, 500, false, playerId);
				Print(string.Format("[HAM Paradrop] sac remis vide, retry contenu (%1)", path), LogLevel.WARNING);
			}
		}
		else
		{
			Print(string.Format("[HAM Paradrop] restauration sac échouée (%1)", path), LogLevel.WARNING);
		}

		return ok;
	}

	static void FillLate(int playerId)
	{
		if (!Replication.IsServer() || !s_mPendingPack || !s_mPendingPack.Contains(playerId))
			return;

		string pack = s_mPendingPack.Get(playerId);
		if (pack.IsEmpty())
		{
			s_mPendingPack.Remove(playerId);
			return;
		}

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		IEntity body = players.GetPlayerControlledEntity(playerId);
		if (!body)
			return;

		IEntity worn = WornBag(body);
		if (!worn)
			return;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return;

		// Déjà rempli entre-temps : ne jamais ré-insérer.
		if (CountInBag(worn) > 0)
		{
			s_mPendingPack.Remove(playerId);
			Delete(playerId);
			Print(string.Format("[HAM Paradrop] contenu sac déjà présent (late) joueur %1", playerId), LogLevel.NORMAL);
			return;
		}

		FillPack(manager, worn, pack);
		if (CountInBag(worn) > 0)
		{
			s_mPendingPack.Remove(playerId);
			Delete(playerId);
			Print(string.Format("[HAM Paradrop] contenu sac rétabli (late) joueur %1", playerId), LogLevel.NORMAL);
		}
	}

	protected static void RememberPack(int playerId, string pack)
	{
		if (!s_mPendingPack)
			s_mPendingPack = new map<int, string>();

		s_mPendingPack.Set(playerId, pack);
	}

	protected static IEntity WornBag(notnull IEntity body)
	{
		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(body.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadout)
			return null;

		return loadout.GetClothFromArea(LoadoutBackpackArea);
	}

	protected static bool EquipSpawnedBag(int playerId, notnull IEntity body, string prefab, string pack)
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(body.FindComponent(EquipedLoadoutStorageComponent));
		if (!manager || !loadout)
			return false;

		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector feet = body.GetOrigin();
		feet[1] = feet[1] + 0.2;
		params.Transform[3] = feet;
		IEntity bag = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		if (!bag)
			return false;

		// Équiper le sac VIDE, puis une seule insertion — un fill avant + après doublait le contenu.
		if (!manager.EquipAny(loadout, bag))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(bag);
			return false;
		}

		IEntity worn = loadout.GetClothFromArea(LoadoutBackpackArea);
		if (!worn)
			worn = bag;

		if (!pack.IsEmpty())
			FillPack(manager, worn, pack);

		return loadout.GetClothFromArea(LoadoutBackpackArea) != null;
	}

	protected static bool ApplyMannequinLoadout(notnull IEntity body, string loadoutJson)
	{
		JsonLoadContext load = new JsonLoadContext();
		if (!load.LoadFromString(loadoutJson))
			return false;

		return SCR_PlayerArsenalLoadout.ApplyLoadoutString(body, load);
	}

	protected static void ClearBackpackSlot(notnull IEntity body)
	{
		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(body.FindComponent(EquipedLoadoutStorageComponent));
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!loadout || !manager)
			return;

		IEntity worn = loadout.GetClothFromArea(LoadoutBackpackArea);
		if (!worn)
			return;

		manager.TryRemoveItemFromInventory(worn);
		SCR_EntityHelper.DeleteEntityAndChildren(worn);
	}

	protected static string PackOf(IEntity body, IEntity bag)
	{
		array<string> prefabs = {};
		array<int> ammos = {};
		array<int> counts = {};
		array<IEntity> items = {};
		GatherItems(body, bag, items);
		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			string prefab = PrefabOf(item);
			if (prefab.IsEmpty())
				continue;

			int ammo = AmmoOf(item);
			int at = -1;
			for (int i = 0; i < prefabs.Count(); i++)
			{
				if (prefabs[i] != prefab)
					continue;

				if (ammos[i] != ammo)
					continue;

				at = i;
				break;
			}

			if (at < 0)
			{
				prefabs.Insert(prefab);
				ammos.Insert(ammo);
				counts.Insert(1);
			}
			else
			{
				counts[at] = counts[at] + 1;
			}
		}

		string packed = "";
		for (int i = 0; i < prefabs.Count(); i++)
		{
			if (!packed.IsEmpty())
				packed = packed + ";";

			packed = packed + prefabs[i] + "~" + ammos[i].ToString() + "~" + counts[i].ToString();
		}

		return packed;
	}

	protected static void FillPack(SCR_InventoryStorageManagerComponent manager, IEntity bag, string pack)
	{
		if (!manager || !bag || pack.IsEmpty())
			return;

		// Toujours repartir d'un sac vide : évite les doublons si un fill a déjà partiellement réussi.
		ClearBagContents(manager, bag);

		array<string> prefabs = {};
		array<int> ammos = {};
		array<int> counts = {};
		ReadPack(pack, prefabs, ammos, counts);
		for (int i = 0; i < prefabs.Count(); i++)
		{
			int count = counts[i];
			if (count < 1)
				count = 1;

			for (int n = 0; n < count; n++)
			{
				if (!SpawnOneIntoBag(manager, bag, prefabs[i], ammos[i]))
					Print(string.Format("[HAM Paradrop] insertion refusée %1", prefabs[i]), LogLevel.WARNING);
			}
		}
	}

	protected static void ClearBagContents(SCR_InventoryStorageManagerComponent manager, IEntity bag)
	{
		if (!bag)
			return;

		array<IEntity> items = {};
		GatherFromBagStorages(bag, items);
		for (int i = items.Count() - 1; i >= 0; i--)
		{
			IEntity item = items[i];
			if (!item)
				continue;

			if (manager)
				manager.TryRemoveItemFromInventory(item);

			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}

	protected static bool SpawnOneIntoBag(SCR_InventoryStorageManagerComponent manager, IEntity bag, string prefab, int ammo)
	{
		if (!manager || !bag || prefab.IsEmpty())
			return false;

		BaseInventoryStorageComponent storage = StorageOf(bag);
		bool spawned = false;
		if (storage && manager.TrySpawnPrefabToStorage(prefab, storage, -1, EStoragePurpose.PURPOSE_ANY))
			spawned = true;

		if (!spawned && storage && manager.TrySpawnPrefabToStorage(prefab, storage, -1, storage.GetPurpose()))
			spawned = true;

		if (!spawned && manager.TrySpawnPrefabToStorage(prefab, null, -1, EStoragePurpose.PURPOSE_DEPOSIT))
		{
			// Spawné sur le personnage : tenter de le pousser dans le sac.
			spawned = MoveNewestPrefabIntoBag(manager, bag, prefab);
		}

		if (!spawned)
			return false;

		if (ammo >= 0 && storage)
			ApplyAmmo(StorageOf(bag), prefab, ammo);

		return true;
	}

	protected static bool MoveNewestPrefabIntoBag(SCR_InventoryStorageManagerComponent manager, IEntity bag, string prefab)
	{
		BaseInventoryStorageComponent storage = StorageOf(bag);
		if (!manager || !storage)
			return false;

		array<IEntity> items = {};
		manager.GetItems(items);
		for (int i = items.Count() - 1; i >= 0; i--)
		{
			IEntity item = items[i];
			if (!item || PrefabOf(item) != prefab)
				continue;

			if (BelongsToBag(item, bag))
				return true;

			if (manager.TryMoveItemToStorage(item, storage, -1))
				return true;
		}

		return false;
	}

	protected static void ApplyAmmo(BaseInventoryStorageComponent storage, string prefab, int ammo)
	{
		if (!storage || ammo < 0)
			return;

		array<IEntity> stored = {};
		storage.GetAll(stored);
		for (int i = stored.Count() - 1; i >= 0; i--)
		{
			IEntity item = stored[i];
			if (!item || PrefabOf(item) != prefab)
				continue;

			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
			if (!magazine)
				continue;

			if (magazine.GetAmmoCount() == ammo)
				continue;

			magazine.SetAmmoCount(ammo);
			return;
		}
	}

	protected static void ReadPack(string pack, array<string> prefabs, array<int> ammos, array<int> counts)
	{
		prefabs.Clear();
		ammos.Clear();
		counts.Clear();
		string rest = pack;
		while (!rest.IsEmpty())
		{
			int cut = rest.IndexOf(";");
			string part = rest;
			if (cut >= 0)
			{
				part = rest.Substring(0, cut);
				rest = rest.Substring(cut + 1, rest.Length() - cut - 1);
			}
			else
			{
				rest = "";
			}

			int tab1 = part.IndexOf("~");
			if (tab1 < 0)
				continue;

			string prefab = part.Substring(0, tab1);
			string tail = part.Substring(tab1 + 1, part.Length() - tab1 - 1);
			int tab2 = tail.IndexOf("~");
			if (tab2 < 0)
				continue;

			string ammoText = tail.Substring(0, tab2);
			string countText = tail.Substring(tab2 + 1, tail.Length() - tab2 - 1);
			if (prefab.IsEmpty())
				continue;

			prefabs.Insert(prefab);
			ammos.Insert(ammoText.ToInt());
			int count = countText.ToInt();
			if (count < 1)
				count = 1;

			counts.Insert(count);
		}
	}

	protected static int CountInBag(IEntity bag)
	{
		array<IEntity> items = {};
		GatherFromBagStorages(bag, items);
		return items.Count();
	}

	protected static void GatherItems(IEntity body, IEntity bag, array<IEntity> items)
	{
		items.Clear();
		if (!bag)
			return;

		GatherFromBagStorages(bag, items);
		if (items.Count() > 0 || !body)
			return;

		// Fallback MP : inventaire personnage, filtré au sac.
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return;

		array<IEntity> all = {};
		manager.GetItems(all);
		foreach (IEntity item : all)
		{
			if (!item || !BelongsToBag(item, bag))
				continue;

			if (items.Find(item) < 0)
				items.Insert(item);
		}
	}

	protected static void GatherFromBagStorages(IEntity bag, array<IEntity> items)
	{
		if (!bag)
			return;

		array<Managed> comps = {};
		bag.FindComponents(BaseInventoryStorageComponent, comps);
		foreach (Managed managed : comps)
		{
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(managed);
			if (!storage)
				continue;

			array<IEntity> stored = {};
			storage.GetAll(stored, true);
			foreach (IEntity item : stored)
			{
				if (item && items.Find(item) < 0)
					items.Insert(item);
			}
		}
	}

	protected static bool BelongsToBag(IEntity item, IEntity bag)
	{
		if (!item || !bag)
			return false;

		IEntity walk = item;
		for (int i = 0; i < 8 && walk; i++)
		{
			if (walk == bag)
				return true;

			walk = walk.GetParent();
		}

		BaseInventoryStorageComponent storage = StorageOf(bag);
		if (!storage)
			return false;

		array<IEntity> stored = {};
		storage.GetAll(stored, true);
		return stored.Find(item) >= 0;
	}

	protected static BaseInventoryStorageComponent StorageOf(IEntity bag)
	{
		if (!bag)
			return null;

		ClothNodeStorageComponent cloth = ClothNodeStorageComponent.Cast(bag.FindComponent(ClothNodeStorageComponent));
		if (cloth)
			return cloth;

		return BaseInventoryStorageComponent.Cast(bag.FindComponent(BaseInventoryStorageComponent));
	}

	protected static int AmmoOf(IEntity item)
	{
		if (!item)
			return -1;

		BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
		if (!magazine)
			return -1;

		return magazine.GetAmmoCount();
	}

	protected static string PrefabOf(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return "";

		return ent.GetPrefabData().GetPrefabName();
	}

	protected static string FactionKeyOf(IEntity body)
	{
		FactionAffiliationComponent factionComponent = FactionAffiliationComponent.Cast(body.FindComponent(FactionAffiliationComponent));
		if (!factionComponent)
			return SCR_PlayerArsenalLoadout.ARSENALLOADOUT_FACTIONKEY_NONE;

		Faction faction = factionComponent.GetAffiliatedFaction();
		if (!faction)
			return SCR_PlayerArsenalLoadout.ARSENALLOADOUT_FACTIONKEY_NONE;

		return faction.GetFactionKey();
	}
}
