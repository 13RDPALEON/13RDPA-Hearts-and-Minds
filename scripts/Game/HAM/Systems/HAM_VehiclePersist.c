//------------------------------------------------------------------------------------------------
// Véhicules du joueur et véhicules posés au QG : prefab, position, fournitures, inventaire.
// Pas de remplissage automatique. Les patrouilles et le trafic civil ne sont pas dans cette liste.
//------------------------------------------------------------------------------------------------

class HAM_StoredItem
{
	string m_sPrefab;
	int m_iParent;
	float m_fSupplies;
}

class HAM_VehicleRecord
{
	string m_sKind;
	string m_sPrefab;
	vector m_vPos;
	float m_fYaw;
	vector m_vHome;
	float m_fSupplies;
	string m_sDeposit;
	float m_fCargoWater;
	float m_fCargoFood;
	float m_fCargoAmmo;
	ref array<ref HAM_StoredItem> m_aItems;
	IEntity m_Entity;
}

class HAM_VehiclePersist
{
	protected static const float HQ_RADIUS = 100;
	protected static const float MATCH_RADIUS = 25;
	protected static ref array<ref HAM_VehicleRecord> s_aTracked;
	protected static ref array<ref HAM_VehicleRecord> s_aLoaded;
	protected static ref array<string> s_aWatchPrefab;
	protected static ref array<vector> s_aWatchPos;
	protected static ref array<int> s_aWatchTries;
	protected static ref array<Vehicle> s_aSeen;
	protected static int s_iSnap;

	//------------------------------------------------------------------------------------------------
	static void NotePlayer(IEntity vehicle)
	{
		if (!vehicle || HAM_Util.IsDead(vehicle))
			return;

		if (FindTracked(vehicle))
			return;

		HAM_VehicleRecord row = new HAM_VehicleRecord();
		row.m_sKind = "player";
		row.m_Entity = vehicle;
		row.m_vHome = WorldPos(vehicle);
		row.m_aItems = {};
		if (!s_aTracked)
			s_aTracked = {};

		s_aTracked.Insert(row);
		HAM_Util.Forget(vehicle);
		StopGain(vehicle);
	}

	//------------------------------------------------------------------------------------------------
	static void WatchSpawn(string prefab, vector pos)
	{
		if (prefab.IsEmpty())
			return;

		if (!s_aWatchPrefab)
		{
			s_aWatchPrefab = {};
			s_aWatchPos = {};
			s_aWatchTries = {};
		}

		s_aWatchPrefab.Insert(prefab);
		s_aWatchPos.Insert(pos);
		s_aWatchTries.Insert(0);
		GetGame().GetCallqueue().CallLater(PollWatch, 1500, false);
	}

	//------------------------------------------------------------------------------------------------
	static bool Covers(string prefab, vector pos)
	{
		HAM_VehicleRecord row = MatchLoaded(prefab, pos);
		return row != null;
	}

	//------------------------------------------------------------------------------------------------
	static void RememberCargo(string prefab, vector pos, string deposit, float water, float food, float ammo)
	{
		HAM_VehicleRecord row = MatchLoaded(prefab, pos);
		if (!row)
			return;

		row.m_sDeposit = deposit;
		row.m_fCargoWater = water;
		row.m_fCargoFood = food;
		row.m_fCargoAmmo = ammo;
	}

	//------------------------------------------------------------------------------------------------
	static void Apply()
	{
		if (!s_aLoaded)
			return;

		array<Vehicle> claimed = {};
		foreach (HAM_VehicleRecord row : s_aLoaded)
		{
			if (!row || row.m_sPrefab.IsEmpty())
				continue;

			Vehicle vehicle;
			if (row.m_sKind == "hq")
				vehicle = FindNear(row.m_sPrefab, row.m_vHome, MATCH_RADIUS, claimed);
			if (!vehicle)
				vehicle = SpawnRecord(row);

			if (!vehicle)
				continue;

			claimed.Insert(vehicle);
			ApplyRecord(vehicle, row);
			row.m_Entity = vehicle;
			if (!s_aTracked)
				s_aTracked = {};

			s_aTracked.Insert(row);
		}

		HAM_Util.Info(string.Format("Véhicules reposés : %1", claimed.Count()));
	}

