//------------------------------------------------------------------------------------------------
// Logistique H&M : dépôts (eau, nourriture, munitions), cargaison des véhicules, livraison,
// entretien des maîtres, production d'hommes, délai de grâce, échange et transfert entre bases.
// Ces quatre stocks ne sont jamais les fournitures du Conflit.
//------------------------------------------------------------------------------------------------

class HAM_CargoSave
{
	string m_sDeposit;
	string m_sPrefab;
	vector m_vPos;
	float m_fWater;
	float m_fFood;
	float m_fAmmo;
}

class HAM_Logistics
{
	protected static const float CRATE_AMOUNT = 100;
	protected static const float CARGO_MATCH_RADIUS = 25;
	protected static const ResourceName SUPPLY_CRATE = "{56A8DC351E604EE1}Prefabs/HAM/Props/HAM_PersonalSupplyStack.et";

	protected static ref map<RplId, ref HAM_Stock> s_mCargo;
	protected static ref map<RplId, string> s_mCargoSource;
	protected static ref map<string, SCR_Task> s_mDemandTasks;
	protected static ref array<Vehicle> s_aScanVehicles;
	protected static ref array<Vehicle> s_aCargoClaimed;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_mCargo = null;
		s_mCargoSource = null;
		s_aCargoClaimed = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_mCargo)
			s_mCargo = new map<RplId, ref HAM_Stock>();

		if (!s_mCargoSource)
			s_mCargoSource = new map<RplId, string>();
	}

	//------------------------------------------------------------------------------------------------
	static void SlowTick(float dt)
	{
		HAM_Save.ApplyLoadedDeposits();
		HAM_Settings settings = HAM_Settings.Get();
		if (settings.m_bDepositsEnabled)
		{
			foreach (HAM_Deposit deposit : HAM_Mission.s_aDeposits)
			{
				RefillDeposit(deposit, dt, settings);
			}
		}

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell.IsMasterPlayer())
				continue;

			cell.m_fCycleLeft -= dt;
			if (cell.m_fCycleLeft <= 0)
			{
				cell.m_fCycleLeft = settings.m_fCycleMinutes * 60;
				RunCycle(cell, settings);
			}

			TickGrace(cell, dt, settings);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Dès qu'une ressource est sous son maximum, elle remonte jusqu'au plafond.
	//! Le rythme est celui du JSON : refillPerMinute, toutes les refillIntervalSeconds.
	protected static void RefillDeposit(HAM_Deposit deposit, float dt, HAM_Settings settings)
	{
		if (!deposit || !deposit.m_Stock)
			return;

		if (settings.m_fDepositWaterMax > 0.01 && deposit.m_Stock.m_fWater + 0.01 < settings.m_fDepositWaterMax)
			deposit.m_bRefillWater = true;

		if (settings.m_fDepositFoodMax > 0.01 && deposit.m_Stock.m_fFood + 0.01 < settings.m_fDepositFoodMax)
			deposit.m_bRefillFood = true;

		if (settings.m_fDepositAmmoMax > 0.01 && deposit.m_Stock.m_fAmmo + 0.01 < settings.m_fDepositAmmoMax)
			deposit.m_bRefillAmmo = true;

		float interval = settings.m_fDepositRefillInterval;
		if (interval < 1)
			interval = 1;

		deposit.m_fRefillLeft -= dt;
		if (deposit.m_fRefillLeft > 0)
			return;

		deposit.m_fRefillLeft = interval;
		float amount = settings.m_fDepositRefillPerMinute * interval / 60.0;
		if (amount <= 0)
			return;

		bool changed = false;

		if (deposit.m_bRefillWater && settings.m_fDepositWaterMax > 0.01)
		{
			float nextWater = deposit.m_Stock.m_fWater + amount;
			if (nextWater > settings.m_fDepositWaterMax)
				nextWater = settings.m_fDepositWaterMax;

			deposit.m_Stock.m_fWater = nextWater;
			deposit.m_bRefillWater = nextWater + 0.01 < settings.m_fDepositWaterMax;
			changed = true;
		}

		if (deposit.m_bRefillFood && settings.m_fDepositFoodMax > 0.01)
		{
			float nextFood = deposit.m_Stock.m_fFood + amount;
			if (nextFood > settings.m_fDepositFoodMax)
				nextFood = settings.m_fDepositFoodMax;

			deposit.m_Stock.m_fFood = nextFood;
			deposit.m_bRefillFood = nextFood + 0.01 < settings.m_fDepositFoodMax;
			changed = true;
		}

		if (deposit.m_bRefillAmmo && settings.m_fDepositAmmoMax > 0.01)
		{
			float nextAmmo = deposit.m_Stock.m_fAmmo + amount;
			if (nextAmmo > settings.m_fDepositAmmoMax)
				nextAmmo = settings.m_fDepositAmmoMax;

			deposit.m_Stock.m_fAmmo = nextAmmo;
			deposit.m_bRefillAmmo = nextAmmo + 0.01 < settings.m_fDepositAmmoMax;
			changed = true;
		}

		if (changed)
		{
			HAM_Mission.RefreshZoneProps(deposit.m_Cell);
			HAM_Mission.MarkDirty();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Entretien, puis production d'hommes avec le seul surplus au-dessus des seuils.
	protected static void RunCycle(HAM_Cell cell, HAM_Settings settings)
	{
		HAM_Stock stock = cell.m_Stock;
		stock.Add(HAM_EResource.WATER, -settings.m_fUpkeepWater);
		stock.Add(HAM_EResource.FOOD, -settings.m_fUpkeepFood);
		stock.Add(HAM_EResource.AMMO, -settings.m_fUpkeepAmmo);

		bool surplus = stock.m_fWater - settings.m_fWaterThreshold >= settings.m_fMenCostWater;
		surplus = surplus && stock.m_fFood - settings.m_fFoodThreshold >= settings.m_fMenCostFood;
		surplus = surplus && stock.m_fAmmo - settings.m_fAmmoThreshold >= settings.m_fMenCostAmmo;
		if (surplus && stock.m_iMen < settings.m_iMenMax)
		{
			stock.Add(HAM_EResource.WATER, -settings.m_fMenCostWater);
			stock.Add(HAM_EResource.FOOD, -settings.m_fMenCostFood);
			stock.Add(HAM_EResource.AMMO, -settings.m_fMenCostAmmo);
			int men = Math.Min(settings.m_iMenPerCycle, settings.m_iMenMax - stock.m_iMen);
			stock.Add(HAM_EResource.MEN, men);
		}

		CheckHold(cell);
		HAM_Mission.RefreshZoneProps(cell);
		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	static bool HoldsAboveThresholds(HAM_Stock stock)
	{
		HAM_Settings settings = HAM_Settings.Get();
		if (stock.m_fWater < settings.m_fWaterThreshold)
			return false;

		if (stock.m_fFood < settings.m_fFoodThreshold)
			return false;

		if (stock.m_fAmmo < settings.m_fAmmoThreshold)
			return false;

		return stock.m_iMen >= settings.m_iMenThreshold;
	}

	//------------------------------------------------------------------------------------------------
	//! Au-dessus des quatre seuils : la grâce s'arrête. En dessous : elle démarre si elle ne courait pas.
	static void CheckHold(HAM_Cell cell)
	{
		if (!cell)
			return;

		if (!cell.IsMasterPlayer())
		{
			ClearDemandTask(cell.m_sName);
			return;
		}

		if (HoldsAboveThresholds(cell.m_Stock))
		{
			if (cell.m_fGraceLeft >= 0)
				HAM_Mission.Notify("Ravitaillement", string.Format("%1 est ravitaillé : le délai de grâce est levé.", cell.m_sName), 6);

			cell.m_fGraceLeft = -1;
			ClearDemandTask(cell.m_sName);
			return;
		}

		if (cell.m_fGraceLeft < 0)
		{
			HAM_Settings settings = HAM_Settings.Get();
			cell.m_fGraceLeft = settings.m_fGraceMinutes * 60;
			int minutes = Math.Round(settings.m_fGraceMinutes);
			HAM_Mission.Notify("Ravitaillement", string.Format("%1 passe sous ses seuils : %2 min pour livrer avant de perdre la zone.", cell.m_sName, minutes), 10);
		}

		ShowDemandTask(cell);
	}

	//------------------------------------------------------------------------------------------------
	static void ClearDemand(HAM_Cell cell)
	{
		if (cell)
			ClearDemandTask(cell.m_sName);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ShowDemandTask(HAM_Cell cell)
	{
		if (!s_mDemandTasks)
			s_mDemandTasks = new map<string, SCR_Task>();

		SCR_Task existing;
		if (s_mDemandTasks.Find(cell.m_sName, existing) && existing)
			return;

		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem)
			return;

		HAM_Settings settings = HAM_Settings.Get();
		HAM_Stock stock = cell.m_Stock;
		string missing = "";
		if (stock.m_fWater < settings.m_fWaterThreshold)
			missing = missing + string.Format(" eau %1/%2", HAM_Util.FormatInt(stock.m_fWater), HAM_Util.FormatInt(settings.m_fWaterThreshold));

		if (stock.m_fFood < settings.m_fFoodThreshold)
			missing = missing + string.Format(" vivres %1/%2", HAM_Util.FormatInt(stock.m_fFood), HAM_Util.FormatInt(settings.m_fFoodThreshold));

		if (stock.m_fAmmo < settings.m_fAmmoThreshold)
			missing = missing + string.Format(" munitions %1/%2", HAM_Util.FormatInt(stock.m_fAmmo), HAM_Util.FormatInt(settings.m_fAmmoThreshold));

		if (stock.m_iMen < settings.m_iMenThreshold)
			missing = missing + string.Format(" hommes %1/%2", stock.m_iMen.ToString(), settings.m_iMenThreshold.ToString());

		string taskId = "HAM_DEMAND_" + cell.m_sName;
		SCR_Task task = taskSystem.CreateTask("{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et", taskId, cell.m_sName + " à ravitailler", "Sous les seuils :" + missing + ".", cell.Pos());
		if (!task)
			return;

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		s_mDemandTasks.Set(cell.m_sName, task);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearDemandTask(string name)
	{
		if (!s_mDemandTasks || name.IsEmpty())
			return;

		SCR_Task task;
		if (!s_mDemandTasks.Find(name, task))
			return;

		s_mDemandTasks.Remove(name);
		DeleteDemandTask(task);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteDemandTask(SCR_Task task)
	{
		if (!Replication.IsServer() || !task)
			return;

		task.SetTaskState(SCR_ETaskState.COMPLETED);
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (taskSystem)
			taskSystem.UnregisterTask(task);

		RplComponent replication = RplComponent.Cast(task.FindComponent(RplComponent));
		if (replication)
			replication.DeleteRplEntity(task, false);
		else
			SCR_EntityHelper.DeleteEntityAndChildren(task);
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickGrace(HAM_Cell cell, float dt, HAM_Settings settings)
	{
		if (cell.m_fGraceLeft < 0)
			return;

		float before = cell.m_fGraceLeft;
		cell.m_fGraceLeft -= dt;
		int beforeMinutes = Math.Ceil(before / 60.0);
		int afterMinutes = 0;
		if (cell.m_fGraceLeft > 0)
			afterMinutes = Math.Ceil(cell.m_fGraceLeft / 60.0);

		if (beforeMinutes != afterMinutes)
			HAM_Mission.MarkClientsDirty();

		if (before > 600 && cell.m_fGraceLeft <= 600)
			HAM_Mission.Notify("Ravitaillement", string.Format("%1 : 10 minutes avant la perte de la zone faute de ravitaillement.", cell.m_sName), 10);
		else if (before > 300 && cell.m_fGraceLeft <= 300)
			HAM_Mission.Notify("Ravitaillement", string.Format("%1 : 5 minutes avant la perte de la zone faute de ravitaillement.", cell.m_sName), 10);

		if (cell.m_fGraceLeft > 0)
			return;

		cell.m_fGraceLeft = -1;
		HAM_Front.LoseMaster(cell, "ravitaillement insuffisant");
	}

	//------------------------------------------------------------------------------------------------
	static void AdoptLoadedCargo(Vehicle vehicle, string deposit, float water, float food, float ammo)
	{
		HAM_Stock cargo = CargoOf(vehicle, true);
		if (!cargo)
			return;

		cargo.m_fWater = water;
		cargo.m_fFood = food;
		cargo.m_fAmmo = ammo;
		NoteSource(vehicle, deposit);
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Stock CargoOf(Vehicle vehicle, bool create)
	{
		Ensure();
		RplId id = HAM_Util.IdOf(vehicle);
		if (!id.IsValid())
			return null;

		HAM_Stock cargo;
		if (s_mCargo.Find(id, cargo))
			return cargo;

		if (!create)
			return null;

		cargo = new HAM_Stock();
		s_mCargo.Set(id, cargo);
		return cargo;
	}

	//------------------------------------------------------------------------------------------------
	static float CargoTotal(HAM_Stock cargo)
	{
		if (!cargo)
			return 0;

		return cargo.m_fWater + cargo.m_fFood + cargo.m_fAmmo;
	}

	//------------------------------------------------------------------------------------------------
	protected static void NoteSource(Vehicle vehicle, string depositName)
	{
		Ensure();
		RplId id = HAM_Util.IdOf(vehicle);
		if (!id.IsValid() || depositName.IsEmpty())
			return;

		s_mCargoSource.Set(id, depositName);
	}

	//------------------------------------------------------------------------------------------------
	protected static string SourceOf(RplId id)
	{
		if (!s_mCargoSource)
			return string.Empty;

		string name;
		if (!s_mCargoSource.Find(id, name))
			return string.Empty;

		return name;
	}

	//------------------------------------------------------------------------------------------------
	static void WriteSave(JsonSaveContext ctx)
	{
		Ensure();
		array<ref HAM_CargoSave> rows = {};
		array<RplId> drop = {};
		foreach (RplId id, HAM_Stock stock : s_mCargo)
		{
			if (!stock || CargoTotal(stock) < 0.5)
			{
				drop.Insert(id);
				continue;
			}

			IEntity ent = HAM_Util.EntityOf(id);
			Vehicle vehicle = Vehicle.Cast(ent);
			if (!vehicle || HAM_Util.IsDead(vehicle) || !vehicle.GetPrefabData())
			{
				RefundCargo(SourceOf(id), stock.m_fWater, stock.m_fFood, stock.m_fAmmo);
				drop.Insert(id);
				continue;
			}

			HAM_CargoSave row = new HAM_CargoSave();
			row.m_sDeposit = SourceOf(id);
			row.m_sPrefab = vehicle.GetPrefabData().GetPrefabName();
			row.m_vPos = vehicle.GetOrigin();
			row.m_fWater = stock.m_fWater;
			row.m_fFood = stock.m_fFood;
			row.m_fAmmo = stock.m_fAmmo;
			rows.Insert(row);
		}

		foreach (RplId gone : drop)
		{
			s_mCargo.Remove(gone);
			s_mCargoSource.Remove(gone);
		}

		ctx.WriteValue("cargoCount", rows.Count());
		for (int i = 0; i < rows.Count(); i++)
		{
			HAM_CargoSave row = rows[i];
			ctx.StartObject("cargo" + i.ToString());
			ctx.WriteValue("deposit", row.m_sDeposit);
			ctx.WriteValue("prefab", row.m_sPrefab);
			ctx.WriteValue("x", row.m_vPos[0]);
			ctx.WriteValue("y", row.m_vPos[1]);
			ctx.WriteValue("z", row.m_vPos[2]);
			ctx.WriteValue("water", row.m_fWater);
			ctx.WriteValue("food", row.m_fFood);
			ctx.WriteValue("ammo", row.m_fAmmo);
			ctx.EndObject();
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ReadSave(JsonLoadContext ctx)
	{
		Ensure();
		s_aCargoClaimed = {};
		int count;
		ctx.ReadValue("cargoCount", count);
		for (int i = 0; i < count; i++)
		{
			if (!ctx.StartObject("cargo" + i.ToString()))
				continue;

			HAM_CargoSave row = new HAM_CargoSave();
			ctx.ReadValue("deposit", row.m_sDeposit);
			ctx.ReadValue("prefab", row.m_sPrefab);
			float x;
			float y;
			float z;
			ctx.ReadValue("x", x);
			ctx.ReadValue("y", y);
			ctx.ReadValue("z", z);
			row.m_vPos = Vector(x, y, z);
			ctx.ReadValue("water", row.m_fWater);
			ctx.ReadValue("food", row.m_fFood);
			ctx.ReadValue("ammo", row.m_fAmmo);
			ctx.EndObject();
			RestoreCargo(row);
		}

		s_aCargoClaimed = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RestoreCargo(HAM_CargoSave row)
	{
		if (!row || row.m_fWater + row.m_fFood + row.m_fAmmo < 0.5)
			return;

		if (HAM_VehiclePersist.Covers(row.m_sPrefab, row.m_vPos))
		{
			HAM_VehiclePersist.RememberCargo(row.m_sPrefab, row.m_vPos, row.m_sDeposit, row.m_fWater, row.m_fFood, row.m_fAmmo);
			return;
		}

		Vehicle vehicle = MatchCargoVehicle(row);
		if (!vehicle)
		{
			RefundCargo(row.m_sDeposit, row.m_fWater, row.m_fFood, row.m_fAmmo);
			return;
		}

		HAM_Stock cargo = CargoOf(vehicle, true);
		if (!cargo)
		{
			RefundCargo(row.m_sDeposit, row.m_fWater, row.m_fFood, row.m_fAmmo);
			return;
		}

		cargo.m_fWater = row.m_fWater;
		cargo.m_fFood = row.m_fFood;
		cargo.m_fAmmo = row.m_fAmmo;
		NoteSource(vehicle, row.m_sDeposit);
		RestoreCrates(vehicle, row.m_fWater);
		RestoreCrates(vehicle, row.m_fFood);
		RestoreCrates(vehicle, row.m_fAmmo);
	}

	//------------------------------------------------------------------------------------------------
	//! Même pose qu'au chargement : une caisse noire par tranche. Les nombres H&M restent si une caisse ne tient pas.
	protected static void RestoreCrates(Vehicle vehicle, float amount)
	{
		float left = amount;
		while (left >= 1)
		{
			float pile = left;
			if (pile > CRATE_AMOUNT)
				pile = CRATE_AMOUNT;

			float stored = InsertPersonalCrate(vehicle, pile);
			if (stored < 1)
				return;

			left -= stored;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static Vehicle MatchCargoVehicle(HAM_CargoSave row)
	{
		if (!s_aScanVehicles)
			s_aScanVehicles = {};

		s_aScanVehicles.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(row.m_vPos, CARGO_MATCH_RADIUS, CollectVehicle);
		Vehicle best;
		float bestSq = CARGO_MATCH_RADIUS * CARGO_MATCH_RADIUS;
		foreach (Vehicle vehicle : s_aScanVehicles)
		{
			if (!vehicle || HAM_Util.IsDead(vehicle) || !vehicle.GetPrefabData())
				continue;

			if (s_aCargoClaimed && s_aCargoClaimed.Find(vehicle) >= 0)
				continue;

			if (vehicle.GetPrefabData().GetPrefabName() != row.m_sPrefab)
				continue;

			float distSq = vector.DistanceSq(vehicle.GetOrigin(), row.m_vPos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = vehicle;
		}

		s_aScanVehicles.Clear();
		if (best && s_aCargoClaimed)
			s_aCargoClaimed.Insert(best);

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RefundCargo(string depositName, float water, float food, float ammo)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Deposit deposit = HAM_Mission.FindDeposit(depositName);
		if (deposit)
		{
			float waterRoom = settings.m_fDepositWaterMax - deposit.m_Stock.m_fWater;
			float foodRoom = settings.m_fDepositFoodMax - deposit.m_Stock.m_fFood;
			float ammoRoom = settings.m_fDepositAmmoMax - deposit.m_Stock.m_fAmmo;
			if (waterRoom < 0)
				waterRoom = 0;
			if (foodRoom < 0)
				foodRoom = 0;
			if (ammoRoom < 0)
				ammoRoom = 0;
			if (water > waterRoom)
				water = waterRoom;
			if (food > foodRoom)
				food = foodRoom;
			if (ammo > ammoRoom)
				ammo = ammoRoom;

			deposit.m_Stock.Add(HAM_EResource.WATER, water);
			deposit.m_Stock.Add(HAM_EResource.FOOD, food);
			deposit.m_Stock.Add(HAM_EResource.AMMO, ammo);
			HAM_Mission.RefreshZoneProps(deposit.m_Cell);
			return;
		}

		if (!HAM_Mission.s_HqStock)
			return;

		HAM_Mission.s_HqStock.Add(HAM_EResource.WATER, water);
		HAM_Mission.s_HqStock.Add(HAM_EResource.FOOD, food);
		HAM_Mission.s_HqStock.Add(HAM_EResource.AMMO, ammo);
	}

	//------------------------------------------------------------------------------------------------
	//! Véhicule non détruit le plus proche dans le rayon.
	static Vehicle NearestVehicle(vector pos, float radius)
	{
		if (!s_aScanVehicles)
			s_aScanVehicles = {};

		s_aScanVehicles.Clear();
		GetGame().GetWorld().QueryEntitiesBySphere(pos, radius, CollectVehicle);
		Vehicle best;
		float bestSq = radius * radius;
		foreach (Vehicle vehicle : s_aScanVehicles)
		{
			if (!vehicle || HAM_Util.IsDead(vehicle))
				continue;

			float distSq = vector.DistanceSq(vehicle.GetOrigin(), pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = vehicle;
		}

		s_aScanVehicles.Clear();
		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool CollectVehicle(IEntity ent)
	{
		Vehicle vehicle = Vehicle.Cast(ent);
		if (vehicle)
			s_aScanVehicles.Insert(vehicle);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Charge le véhicule le plus proche depuis un dépôt. kind = WATER, FOOD, AMMO ; MEN = les trois à parts égales.
	static void LoadFromDeposit(string depositName, int playerId, HAM_EResource kind)
	{
		if (!HAM_Mission.IsRunning())
			return;

		HAM_Settings settings = HAM_Settings.Get();
		HAM_Deposit deposit = HAM_Mission.FindDeposit(depositName);
		if (!deposit || !deposit.m_Prop)
			return;

		vector pos = deposit.m_Prop.GetOrigin();
		if (HAM_Garrison.EnemyAliveNear(pos, 40))
		{
			HAM_Net.NotifyPlayer(playerId, "Dépôt", "Des ennemis tiennent encore le dépôt.", 6);
			return;
		}

		Vehicle vehicle = NearestVehicle(pos, settings.m_fLoadRadius);
		if (!vehicle)
		{
			HAM_Net.NotifyPlayer(playerId, "Dépôt", string.Format("Aucun véhicule à moins de %1 m du dépôt.", Math.Round(settings.m_fLoadRadius)), 6);
			return;
		}

		HAM_Stock cargo = CargoOf(vehicle, true);
		if (!cargo)
			return;

		float room = settings.m_fVehicleCapacity - CargoTotal(cargo);
		if (room < 1)
		{
			HAM_Net.NotifyPlayer(playerId, "Dépôt", string.Format("Ce véhicule a déjà ses %1 fournitures H&M.", HAM_Util.FormatInt(settings.m_fVehicleCapacity)), 6);
			return;
		}

		float loadedWater = 0;
		float loadedFood = 0;
		float loadedAmmo = 0;
		if (kind == HAM_EResource.MEN)
			TakeAllCrates(vehicle, deposit.m_Stock, room, loadedWater, loadedFood, loadedAmmo);
		else if (kind == HAM_EResource.WATER)
			loadedWater = TakeCrates(vehicle, deposit.m_Stock.m_fWater, room);
		else if (kind == HAM_EResource.FOOD)
			loadedFood = TakeCrates(vehicle, deposit.m_Stock.m_fFood, room);
		else if (kind == HAM_EResource.AMMO)
			loadedAmmo = TakeCrates(vehicle, deposit.m_Stock.m_fAmmo, room);

		float total = loadedWater + loadedFood + loadedAmmo;
		if (total < 0.5)
		{
			HAM_Net.NotifyPlayer(playerId, "Dépôt", "Le dépôt est vide, ou le véhicule n'a plus de place pour les fournitures.", 6);
			return;
		}

		deposit.m_Stock.Add(HAM_EResource.WATER, -loadedWater);
		deposit.m_Stock.Add(HAM_EResource.FOOD, -loadedFood);
		deposit.m_Stock.Add(HAM_EResource.AMMO, -loadedAmmo);
		cargo.Add(HAM_EResource.WATER, loadedWater);
		cargo.Add(HAM_EResource.FOOD, loadedFood);
		cargo.Add(HAM_EResource.AMMO, loadedAmmo);
		NoteSource(vehicle, deposit.m_Def.m_sName);

		HAM_Mission.RefreshZoneProps(deposit.m_Cell);
		HAM_Mission.MarkDirty();
		HAM_Net.NotifyPlayer(playerId, "Chargement", string.Format("Chargé : eau %1, nourriture %2, munitions %3.", HAM_Util.FormatInt(loadedWater), HAM_Util.FormatInt(loadedFood), HAM_Util.FormatInt(loadedAmmo)), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! Eau, nourriture et munitions, une caisse à la fois, jusqu'au coffre plein ou à la capacité H&M.
	protected static void TakeAllCrates(Vehicle vehicle, HAM_Stock stock, float room, out float water, out float food, out float ammo)
	{
		water = 0;
		food = 0;
		ammo = 0;
		while (room >= 1)
		{
			float added = TakeCrates(vehicle, Math.Min(CRATE_AMOUNT, stock.m_fWater - water), room);
			if (added < 0)
				return;

			water += added;
			room -= added;
			added = TakeCrates(vehicle, Math.Min(CRATE_AMOUNT, stock.m_fFood - food), room);
			if (added < 0)
				return;

			food += added;
			room -= added;
			added = TakeCrates(vehicle, Math.Min(CRATE_AMOUNT, stock.m_fAmmo - ammo), room);
			if (added < 0)
				return;

			ammo += added;
			room -= added;
			if (water + food + ammo < 1)
				return;

			if (stock.m_fWater - water < 1 && stock.m_fFood - food < 1 && stock.m_fAmmo - ammo < 1)
				return;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Pose une caisse noire dans le coffre du véhicule, dans la place H&M qui reste. Négatif : plus de place.
	protected static float TakeCrates(Vehicle vehicle, float available, float room)
	{
		if (available < 1 || room < 1)
			return 0;

		float pile = available;
		if (pile > CRATE_AMOUNT)
			pile = CRATE_AMOUNT;

		if (pile > room)
			pile = room;

		return InsertPersonalCrate(vehicle, pile);
	}

	//------------------------------------------------------------------------------------------------
	protected static float InsertPersonalCrate(Vehicle vehicle, float amount)
	{
		if (!vehicle || amount < 1)
			return -1;

		InventoryStorageManagerComponent manager = CargoManager(vehicle);
		if (!manager)
			return -1;

		array<IEntity> before = {};
		GatherCargo(manager, before);
		if (!SpawnCrateInto(vehicle, manager))
			return -1;

		array<IEntity> after = {};
		GatherCargo(manager, after);
		IEntity crate;
		foreach (IEntity item : after)
		{
			if (!item || before.Find(item) >= 0 || !IsPersonalCrate(item))
				continue;

			crate = item;
			break;
		}

		if (!crate)
		{
			foreach (IEntity item : after)
			{
				if (item && before.Find(item) < 0)
					HAM_Util.Delete(item);
			}

			return -1;
		}

		return SetCrateAmount(crate, amount);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpawnCrateInto(Vehicle vehicle, InventoryStorageManagerComponent manager)
	{
		BaseInventoryStorageComponent storage = manager.FindStorageForResource(SUPPLY_CRATE, EStoragePurpose.PURPOSE_DEPOSIT);
		if (!storage)
			storage = manager.FindStorageForResource(SUPPLY_CRATE, EStoragePurpose.PURPOSE_ANY);

		if (storage && manager.TrySpawnPrefabToStorage(SUPPLY_CRATE, storage, -1, storage.GetPurpose()))
			return true;

		if (manager.CanInsertResource(SUPPLY_CRATE) && manager.TrySpawnPrefabToStorage(SUPPLY_CRATE, null, -1, EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (manager.CanInsertResource(SUPPLY_CRATE) && manager.TrySpawnPrefabToStorage(SUPPLY_CRATE, null, -1, EStoragePurpose.PURPOSE_ANY))
			return true;

		Resource resource = Resource.Load(SUPPLY_CRATE);
		if (!resource || !resource.IsValid())
			return false;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		Math3D.MatrixIdentity4(params.Transform);
		params.Transform[3] = vehicle.GetOrigin();
		IEntity item = GetGame().SpawnEntityPrefab(resource, vehicle.GetWorld(), params);
		if (!item)
			return false;

		if (manager.TryInsertItem(item, EStoragePurpose.PURPOSE_DEPOSIT))
			return true;

		if (manager.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY))
			return true;

		HAM_Util.Delete(item);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static float SetCrateAmount(IEntity crate, float amount)
	{
		SCR_ResourceContainer container = CrateContainer(crate);
		if (!container)
		{
			HAM_Util.Delete(crate);
			return -1;
		}

		if (container.GetMaxResourceValue() < amount)
			container.SetMaxResourceValue(amount, true);

		SCR_ResourceEncapsulator own = CrateEncapsulator(crate);
		if (own)
		{
			float missing = amount - container.GetResourceValue();
			if (missing >= 1)
				own.RequestGeneration(missing, true);
		}

		float posed = container.GetResourceValue();
		bool shortOf = posed + 0.5 < amount;
		bool over = posed > amount + 0.5;
		if (shortOf || over)
		{
			SCR_ResourceEncapsulator linked = container.GetResourceEncapsulator();
			if (linked && linked.GetOwner() != crate)
				container.SetResourceValueUnsafe(amount, true);
			else
				container.SetResourceValue(amount, true);
		}

		float stored = container.GetResourceValue();
		if (stored < 1)
		{
			HAM_Util.Delete(crate);
			return -1;
		}

		return stored;
	}

	//------------------------------------------------------------------------------------------------
	//! Encapsuleur SUPPLIES porté par la caisse. Celui du véhicule n'est pas le sien.
	protected static SCR_ResourceEncapsulator CrateEncapsulator(IEntity crate)
	{
		if (!crate)
			return null;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(crate.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return null;

		SCR_ResourceEncapsulator enc = resources.GetEncapsulator(EResourceType.SUPPLIES);
		if (!enc || enc.GetOwner() != crate)
			return null;

		return enc;
	}

	//------------------------------------------------------------------------------------------------
	protected static float TakeVehicleSupplies(Vehicle vehicle, float amount)
	{
		if (!vehicle || amount < 1)
			return 0;

		InventoryStorageManagerComponent manager = CargoManager(vehicle);
		if (!manager)
			return 0;

		array<IEntity> items = {};
		GatherCargo(manager, items);
		float left = amount;
		foreach (IEntity item : items)
		{
			if (!IsPersonalCrate(item))
				continue;

			SCR_ResourceContainer container = CrateContainer(item);
			float value = 0;
			if (container)
				value = container.GetResourceValue();

			if (value <= left + 0.5)
			{
				left -= value;
				if (!manager.TryDeleteItem(item))
					HAM_Util.Delete(item);
			}
			else if (container)
			{
				container.SetResourceValue(value - left, true);
				left = 0;
			}

			if (left < 1)
				break;
		}

		float removed = amount - left;
		if (removed < 0)
			removed = 0;

		return removed;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeletePersonalCrates(Vehicle vehicle)
	{
		InventoryStorageManagerComponent manager = CargoManager(vehicle);
		if (!manager)
			return;

		array<IEntity> items = {};
		GatherCargo(manager, items);
		foreach (IEntity item : items)
		{
			if (!IsPersonalCrate(item))
				continue;

			if (!manager.TryDeleteItem(item))
				HAM_Util.Delete(item);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsPersonalCrate(IEntity item)
	{
		if (!item || !item.GetPrefabData())
			return false;

		return item.GetPrefabData().GetPrefabName().Contains("HAM_PersonalSupplyStack");
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasPersonalCrate(Vehicle vehicle)
	{
		InventoryStorageManagerComponent manager = CargoManager(vehicle);
		if (!manager)
			return false;

		array<IEntity> items = {};
		GatherCargo(manager, items);
		foreach (IEntity item : items)
		{
			if (IsPersonalCrate(item))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ResourceContainer CrateContainer(IEntity item)
	{
		if (!item)
			return null;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(item.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return null;

		return resources.GetContainer(EResourceType.SUPPLIES);
	}

	//------------------------------------------------------------------------------------------------
	protected static InventoryStorageManagerComponent CargoManager(IEntity vehicle)
	{
		array<IEntity> nodes = {};
		CollectCargoNodes(vehicle, nodes);
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

	//------------------------------------------------------------------------------------------------
	protected static void CollectCargoNodes(IEntity root, array<IEntity> nodes)
	{
		if (!root)
			return;

		nodes.Insert(root);
		IEntity child = root.GetChildren();
		while (child)
		{
			CollectCargoNodes(child, nodes);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void GatherCargo(InventoryStorageManagerComponent manager, array<IEntity> items)
	{
		items.Clear();
		if (!manager)
			return;

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
			foreach (IEntity storedItem : stored)
			{
				if (storedItem && items.Find(storedItem) < 0)
					items.Insert(storedItem);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Décharge le véhicule le plus proche sur une base alliée (maître tenu ou QG).
	static void Unload(string baseName, int playerId, vector near)
	{
		if (!HAM_Mission.IsRunning())
			return;

		if (!HAM_Mission.IsAlliedBase(baseName))
		{
			HAM_Net.NotifyPlayer(playerId, "Livraison", "Cette zone n'est pas tenue : on ne livre qu'à un maître tenu ou au QG.", 6);
			return;
		}

		HAM_Settings settings = HAM_Settings.Get();
		Vehicle vehicle = NearestVehicle(near, settings.m_fLoadRadius);
		HAM_Stock cargo = CargoOf(vehicle, false);
		if (!vehicle || !cargo || CargoTotal(cargo) < 0.5)
		{
			HAM_Net.NotifyPlayer(playerId, "Livraison", string.Format("Aucun véhicule chargé à moins de %1 m.", Math.Round(settings.m_fLoadRadius)), 6);
			return;
		}

		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		if (!stock)
			return;

		float water = cargo.m_fWater;
		float food = cargo.m_fFood;
		float ammo = cargo.m_fAmmo;
		float wanted = water + food + ammo;
		float removed = TakeVehicleSupplies(vehicle, wanted);
		DeletePersonalCrates(vehicle);
		if (removed < 0.5 && wanted > 0.5)
		{
			if (!CargoManager(vehicle))
			{
				HAM_Net.NotifyPlayer(playerId, "Livraison", "Le coffre du véhicule est illisible. La cargaison H&M reste dans le véhicule.", 8);
				return;
			}

			if (!HasPersonalCrate(vehicle))
			{
				string source = SourceOf(HAM_Util.IdOf(vehicle));
				if (HAM_Mission.FindDeposit(source) || HAM_Mission.s_HqStock)
				{
					RefundCargo(source, cargo.m_fWater, cargo.m_fFood, cargo.m_fAmmo);
					cargo.Clear();
					if (s_mCargoSource)
						s_mCargoSource.Remove(HAM_Util.IdOf(vehicle));

					HAM_Mission.MarkDirty();
					HAM_Net.NotifyPlayer(playerId, "Livraison", "Les caisses noires ne sont plus dans le véhicule : les fournitures retournent au dépôt d'origine.", 8);
					return;
				}
			}

			HAM_Net.NotifyPlayer(playerId, "Livraison", "Aucune caisse noire n'a été déchargée. La cargaison H&M reste dans le véhicule.", 8);
			return;
		}

		stock.Add(HAM_EResource.WATER, water);
		stock.Add(HAM_EResource.FOOD, food);
		stock.Add(HAM_EResource.AMMO, ammo);
		cargo.Clear();

		HAM_Cell cell = HAM_Mission.FindCell(baseName);
		if (cell)
			CheckHold(cell);

		HAM_Mission.RefreshBaseProp(baseName);
		HAM_Mission.MarkDirty();
		HAM_Net.NotifyPlayer(playerId, "Livraison", string.Format("Livré à %1 : eau %2, nourriture %3, munitions %4.", baseName, HAM_Util.FormatInt(water), HAM_Util.FormatInt(food), HAM_Util.FormatInt(ammo)), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! Échange sur place : une ressource contre une autre, au taux réglé. Eau, nourriture, munitions seulement.
	static bool Exchange(string baseName, HAM_EResource from, HAM_EResource to, float amount, int playerId)
	{
		if (!HAM_Mission.IsAlliedBase(baseName) || from == to)
			return false;

		if (from == HAM_EResource.MEN || to == HAM_EResource.MEN || from == HAM_EResource.SUPPLIES || to == HAM_EResource.SUPPLIES)
			return false;

		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		if (!stock || amount <= 0)
			return false;

		if (!stock.Take(from, amount))
		{
			HAM_Net.NotifyPlayer(playerId, "Échange", string.Format("Pas assez de %1 à %2.", HAM_Stock.Label(from), baseName), 6);
			return false;
		}

		float received = amount * HAM_Settings.Get().m_fExchangeRatio;
		stock.Add(to, received);
		HAM_Cell cell = HAM_Mission.FindCell(baseName);
		if (cell)
			CheckHold(cell);

		HAM_Mission.RefreshBaseProp(baseName);
		HAM_Mission.MarkDirty();
		HAM_Net.NotifyPlayer(playerId, "Échange", string.Format("%1 : %2 %3 contre %4 %5.", baseName, HAM_Util.FormatInt(amount), HAM_Stock.Label(from), HAM_Util.FormatInt(received), HAM_Stock.Label(to)), 6);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Transfert vers une autre base alliée. Les hommes partent en convoi ; les fournitures passent par le Conflit.
	static bool Transfer(string fromBase, string toBase, HAM_EResource kind, float amount, int playerId)
	{
		if (fromBase == toBase || amount <= 0)
			return false;

		if (!HAM_Mission.IsAlliedBase(fromBase) || !HAM_Mission.IsAlliedBase(toBase))
		{
			HAM_Net.NotifyPlayer(playerId, "Transfert", "Les transferts se font seulement entre bases alliées et le QG.", 6);
			return false;
		}

		if (kind == HAM_EResource.MEN)
			return HAM_Convoys.Order(fromBase, toBase, Math.Round(amount), playerId);

		if (kind == HAM_EResource.SUPPLIES)
			return TransferSupplies(fromBase, toBase, Math.Round(amount), playerId);

		HAM_Stock source = HAM_Mission.StockOf(fromBase);
		HAM_Stock target = HAM_Mission.StockOf(toBase);
		if (!source || !target)
			return false;

		if (!source.Take(kind, amount))
		{
			HAM_Net.NotifyPlayer(playerId, "Transfert", string.Format("Pas assez de %1 à %2.", HAM_Stock.Label(kind), fromBase), 6);
			return false;
		}

		target.Add(kind, amount);
		HAM_Cell sourceCell = HAM_Mission.FindCell(fromBase);
		if (sourceCell)
			CheckHold(sourceCell);

		HAM_Cell targetCell = HAM_Mission.FindCell(toBase);
		if (targetCell)
			CheckHold(targetCell);

		HAM_Mission.RefreshBaseProp(fromBase);
		HAM_Mission.RefreshBaseProp(toBase);
		HAM_Mission.MarkDirty();
		HAM_Net.NotifyPlayer(playerId, "Transfert", string.Format("%1 %2 : %3 → %4.", HAM_Util.FormatInt(amount), HAM_Stock.Label(kind), fromBase, toBase), 6);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Fournitures du Conflit : seulement entre bases Conflit (QG de départ, QG désignés).
	protected static bool TransferSupplies(string fromBase, string toBase, int amount, int playerId)
	{
		SCR_CampaignMilitaryBaseComponent source = HAM_HqDesignation.ConflictBaseOf(fromBase);
		SCR_CampaignMilitaryBaseComponent target = HAM_HqDesignation.ConflictBaseOf(toBase);
		if (!source || !target)
		{
			HAM_Net.NotifyPlayer(playerId, "Transfert", "Les fournitures ne circulent qu'entre bases Conflit (QG de départ et QG désignés).", 8);
			return false;
		}

		if (source.GetSupplies() < amount)
		{
			HAM_Net.NotifyPlayer(playerId, "Transfert", string.Format("Pas assez de fournitures à %1.", fromBase), 6);
			return false;
		}

		source.AddSupplies(-amount);
		target.AddSupplies(amount);
		HAM_Net.NotifyPlayer(playerId, "Transfert", string.Format("%1 fournitures : %2 → %3.", amount, fromBase, toBase), 6);
		return true;
	}
}
