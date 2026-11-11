//------------------------------------------------------------------------------------------------
// Le maître, comme mission.sqf : charge les réglages, les pools, la carte et la sauvegarde,
// pose les prefabs de zone, puis fait tourner les systèmes du front.
//------------------------------------------------------------------------------------------------

class HAM_MissionComponentClass : SCR_BaseGameModeComponentClass
{
}

class HAM_MissionComponent : SCR_BaseGameModeComponent
{
	[Attribute("Cain", desc: "Clé de la carte dans HAM_MapRegistry")]
	protected string m_sMapKey;

	//------------------------------------------------------------------------------------------------
	string GetMapKey()
	{
		return m_sMapKey;
	}

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		HAM_Mission.RegisterComponent(this);
		GetGame().GetCallqueue().CallLater(HAM_Mission.Boot, 60000, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		HAM_Mission.RegisterComponent(this);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(HAM_Mission.Boot, 3000, false);
	}

	//------------------------------------------------------------------------------------------------
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		if (Replication.IsServer())
			GetGame().GetCallqueue().CallLater(HAM_Net.SyncPlayer, 1000, false, playerId);
	}

	//------------------------------------------------------------------------------------------------
	override void OnControllableDestroyed(notnull SCR_InstigatorContextData instigatorContextData)
	{
		super.OnControllableDestroyed(instigatorContextData);
		if (Replication.IsServer())
			HAM_Mission.OnKill(instigatorContextData);
	}

	//------------------------------------------------------------------------------------------------
	//! Les marqueurs gardent leur widget : ils doivent partir avec le monde, pas après le jeu.
	override void OnDelete(IEntity owner)
	{
		HAM_ClientMarkers.Release();
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameEnd()
	{
		if (Replication.IsServer())
			HAM_Mission.SaveImmediate();

		super.OnGameEnd();
	}

	//------------------------------------------------------------------------------------------------
	override void OnGameModeEnd(SCR_GameModeEndData data)
	{
		if (Replication.IsServer())
			HAM_Mission.SaveImmediate();

		super.OnGameModeEnd(data);
	}
}

//------------------------------------------------------------------------------------------------
void HAM_OnPlayerSpawned(int playerId, IEntity body)
{
	HAM_Mission.OnPlayerBody(playerId, body);
}

class HAM_Mission
{
	static const string HQ_NAME = "QG";
	static const int TICK_MS = 3000;
	static const int SLOW_TICK_MS = 30000;
	static const float CELL_MAX_RADIUS = 2500;
	static const float BASE_MATCH_RADIUS = 500;

	protected static HAM_MissionComponent s_Component;
	protected static bool s_bBooted;
	protected static bool s_bRunning;
	protected static bool s_bEnded;
	protected static bool s_bClientsDirty;
	protected static bool s_bSaveDirty;
	protected static float s_fLastSave;
	protected static float s_fSupplyArmedAt;
	protected static float s_fMarkerStamp;

	static ref HAM_MapDef s_Map;
	static ref array<ref HAM_Cell> s_aCells;
	static ref array<ref HAM_Point> s_aPoints;
	static ref array<ref HAM_Deposit> s_aDeposits;
	static ref array<ref HAM_Line> s_aLines;
	static HAM_MapPoint s_HqPoint;
	static ref HAM_Stock s_HqStock;
	static SCR_CampaignMilitaryBaseComponent s_HqBase;
	static SCR_CampaignMilitaryBaseComponent s_EnemyHqBase;
	static string s_sDesignatedHq = "QG";
	static float s_fGlobalRep;
	static bool s_bTriSeeded;
	static ref array<int> s_aPlayerIds;
	static ref array<IEntity> s_aPlayerBodies;
	protected static ref array<IEntity> s_aSupplyArmPending;
	protected static bool s_bArmingSupply;