	//------------------------------------------------------------------------------------------------
	static void ScanHq()
	{
		if (!HAM_Mission.s_HqBase || !HAM_Mission.s_HqBase.GetOwner())
			return;

		array<Vehicle> found = VehiclesNear(HAM_Mission.s_HqBase.GetOwner().GetOrigin(), HQ_RADIUS);
		int added = 0;
		foreach (Vehicle vehicle : found)
		{
			if (!vehicle || HAM_Util.IsDead(vehicle) || FindTracked(vehicle))
				continue;

			if (!vehicle.GetPrefabData())
				continue;

			string prefab = vehicle.GetPrefabData().GetPrefabName();
			if (prefab.Contains("CIV"))
				continue;

			HAM_VehicleRecord row = new HAM_VehicleRecord();
			row.m_sKind = "hq";
			row.m_Entity = vehicle;
			row.m_vHome = WorldPos(vehicle);
			row.m_aItems = {};
			if (!s_aTracked)
				s_aTracked = {};

			s_aTracked.Insert(row);
			HAM_Util.Forget(vehicle);
			StopGain(vehicle);
			added++;
		}

		if (added > 0)
			HAM_Util.Info(string.Format("Véhicules du QG notés : %1", added));
	}

	//------------------------------------------------------------------------------------------------
	static void BeginSnapshot()
	{
		s_iSnap = 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool SnapshotOne()
	{
		if (!s_aTracked || s_iSnap >= s_aTracked.Count())
			return false;

		HAM_VehicleRecord row = s_aTracked[s_iSnap];
		s_iSnap++;
		if (!row || !row.m_Entity || HAM_Util.IsDead(row.m_Entity))
			return true;

		FillRecord(row, row.m_Entity);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void SnapshotAll()
	{
		if (!s_aTracked)
			return;

		foreach (HAM_VehicleRecord row : s_aTracked)
		{
			if (!row || !row.m_Entity || HAM_Util.IsDead(row.m_Entity))
				continue;

			FillRecord(row, row.m_Entity);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void WriteSave(JsonSaveContext ctx)
	{
		array<ref HAM_VehicleRecord> rows = {};
		if (s_aTracked)
		{
			foreach (HAM_VehicleRecord row : s_aTracked)
			{
				if (!row || row.m_sPrefab.IsEmpty() || !row.m_Entity || HAM_Util.IsDead(row.m_Entity))
					continue;

				rows.Insert(row);
			}
		}

		ctx.WriteValue("vehicleCount", rows.Count());
		for (int i = 0; i < rows.Count(); i++)
		{
			HAM_VehicleRecord row = rows[i];
			ctx.StartObject("vehicle" + i.ToString());
			ctx.WriteValue("kind", row.m_sKind);
			ctx.WriteValue("prefab", row.m_sPrefab);
			ctx.WriteValue("x", row.m_vPos[0]);
			ctx.WriteValue("y", row.m_vPos[1]);
			ctx.WriteValue("z", row.m_vPos[2]);
			ctx.WriteValue("yaw", row.m_fYaw);
			ctx.WriteValue("homeX", row.m_vHome[0]);
			ctx.WriteValue("homeY", row.m_vHome[1]);
			ctx.WriteValue("homeZ", row.m_vHome[2]);
			ctx.WriteValue("supplies", row.m_fSupplies);
			ctx.WriteValue("deposit", row.m_sDeposit);
			ctx.WriteValue("cargoWater", row.m_fCargoWater);
			ctx.WriteValue("cargoFood", row.m_fCargoFood);
			ctx.WriteValue("cargoAmmo", row.m_fCargoAmmo);
			int itemCount = 0;
			if (row.m_aItems)
				itemCount = row.m_aItems.Count();

			ctx.WriteValue("itemCount", itemCount);
			for (int n = 0; n < itemCount; n++)
			{
				HAM_StoredItem item = row.m_aItems[n];
				ctx.StartObject("item" + n.ToString());
				ctx.WriteValue("prefab", item.m_sPrefab);
				ctx.WriteValue("parent", item.m_iParent);
				ctx.WriteValue("supplies", item.m_fSupplies);
				ctx.EndObject();
			}

			ctx.EndObject();
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ReadSave(JsonLoadContext ctx)
	{
		s_aLoaded = {};
		int count;
		ctx.ReadValue("vehicleCount", count);
		for (int i = 0; i < count; i++)
		{
			if (!ctx.StartObject("vehicle" + i.ToString()))
				continue;

			HAM_VehicleRecord row = new HAM_VehicleRecord();
			ctx.ReadValue("kind", row.m_sKind);
			ctx.ReadValue("prefab", row.m_sPrefab);
			float x;
			float y;
			float z;
			float yaw;
			float homeX;
			float homeY;
			float homeZ;
			ctx.ReadValue("x", x);
			ctx.ReadValue("y", y);
			ctx.ReadValue("z", z);
			ctx.ReadValue("yaw", yaw);
			ctx.ReadValue("homeX", homeX);
			ctx.ReadValue("homeY", homeY);
			ctx.ReadValue("homeZ", homeZ);
			ctx.ReadValue("supplies", row.m_fSupplies);
			ctx.ReadValue("deposit", row.m_sDeposit);
			ctx.ReadValue("cargoWater", row.m_fCargoWater);
			ctx.ReadValue("cargoFood", row.m_fCargoFood);
			ctx.ReadValue("cargoAmmo", row.m_fCargoAmmo);
			row.m_vPos = Vector(x, y, z);
			row.m_fYaw = yaw;
			row.m_vHome = Vector(homeX, homeY, homeZ);
			row.m_aItems = {};
			int itemCount;
			ctx.ReadValue("itemCount", itemCount);
			for (int n = 0; n < itemCount; n++)
			{
				if (!ctx.StartObject("item" + n.ToString()))
					continue;

				HAM_StoredItem item = new HAM_StoredItem();
				ctx.ReadValue("prefab", item.m_sPrefab);
				ctx.ReadValue("parent", item.m_iParent);
				ctx.ReadValue("supplies", item.m_fSupplies);
				ctx.EndObject();
				row.m_aItems.Insert(item);
			}

			ctx.EndObject();
			if (!row.m_sPrefab.IsEmpty())
				s_aLoaded.Insert(row);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PollWatch()
	{
		if (!s_aWatchPrefab)
			return;

		bool again = false;
		for (int i = s_aWatchPrefab.Count() - 1; i >= 0; i--)
		{
			int tries = s_aWatchTries[i] + 1;
			s_aWatchTries[i] = tries;
			Vehicle found = FindNear(s_aWatchPrefab[i], s_aWatchPos[i], 40, null);
			if (found && !FindTracked(found))
			{
				NotePlayer(found);
				s_aWatchPrefab.Remove(i);
				s_aWatchPos.Remove(i);
				s_aWatchTries.Remove(i);
				continue;
			}

			if (tries < 8)
				again = true;
			else
			{
				s_aWatchPrefab.Remove(i);
				s_aWatchPos.Remove(i);
				s_aWatchTries.Remove(i);
			}
		}

		if (again)
			GetGame().GetCallqueue().CallLater(PollWatch, 1500, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_VehicleRecord FindTracked(IEntity vehicle)
	{
		if (!s_aTracked || !vehicle)
			return null;

		foreach (HAM_VehicleRecord row : s_aTracked)
		{
			if (row && row.m_Entity == vehicle)
				return row;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_VehicleRecord MatchLoaded(string prefab, vector pos)
	{
		if (!s_aLoaded || prefab.IsEmpty())
			return null;

		HAM_VehicleRecord best;
		float bestSq = 40 * 40;
		foreach (HAM_VehicleRecord row : s_aLoaded)
		{
			if (!row || row.m_sPrefab != prefab)
				continue;

			float distSq = vector.DistanceSq(row.m_vPos, pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = row;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static void FillRecord(HAM_VehicleRecord row, IEntity vehicle)
	{
		if (!row || !vehicle || !vehicle.GetPrefabData())
			return;

		row.m_sPrefab = vehicle.GetPrefabData().GetPrefabName();
		vector transform[4];
		vehicle.GetWorldTransform(transform);
		row.m_vPos = transform[3];
		vector angles = Math3D.MatrixToAngles(transform);
		row.m_fYaw = angles[0];
		row.m_fSupplies = SuppliesOf(vehicle);
		row.m_aItems = {};
		CaptureItems(vehicle, row.m_aItems);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyRecord(Vehicle vehicle, HAM_VehicleRecord row)
	{
		vector angles = Vector(row.m_fYaw, 0, 0);
		vector transform[4];
		Math3D.AnglesToMatrix(angles, transform);
		transform[3] = row.m_vPos;
		vehicle.SetWorldTransform(transform);
		SetSupplies(vehicle, row.m_fSupplies);
		StopGain(vehicle);
		ClearCargo(vehicle);
		RestoreItems(vehicle, row.m_aItems);
		if (row.m_fCargoWater + row.m_fCargoFood + row.m_fCargoAmmo >= 0.5)
			HAM_Logistics.AdoptLoadedCargo(vehicle, row.m_sDeposit, row.m_fCargoWater, row.m_fCargoFood, row.m_fCargoAmmo);

		CarControllerComponent car = CarControllerComponent.Cast(vehicle.FindComponent(CarControllerComponent));
		if (car)
			car.SetPersistentHandBrake(true);

		HAM_Util.Forget(vehicle);
	}

	//------------------------------------------------------------------------------------------------
	protected static Vehicle SpawnRecord(HAM_VehicleRecord row)
	{
		Resource resource = Resource.Load(row.m_sPrefab);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		vector angles = Vector(row.m_fYaw, 0, 0);
		Math3D.AnglesToMatrix(angles, params.Transform);
		params.Transform[3] = row.m_vPos;
		IEntity spawned = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		return Vehicle.Cast(spawned);
	}

	//------------------------------------------------------------------------------------------------
	protected static float SuppliesOf(IEntity vehicle)
	{
		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(vehicle.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return 0;

		SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
		if (!container)
			return 0;

		return container.GetResourceValue();
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetSupplies(IEntity vehicle, float value)
	{
		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(vehicle.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return;

		SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
		if (!container)
			return;

		if (value < 0)
			value = 0;

		float maxValue = container.GetMaxResourceValue();
		if (maxValue > 0.01 && value > maxValue)
			value = maxValue;

		container.SetResourceValue(value, true);
		container.EnableGain(false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void StopGain(IEntity node)
	{
		if (!node)
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container)
				container.EnableGain(false);
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			StopGain(child);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureItems(IEntity host, array<ref HAM_StoredItem> into)
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(host.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> items = {};
			storage.GetAll(items);
			foreach (IEntity item : items)
				CaptureItem(item, -1, into, 0);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CaptureItem(IEntity item, int parent, array<ref HAM_StoredItem> into, int depth)
	{
		if (!item || !item.GetPrefabData() || depth > 4)
			return;

		HAM_StoredItem row = new HAM_StoredItem();
		row.m_sPrefab = item.GetPrefabData().GetPrefabName();
		row.m_iParent = parent;
		row.m_fSupplies = -1;
		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(item.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container)
				row.m_fSupplies = container.GetResourceValue();
		}

		int index = into.Count();
		into.Insert(row);
		SCR_InventoryStorageManagerComponent nested = SCR_InventoryStorageManagerComponent.Cast(item.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!nested)
			return;

		array<BaseInventoryStorageComponent> storages = {};
		nested.GetStorages(storages);
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> items = {};
			storage.GetAll(items);
			foreach (IEntity child : items)
				CaptureItem(child, index, into, depth + 1);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearCargo(IEntity vehicle)
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(vehicle.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		array<IEntity> doomed = {};
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> items = {};
			storage.GetAll(items);
			foreach (IEntity item : items)
			{
				if (item)
					doomed.Insert(item);
			}
		}

		foreach (IEntity item : doomed)
			HAM_Util.Delete(item);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RestoreItems(IEntity vehicle, array<ref HAM_StoredItem> items)
	{
		if (!items)
			return;

		array<IEntity> spawned = {};
		foreach (HAM_StoredItem row : items)
		{
			IEntity host = vehicle;
			if (row.m_iParent >= 0 && row.m_iParent < spawned.Count())
				host = spawned[row.m_iParent];

			IEntity made = null;
			if (host)
				made = InsertItem(host, row);

			spawned.Insert(made);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity InsertItem(IEntity host, HAM_StoredItem row)
	{
		if (!host || !row || row.m_sPrefab.IsEmpty())
			return null;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(host.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return null;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		bool placed = false;
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			if (manager.TrySpawnPrefabToStorage(row.m_sPrefab, storage, -1, storage.GetPurpose()))
			{
				placed = true;
				break;
			}
		}

		if (!placed && manager.CanInsertResource(row.m_sPrefab))
			placed = manager.TrySpawnPrefabToStorage(row.m_sPrefab, null, -1, EStoragePurpose.PURPOSE_ANY);

		if (!placed || row.m_fSupplies < 0)
			return LastItem(host, row.m_sPrefab);

		IEntity made = LastItem(host, row.m_sPrefab);
		if (!made)
			return null;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(made.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return made;

		SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
		if (!container)
			return made;

		container.EnableGain(false);
		float maxValue = container.GetMaxResourceValue();
		float value = row.m_fSupplies;
		if (maxValue > 0.01 && value > maxValue)
			value = maxValue;

		container.SetResourceValue(value, true);
		return made;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity LastItem(IEntity host, string prefab)
	{
		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(host.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager)
			return null;

		array<BaseInventoryStorageComponent> storages = {};
		manager.GetStorages(storages);
		IEntity found;
		foreach (BaseInventoryStorageComponent storage : storages)
		{
			if (!storage)
				continue;

			array<IEntity> items = {};
			storage.GetAll(items);
			foreach (IEntity item : items)
			{
				if (!item || !item.GetPrefabData())
					continue;

				if (item.GetPrefabData().GetPrefabName() == prefab)
					found = item;
			}
		}

		return found;
	}

	//------------------------------------------------------------------------------------------------
	protected static Vehicle FindNear(string prefab, vector pos, float radius, array<Vehicle> claimed)
	{
		array<Vehicle> found = VehiclesNear(pos, radius);
		Vehicle best;
		float bestSq = radius * radius;
		foreach (Vehicle vehicle : found)
		{
			if (!vehicle || HAM_Util.IsDead(vehicle) || !vehicle.GetPrefabData())
				continue;

			if (claimed && claimed.Find(vehicle) >= 0)
				continue;

			if (FindTracked(vehicle))
				continue;

			if (vehicle.GetPrefabData().GetPrefabName() != prefab)
				continue;

			float distSq = vector.DistanceSq(WorldPos(vehicle), pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = vehicle;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<Vehicle> VehiclesNear(vector pos, float radius)
	{
		s_aSeen = {};
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return {};

		world.QueryEntitiesBySphere(pos, radius, SeeVehicle);
		array<Vehicle> copy = {};
		foreach (Vehicle vehicle : s_aSeen)
			copy.Insert(vehicle);

		s_aSeen.Clear();
		return copy;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeVehicle(IEntity ent)
	{
		Vehicle vehicle = Vehicle.Cast(ent);
		if (!vehicle)
			return true;

		if (!s_aSeen)
			s_aSeen = {};

		s_aSeen.Insert(vehicle);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector WorldPos(IEntity ent)
	{
		vector transform[4];
		ent.GetWorldTransform(transform);
		return transform[3];
	}
}
