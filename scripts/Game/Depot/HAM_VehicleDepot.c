//------------------------------------------------------------------------------------------------
// Atelier véhicules : arsenal vers le coffre, presets, modèle par type, photo au redémarrage.
// La photo ne contient que les objets encore présents, munitions comprises.
//------------------------------------------------------------------------------------------------

class HAM_VehicleDepot
{
	static const int SUPPLIES_UNLIMITED = -1;

	protected static const string CARGO_FILE = "$profile:13RDPA_HeartsAndMinds/VehicleCargo.json";
	protected static const string LOADOUT_FILE = "$profile:13RDPA_HeartsAndMinds/VehicleLoadouts.json";
	protected static const string PRESET_FILE = "$profile:13RDPA_HeartsAndMinds/VehiclePresets.json";
	protected static const float MATCH_DIST = 12;

	protected static bool s_bStarted;
	protected static bool s_bBusy;
	protected static bool s_bRestoreDone;
	protected static bool s_bCargoDirty;
	protected static const int MAX_ADD_COUNT = 50;
	protected static bool s_bLoadoutDirty;
	protected static bool s_bPresetDirty;

	protected static ref array<string> s_aCargoPrefab;
	protected static ref array<float> s_aCargoX;
	protected static ref array<float> s_aCargoY;
	protected static ref array<float> s_aCargoZ;
	protected static ref array<string> s_aCargoPack;

	protected static ref array<string> s_aLoadPrefab;
	protected static ref array<string> s_aLoadPack;

	protected static ref array<string> s_aPresetId;
	protected static ref array<string> s_aPresetName;
	protected static ref array<string> s_aPresetPack;

	protected static ref array<IEntity> s_aWait;
	protected static ref array<RplId> s_aKnown;
	protected static ref array<IEntity> s_aQuery;

	static void Start()
	{
		if (s_bStarted || !Replication.IsServer())
			return;

		s_bStarted = true;
		SCR_ResourcePlayerControllerInventoryComponent.GetOnArsenalItemRequested().Insert(HAM_DepotOnArsenalBuy);
		s_aCargoPrefab = {};
		s_aCargoX = {};
		s_aCargoY = {};
		s_aCargoZ = {};
		s_aCargoPack = {};
		s_aLoadPrefab = {};
		s_aLoadPack = {};
		s_aPresetId = {};
		s_aPresetName = {};
		s_aPresetPack = {};
		s_aWait = {};
		s_aKnown = {};
		s_aQuery = {};
		HAM_ProfileFiles.EnsureDir("$profile:13RDPA_HeartsAndMinds/Depot/");
		HAM_ProfileFiles.EnsureDir("$profile:13RDPA_HeartsAndMinds/Depot/Item/");
		HAM_ProfileFiles.EnsureDir("$profile:13RDPA_HeartsAndMinds/Depot/Preset/");
		HAM_ProfileFiles.EnsureDir("$profile:13RDPA_HeartsAndMinds/Depot/Persistence/");
		LoadCargo();
		LoadLoadouts();
		LoadPresets();
		HAM_VehicleDepotFilter.Start();
		GetGame().GetCallqueue().CallLater(RestorePass, 25000, false);
		GetGame().GetCallqueue().CallLater(FinishBoot, 80000, false);
		// Plus aucun instantané périodique : la cargaison n'est écrite que sur « Valider le coffre ».
	}

	static void ConsiderSpawn(IEntity vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		Start();
		if (!s_bRestoreDone)
		{
			if (s_aWait && s_aWait.Find(vehicle) < 0)
				s_aWait.Insert(vehicle);
			return;
		}

		TryTemplate(vehicle, true);
	}

	static bool PlayerAtDepot(int playerId, IEntity depot)
	{
		if (!depot || !depot.FindComponent(HAM_VehicleDepotComponent))
			return false;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;

		IEntity player = players.GetPlayerControlledEntity(playerId);
		if (!player)
			return false;

		vector delta = player.GetOrigin() - depot.GetOrigin();
		delta[1] = 0;
		float limit = Radius() + 15.0;
		return delta.LengthSq() <= limit * limit;
	}

	static float Radius()
	{
		return HAM_SpawnConfig.Get().DepotRadius();
	}

	static bool NearDepot(IEntity depot, IEntity vehicle)
	{
		if (!depot || !vehicle)
			return false;

		vector delta = vehicle.GetOrigin() - depot.GetOrigin();
		delta[1] = 0;
		float radius = Radius() + 2;
		return delta.LengthSq() <= radius * radius;
	}

	static void Collect(vector origin, array<IEntity> result)
	{
		result.Clear();
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		if (!s_aQuery)
			s_aQuery = {};

		s_aQuery.Clear();
		float radius = Radius();
		world.QueryEntitiesBySphere(origin, radius, QueryOne);
		foreach (IEntity ent : s_aQuery)
		{
			if (!ent)
				continue;

			vector delta = ent.GetOrigin() - origin;
			delta[1] = 0;
			if (delta.LengthSq() > radius * radius)
				continue;

			result.Insert(ent);
		}
	}

	static bool QueryOne(IEntity ent)
	{
		if (!Vehicle.Cast(ent))
			return true;

		if (s_aQuery && s_aQuery.Find(ent) < 0)
			s_aQuery.Insert(ent);

		return true;
	}

	static void AddCount(int playerId, IEntity depot, IEntity vehicle, string prefab, int count, int ammo)
	{
		if (!Replication.IsServer() || count < 1 || prefab.IsEmpty())
			return;

		if (!NearDepot(depot, vehicle))
		{
			Tell(playerId, "Véhicule trop loin de l'atelier.");
			return;
		}

		// Contrôles serveur : vrai dépôt, joueur sur place, objet de l'arsenal, quantité plafonnée.
		if (!PlayerAtDepot(playerId, depot))
		{
			Tell(playerId, "Vous êtes trop loin de l'atelier.");
			return;
		}
		if (!GarageAllows(prefab))
		{
			Print("[13RDPA H&M Depot] ajout refusé : objet hors tableau ou hors garage", LogLevel.WARNING);
			return;
		}
		if (count > MAX_ADD_COUNT)
			count = MAX_ADD_COUNT;

		int cost = ItemCost(prefab, true);
		int done = 0;
		s_bBusy = true;
		for (int i = 0; i < count; i++)
		{
			if (!Pay(depot.GetOrigin(), cost))
			{
				Tell(playerId, "Ravitaillement insuffisant.");
				break;
			}

			if (!SpawnOne(vehicle, prefab, ammo, playerId))
			{
				Pay(depot.GetOrigin(), -cost);
				Tell(playerId, "Le coffre n'accepte pas cet objet.");
				break;
			}

			done = done + 1;
		}

		s_bBusy = false;
		if (done > 0)
		{
			MirrorSupplies(depot);
			Tell(playerId, "Ajouté : " + done.ToString() + " (−" + (cost * done).ToString() + ")");
		}
		PushView(playerId, vehicle);
	}