	//------------------------------------------------------------------------------------------------
	static void RegisterComponent(HAM_MissionComponent component)
	{
		if (!component || component == s_Component)
			return;

		// Nouveau monde (rotation de mission, relance en Workbench) : les statiques de la session précédente partent.
		if (s_Component || s_bBooted)
			ResetSession();

		s_Component = component;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ResetSession()
	{
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().Remove(SlowTick);
		s_bBooted = false;
		s_bRunning = false;
		s_bEnded = false;
		s_bClientsDirty = false;
		s_bSaveDirty = false;
		s_Map = null;
		s_aCells = null;
		s_aPoints = null;
		s_aDeposits = null;
		s_aLines = null;
		s_HqPoint = null;
		s_HqStock = null;
		s_HqBase = null;
		s_EnemyHqBase = null;
		s_sDesignatedHq = HQ_NAME;
		s_bTriSeeded = false;
		s_aSupplyArmPending = null;
		s_bArmingSupply = false;
		HAM_Settings.Reload();
		HAM_Front.ResetSession();
		HAM_Garrison.ResetSession();
		HAM_Logistics.ResetSession();
		HAM_Convoys.ResetSession();
		HAM_CounterAttack.ResetSession();
		HAM_Save.ResetSession();
		HAM_Devices.ResetSession();
		HAM_Intel.ResetSession();
		HAM_SideMissions.ResetSession();
		HAM_Hospital.ResetSession();
	}

	//------------------------------------------------------------------------------------------------
	//! Le mode de jeu porte le composant H&M (vrai aussi chez les clients).
	static bool IsActive()
	{
		if (s_Component)
			return true;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return false;

		s_Component = HAM_MissionComponent.Cast(gameMode.FindComponent(HAM_MissionComponent));
		return s_Component != null;
	}

	//------------------------------------------------------------------------------------------------
	//! Le front tourne (serveur).
	static bool IsRunning()
	{
		return s_bRunning && !s_bEnded;
	}

	//------------------------------------------------------------------------------------------------
	static void Boot()
	{
		if (s_bBooted || !GetGame().InPlayMode() || !Replication.IsServer() || !IsActive())
			return;

		s_bBooted = true;
		HAM_Save.WatchMoves();
		HAM_Settings.Get();
		HAM_Units.Init();
		HAM_Tri.Init();
		GetGame().GetCallqueue().CallLater(WaitConflict, 2000, false, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected static void WaitConflict(int attempt)
	{
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (campaign && !campaign.HasStarted() && attempt < 30)
		{
			GetGame().GetCallqueue().CallLater(WaitConflict, 2000, false, attempt + 1);
			return;
		}

		Start();
	}

	//------------------------------------------------------------------------------------------------
	protected static void Start()
	{
		string key = s_Component.GetMapKey();
		s_Map = new HAM_MapDef();
		if (!HAM_MapRegistry.Build(key, s_Map))
		{
			Print("[13RDPA H&M] Carte « " + key + " » absente de HAM_MapRegistry : pas de front.", LogLevel.WARNING);
			return;
		}

		s_Map.Validate();
		s_HqPoint = s_Map.FindRole(HAM_ERole.HQ);
		if (!s_HqPoint)
		{
			Print("[13RDPA H&M] Carte « " + key + " » sans ligne Hq : pas de front. Les coordonnées sont à écrire dans HAM_Map_" + key + ".c.", LogLevel.WARNING);
			return;
		}

		BuildState();
		FindConflictBases();
		HAM_Save.Load();
		PlaceZonePrefabs();
		HAM_SideMissions.DropPersisted();
		GetGame().GetCallqueue().CallLater(HAM_SideMissions.DropPersisted, 2000, false);
		HAM_HqDesignation.Restore();

		// Une fois par campagne : ensuite les fichiers de tri du profil gardent les réglages faits en jeu.
		if (!s_bTriSeeded)
		{
			int seeded = HAM_Tri.SeedBoards();
			s_bTriSeeded = true;
			if (seeded > 0)
				Print(string.Format("[13RDPA H&M] Tri amorcé : %1 lignes", seeded), LogLevel.NORMAL);
		}

		foreach (HAM_Cell cell : s_aCells)
		{
			if (cell.IsComplete())
				cell.m_bRevealed = true;

			RefreshZoneProps(cell);
		}

		GetGame().GetCallqueue().CallLater(RefreshLoadedStocks, 2000, false);

		s_bRunning = true;
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (mode)
			mode.GetOnPlayerSpawned().Insert(HAM_OnPlayerSpawned);

		s_fLastSave = HAM_Util.Now();
		s_aPlayerIds = {};
		s_aPlayerBodies = {};
		MarkClientsDirty();
		GetGame().GetCallqueue().CallLater(HAM_VehiclePersist.Apply, 4000, false);
		GetGame().GetCallqueue().CallLater(HAM_VehiclePersist.ScanHq, 8000, false);

		GetGame().GetCallqueue().CallLater(Tick, TICK_MS, true);
		GetGame().GetCallqueue().CallLater(SlowTick, SLOW_TICK_MS, true);
		RefreshMatchScore();
		Print(string.Format("[13RDPA H&M] Front démarré sur %1 : %2 cellules, %3 points, %4 dépôts, %5 lignes", key, s_aCells.Count(), s_aPoints.Count(), s_aDeposits.Count(), s_aLines.Count()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Compteurs du HUD : maîtres ennemis à gauche (US), maîtres joueurs à droite.
	static void RefreshMatchScore()
	{
		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign || !campaign.GetBaseManager())
			return;

		campaign.GetBaseManager().EvaluateControlPoints();
	}

	//------------------------------------------------------------------------------------------------
	protected static void BuildState()
	{
		s_aCells = {};
		s_aPoints = {};
		s_aDeposits = {};
		s_aLines = {};
		s_HqStock = new HAM_Stock();
		s_HqStock.m_fWater = 200;
		s_HqStock.m_fFood = 200;
		s_HqStock.m_fAmmo = 200;
		HAM_Settings settings = HAM_Settings.Get();
		s_fGlobalRep = settings.m_iRepStart;

		foreach (HAM_MapPoint masterDef : s_Map.m_aPoints)
		{
			if (!masterDef.IsCellMaster())
				continue;

			HAM_Cell cell = new HAM_Cell();
			cell.m_sName = masterDef.m_sName;
			cell.m_bEnemyHq = masterDef.m_eRole == HAM_ERole.ENEMY_HQ;
			cell.m_fReinforceLeft = settings.m_fReinforcementMinutes * 60;

			HAM_Point master = new HAM_Point();
			master.m_Def = masterDef;
			master.m_Cell = cell;
			cell.m_Master = master;

			foreach (string neighbor : masterDef.m_aNeighbors)
			{
				if (neighbor == HQ_NAME)
					cell.m_bTouchesHq = true;
				else if (cell.m_aNeighbors.Find(neighbor) < 0)
					cell.m_aNeighbors.Insert(neighbor);
			}

			s_aPoints.Insert(master);
			s_aCells.Insert(cell);
		}

		// Voisinage symétrique : une cellule citée cite en retour.
		foreach (HAM_Cell source : s_aCells)
		{
			foreach (string otherName : source.m_aNeighbors)
			{
				HAM_Cell other = FindCell(otherName);
				if (!other)
				{
					Print(string.Format("[13RDPA H&M] Cellule %1 : voisin %2 inconnu", source.m_sName, otherName), LogLevel.WARNING);
					continue;
				}

				if (other.m_aNeighbors.Find(source.m_sName) < 0)
					other.m_aNeighbors.Insert(source.m_sName);
			}
		}

		foreach (HAM_MapPoint def : s_Map.m_aPoints)
		{
			if (def.m_eRole == HAM_ERole.HQ || def.m_eRole == HAM_ERole.SUPPLY || def.IsCellMaster())
				continue;

			HAM_Cell owner = FindCell(def.m_sCell);
			if (!owner)
				continue;

			if (def.m_eRole == HAM_ERole.SLAVE)
			{
				HAM_Point slave = new HAM_Point();
				slave.m_Def = def;
				slave.m_Cell = owner;
				s_aPoints.Insert(slave);
				owner.m_aSlaves.Insert(slave);
				if (def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
				{
					HAM_Deposit deposit = new HAM_Deposit();
					deposit.m_Def = def;
					deposit.m_Cell = owner;
					deposit.m_Stock.m_fWater = settings.m_fDepositWaterMax;
					deposit.m_Stock.m_fFood = settings.m_fDepositFoodMax;
					deposit.m_Stock.m_fAmmo = settings.m_fDepositAmmoMax;
					s_aDeposits.Insert(deposit);
					owner.m_aDeposits.Insert(deposit);
				}

				continue;
			}

			if (def.m_eRole == HAM_ERole.DEPOSIT)
			{
				HAM_Deposit deposit = new HAM_Deposit();
				deposit.m_Def = def;
				deposit.m_Cell = owner;
				deposit.m_Stock.m_fWater = settings.m_fDepositWaterMax;
				deposit.m_Stock.m_fFood = settings.m_fDepositFoodMax;
				deposit.m_Stock.m_fAmmo = settings.m_fDepositAmmoMax;
				s_aDeposits.Insert(deposit);
				owner.m_aDeposits.Insert(deposit);
				continue;
			}

			HAM_Line line = new HAM_Line();
			line.m_Def = def;
			line.m_Cell = owner;
			s_aLines.Insert(line);
			owner.m_aLines.Insert(line);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Les bases Conflit posées dans le monde aux coordonnées du QG et du QG ennemi.
	protected static void FindConflictBases()
	{
		s_HqBase = FindBaseNear(s_HqPoint.m_vPos, BASE_MATCH_RADIUS);
		if (!s_HqBase)
			Print("[13RDPA H&M] Aucune base Conflit près de la ligne Hq : pose la base HAM_HQ dans le monde à cette coordonnée.", LogLevel.WARNING);

		HAM_MapPoint enemyDef = s_Map.FindRole(HAM_ERole.ENEMY_HQ);
		if (enemyDef)
			s_EnemyHqBase = FindBaseNear(enemyDef.m_vPos, BASE_MATCH_RADIUS);

		DisableBasePatrols(s_HqBase);
		DisableBasePatrols(s_EnemyHqBase);
		PowerHqRadio(s_HqBase);
	}

	//------------------------------------------------------------------------------------------------
	//! Éteint les patrouilles vanilla d'une base Conflit : l'IA des QG vient seulement des garnisons H&M.
	static void DisableBasePatrols(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner())
			return;

		IEntity child = base.GetOwner().GetChildren();
		while (child)
		{
			SCR_AmbientPatrolSpawnPointComponent patrol = SCR_AmbientPatrolSpawnPointComponent.Cast(child.FindComponent(SCR_AmbientPatrolSpawnPointComponent));
			if (patrol)
			{
				patrol.SetIsEliminated(true);
				patrol.SetIsPaused(true);
				SCR_AIGroup spawned = patrol.GetSpawnedGroup();
				if (spawned)
					HAM_Util.Delete(spawned);
			}

			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PowerHqRadio(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner())
			return;

		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		if (!radio)
			return;

		radio.SetPower(true);
	}

	//------------------------------------------------------------------------------------------------
	static SCR_CampaignMilitaryBaseComponent FindBaseNear(vector pos, float radius)
	{
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return null;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		SCR_CampaignMilitaryBaseComponent best;
		float bestSq = radius * radius;
		foreach (SCR_MilitaryBaseComponent base : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(base);
			if (!campaignBase || !campaignBase.GetOwner())
				continue;

			float distSq = vector.DistanceSqXZ(campaignBase.GetOwner().GetOrigin(), pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = campaignBase;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsNearEnemyHqBase(vector pos)
	{
		if (!s_EnemyHqBase || !s_EnemyHqBase.GetOwner())
			return false;

		return vector.DistanceSqXZ(s_EnemyHqBase.GetOwner().GetOrigin(), pos) < 160000;
	}

	//------------------------------------------------------------------------------------------------
	//! Premier boot : crée les prefabs. Boots suivants : reprend ceux qui existent encore.
	protected static void PlaceZonePrefabs()
	{
		HAM_Save.BeginSystemPlace();
		HAM_ZoneComponent.DeleteRole(HAM_ERole.MASTER);
		HAM_ZoneComponent.DeleteRole(HAM_ERole.HQ);
		foreach (HAM_Point point : s_aPoints)
		{
			if (point.IsMaster())
			{
				point.m_Prop = null;
				continue;
			}

			if (point.m_Def.m_eType == HAM_EMasterType.SUPPLY_VANILLA)
			{
				vector spot = point.Pos();
				ClearOldSupplyProps(spot);
				IEntity cache = ReuseNear(spot, "253C2F12F79B289D");
				if (!cache)
					cache = HAM_Util.SpawnLevelled(HAM_Units.SUPPLY_CACHE_US, spot, 0);
				else
					HAM_Util.Forget(cache);

				point.m_Prop = cache;
				string supplyFaction = HAM_Units.ENEMY_FACTION;
				if (point.m_bPlayer)
					supplyFaction = HAM_Units.PLAYER_FACTION;

				HAM_MasterBases.OpenBuilt(cache, supplyFaction);
				HAM_MasterLinkComponent.SetEntityFaction(cache, supplyFaction);
				RearmVanillaSupply(cache);
				continue;
			}

			if (point.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
			{
				IEntity actions = ReuseNear(point.Pos(), "7178DA7410FD44E1");
				if (actions)
				{
					HAM_Util.Forget(actions);
					HAM_ZoneComponent depositZone = HAM_ZoneComponent.Cast(actions.FindComponent(HAM_ZoneComponent));
					if (depositZone)
						depositZone.SetZone(point.m_Def.m_sName, HAM_ERole.DEPOSIT);
				}
				else
					actions = PlaceProp(HAM_Units.ZONE_DEPOSIT, point.m_Def.m_sName, HAM_ERole.DEPOSIT, point.Pos(), 25);
				point.m_Prop = actions;
				if (s_aDeposits)
				{
					foreach (HAM_Deposit deposit : s_aDeposits)
					{
						if (deposit && deposit.m_Def.m_sName == point.m_Def.m_sName)
							deposit.m_Prop = actions;
					}
				}

				continue;
			}

			point.m_Prop = PlaceProp(HAM_Units.ZONE_SLAVE, point.m_Def.m_sName, point.m_Def.m_eRole, point.Pos(), 25);
		}

		foreach (HAM_Deposit deposit : s_aDeposits)
		{
			if (deposit.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
				continue;

			deposit.m_Prop = PlaceProp(HAM_Units.ZONE_DEPOSIT, deposit.m_Def.m_sName, HAM_ERole.DEPOSIT, deposit.Pos(), 25);
		}

		vector hqPos = s_HqPoint.m_vPos;
		if (s_HqBase)
			hqPos = s_HqBase.GetOwner().GetOrigin();

		PlaceArsenalBoard(hqPos);
		HAM_MasterBases.PlaceAll();
		GetGame().GetCallqueue().CallLater(HAM_Save.SpawnBuilt, 2500, false);
		GetGame().GetCallqueue().CallLater(HAM_Save.ApplyBaseSupplies, 4000, false, 0);
		GetGame().GetCallqueue().CallLater(HAM_Save.ApplyCampaignSupplies, 4000, false, 0);
		HAM_Save.EndSystemPlace();
		SCR_ResourceSystem.HAM_RefreshDepotRegen();
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity s_ReuseFound;
	protected static string s_sReuseToken;
	protected static ref array<IEntity> s_aOldSupply;

	//------------------------------------------------------------------------------------------------
	protected static bool SeeReuse(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		if (!s_ReuseFound && ent.GetPrefabData().GetPrefabName().Contains(s_sReuseToken))
			s_ReuseFound = ent;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity ReuseNear(vector spot, string token)
	{
		s_ReuseFound = null;
		s_sReuseToken = token;
		GetGame().GetWorld().QueryEntitiesBySphere(spot, 20, SeeReuse);
		return s_ReuseFound;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeOldSupply(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		string prefabName = ent.GetPrefabData().GetPrefabName();
		if (prefabName.Contains("CampaignRemnantsSupplyDepot") || prefabName.Contains("SupplyStack_01") || prefabName.Contains("HAM_SupplyDepot") || prefabName.Contains("SupplyStorage_S_US") || prefabName.Contains("SupplyStorage_Base.et"))
			s_aOldSupply.Insert(ent);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearOldSupplyProps(vector spot)
	{
		s_aOldSupply = {};
		GetGame().GetWorld().QueryEntitiesBySphere(spot, 20, SeeOldSupply);
		foreach (IEntity old : s_aOldSupply)
			HAM_Util.Delete(old);
	}

	//------------------------------------------------------------------------------------------------
	static void RearmVanillaSupply(IEntity cache)
	{
		if (!cache)
			return;

		if (!s_aSupplyArmPending)
			s_aSupplyArmPending = {};

		if (s_aSupplyArmPending.Contains(cache))
			return;

		s_aSupplyArmPending.Insert(cache);
		GetGame().GetCallqueue().CallLater(ArmVanillaCacheLater, 2000, false, cache, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Un conteneur de fournitures vient de s'enregistrer. Les caisses du cache naissent après le prefab :
	//! tant que la valeur sauvée n'est pas posée, on réarme.
	static void NoteSupplyContainer(SCR_ResourceContainer container)
	{
		if (s_bArmingSupply || !container || !s_aPoints)
			return;

		if (container.GetResourceType() != EResourceType.SUPPLIES)
			return;

		IEntity owner = container.GetOwner();
		if (!owner)
			return;

		vector pos = owner.GetOrigin();
		foreach (HAM_Point point : s_aPoints)
		{
			if (!point || !point.m_Def || !point.m_Prop || point.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA)
				continue;

			if (HAM_Save.SupplyArmed(point.m_Def.m_sName))
				continue;

			if (vector.DistanceXZ(point.m_Prop.GetOrigin(), pos) > 40)
				continue;

			RearmVanillaSupply(point.m_Prop);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropSupplyArm(IEntity cache)
	{
		if (s_aSupplyArmPending && cache)
			s_aSupplyArmPending.RemoveItem(cache);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ResolveSupply(IEntity cache, out SCR_ResourceContainer stock, out SCR_ResourceEncapsulator enc)
	{
		stock = null;
		enc = null;
		if (!cache)
			return;

		IEntity storage = FindSupplyStorage(cache);
		IEntity host = storage;
		if (!host)
			host = cache;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(host.FindComponent(SCR_ResourceComponent));
		if (!resources)
			resources = SCR_ResourceComponent.Cast(cache.FindComponent(SCR_ResourceComponent));

		if (!resources)
			return;

		stock = resources.GetContainer(EResourceType.SUPPLIES);
		enc = resources.GetEncapsulator(EResourceType.SUPPLIES);
		if (enc && enc.GetContainerRepresentative())
			stock = enc.GetContainerRepresentative();
	}

	//------------------------------------------------------------------------------------------------
	protected static float SupplyLive(SCR_ResourceContainer stock, SCR_ResourceEncapsulator enc)
	{
		if (enc && enc.GetContainerCount() > 0)
			return enc.GetAggregatedResourceValue();

		if (stock)
			return stock.GetResourceValue();

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static float SupplyLiveMax(SCR_ResourceContainer stock, SCR_ResourceEncapsulator enc)
	{
		float maxValue = 0;
		if (enc && enc.GetContainerCount() > 0)
			maxValue = enc.GetAggregatedMaxResourceValue();

		if (maxValue <= 0.01 && stock)
			maxValue = stock.GetMaxResourceValue();

		return maxValue;
	}

	//------------------------------------------------------------------------------------------------
	//! La valeur affichée est la somme des caisses. Écrire seulement le conteneur représentatif
	//! se fait écraser par le max du prefab dès que les caisses s'arment.
	protected static void WriteSupplyBoxes(IEntity node, SCR_ResourceContainer stock, float wanted)
	{
		array<SCR_ResourceContainer> boxes = {};
		CollectSupplyBoxes(node, stock, boxes);
		if (boxes.IsEmpty())
		{
			if (stock)
				stock.SetResourceValue(wanted, true);

			return;
		}

		float capSum = 0;
		foreach (SCR_ResourceContainer box : boxes)
		{
			if (box)
				capSum = capSum + box.GetMaxResourceValue();
		}

		float left = wanted;
		if (left < 0)
			left = 0;

		int last = boxes.Count() - 1;
		for (int i = 0; i < boxes.Count(); i++)
		{
			SCR_ResourceContainer box = boxes[i];
			if (!box)
				continue;

			float cap = box.GetMaxResourceValue();
			float share = left;
			if (i < last && capSum > 0.01)
				share = wanted * (cap / capSum);

			if (share > cap)
				share = cap;

			if (share > left)
				share = left;

			if (share < 0)
				share = 0;

			box.SetResourceValue(share, true);
			left = left - share;
		}

		if (stock)
			stock.SetResourceValue(wanted, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectSupplyBoxes(IEntity node, SCR_ResourceContainer skip, notnull array<SCR_ResourceContainer> boxes)
	{
		if (!node)
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container && container != skip && boxes.Find(container) < 0)
				boxes.Insert(container);
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			CollectSupplyBoxes(child, skip, boxes);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Le prefab réécrit souvent son maximum juste après l'armement. On repose la sauvegarde
	//! si le stock est revenu au plafond alors que la valeur gardée est plus basse.
	protected static void ReassertVanillaSupply(IEntity cache)
	{
		if (!cache)
			return;

		string supplyName = SupplyPointName(cache);
		float saved;
		if (!HAM_Save.SupplyAmount(supplyName, saved))
			return;

		SCR_ResourceContainer stock;
		SCR_ResourceEncapsulator enc;
		ResolveSupply(cache, stock, enc);
		float live = SupplyLive(stock, enc);
		float maxValue = SupplyLiveMax(stock, enc);
		float gap = live - saved;
		if (gap < 0)
			gap = -gap;

		if (gap <= 1)
			return;

		if (maxValue <= 0.01 || live + 1 < maxValue || saved + 50 >= maxValue)
			return;

		IEntity storage = FindSupplyStorage(cache);
		if (!storage)
			storage = cache;

		WriteSupplyBoxes(storage, stock, saved);
		HAM_Save.RememberSupply(supplyName, saved);
		MarkClientsDirty();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SupplyBoxesReady(SCR_ResourceContainer stock, SCR_ResourceEncapsulator enc)
	{
		if (!stock)
			return false;

		if (enc && enc.GetContainerCount() < 1)
			return false;

		return SupplyLiveMax(stock, enc) > 0.01;
	}

	//------------------------------------------------------------------------------------------------
	//! Même pose que le layer Conflit : le cache contient déjà son SupplyStorage.
	//! Les caisses enfants n'existent pas à la frame du spawn. L'encapsuleur recopie leur somme
	//! sur le conteneur affiché : poser la valeur avant les caisses la remet à 0.
	protected static void ArmVanillaCacheLater(IEntity cache, int tries)
	{
		if (!cache)
		{
			DropSupplyArm(cache);
			return;
		}

		s_bArmingSupply = true;
		IEntity storage = FindSupplyStorage(cache);
		SCR_ResourceContainer stock;
		SCR_ResourceEncapsulator enc;
		ResolveSupply(cache, stock, enc);

		string supplyName = SupplyPointName(cache);
		float savedSupply;
		bool haveSaved = HAM_Save.SupplyAmount(supplyName, savedSupply);
		bool ready = SupplyBoxesReady(stock, enc);
		if (!ready && tries >= 24 && stock && stock.GetMaxResourceValue() > 0.01)
			ready = true;

		if (!ready)
		{
			s_bArmingSupply = false;
			if (tries < 24)
			{
				GetGame().GetCallqueue().CallLater(ArmVanillaCacheLater, 1000, false, cache, tries + 1);
				return;
			}

			if (!haveSaved && stock)
			{
				stock.SetMaxResourceValue(4500, true);
				stock.SetResourceValue(4500, true);
				MarkClientsDirty();
			}

			HAM_SupplyWatch.Arm(cache);
			DropSupplyArm(cache);
			return;
		}

		float maxValue = SupplyLiveMax(stock, enc);
		if (stock && maxValue > 0.01 && stock.GetMaxResourceValue() + 0.01 < maxValue)
			stock.SetMaxResourceValue(maxValue, true);

		float wanted = maxValue;
		if (haveSaved)
		{
			wanted = savedSupply;
			if (wanted < 0)
				wanted = 0;

			if (maxValue > 0.01 && wanted > maxValue)
				wanted = maxValue;
		}
		else if (storage && stock)
		{
			FillOwnedContainers(storage, stock);
		}

		float live = SupplyLive(stock, enc);
		float gap = live - wanted;
		if (gap < 0)
			gap = -gap;

		if (gap > 1)
		{
			IEntity boxes = storage;
			if (!boxes)
				boxes = cache;

			WriteSupplyBoxes(boxes, stock, wanted);
		}

		live = SupplyLive(stock, enc);
		gap = live - wanted;
		if (gap < 0)
			gap = -gap;

		if (stock && gap <= 1)
		{
			HAM_SupplyWatch.Arm(cache);
			MarkClientsDirty();
		}

		s_bArmingSupply = false;
		if (gap <= 1 && tries >= 6)
		{
			HAM_Save.RememberSupply(supplyName, live);
			HAM_Save.MarkSupplyArmed(supplyName);
			s_fSupplyArmedAt = HAM_Util.Now();
			DropSupplyArm(cache);
			string regen = "arrêtée";
			if (!HAM_MilsimSettingsComponent.HAM_NoSupplyRegenDepots() && HAM_MilsimSettingsComponent.HAM_DepotRegenAmount() > 0)
				regen = string.Format("%1 / %2 s", Math.Round(HAM_MilsimSettingsComponent.HAM_DepotRegenAmount()), Math.Round(HAM_MilsimSettingsComponent.HAM_DepotRegenTickrate()));

			HAM_Util.Info(string.Format("Dépôt vanilla %1 : %2 / %3, regen %4", supplyName, Math.Round(live), Math.Round(maxValue), regen));
			return;
		}

		if (tries < 24)
		{
			GetGame().GetCallqueue().CallLater(ArmVanillaCacheLater, 1000, false, cache, tries + 1);
			return;
		}

		if (gap <= 1)
		{
			HAM_Save.RememberSupply(supplyName, live);
			HAM_Save.MarkSupplyArmed(supplyName);
		}

		HAM_SupplyWatch.Arm(cache);
		DropSupplyArm(cache);
	}

	//------------------------------------------------------------------------------------------------
	//! Stock affiché sur la carte : la somme des conteneurs du cache, sinon le conteneur représentatif.
	protected static string SupplyPointName(IEntity cache)
	{
		if (!cache || !s_aPoints)
			return string.Empty;

		string nearestName = string.Empty;
		float best = 1600;
		vector pos = cache.GetOrigin();
		foreach (HAM_Point point : s_aPoints)
		{
			if (!point || !point.m_Def || point.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA)
				continue;

			if (point.m_Prop == cache)
				return point.m_Def.m_sName;

			float dist = vector.DistanceSqXZ(point.Pos(), pos);
			if (dist >= best)
				continue;

			best = dist;
			nearestName = point.m_Def.m_sName;
		}

		return nearestName;
	}

	//------------------------------------------------------------------------------------------------
	static void VanillaSupplyStock(IEntity cache, out int value, out int maxValue)
	{
		value = 0;
		maxValue = 0;
		IEntity storage = FindSupplyStorage(cache);
		if (!storage)
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(storage.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return;

		SCR_ResourceContainer stock = resources.GetContainer(EResourceType.SUPPLIES);
		SCR_ResourceEncapsulator enc = resources.GetEncapsulator(EResourceType.SUPPLIES);
		if (enc)
		{
			value = Math.Round(enc.GetAggregatedResourceValue());
			maxValue = Math.Round(enc.GetAggregatedMaxResourceValue());
		}

		if (maxValue <= 0 && stock)
		{
			value = Math.Round(stock.GetResourceValue());
			maxValue = Math.Round(stock.GetMaxResourceValue());
		}
	}

	//------------------------------------------------------------------------------------------------
	static IEntity FindSupplyStorage(IEntity node)
	{
		if (!node)
			return null;

		if (node.GetPrefabData() && node.GetPrefabData().GetPrefabName().Contains("Storage_Supplies"))
			return node;

		IEntity child = node.GetChildren();
		while (child)
		{
			IEntity found = FindSupplyStorage(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Un conteneur vide reprend son maximum d'origine. On ne choisit pas un autre chiffre.
	protected static void FillOwnedContainers(IEntity node, SCR_ResourceContainer stock)
	{
		if (!node)
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container && container != stock)
			{
				float childMax = container.GetMaxResourceValue();
				if (childMax <= 0.01)
				{
					childMax = 1000;
					container.SetMaxResourceValue(childMax, true);
				}

				if (container.GetResourceValue() <= 0.01)
					container.SetResourceValue(childMax, true);
			}
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			FillOwnedContainers(child, stock);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceArsenalBoard(vector hqPos)
	{
		SCR_EditableEntityCore core = SCR_EditableEntityCore.Cast(SCR_EditableEntityCore.GetInstance(SCR_EditableEntityCore));
		IEntity keep;
		float keepSq;
		array<IEntity> extras = {};
		if (core)
		{
			set<SCR_EditableEntityComponent> entities = new set<SCR_EditableEntityComponent>();
			core.GetAllEntities(entities);
			foreach (SCR_EditableEntityComponent editable : entities)
			{
				if (!editable)
					continue;

				IEntity ent = editable.GetOwner();
				if (!ent || !ent.GetPrefabData())
					continue;

				if (!ent.GetPrefabData().GetPrefabName().Contains("HAM_ArsenalBoard"))
					continue;

				float distSq = vector.DistanceSqXZ(ent.GetOrigin(), hqPos);
				if (!keep)
				{
					keep = ent;
					keepSq = distSq;
				}
				else if (distSq < keepSq)
				{
					extras.Insert(keep);
					keep = ent;
					keepSq = distSq;
				}
				else
					extras.Insert(ent);
			}
		}

		foreach (IEntity extra : extras)
		{
			if (!extra)
				continue;

			HAM_Util.Forget(extra);
			HAM_Util.Delete(extra);
		}

		if (keep)
		{
			HAM_Util.Forget(keep);
			return;
		}

		HAM_Util.Spawn(HAM_Units.ARSENAL_BOARD, HAM_Util.FreeSpot(hqPos + Vector(18, 0, 6), 8), 0);
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity PlaceProp(ResourceName prefab, string zoneName, HAM_ERole role, vector pos, float spread)
	{
		HAM_ZoneComponent existing = HAM_ZoneComponent.Find(zoneName, role);
		if (existing && existing.GetOwner())
		{
			HAM_Util.Forget(existing.GetOwner());
			return existing.GetOwner();
		}

		IEntity prop = HAM_Util.Spawn(prefab, HAM_Util.FreeSpot(pos, spread), Math.RandomFloat(0, 360));
		if (!prop)
			return null;

		HAM_ZoneComponent zone = HAM_ZoneComponent.Cast(prop.FindComponent(HAM_ZoneComponent));
		if (zone)
			zone.SetZone(zoneName, role);

		return prop;
	}

	//------------------------------------------------------------------------------------------------
	//! Deuxième passage : le premier envoi part parfois avant que le prop soit répliqué.
	protected static void RefreshLoadedStocks()
	{
		HAM_Save.ApplyLoadedDeposits();
		if (!s_aCells)
			return;

		foreach (HAM_Cell cell : s_aCells)
			RefreshZoneProps(cell, true);

		if (!s_HqStock)
			return;

		HAM_ZoneComponent hq = HAM_ZoneComponent.Find(HQ_NAME, HAM_ERole.HQ);
		if (hq)
			hq.PushStocks(s_HqStock.m_fWater, s_HqStock.m_fFood, s_HqStock.m_fAmmo, s_HqStock.m_iMen);
	}

	//------------------------------------------------------------------------------------------------
	//! Recopie camp et stocks sur les props de la cellule, pour les actions des clients.
	static void RefreshZoneProps(HAM_Cell cell)
	{
		RefreshZoneProps(cell, false);
	}

	//------------------------------------------------------------------------------------------------
	static void RefreshZoneProps(HAM_Cell cell, bool force)
	{
		if (!cell)
			return;

		SetPropState(cell.m_Master.m_Prop, cell.m_Master.m_bPlayer, cell.m_Stock, force);
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			SetPropState(slave.m_Prop, slave.m_bPlayer, null, false);
		}

		foreach (HAM_Deposit deposit : cell.m_aDeposits)
		{
			SetPropState(deposit.m_Prop, cell.IsComplete(), deposit.m_Stock, force);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SetPropState(IEntity prop, bool player, HAM_Stock stock, bool force)
	{
		if (!prop)
			return;

		HAM_ZoneComponent zone = HAM_ZoneComponent.Cast(prop.FindComponent(HAM_ZoneComponent));
		if (!zone)
			return;

		zone.SetPlayer(player);
		if (!stock)
			return;

		if (force)
			zone.PushStocks(stock.m_fWater, stock.m_fFood, stock.m_fAmmo, stock.m_iMen);
		else
			zone.SetStocks(stock.m_fWater, stock.m_fFood, stock.m_fAmmo, stock.m_iMen);
	}

	//------------------------------------------------------------------------------------------------
	//! Le premier envoi des marqueurs fige le stock. On renvoie dès que le dépôt ou les ressources bougent.
	protected static void RefreshZoneMarkers()
	{
		float stamp = 0;
		if (s_aPoints)
		{
			foreach (HAM_Point point : s_aPoints)
			{
				if (!point || !point.m_Def || point.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA || !point.m_Prop)
					continue;

				int value;
				int maxValue;
				VanillaSupplyStock(point.m_Prop, value, maxValue);
				stamp = stamp + value + maxValue * 0.001;
			}
		}

		if (s_aDeposits)
		{
			foreach (HAM_Deposit deposit : s_aDeposits)
			{
				if (!deposit || !deposit.m_Stock)
					continue;

				stamp = stamp + deposit.m_Stock.m_fWater + deposit.m_Stock.m_fFood * 3 + deposit.m_Stock.m_fAmmo * 7;
			}
		}

		if (s_aCells)
		{
			foreach (HAM_Cell cell : s_aCells)
			{
				if (!cell || !cell.m_Stock)
					continue;

				stamp = stamp + cell.m_Stock.m_fWater * 11 + cell.m_Stock.m_fFood * 13 + cell.m_Stock.m_fAmmo * 17 + cell.m_Stock.m_iMen;
			}
		}

		float delta = stamp - s_fMarkerStamp;
		if (delta < 0)
			delta = -delta;

		if (delta <= 0.5)
			return;

		s_fMarkerStamp = stamp;
		MarkClientsDirty();
	}

	//------------------------------------------------------------------------------------------------
	//! Mémorise le stock vanilla déjà armé. Un retour au plafond dans les premières secondes
	//! est le prefab qui se réécrit, pas une livraison du joueur.
	static void TrackVanillaSupplies()
	{
		if (!s_aPoints)
			return;

		foreach (HAM_Point point : s_aPoints)
		{
			if (!point || !point.m_Def || point.m_Def.m_eType != HAM_EMasterType.SUPPLY_VANILLA || !point.m_Prop)
				continue;

			if (!HAM_Save.SupplyArmed(point.m_Def.m_sName))
				continue;

			int value;
			int maxValue;
			VanillaSupplyStock(point.m_Prop, value, maxValue);
			float saved;
			bool haveSaved = HAM_Save.SupplyAmount(point.m_Def.m_sName, saved);
			float live = value;
			if (haveSaved && HAM_Util.Now() - s_fSupplyArmedAt < 20 && maxValue > 0 && live + 1 >= maxValue && saved + 50 < maxValue)
			{
				IEntity storage = FindSupplyStorage(point.m_Prop);
				if (!storage)
					storage = point.m_Prop;

				SCR_ResourceContainer stock;
				SCR_ResourceEncapsulator enc;
				ResolveSupply(point.m_Prop, stock, enc);
				WriteSupplyBoxes(storage, stock, saved);
				continue;
			}

			float gap = live - saved;
			if (gap < 0)
				gap = -gap;

			if (haveSaved && gap <= 1)
				continue;

			HAM_Save.RememberSupply(point.m_Def.m_sName, live);
			MarkSaveDirty();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Tick()
	{
		if (!IsRunning())
			return;

		HAM_Util.AlivePlayers(s_aPlayerIds, s_aPlayerBodies);
		HAM_Hospital.Tick(s_aPlayerIds, s_aPlayerBodies);
		HAM_Front.Tick(TICK_MS * 0.001);
		HAM_Garrison.Tick();
		HAM_Devices.Tick();
		HAM_SideMissions.Tick();
		HAM_Convoys.Tick();
		HAM_Intel.Tick();
		HAM_Interzone.Tick();
		RefreshZoneMarkers();

		if (s_bClientsDirty)
		{
			s_bClientsDirty = false;
			HAM_Net.BroadcastState();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SlowTick()
	{
		if (!IsRunning())
			return;

		float dt = SLOW_TICK_MS * 0.001;
		TrackVanillaSupplies();
		HAM_SupplyWatch.Tick();
		HAM_Logistics.SlowTick(dt);
		HAM_Garrison.SlowTick(dt);
		HAM_CounterAttack.SlowTick(dt);
		HAM_Intel.SlowTick(dt);

		float interval = HAM_Settings.Get().m_fSaveIntervalMinutes * 60;
		if (interval < 60)
			interval = 60;

		if (HAM_Util.Now() - s_fLastSave >= interval)
			SaveNow();
	}

	//------------------------------------------------------------------------------------------------
	static void SaveNow()
	{
		if (!s_aCells)
			return;

		HAM_Save.Request(false);
	}

	//------------------------------------------------------------------------------------------------
	static void SaveImmediate()
	{
		if (!s_aCells)
			return;

		HAM_Save.Request(true);
	}

	//------------------------------------------------------------------------------------------------
	static void NoteSaved()
	{
		s_bSaveDirty = false;
		s_fLastSave = HAM_Util.Now();
	}

	//------------------------------------------------------------------------------------------------
	static void MarkDirty()
	{
		s_bSaveDirty = true;
		s_bClientsDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	static void MarkClientsDirty()
	{
		s_bClientsDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	//! Stocks seulement : les props répliquent déjà les valeurs, les marqueurs ne changent pas.
	static void MarkSaveDirty()
	{
		s_bSaveDirty = true;
	}

	//------------------------------------------------------------------------------------------------
	static void OnPlayerJoined(int playerId)
	{
		if (!IsActive())
			return;

		HAM_Net.SyncPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! Nouveau corps (apparition ou reconnexion) : la couverture radio du client est recalculée.
	static void OnPlayerBody(int playerId, IEntity body)
	{
		if (!Replication.IsServer() || !IsRunning())
			return;

		if (s_HqBase)
			s_HqBase.RecalculateRadioRange();

		GetGame().GetCallqueue().CallLater(SCR_RadioCoverageSystem.UpdateAll, 500, false, false, false);
		HAM_Net.SyncPlayerNow(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static void OnKill(notnull SCR_InstigatorContextData data)
	{
		if (!IsRunning())
			return;

		IEntity victim = data.GetVictimEntity();
		if (!victim)
			return;

		HAM_Garrison.OnUnitKilled(victim);
		HAM_Intel.OnUnitKilled(victim, data);
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Cell FindCell(string name)
	{
		if (!s_aCells)
			return null;

		foreach (HAM_Cell cell : s_aCells)
		{
			if (cell.m_sName == name)
				return cell;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Point FindPoint(string name)
	{
		if (!s_aPoints)
			return null;

		foreach (HAM_Point point : s_aPoints)
		{
			if (point.m_Def.m_sName == name)
				return point;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Point NearestPoint(vector pos)
	{
		HAM_Point best;
		float bestSq = -1;
		if (!s_aPoints)
			return null;

		foreach (HAM_Point point : s_aPoints)
		{
			if (!point)
				continue;

			float distSq = vector.DistanceSqXZ(pos, point.Pos());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = point;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Point NearestSlave(HAM_Cell cell, vector pos)
	{
		if (!cell)
			return null;

		HAM_Point best;
		float bestSq = -1;
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			if (!slave)
				continue;

			float distSq = vector.DistanceSqXZ(pos, slave.Pos());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = slave;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	//! L'esclave le plus proche est aux joueurs, et ce point est dans son rayon.
	static bool SlaveHolds(HAM_Cell cell, vector pos)
	{
		HAM_Point slave = NearestSlave(cell, pos);
		if (!slave || !slave.m_bPlayer)
			return false;

		return vector.DistanceXZ(slave.Pos(), pos) <= slave.Radius();
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Deposit FindDeposit(string name)
	{
		if (!s_aDeposits)
			return null;

		foreach (HAM_Deposit deposit : s_aDeposits)
		{
			if (deposit.m_Def.m_sName == name)
				return deposit;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Cellule dont le maître est le plus proche (territoire de Voronoï), ou null si on est au QG ou trop loin.
	static HAM_Cell CellAt(vector pos)
	{
		if (!s_aCells || !s_HqPoint)
			return null;

		HAM_Cell best;
		float bestSq = CELL_MAX_RADIUS * CELL_MAX_RADIUS;
		foreach (HAM_Cell cell : s_aCells)
		{
			float distSq = vector.DistanceSqXZ(cell.Pos(), pos);
			if (distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = cell;
		}

		if (!best)
			return null;

		float hqSq = vector.DistanceSqXZ(s_HqPoint.m_vPos, pos);
		if (hqSq < bestSq)
			return null;

		return best;
	}

	//------------------------------------------------------------------------------------------------
	//! Cellule ennemie collée au QG ou à une cellule entière aux joueurs.
	static bool IsFront(HAM_Cell cell)
	{
		if (!cell || cell.IsComplete())
			return false;

		if (cell.m_bTouchesHq)
			return true;

		foreach (string neighborName : cell.m_aNeighbors)
		{
			HAM_Cell neighbor = FindCell(neighborName);
			if (neighbor && neighbor.IsComplete())
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Cellule dont le maître est tenu par les joueurs, même si des esclaves restent à prendre.
	static bool IsAlliedCell(HAM_Cell cell)
	{
		return cell && cell.IsMasterPlayer();
	}

	//------------------------------------------------------------------------------------------------
	//! Bases alliées : le QG de départ et chaque maître tenu.
	static bool IsAlliedBase(string baseName)
	{
		if (baseName == HQ_NAME)
			return true;

		return IsAlliedCell(FindCell(baseName));
	}

	//------------------------------------------------------------------------------------------------
	static void AlliedBases(notnull array<string> names)
	{
		names.Clear();
		names.Insert(HQ_NAME);
		if (!s_aCells)
			return;

		foreach (HAM_Cell cell : s_aCells)
		{
			if (IsAlliedCell(cell))
				names.Insert(cell.m_sName);
		}
	}

	//------------------------------------------------------------------------------------------------
	static vector BasePos(string baseName)
	{
		if (baseName == HQ_NAME)
		{
			if (s_HqBase)
				return s_HqBase.GetOwner().GetOrigin();

			return s_HqPoint.m_vPos;
		}

		HAM_Cell cell = FindCell(baseName);
		if (!cell)
			return vector.Zero;

		return cell.Pos();
	}

	//------------------------------------------------------------------------------------------------
	static HAM_Stock StockOf(string baseName)
	{
		if (baseName == HQ_NAME)
			return s_HqStock;

		HAM_Cell cell = FindCell(baseName);
		if (!cell)
			return null;

		return cell.m_Stock;
	}

	//------------------------------------------------------------------------------------------------
	static void RefreshBaseProp(string baseName)
	{
		if (baseName == HQ_NAME)
			return;

		RefreshZoneProps(FindCell(baseName));
	}

	//------------------------------------------------------------------------------------------------
	//! Base alliée la plus proche d'une position (tableau ouvert, livraison).
	static string NearestAlliedBase(vector pos, float radius)
	{
		string best;
		float bestSq = radius * radius;
		array<string> names = {};
		AlliedBases(names);
		foreach (string baseName : names)
		{
			float distSq = vector.DistanceSqXZ(BasePos(baseName), pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = baseName;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static string DesignatedHq()
	{
		return s_sDesignatedHq;
	}

	//------------------------------------------------------------------------------------------------
	static vector DesignatedHqPos()
	{
		return BasePos(s_sDesignatedHq);
	}

	//------------------------------------------------------------------------------------------------
	//! Base Conflit qui reçoit les fournitures décidées par nos scripts.
	static SCR_CampaignMilitaryBaseComponent DesignatedHqBase()
	{
		SCR_CampaignMilitaryBaseComponent designated = HAM_HqDesignation.CurrentBase();
		if (designated)
			return designated;

		return s_HqBase;
	}

	//------------------------------------------------------------------------------------------------
	//! +1000 dans un dépôt vanilla du maître capturé, sinon au QG de départ. N'efface pas un stock.
	static void PourMasterSupplies(HAM_Cell cell)
	{
		if (!cell)
			return;

		bool poured = false;
		SCR_CampaignMilitaryBaseComponent base = FindBaseNear(cell.Pos(), 120);
		if (base && base.GetOwner())
		{
			HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
			if (link)
				poured = link.PourOwnedSupplies(1000);
		}

		if (poured)
		{
			Print(string.Format("[13RDPA H&M] +1000 fournitures au dépôt de %1", cell.m_sName), LogLevel.NORMAL);
			return;
		}

		GrantStartingHqSupplies(1000, "prise de " + cell.m_sName);
	}

	//------------------------------------------------------------------------------------------------
	//! Ajoute des fournitures à un cache vanilla déjà posé. Faux s'il n'y a pas de conteneur.
	static bool AddVanillaSupplies(IEntity node, int amount)
	{
		if (!node || amount <= 0)
			return false;

		IEntity storage = FindSupplyStorage(node);
		if (!storage)
			return false;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(storage.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return false;

		SCR_ResourceContainer stock = resources.GetContainer(EResourceType.SUPPLIES);
		if (!stock)
			return false;

		float next = stock.GetResourceValue() + amount;
		if (stock.GetMaxResourceValue() < next)
			stock.SetMaxResourceValue(next, true);

		stock.IncreaseResourceValue(amount, true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Fournitures vanilla du QG de départ, sans remettre son stock à zéro.
	static void GrantStartingHqSupplies(int amount, string reason)
	{
		if (amount <= 0)
			return;

		if (!s_HqBase)
		{
			Print("[13RDPA H&M] Versement perdu, QG de départ absent : " + reason, LogLevel.WARNING);
			return;
		}

		s_HqBase.AddSupplies(amount);
		Print(string.Format("[13RDPA H&M] +%1 fournitures au QG de départ (%2)", amount, reason), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Le seul chemin des versements de fournitures : la base désignée comme QG.
	static void PaySupplies(int amount, string reason)
	{
		if (amount <= 0)
			return;

		SCR_CampaignMilitaryBaseComponent base = DesignatedHqBase();
		if (!base)
		{
			Print("[13RDPA H&M] Versement perdu, aucune base QG : " + reason, LogLevel.WARNING);
			return;
		}

		base.AddSupplies(amount);
		Print(string.Format("[13RDPA H&M] +%1 fournitures au QG %2 (%3)", amount, s_sDesignatedHq, reason), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	static string FormatRep(float value)
	{
		return value.ToString(0, 2);
	}

	//------------------------------------------------------------------------------------------------
	static void AddGlobalRep(float delta, string reason)
	{
		if (Math.AbsFloat(delta) < 0.001)
			return;

		HAM_Settings settings = HAM_Settings.Get();
		s_fGlobalRep = Math.Clamp(s_fGlobalRep + delta, settings.m_iRepMin, settings.m_iRepMax);
		Print(string.Format("[13RDPA H&M] Réputation globale %1 (%2 : %3)", FormatRep(s_fGlobalRep), reason, FormatRep(delta)), LogLevel.NORMAL);
		MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	static void AddLocalRep(HAM_Cell cell, int delta)
	{
		if (!cell || delta == 0)
			return;

		HAM_Settings settings = HAM_Settings.Get();
		cell.m_iLocalRep = Math.Clamp(cell.m_iLocalRep + delta, settings.m_iRepMin, settings.m_iRepMax);
		MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	static void Notify(string title, string message, int seconds = 8)
	{
		HAM_Net.NotifyAll(title, message, seconds);
	}

	//------------------------------------------------------------------------------------------------
	//! Fin de partie : victoire du camp joueur.
	static void Victory()
	{
		if (s_bEnded)
			return;

		s_bEnded = true;
		SaveImmediate();
		Notify("Victoire", "La base principale ennemie est tombée. La campagne est gagnée.", 20);

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		FactionManager factions = GetGame().GetFactionManager();
		Faction playerFaction = HAM_Util.FactionByKey(HAM_Units.PLAYER_FACTION);
		if (!campaign || !factions || !playerFaction)
			return;

		campaign.HAM_EndWithVictory(factions.GetFactionIndex(playerFaction));
	}
}

//------------------------------------------------------------------------------------------------
//! Les caisses du cache vanilla ont le gain coupé par l'action d'encapsuleur
//! (montant 0, case décochée). Le conteneur virtuel est recopié sur la somme des caisses :
//! un gain posé dessus ne se voit pas. On ajoute donc le montant du JSON sur une caisse qui a de la place.
class HAM_SupplyWatch
{
	protected static const float GAIN = 10;
	protected static ref array<IEntity> s_aCaches;
	protected static bool s_bPulse;

	static void Arm(IEntity cache)
	{
		if (!cache)
			return;

		StopPrefabGain(cache);
		if (!s_aCaches)
			s_aCaches = {};

		if (s_aCaches.Find(cache) < 0)
			s_aCaches.Insert(cache);

		if (s_bPulse)
			return;

		s_bPulse = true;
		GetGame().GetCallqueue().CallLater(Pulse, 20000, true);
	}

	static void Tick()
	{
	}

	//------------------------------------------------------------------------------------------------
	//! 10 fournitures toutes les 20 s, seulement sous le plafond, seulement sur un dépôt vanilla.
	protected static void Pulse()
	{
		if (!HAM_Mission.IsRunning() || !s_aCaches)
			return;

		bool changed = false;
		for (int i = s_aCaches.Count() - 1; i >= 0; i--)
		{
			IEntity cache = s_aCaches[i];
			if (!cache)
			{
				s_aCaches.Remove(i);
				continue;
			}

			if (Grant(cache, GAIN))
				changed = true;
		}

		if (!changed)
			return;

		HAM_Mission.MarkClientsDirty();
		HAM_Mission.MarkSaveDirty();
	}

	protected static bool Grant(IEntity cache, float amount)
	{
		IEntity storage = HAM_Mission.FindSupplyStorage(cache);
		if (!storage)
			storage = cache;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(storage.FindComponent(SCR_ResourceComponent));
		if (!resources)
			resources = SCR_ResourceComponent.Cast(cache.FindComponent(SCR_ResourceComponent));

		if (!resources)
			return false;

		SCR_ResourceEncapsulator enc = resources.GetEncapsulator(EResourceType.SUPPLIES);
		if (enc && enc.GetContainerCount() > 0)
		{
			float before = enc.GetAggregatedResourceValue();
			float maxValue = enc.GetAggregatedMaxResourceValue();
			if (before + 0.5 >= maxValue)
				return false;

			enc.RequestGeneration(amount);
			if (enc.GetAggregatedResourceValue() > before + 0.01)
				return true;

			return AddToBox(storage, amount);
		}

		SCR_ResourceContainer stock = resources.GetContainer(EResourceType.SUPPLIES);
		if (!stock)
			return false;

		float next = stock.GetResourceValue() + amount;
		if (next > stock.GetMaxResourceValue())
			next = stock.GetMaxResourceValue();

		if (next <= stock.GetResourceValue() + 0.01)
			return false;

		stock.SetResourceValue(next, true);
		stock.EnableGain(false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool AddToBox(IEntity node, float amount)
	{
		if (!node)
			return false;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container && container.GetResourceValue() + 1 < container.GetMaxResourceValue())
			{
				float next = container.GetResourceValue() + amount;
				if (next > container.GetMaxResourceValue())
					next = container.GetMaxResourceValue();

				container.EnableGain(false);
				container.SetResourceValue(next, true);
				return true;
			}
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			if (AddToBox(child, amount))
				return true;

			child = child.GetSibling();
		}

		return false;
	}

	protected static void StopPrefabGain(IEntity node)
	{
		if (!node)
			return;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container && container.IsResourceGainEnabled())
				container.EnableGain(false);
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			StopPrefabGain(child);
			child = child.GetSibling();
		}
	}
}