	static void RemoveCount(int playerId, IEntity depot, IEntity vehicle, string prefab, int ammo, int count)
	{
		if (!Replication.IsServer() || count < 1 || prefab.IsEmpty())
			return;

		if (!NearDepot(depot, vehicle))
		{
			Tell(playerId, "Véhicule trop loin de l'atelier.");
			return;
		}

		if (!PlayerAtDepot(playerId, depot))
		{
			Tell(playerId, "Vous êtes trop loin de l'atelier.");
			return;
		}

		int refund = ItemCost(prefab, false);
		int done = 0;
		s_bBusy = true;
		for (int i = 0; i < count; i++)
		{
			if (!DeleteOne(vehicle, prefab, ammo, depot.GetOrigin()))
				break;

			done = done + 1;
		}

		s_bBusy = false;
		if (done > 0)
		{
			MirrorSupplies(depot);
			Tell(playerId, "Retiré : " + done.ToString() + " (+" + (refund * done).ToString() + ")");
		}
		else
		{
			Tell(playerId, "Cet objet n'est pas dans le véhicule.");
		}

		PushView(playerId, vehicle);
	}

	static void ClearCargo(int playerId, IEntity depot, IEntity vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		if (!NearDepot(depot, vehicle))
		{
			Tell(playerId, "Véhicule trop loin de l'atelier.");
			return;
		}

		if (!PlayerAtDepot(playerId, depot))
		{
			Tell(playerId, "Vous êtes trop loin de l'atelier.");
			return;
		}

		int done = 0;
		s_bBusy = true;
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (manager)
		{
			for (int pass = 0; pass < 12; pass++)
			{
			array<IEntity> items = {};
			GatherItems(manager, items);
			if (items.IsEmpty())
					break;

				bool removed = false;
				foreach (IEntity item : items)
				{
					if (!item)
						continue;

					string prefab = PrefabOf(item);
					int ammo = AmmoOf(item);
					if (DeleteOne(vehicle, prefab, ammo, depot.GetOrigin()))
					{
						done = done + 1;
						removed = true;
					}
				}

				if (!removed)
					break;
			}
		}

		s_bBusy = false;
		Tell(playerId, "Coffre vidé : " + done.ToString());
		PushView(playerId, vehicle);
	}

	static void SavePreset(int playerId, IEntity vehicle, string name)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		string clean = name;
		if (clean.IsEmpty())
		{
			Tell(playerId, "Donne un nom au preset.");
			return;
		}

		string pack = PackOf(vehicle);
		int row = PresetRow(clean);
		if (row < 0)
		{
			s_aPresetId.Insert("shared");
			s_aPresetName.Insert(clean);
			s_aPresetPack.Insert(pack);
		}
		else
		{
			s_aPresetPack[row] = pack;
		}

		s_bPresetDirty = true;
		SavePresets();
		Tell(playerId, "Preset enregistré.");
		PushView(playerId, vehicle);
	}

	static void DeletePreset(int playerId, IEntity vehicle, string name)
	{
		if (!Replication.IsServer())
			return;

		int row = PresetRow(name);
		if (row < 0)
			return;

		s_aPresetId.Remove(row);
		s_aPresetName.Remove(row);
		s_aPresetPack.Remove(row);
		s_bPresetDirty = true;
		SavePresets();
		Tell(playerId, "Preset supprimé.");
		PushView(playerId, vehicle);
	}

	static void ApplyPreset(int playerId, IEntity depot, IEntity vehicle, string name)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		if (!NearDepot(depot, vehicle))
		{
			Tell(playerId, "Véhicule trop loin de l'atelier.");
			return;
		}

		if (!PlayerAtDepot(playerId, depot))
		{
			Tell(playerId, "Vous êtes trop loin de l'atelier.");
			return;
		}

		int row = PresetRow(name);
		if (row < 0)
		{
			Tell(playerId, "Preset introuvable.");
			return;
		}

		ClearCargo(playerId, depot, vehicle);
		FillPack(playerId, depot, vehicle, s_aPresetPack[row], true);
		Tell(playerId, "Preset appliqué.");
		PushView(playerId, vehicle);
	}

	static void SetPersistent(int playerId, IEntity vehicle, bool enabled)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		string prefab = PrefabOf(vehicle);
		if (prefab.IsEmpty())
			return;

		int row = LoadoutRow(prefab);
		if (!enabled)
		{
			if (row >= 0)
			{
				s_aLoadPrefab.Remove(row);
				s_aLoadPack.Remove(row);
				s_bLoadoutDirty = true;
				SaveLoadouts();
			}

			Tell(playerId, "Inventaire persistant retiré.");
			PushView(playerId, vehicle);
			return;
		}

		string pack = PackOf(vehicle);
		if (row < 0)
		{
			s_aLoadPrefab.Insert(prefab);
			s_aLoadPack.Insert(pack);
		}
		else
		{
			s_aLoadPack[row] = pack;
		}

		s_bLoadoutDirty = true;
		SaveLoadouts();
		Tell(playerId, "Ce contenu sera repris sur les prochains véhicules de ce type.");
		PushView(playerId, vehicle);
	}

	static string PresetText(int playerId)
	{
		string packed = "";
		for (int i = 0; i < s_aPresetName.Count(); i++)
		{
			if (!packed.IsEmpty())
				packed = packed + "|";

			packed = packed + s_aPresetName[i];
		}

		return packed;
	}

	static bool IsPersistent(IEntity vehicle)
	{
		if (!vehicle)
			return false;

		return LoadoutRow(PrefabOf(vehicle)) >= 0;
	}

	protected static void RestorePass()
	{
		if (!Replication.IsServer() || !s_aCargoPrefab)
			return;

		for (int i = 0; i < s_aCargoPrefab.Count(); i++)
			RestoreOne(i);
	}

	protected static void FinishBoot()
	{
		RestorePass();
		s_bRestoreDone = true;
		if (!s_aWait)
			return;

		foreach (IEntity vehicle : s_aWait)
		{
			if (vehicle)
				TryTemplate(vehicle, false);
		}

		s_aWait.Clear();
	}

	protected static void RestoreOne(int row)
	{
		IEntity vehicle = MatchVehicle(s_aCargoPrefab[row], s_aCargoX[row], s_aCargoY[row], s_aCargoZ[row]);
		if (!vehicle)
			return;

		// Déjà restauré au premier passage : on ne recommence pas.
		if (IsKnown(vehicle))
			return;

		MarkKnown(vehicle);
		// Remise à l'état validé : le contenu actuel est remplacé par la dernière photo.
		s_bBusy = true;
		StripFactoryItems(vehicle);
		SpawnPack(vehicle, s_aCargoPack[row]);
		s_bBusy = false;
	}

	// fresh = véhicule apparu après le chargement de la sauvegarde (demande, construction).
	protected static void TryTemplate(IEntity vehicle, bool fresh)
	{
		if (!vehicle || IsKnown(vehicle))
			return;

		if (IsScriptSide(vehicle))
			return;

		int row = LoadoutRow(PrefabOf(vehicle));
		if (row < 0)
			return;
		// Véhicule rechargé au démarrage avec un contenu : on n'y touche pas.
		if (!fresh && ItemCount(vehicle) > 0)
			return;

		// Training : pas de base Conflict — le modèle s'applique sans débit QG.
		s_bBusy = true;
		int removed = StripFactoryItems(vehicle);
		SpawnPack(vehicle, s_aLoadPack[row]);
		s_bBusy = false;
		Print(string.Format("[13RDPA H&M Depot] modèle appliqué sur un véhicule neuf (%1 objets d'usine retirés)", removed), LogLevel.NORMAL);
	}

	protected static int StripFactoryItems(IEntity vehicle)
	{
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (!manager)
			return 0;

		int removed = 0;
		for (int pass = 0; pass < 6; pass++)
		{
			array<IEntity> items = {};
			GatherItems(manager, items);
			if (items.IsEmpty())
				break;

			bool any = false;
			foreach (IEntity item : items)
			{
				if (!item)
					continue;

				bool deleted = manager.TryDeleteItem(item);
				if (!deleted)
					continue;

				removed++;
				any = true;
			}

			if (!any)
				break;
		}

		return removed;
	}

	protected static void FillPack(int playerId, IEntity depot, IEntity vehicle, string pack, bool charge)
	{
		if (pack.IsEmpty())
			return;

		array<string> prefabs = {};
		array<int> ammos = {};
		array<int> counts = {};
		ReadPack(pack, prefabs, ammos, counts);
		s_bBusy = true;
		for (int i = 0; i < prefabs.Count(); i++)
		{
			if (!GarageAllows(prefabs[i]))
				continue;

			int cost = 0;
			if (charge)
				cost = ItemCost(prefabs[i], true);

			for (int n = 0; n < counts[i]; n++)
			{
				int bill = cost;
				if (charge && !Pay(depot.GetOrigin(), bill))
				{
					Tell(playerId, "Ravitaillement insuffisant pour finir le chargement.");
					s_bBusy = false;
					return;
				}

				if (!SpawnOne(vehicle, prefabs[i], ammos[i], playerId))
				{
					if (charge)
						Pay(depot.GetOrigin(), -bill);
				}
			}
		}

		s_bBusy = false;
	}

	protected static bool SpawnOne(IEntity vehicle, string prefab, int ammo, int playerId)
	{
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (!manager)
		{
			Print("[13RDPA H&M Depot] aucun gestionnaire d'inventaire", LogLevel.WARNING);
			return false;
		}

		if (prefab.IsEmpty())
		{
			Print("[13RDPA H&M Depot] prefab vide", LogLevel.WARNING);
			return false;
		}

		array<IEntity> before = {};
		GatherItems(manager, before);
		bool spawned = false;
		BaseInventoryStorageComponent trunk = TrunkOf(vehicle);
		if (playerId != 0 && trunk)
		{
			IEntity user = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			SCR_InventoryStorageManagerComponent playerInv;
			if (user)
				playerInv = SCR_InventoryStorageManagerComponent.Cast(user.FindComponent(SCR_InventoryStorageManagerComponent));

			if (playerInv && playerInv.TrySpawnPrefabToStorage(prefab, trunk, -1, trunk.GetPurpose()))
				spawned = true;
		}

		if (!spawned)
		{
			array<BaseInventoryStorageComponent> storages = {};
			manager.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);
			foreach (BaseInventoryStorageComponent storage : storages)
			{
				if (!storage)
					continue;

				if (manager.TrySpawnPrefabToStorage(prefab, storage, -1, storage.GetPurpose()))
				{
					spawned = true;
					break;
				}
			}
		}

		if (!spawned)
			spawned = manager.TrySpawnPrefabToStorage(prefab, null, -1, EStoragePurpose.PURPOSE_ANY);

		if (!spawned)
			spawned = TrySpawnInto(vehicle, prefab);

		if (!spawned)
			spawned = InsertSpawned(vehicle, manager, prefab);

		if (!spawned)
		{
			Print("[13RDPA H&M Depot] insertion refusee " + prefab, LogLevel.WARNING);
			return false;
		}

		if (ammo < 0)
			return true;

		array<IEntity> after = {};
		GatherItems(manager, after);
		foreach (IEntity item : after)
		{
			if (!item || before.Find(item) >= 0)
				continue;

			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(item.FindComponent(BaseMagazineComponent));
			if (magazine)
				magazine.SetAmmoCount(ammo);

			break;
		}

		return true;
	}

	protected static bool InsertSpawned(IEntity vehicle, InventoryStorageManagerComponent manager, string prefab)
	{
		Resource resource = Resource.Load(prefab);
		if (!resource || !resource.IsValid())
		{
			Print("[13RDPA H&M Depot] prefab invalide " + prefab, LogLevel.WARNING);
			return false;
		}

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = vehicle.GetOrigin();
		IEntity item = GetGame().SpawnEntityPrefab(resource, vehicle.GetWorld(), params);
		if (!item)
			return false;

		if (manager.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY))
			return true;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (storage && manager.TryInsertItemInStorage(item, storage))
				return true;
		}

		SCR_EntityHelper.DeleteEntityAndChildren(item);
		return false;
	}

	protected static void SpawnPack(IEntity vehicle, string pack)
	{
		array<string> prefabs = {};
		array<int> ammos = {};
		array<int> counts = {};
		ReadPack(pack, prefabs, ammos, counts);
		for (int i = 0; i < prefabs.Count(); i++)
		{
			for (int n = 0; n < counts[i]; n++)
				SpawnOne(vehicle, prefabs[i], ammos[i], 0);
		}
	}

	protected static bool DeleteOne(IEntity vehicle, string prefab, int ammo, vector refundAt)
	{
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (!manager)
			return false;

		array<IEntity> items = {};
		GatherItems(manager, items);
		foreach (IEntity item : items)
		{
			if (!item || PrefabOf(item) != prefab)
				continue;

			if (ammo >= 0 && AmmoOf(item) != ammo)
				continue;

			int refund = ItemCost(prefab, false);
			bool deleted = manager.TryDeleteItem(item);
			if (!deleted)
				return false;

			Pay(refundAt, -refund);
			return true;
		}

		return false;
	}

	//! Base joueur la plus proche du point : le maître capturé, ou le QG si rien n'est plus près.
	static SCR_CampaignMilitaryBaseComponent SupplyBaseAt(vector pos)
	{
		if (!HAM_Mission.IsRunning())
			return null;

		if (pos == vector.Zero)
			return HAM_Mission.DesignatedHqBase();

		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		SCR_CampaignMilitaryBaseComponent nearest;
		float bestSq = -1;
		if (system)
		{
			array<SCR_MilitaryBaseComponent> bases = {};
			system.GetBases(bases);
			foreach (SCR_MilitaryBaseComponent listed : bases)
			{
				SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(listed);
				if (!campaignBase || !campaignBase.GetOwner())
					continue;

				Faction faction = campaignBase.GetFaction();
				if (!faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
					continue;

				float distSq = vector.DistanceSqXZ(campaignBase.GetOwner().GetOrigin(), pos);
				if (bestSq >= 0 && distSq >= bestSq)
					continue;

				bestSq = distSq;
				nearest = campaignBase;
			}
		}

		if (nearest)
			return nearest;

		return HAM_Mission.DesignatedHqBase();
	}

	protected static bool Pay(vector pos, int cost)
	{
		if (cost == 0)
			return true;

		SCR_CampaignMilitaryBaseComponent base = SupplyBaseAt(pos);
		if (!base)
			return true;

		if (cost > 0 && base.GetSupplies() < cost)
			return false;

		base.AddSupplies(-cost);
		return true;
	}

	// Tableau = liste maître. Le filtre du garage est la seconde couche.
	protected static bool GarageAllows(string prefab)
	{
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.IsShown(prefab))
			return false;

		if (!HAM_VehicleDepotFilter.IsShown(prefab, true))
			return false;

		return ArsenalData(prefab) != null;
	}

	protected static int ItemCost(string prefab, bool buy)
	{
		SCR_ArsenalItem data = ArsenalData(prefab);
		if (!data)
			return 0;

		int cost = data.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false);
		if (HAM_ArsenalBoardStore.HasCostOverride(prefab))
			cost = HAM_ArsenalBoardStore.BoardPrice(prefab);
		else
		{
			string named = data.GetItemResourceName();
			if (named != prefab && HAM_ArsenalBoardStore.HasCostOverride(named))
				cost = HAM_ArsenalBoardStore.BoardPrice(named);
		}

		if (!buy && cost < 0)
			return 0;

		return cost;
	}

	protected static SCR_ArsenalItem ArsenalData(string prefab)
	{
		if (prefab.IsEmpty())
			return null;

		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalog)
			return null;

		// Toutes factions + catalogue global.
		string path = PrefabPath(prefab);
		array<SCR_ArsenalItem> items = {};
		catalog.GetAllArsenalItems(items, -1, -1, -1, -1);
		foreach (SCR_ArsenalItem item : items)
		{
			if (!item)
				continue;

			string name = item.GetItemResourceName();
			if (name == prefab)
				return item;

			if (PrefabPath(name) == path)
				return item;
		}

		return null;
	}

	protected static string PrefabPath(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0)
			return prefab;

		if (brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	// « Valider le coffre » : photo du coffre de CE véhicule, puis une seule écriture JSON.
	// Si le modèle par type est coché pour ce véhicule, le modèle est mis à jour en même temps.
	static void ConfirmCargo(int playerId, IEntity depot, IEntity vehicle)
	{
		if (!Replication.IsServer() || !vehicle)
			return;

		Start();
		if (!NearDepot(depot, vehicle))
		{
			Tell(playerId, "Véhicule trop loin de l'atelier.");
			return;
		}
		if (!PlayerAtDepot(playerId, depot))
		{
			Tell(playerId, "Vous êtes trop loin de l'atelier.");
			return;
		}
		if (!RememberRow(vehicle))
		{
			Tell(playerId, "Ce véhicule ne peut pas être enregistré.");
			return;
		}

		MarkKnown(vehicle);
		SaveCargo();
		int row = LoadoutRow(PrefabOf(vehicle));
		if (row >= 0)
		{
			s_aLoadPack[row] = PackOf(vehicle);
			s_bLoadoutDirty = true;
			SaveLoadouts();
			Tell(playerId, "Coffre validé : enregistré et appliqué comme modèle de ce type.");
		}
		else
		{
			Tell(playerId, "Coffre validé et enregistré.");
		}

		Print("[13RDPA H&M Depot] coffre validé par un joueur (1 écriture)", LogLevel.NORMAL);
		PushView(playerId, vehicle);
	}

	// Met à jour la ligne de cargaison sans écrire le fichier.
	protected static bool RememberRow(IEntity vehicle)
	{
		if (!vehicle || IsScriptSide(vehicle))
			return false;

		string prefab = PrefabOf(vehicle);
		if (prefab.IsEmpty())
			return false;

		vector pos = vehicle.GetOrigin();
		string pack = PackOf(vehicle);
		int row = CargoRow(prefab, pos);
		if (row < 0)
		{
			s_aCargoPrefab.Insert(prefab);
			s_aCargoX.Insert(pos[0]);
			s_aCargoY.Insert(pos[1]);
			s_aCargoZ.Insert(pos[2]);
			s_aCargoPack.Insert(pack);
		}
		else
		{
			s_aCargoX[row] = pos[0];
			s_aCargoY[row] = pos[1];
			s_aCargoZ[row] = pos[2];
			s_aCargoPack[row] = pack;
		}

		s_bCargoDirty = true;
		return true;
	}

	protected static string PackOf(IEntity vehicle)
	{
		array<string> prefabs = {};
		array<int> ammos = {};
		array<int> counts = {};
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (manager)
		{
			array<IEntity> items = {};
			GatherItems(manager, items);
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

	static void SplitPack(string pack, array<string> prefabs, array<int> ammos, array<int> counts)
	{
		ReadPack(pack, prefabs, ammos, counts);
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

	protected static int ItemCount(IEntity vehicle)
	{
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (!manager)
			return 0;

		array<IEntity> items = {};
		GatherItems(manager, items);
		return items.Count();
	}

	protected static void GatherItems(InventoryStorageManagerComponent manager, array<IEntity> items)
	{
		items.Clear();
		array<IEntity> direct = {};
		manager.GetItems(direct);
		foreach (IEntity item : direct)
		{
			if (item && items.Find(item) < 0)
				items.Insert(item);
		}

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages, EStoragePurpose.PURPOSE_ANY);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> stored = {};
			storage.GetAll(stored);
			foreach (IEntity item : stored)
			{
				if (item && items.Find(item) < 0)
					items.Insert(item);
			}
		}
	}

	protected static BaseInventoryStorageComponent TrunkOf(IEntity vehicle)
	{
		array<IEntity> nodes = {};
		CollectNodes(vehicle, nodes);
		BaseInventoryStorageComponent bestDeposit;
		BaseInventoryStorageComponent bestAny;
		float bestDepositVolume = -1;
		float bestAnyVolume = -1;
		foreach (IEntity node : nodes)
		{
			if (!node)
				continue;

			array<Managed> components = {};
			node.FindComponents(UniversalInventoryStorageComponent, components);
			foreach (Managed managed : components)
			{
				UniversalInventoryStorageComponent storage = UniversalInventoryStorageComponent.Cast(managed);
				if (!storage)
					continue;

				float volume = storage.GetMaxVolumeCapacity();
				if (volume > bestAnyVolume)
				{
					bestAny = storage;
					bestAnyVolume = volume;
				}

				if (storage.GetPurpose() != EStoragePurpose.PURPOSE_DEPOSIT)
					continue;

				if (volume < bestDepositVolume)
					continue;

				bestDeposit = storage;
				bestDepositVolume = volume;
			}
		}

		if (bestDeposit)
			return bestDeposit;

		return bestAny;
	}

	static BaseInventoryStorageComponent Trunk(IEntity vehicle)
	{
		return TrunkOf(vehicle);
	}

	static void ReadTrunk(IEntity vehicle, array<string> prefabs, array<int> ammos, array<int> counts)
	{
		prefabs.Clear();
		ammos.Clear();
		counts.Clear();
		array<IEntity> items = {};
		InventoryStorageManagerComponent manager = ManagerOf(vehicle);
		if (manager)
			GatherItems(manager, items);

		BaseInventoryStorageComponent trunk = TrunkOf(vehicle);
		if (trunk)
		{
			array<IEntity> stored = {};
			trunk.GetAll(stored);
			foreach (IEntity item : stored)
			{
				if (item && items.Find(item) < 0)
					items.Insert(item);
			}
		}

		foreach (IEntity item : items)
		{
			if (!item)
				continue;

			string prefab = PrefabOf(item);
			if (prefab.IsEmpty())
				continue;

			int ammo = AmmoOf(item);
			int found = -1;
			for (int i = 0; i < prefabs.Count(); i++)
			{
				if (prefabs[i] != prefab)
					continue;

				if (ammos[i] != ammo)
					continue;

				found = i;
				break;
			}

			if (found >= 0)
			{
				counts[found] = counts[found] + 1;
				continue;
			}

			prefabs.Insert(prefab);
			ammos.Insert(ammo);
			counts.Insert(1);
		}
	}

	protected static void CollectNodes(IEntity root, array<IEntity> nodes)
	{
		if (!root)
			return;

		nodes.Insert(root);
		IEntity child = root.GetChildren();
		while (child)
		{
			CollectNodes(child, nodes);
			child = child.GetSibling();
		}
	}

	protected static InventoryStorageManagerComponent ManagerOf(IEntity vehicle)
	{
		array<IEntity> nodes = {};
		CollectNodes(vehicle, nodes);
		InventoryStorageManagerComponent fallback;
		foreach (IEntity node : nodes)
		{
			if (!node)
				continue;

			InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(node.FindComponent(InventoryStorageManagerComponent));
			if (!manager)
				continue;

			if (!fallback)
				fallback = manager;

			array<BaseInventoryStorageComponent> storages = {};
			if (manager.GetStorages(storages) > 0)
				return manager;
		}

		return fallback;
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

	//! Les garages donnent des véhicules de toutes les factions : seule l'IA posée par H&M est écartée.
	protected static bool IsScriptSide(IEntity vehicle)
	{
		return HAM_Garrison.IsScriptVehicle(vehicle);
	}

	protected static void MarkKnown(IEntity vehicle)
	{
		RplId id = HAM_RplUtil.IdOf(vehicle);
		if (!s_aKnown)
			s_aKnown = {};

		if (s_aKnown.Find(id) < 0)
			s_aKnown.Insert(id);
	}

	protected static bool IsKnown(IEntity vehicle)
	{
		if (!s_aKnown)
			return false;

		return s_aKnown.Find(HAM_RplUtil.IdOf(vehicle)) >= 0;
	}

	protected static IEntity MatchVehicle(string prefab, float x, float y, float z)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		vector pos = Vector(x, y, z);
		if (!s_aQuery)
			s_aQuery = {};

		s_aQuery.Clear();
		world.QueryEntitiesBySphere(pos, MATCH_DIST, QueryOne);
		IEntity best;
		float bestDist = -1;
		foreach (IEntity vehicle : s_aQuery)
		{
			if (!vehicle || PrefabOf(vehicle) != prefab)
				continue;

			if (IsScriptSide(vehicle))
				continue;

			vector delta = vehicle.GetOrigin() - pos;
			delta[1] = 0;
			float dist = delta.LengthSq();
			if (bestDist >= 0 && dist >= bestDist)
				continue;

			bestDist = dist;
			best = vehicle;
		}

		return best;
	}

	protected static int CargoRow(string prefab, vector pos)
	{
		float limit = MATCH_DIST * MATCH_DIST;
		for (int i = 0; i < s_aCargoPrefab.Count(); i++)
		{
			if (s_aCargoPrefab[i] != prefab)
				continue;

			vector gap = Vector(s_aCargoX[i], s_aCargoY[i], s_aCargoZ[i]) - pos;
			gap[1] = 0;
			if (gap.LengthSq() <= limit)
				return i;
		}

		return -1;
	}

	protected static int LoadoutRow(string prefab)
	{
		if (!s_aLoadPrefab)
			return -1;

		for (int i = 0; i < s_aLoadPrefab.Count(); i++)
		{
			if (s_aLoadPrefab[i] == prefab)
				return i;
		}

		return -1;
	}

	protected static int PresetRow(string name)
	{
		if (!s_aPresetName)
			return -1;

		for (int i = 0; i < s_aPresetName.Count(); i++)
		{
			if (s_aPresetName[i] == name)
				return i;
		}

		return -1;
	}

	//! Fournitures de la base joueur la plus proche. Sans base (H&M arrêté), l'atelier ne compte rien.
	static int HomeSupplies(vector pos)
	{
		SCR_CampaignMilitaryBaseComponent base = SupplyBaseAt(pos);
		if (!base)
			return SUPPLIES_UNLIMITED;

		return Math.Round(base.GetSupplies());
	}

	protected static void Tell(int playerId, string text)
	{
		Print("[13RDPA H&M Depot] " + text, LogLevel.NORMAL);
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (player)
			player.HAM_ShowAdminNote(text);
	}

	static void PushView(int playerId, IEntity vehicle)
	{
		PlayerController controller = GetGame().GetPlayerManager().GetPlayerController(playerId);
		SCR_PlayerController player = SCR_PlayerController.Cast(controller);
		if (!player)
			return;

		string cargo = "";
		int persistent = 0;
		if (vehicle)
		{
			cargo = PackOf(vehicle);
			if (IsPersistent(vehicle))
				persistent = 1;
		}

		vector supplyPos = vector.Zero;
		if (vehicle)
			supplyPos = vehicle.GetOrigin();

		player.HAM_DepotView(cargo, PresetText(playerId), persistent, HomeSupplies(supplyPos));
	}

	protected static string FileTail(string path)
	{
		int slash = path.LastIndexOf("/");
		int back = path.LastIndexOf("\\");
		if (back > slash)
			slash = back;

		if (slash >= 0 && slash + 1 < path.Length())
			return path.Substring(slash + 1, path.Length() - slash - 1);

		return path;
	}

	// Écrit d'abord, puis retire les JSON qui ne font plus partie de l'enregistrement.
	protected static void DeleteStaleJson(string directory, array<string> written)
	{
		array<string> keep = {};
		foreach (string path : written)
			keep.Insert(FileTail(path));

		array<string> files = {};
		HAM_ProfileFiles.ListJson(directory, files);
		foreach (string path : files)
		{
			if (keep.Find(FileTail(path)) >= 0)
				continue;

			FileIO.DeleteFile(path);
		}
	}

	protected static void SaveCargo()
	{
		if (!s_bCargoDirty || !Replication.IsServer())
			return;

		s_bCargoDirty = false;
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Item/";
		HAM_ProfileFiles.EnsureDir(directory);
		array<string> written = {};
		for (int i = 0; i < s_aCargoPrefab.Count(); i++)
		{
			JsonSaveContext ctx = new JsonSaveContext();
			ctx.WriteValue("prefab", s_aCargoPrefab[i]);
			ctx.WriteValue("x", s_aCargoX[i]);
			ctx.WriteValue("y", s_aCargoY[i]);
			ctx.WriteValue("z", s_aCargoZ[i]);
			ctx.WriteValue("pack", s_aCargoPack[i]);
			string path = HAM_ProfileFiles.ItemPath(directory, "Item_", i + 1, s_aCargoPrefab[i]);
			HAM_JsonQueue.Add(ctx, path);
			written.Insert(path);
		}

		DeleteStaleJson(directory, written);
	}

	protected static void LoadCargo()
	{
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Item/";
		array<string> files = {};
		HAM_ProfileFiles.ListJson(directory, files);
		if (!files.IsEmpty())
		{
			foreach (string path : files)
			{
				JsonLoadContext ctx = new JsonLoadContext();
				if (!ctx.LoadFromFile(path))
					continue;

				string prefab;
				float x;
				float y;
				float z;
				string pack;
				ctx.ReadValue("prefab", prefab);
				ctx.ReadValue("x", x);
				ctx.ReadValue("y", y);
				ctx.ReadValue("z", z);
				ctx.ReadValue("pack", pack);
				if (prefab.IsEmpty())
					continue;

				s_aCargoPrefab.Insert(prefab);
				s_aCargoX.Insert(x);
				s_aCargoY.Insert(y);
				s_aCargoZ.Insert(z);
				s_aCargoPack.Insert(pack);
			}

			return;
		}

		if (!FileIO.FileExists(CARGO_FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(CARGO_FILE))
			return;

		ctx.ReadValue("prefabs", s_aCargoPrefab);
		ctx.ReadValue("xs", s_aCargoX);
		ctx.ReadValue("ys", s_aCargoY);
		ctx.ReadValue("zs", s_aCargoZ);
		ctx.ReadValue("packs", s_aCargoPack);
		if (!s_aCargoPrefab)
			s_aCargoPrefab = {};
		if (!s_aCargoX)
			s_aCargoX = {};
		if (!s_aCargoY)
			s_aCargoY = {};
		if (!s_aCargoZ)
			s_aCargoZ = {};
		if (!s_aCargoPack)
			s_aCargoPack = {};

		s_bCargoDirty = true;
		SaveCargo();
	}

	protected static void SaveLoadouts()
	{
		if (!s_bLoadoutDirty || !Replication.IsServer())
			return;

		s_bLoadoutDirty = false;
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Persistence/";
		HAM_ProfileFiles.EnsureDir(directory);
		array<string> written = {};
		for (int i = 0; i < s_aLoadPrefab.Count(); i++)
		{
			JsonSaveContext ctx = new JsonSaveContext();
			ctx.WriteValue("prefab", s_aLoadPrefab[i]);
			ctx.WriteValue("pack", s_aLoadPack[i]);
			string path = HAM_ProfileFiles.ItemPath(directory, "Vehicule_", i + 1, s_aLoadPrefab[i]);
			HAM_JsonQueue.Add(ctx, path);
			written.Insert(path);
		}

		DeleteStaleJson(directory, written);
	}

	protected static void LoadLoadouts()
	{
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Persistence/";
		array<string> files = {};
		HAM_ProfileFiles.ListJson(directory, files);
		if (!files.IsEmpty())
		{
			foreach (string path : files)
			{
				JsonLoadContext ctx = new JsonLoadContext();
				if (!ctx.LoadFromFile(path))
					continue;

				string prefab;
				string pack;
				ctx.ReadValue("prefab", prefab);
				ctx.ReadValue("pack", pack);
				if (prefab.IsEmpty())
					continue;

				s_aLoadPrefab.Insert(prefab);
				s_aLoadPack.Insert(pack);
			}

			return;
		}

		if (!FileIO.FileExists(LOADOUT_FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(LOADOUT_FILE))
			return;

		ctx.ReadValue("prefabs", s_aLoadPrefab);
		ctx.ReadValue("packs", s_aLoadPack);
		if (!s_aLoadPrefab)
			s_aLoadPrefab = {};
		if (!s_aLoadPack)
			s_aLoadPack = {};

		s_bLoadoutDirty = true;
		SaveLoadouts();
	}

	protected static void SavePresets()
	{
		if (!s_bPresetDirty || !Replication.IsServer())
			return;

		s_bPresetDirty = false;
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Preset/";
		HAM_ProfileFiles.EnsureDir(directory);
		array<string> written = {};
		for (int i = 0; i < s_aPresetName.Count(); i++)
		{
			JsonSaveContext ctx = new JsonSaveContext();
			ctx.WriteValue("name", s_aPresetName[i]);
			ctx.WriteValue("pack", s_aPresetPack[i]);
			string path = HAM_ProfileFiles.ItemPath(directory, "Preset_", i + 1, s_aPresetName[i]);
			HAM_JsonQueue.Add(ctx, path);
			written.Insert(path);
		}

		DeleteStaleJson(directory, written);
	}

	protected static bool TrySpawnInto(IEntity vehicle, string prefab)
	{
		array<IEntity> nodes = {};
		CollectNodes(vehicle, nodes);
		foreach (IEntity node : nodes)
		{
			if (!node)
				continue;

			InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(node.FindComponent(InventoryStorageManagerComponent));
			if (!manager)
				continue;

			BaseInventoryStorageComponent storage = manager.FindStorageForResource(prefab, EStoragePurpose.PURPOSE_DEPOSIT);
			if (!storage)
				storage = manager.FindStorageForResource(prefab, EStoragePurpose.PURPOSE_ANY);

			if (storage && manager.TrySpawnPrefabToStorage(prefab, storage, -1, storage.GetPurpose()))
				return true;

			if (manager.CanInsertResource(prefab) && manager.TrySpawnPrefabToStorage(prefab, null, -1, EStoragePurpose.PURPOSE_ANY))
				return true;
		}

		return false;
	}

	protected static float s_fRefundBefore;
	protected static bool s_bRefundDepot;

	static void MarkDepotRefund(SCR_ResourceComponent resources)
	{
		s_bRefundDepot = false;
		s_fRefundBefore = 0;
		if (!resources)
			return;

		IEntity owner = resources.GetOwner();
		if (!owner || !owner.FindComponent(HAM_VehicleDepotComponent))
			return;

		s_bRefundDepot = true;
		s_fRefundBefore = ContainerSupplies(resources);
	}

	static void SettleDepotRefund(SCR_ResourceComponent resources)
	{
		if (!s_bRefundDepot || !resources)
			return;

		IEntity owner = resources.GetOwner();
		s_bRefundDepot = false;
		if (!owner)
			return;

		int gained = Whole(ContainerSupplies(resources) - s_fRefundBefore);
		if (gained > 0)
			Credit(owner, gained);

		MirrorSupplies(owner);
	}

	static void MirrorSupplies(IEntity depot)
	{
		if (!Replication.IsServer() || !depot)
			return;

		if (!depot.FindComponent(HAM_VehicleDepotComponent))
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(depot.FindComponent(SCR_ResourceComponent));
		SCR_ResourceContainer container = ContainerOf(resources);
		if (!container)
			return;

		float amount = 999999;
		SCR_CampaignMilitaryBaseComponent base = SupplyBaseAt(depot.GetOrigin());
		if (base)
			amount = base.GetSupplies();

		container.SetResourceValue(amount, false);
	}

	static void OnArsenalBuy(SCR_ResourceComponent resourceComponent, int resourceValue)
	{
		if (!Replication.IsServer() || !resourceComponent || resourceValue <= 0)
			return;

		IEntity owner = resourceComponent.GetOwner();
		if (!owner || !owner.FindComponent(HAM_VehicleDepotComponent))
			return;

		Pay(owner.GetOrigin(), resourceValue);
		MirrorSupplies(owner);
	}

	static void Credit(IEntity depot, int amount)
	{
		if (!depot || amount <= 0)
			return;

		Pay(depot.GetOrigin(), -amount);
	}

	static float ContainerSupplies(SCR_ResourceComponent resources)
	{
		SCR_ResourceContainer container = ContainerOf(resources);
		if (!container)
			return 0;

		return container.GetResourceValue();
	}

	static int Whole(float value)
	{
		int result = 0;
		if (value > 0)
		{
			while (value >= 1)
			{
				result = result + 1;
				value = value - 1;
			}

			return result;
		}

		while (value <= -1)
		{
			result = result - 1;
			value = value + 1;
		}

		return result;
	}

	protected static SCR_ResourceContainer ContainerOf(SCR_ResourceComponent resources)
	{
		if (!resources)
			return null;

		return resources.GetContainer(EResourceType.SUPPLIES);
	}

	protected static void LoadPresets()
	{
		string directory = "$profile:13RDPA_HeartsAndMinds/Depot/Preset/";
		array<string> files = {};
		HAM_ProfileFiles.ListJson(directory, files);
		if (!files.IsEmpty())
		{
			foreach (string path : files)
			{
				JsonLoadContext ctx = new JsonLoadContext();
				if (!ctx.LoadFromFile(path))
					continue;

				string name;
				string pack;
				ctx.ReadValue("name", name);
				ctx.ReadValue("pack", pack);
				if (name.IsEmpty())
					continue;

				int row = PresetRow(name);
				if (row < 0)
				{
					s_aPresetId.Insert("shared");
					s_aPresetName.Insert(name);
					s_aPresetPack.Insert(pack);
				}
				else
				{
					s_aPresetPack[row] = pack;
				}
			}

			return;
		}

		if (!FileIO.FileExists(PRESET_FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(PRESET_FILE))
			return;

		array<string> ids = {};
		array<string> names = {};
		array<string> packs = {};
		ctx.ReadValue("ids", ids);
		ctx.ReadValue("names", names);
		ctx.ReadValue("packs", packs);
		for (int i = 0; i < names.Count(); i++)
		{
			if (i >= packs.Count() || names[i].IsEmpty())
				continue;

			int row = PresetRow(names[i]);
			if (row < 0)
			{
				s_aPresetId.Insert("shared");
				s_aPresetName.Insert(names[i]);
				s_aPresetPack.Insert(packs[i]);
			}
			else
			{
				s_aPresetPack[row] = packs[i];
			}
		}

		s_bPresetDirty = true;
		SavePresets();
	}
}

void HAM_DepotConsiderSpawn(IEntity entity)
{
	HAM_VehicleDepot.ConsiderSpawn(entity);
}

modded class SCR_BaseGameMode
{
	override void OnControllableSpawned(IEntity entity)
	{
		super.OnControllableSpawned(entity);
		if (!Replication.IsServer() || !entity)
			return;

		if (!Vehicle.Cast(entity))
			return;

		GetGame().GetCallqueue().CallLater(HAM_DepotConsiderSpawn, 2000, false, entity);
	}
}

class HAM_VehicleDepotComponentClass : ScriptComponentClass
{
}

class HAM_VehicleDepotComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer() || !owner || !GetGame() || !GetGame().GetCallqueue())
			return;

		// Charge les fichiers du dépôt et programme la restauration (une seule fois).
		HAM_VehicleDepot.Start();

		GetGame().GetCallqueue().CallLater(HAM_DepotEnsureArsenal, 1000, false, owner);
		GetGame().GetCallqueue().CallLater(HAM_DepotEnsureSign, 1200, false, owner);
	}
}

void HAM_DepotEnsureArsenal(IEntity depot)
{
	if (!depot)
		return;

	IEntity child = depot.GetChildren();
	while (child)
	{
		if (child.FindComponent(SCR_ArsenalComponent) && child.FindComponent(SCR_ArsenalInventoryStorageManagerComponent))
			return;

		child = child.GetSibling();
	}

	Resource resource = Resource.Load("{BE99D77D1155B127}Prefabs/Props/Military/Arsenal/ArsenalBoxes/US/ArsenalBox_US.et");
	if (!resource || !resource.IsValid())
		return;

	EntitySpawnParams params = new EntitySpawnParams();
	params.TransformMode = ETransformMode.WORLD;
	depot.GetWorldTransform(params.Transform);
	IEntity box = GetGame().SpawnEntityPrefab(resource, depot.GetWorld(), params);
	if (!box)
		return;

	depot.AddChild(box, -1);
	vector mat[4];
	depot.GetWorldTransform(mat);
	mat[3] = depot.GetOrigin() + Vector(1.5, 0, 2.2);
	box.SetWorldTransform(mat);
}

void HAM_DepotEnsureSign(IEntity depot)
{
	if (!depot || !Replication.IsServer())
		return;

	IEntity child = depot.GetChildren();
	while (child)
	{
		if (child.GetPrefabData())
		{
			string prefab = child.GetPrefabData().GetPrefabName();
			if (prefab.Contains("Sign_"))
				return;
		}

		child = child.GetSibling();
	}

	Resource resource = Resource.Load("{D473C7DF1A7A8AAE}PrefabsEditable/Auto/Structures/Signs/Military/E_Sign_VehicleRepair_US_01.et");
	if (!resource || !resource.IsValid())
		return;

	EntitySpawnParams params = new EntitySpawnParams();
	params.TransformMode = ETransformMode.WORLD;
	depot.GetWorldTransform(params.Transform);
	IEntity sign = GetGame().SpawnEntityPrefab(resource, depot.GetWorld(), params);
	if (!sign)
		return;

	depot.AddChild(sign, -1);
	vector mat[4];
	depot.GetWorldTransform(mat);
	vector forward = mat[2];
	mat[3] = depot.GetOrigin() + forward * 3.1;
	sign.SetWorldTransform(mat);
}

void HAM_DepotOnArsenalBuy(SCR_ResourceComponent resourceComponent, ResourceName resourceName, IEntity requesterEntity, BaseInventoryStorageComponent inventoryStorageComponent, EResourceType resourceType, int resourceValue)
{
	HAM_VehicleDepot.OnArsenalBuy(resourceComponent, resourceValue);
}

class HAM_VehicleDepotFilter
{
	protected static const string FILE = "$profile:13RDPA_HeartsAndMinds/VehicleDepotFilter.json";
	protected static ref array<string> s_aShow;
	protected static ref array<string> s_aHide;
	protected static bool s_bLoaded;

	static void Start()
	{
		if (s_bLoaded)
			return;

		s_bLoaded = true;
		s_aShow = {};
		s_aHide = {};
		if (!Replication.IsServer())
			return;

		Load();
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

	static bool IsShown(string prefab, bool fallback)
	{
		Start();
		if (Listed(s_aShow, prefab))
			return true;

		if (Listed(s_aHide, prefab))
			return false;

		return fallback;
	}

	static void SetShown(string prefab, bool shown)
	{
		Start();
		Drop(s_aShow, prefab);
		Drop(s_aHide, prefab);
		if (shown)
			s_aShow.Insert(prefab);
		else
			s_aHide.Insert(prefab);

		if (Replication.IsServer())
			HAM_VehicleBoardStore.SetDepotShown(prefab, shown);
	}

	static void Remember(string prefab, bool shown)
	{
		if (prefab.IsEmpty())
			return;

		if (!s_aShow)
			s_aShow = {};

		if (!s_aHide)
			s_aHide = {};

		Drop(s_aShow, prefab);
		Drop(s_aHide, prefab);
		if (shown)
			s_aShow.Insert(prefab);
		else
			s_aHide.Insert(prefab);

		s_bLoaded = true;
	}

	static void PackChunks(notnull array<string> chunks)
	{
		Start();
		chunks.Clear();
		string current = "";
		if (s_aShow)
		{
			foreach (string prefab : s_aShow)
				HAM_AppendChunk(chunks, current, "1~" + prefab + "|");
		}

		if (s_aHide)
		{
			foreach (string prefab : s_aHide)
				HAM_AppendChunk(chunks, current, "0~" + prefab + "|");
		}

		if (!current.IsEmpty())
			chunks.Insert(current);
	}

	protected static void HAM_AppendChunk(notnull array<string> chunks, out string current, string piece)
	{
		if (current.Length() > 0 && current.Length() + piece.Length() > 1500)
		{
			chunks.Insert(current);
			current = "";
		}

		current = current + piece;
	}

	static void ApplyPack(string pack, bool reset)
	{
		if (reset)
		{
			s_aShow = {};
			s_aHide = {};
		}

		s_bLoaded = true;
		if (!s_aShow)
			s_aShow = {};

		if (!s_aHide)
			s_aHide = {};
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
				rest = "";
			}

			if (part.IsEmpty())
				continue;

			int mark = part.IndexOf("~");
			if (mark < 1)
				continue;

			string flag = part.Substring(0, mark);
			string prefab = part.Substring(mark + 1, part.Length() - mark - 1);
			if (prefab.IsEmpty())
				continue;

			if (flag == "1")
				s_aShow.Insert(prefab);
			else
				s_aHide.Insert(prefab);
		}
	}

	protected static bool Listed(array<string> list, string prefab)
	{
		if (!list || prefab.IsEmpty())
			return false;

		string path = PrefabPath(prefab);
		foreach (string entry : list)
		{
			if (entry == prefab || PrefabPath(entry) == path)
				return true;
		}

		return false;
	}

	protected static string PrefabPath(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}

	protected static void Drop(array<string> list, string prefab)
	{
		if (!list || prefab.IsEmpty())
			return;

		string path = PrefabPath(prefab);
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			if (list[i] == prefab || PrefabPath(list[i]) == path)
				list.Remove(i);
		}
	}

	protected static void Load()
	{
		if (!FileIO.FileExists(FILE))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(FILE))
			return;

		ctx.ReadValue("show", s_aShow);
		ctx.ReadValue("hide", s_aHide);
		if (!s_aShow)
			s_aShow = {};

		if (!s_aHide)
			s_aHide = {};

		if (!Replication.IsServer())
			return;

		foreach (string prefab : s_aShow)
			HAM_VehicleBoardStore.SetDepotShown(prefab, true);

		foreach (string prefab : s_aHide)
			HAM_VehicleBoardStore.SetDepotShown(prefab, false);
	}
}
