//------------------------------------------------------------------------------------------------
// Missions secondaires lancées depuis le tableau d'une base tenue, vers une zone ennemie de la ligne
// de front choisie par le joueur (maître, QG ennemi, esclave ou dépôt). Une mission à la fois par type,
// avec une tâche du jeu (marqueur carte). Les groupes ne se remplument jamais.
// Échec si la cible disparaît sans la condition prévue, si le temps est écoulé, ou si la zone visée
// change de camp avant la fin.
//------------------------------------------------------------------------------------------------

enum HAM_EMissionKind
{
	TOWER,
	CHECKPOINT,
	OFFICER,
	HOSTAGE,
	CACHE,
	IED
}

class HAM_SideMission
{
	HAM_EMissionKind m_eKind;
	string m_sBase;
	string m_sCell;
	string m_sZone;
	vector m_vZone;
	vector m_vObjective;
	SCR_Task m_Task;
	float m_fStarted;
	bool m_bContact;
	bool m_bPinned;
	bool m_bNoReward;
	bool m_bFreed;
	IEntity m_Target;
	IEntity m_HostageAction;
	ref array<IEntity> m_aProps = {};
	ref array<SCR_AIGroup> m_aGroups = {};
}

class HAM_SideMissions
{
	static const int KIND_COUNT = 6;
	static const float CONTACT_RADIUS = 400;
	protected static const string TASK_MARK = "HAM_TASK";
	protected static const ResourceName TASK_PREFAB = "{CF6A2A13DF7B1018}Prefabs/Tasks/ExtendedTask.et";
	protected static ref array<IEntity> s_aSweep;

	protected static ref array<ref HAM_SideMission> s_aMissions;
	protected static ref array<ref HAM_SideMission> s_aClosing;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aMissions = null;
		s_aClosing = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aMissions)
			s_aMissions = {};

		if (!s_aClosing)
			s_aClosing = {};
	}

	//------------------------------------------------------------------------------------------------
	//! Un civil d'une planque a parlé : tâche de destruction sur la cache déjà posée.
	static bool OpenHideout(HAM_Point point, bool announce = true)
	{
		Ensure();
		if (!point || !point.m_Cache || !point.m_Cell)
			return false;

		foreach (HAM_SideMission existing : s_aMissions)
		{
			if (existing.m_bPinned && existing.m_sZone == point.m_Def.m_sName)
				return true;
		}

		HAM_SideMission mission = new HAM_SideMission();
		mission.m_eKind = HAM_EMissionKind.CACHE;
		mission.m_sBase = HAM_Mission.HQ_NAME;
		mission.m_sCell = point.m_Cell.m_sName;
		mission.m_sZone = point.m_Def.m_sName;
		mission.m_vZone = point.Pos();
		mission.m_vObjective = point.m_Cache.GetOrigin();
		mission.m_Target = point.m_Cache;
		mission.m_bPinned = true;
		mission.m_fStarted = HAM_Util.Now();
		s_aMissions.Insert(mission);
		CreateTask(mission);
		TrackTransient(mission);
		if (announce)
		{
			HAM_Mission.Notify("Planque", string.Format("Cache signalée : %1. Détruisez-la.", point.m_Def.m_sName), 10);
			HAM_Mission.MarkDirty();
			HAM_Mission.SaveNow();
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool HideoutTaskOpen(string zone)
	{
		if (!s_aMissions || zone.IsEmpty())
			return false;

		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (mission && mission.m_bPinned && mission.m_eKind == HAM_EMissionKind.CACHE && mission.m_sZone == zone)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Au chargement : aucune tâche temporaire, ni ses unités, ne reprend. Les vieilles clés JSON sont ignorées.
	static void DropPersisted()
	{
		if (!Replication.IsServer())
			return;

		DropPersistedTasks();
		DropPersistedUnits();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TaskIsLive(SCR_Task task)
	{
		if (!task || !s_aMissions)
			return false;

		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (mission && mission.m_Task == task)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropPersistedTasks()
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem)
			return;

		array<SCR_Task> tasks = {};
		taskSystem.GetTasks(tasks);
		foreach (SCR_Task task : tasks)
		{
			if (!task || TaskIsLive(task))
				continue;

			string id = task.GetTaskID();
			if (id.IndexOf("HAM_") != 0)
				continue;

			HAM_Util.Forget(task);
			DeleteTask(task);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeTransient(IEntity ent)
	{
		if (!ent)
			return true;

		SCR_AIGroup group = SCR_AIGroup.Cast(ent);
		if (group && group.GetCustomName() == TASK_MARK)
		{
			if (!s_aSweep.Contains(group))
				s_aSweep.Insert(group);

			return true;
		}

		if (!ent.GetPrefabData())
			return true;

		string prefab = ent.GetPrefabData().GetPrefabName();
		if (prefab.Contains("Ied_Zone") || prefab.Contains("HAM_HostageAction"))
		{
			if (!s_aSweep.Contains(ent))
				s_aSweep.Insert(ent);
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropPersistedUnits()
	{
		if (!GetGame().GetWorld())
			return;

		s_aSweep = {};
		if (HAM_Mission.s_aPoints)
		{
			foreach (HAM_Point point : HAM_Mission.s_aPoints)
			{
				if (point)
					GetGame().GetWorld().QueryEntitiesBySphere(point.Pos(), 1500, SeeTransient);
			}
		}

		if (HAM_Mission.s_HqPoint)
			GetGame().GetWorld().QueryEntitiesBySphere(HAM_Mission.s_HqPoint.m_vPos, 1500, SeeTransient);

		foreach (IEntity ent : s_aSweep)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(ent);
			if (group)
			{
				if (GroupIsLive(group))
					continue;

				HAM_Util.ForgetGroup(group);
				HAM_Util.DeleteGroup(group);
				continue;
			}

			if (PropIsLive(ent))
				continue;

			HAM_Util.Forget(ent);
			HAM_Util.Delete(ent);
		}

		s_aSweep = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool GroupIsLive(SCR_AIGroup group)
	{
		if (!s_aMissions)
			return false;

		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (mission && mission.m_aGroups && mission.m_aGroups.Contains(group))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PropIsLive(IEntity ent)
	{
		if (!s_aMissions)
			return false;

		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (!mission)
				continue;

			if (mission.m_Target == ent || mission.m_HostageAction == ent)
				return true;

			if (mission.m_aProps && mission.m_aProps.Contains(ent))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TrackTransient(HAM_SideMission mission)
	{
		if (!mission || !GetGame().GetCallqueue())
			return;

		ForgetTransient(mission);
		GetGame().GetCallqueue().CallLater(ForgetTransient, 8000, false, mission);
		GetGame().GetCallqueue().CallLater(ForgetTransient, 20000, false, mission);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ForgetTransient(HAM_SideMission mission)
	{
		if (!mission)
			return;

		HAM_Util.Forget(mission.m_Target);
		HAM_Util.Forget(mission.m_HostageAction);
		HAM_Util.Forget(mission.m_Task);
		if (mission.m_aProps)
		{
			foreach (IEntity prop : mission.m_aProps)
				HAM_Util.Forget(prop);
		}

		if (!mission.m_aGroups)
			return;

		foreach (SCR_AIGroup group : mission.m_aGroups)
		{
			if (!group)
				continue;

			group.SetCustomName(TASK_MARK, 0);
			HAM_Util.ForgetGroup(group);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un civil demande qu'on libère un proche : civil immobilisé dans l'esclave le plus proche du joueur.
	static bool OpenFamilyRescue(vector from)
	{
		Ensure();
		foreach (HAM_SideMission existing : s_aMissions)
		{
			if (existing.m_eKind == HAM_EMissionKind.HOSTAGE && existing.m_bNoReward && !existing.m_bFreed)
				return true;
		}

		HAM_Point slave = NearestSlave(from);
		if (!slave)
			return false;

		float reach = slave.Radius();
		if (reach < 30)
			reach = 30;

		vector spot = HAM_Util.DryAround(slave.Pos(), 12, reach * 0.6);
		IEntity body = HAM_Util.Spawn(HAM_Units.MISSION_HOSTAGE, spot, Math.RandomFloat(0, 360));
		if (!body)
			return false;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetCanMove(false);

		HAM_SideMission mission = new HAM_SideMission();
		mission.m_eKind = HAM_EMissionKind.HOSTAGE;
		mission.m_sBase = HAM_Mission.HQ_NAME;
		if (slave.m_Cell)
			mission.m_sCell = slave.m_Cell.m_sName;

		mission.m_sZone = slave.m_Def.m_sName;
		mission.m_vZone = slave.Pos();
		mission.m_vObjective = body.GetOrigin();
		mission.m_Target = body;
		mission.m_bPinned = true;
		mission.m_bNoReward = true;
		mission.m_fStarted = HAM_Util.Now();
		s_aMissions.Insert(mission);
		CreateTask(mission);
		TrackTransient(mission);
		HAM_Mission.Notify("Famille", string.Format("Quelqu'un est retenu près de %1.", slave.m_Def.m_sName), 10);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_Point NearestSlave(vector from)
	{
		if (!HAM_Mission.s_aCells)
			return null;

		HAM_Point best;
		float bestSq = -1;
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell)
				continue;

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (!slave)
					continue;

				float distSq = vector.DistanceSqXZ(from, slave.Pos());
				if (bestSq >= 0 && distSq >= bestSq)
					continue;

				bestSq = distSq;
				best = slave;
			}
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	static string KindName(HAM_EMissionKind kind)
	{
		switch (kind)
		{
			case HAM_EMissionKind.TOWER:
				return "Tour radio";
			case HAM_EMissionKind.CHECKPOINT:
				return "Checkpoint";
			case HAM_EMissionKind.OFFICER:
				return "Gradé";
			case HAM_EMissionKind.HOSTAGE:
				return "Otage";
			case HAM_EMissionKind.CACHE:
				return "Cache";
			case HAM_EMissionKind.IED:
				return "Zone IED";
		}

		return "Mission";
	}

	//------------------------------------------------------------------------------------------------
	static HAM_SideMission Find(HAM_EMissionKind kind)
	{
		Ensure();
		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (mission.m_eKind == kind)
				return mission;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Une ligne par type pour le tableau : « Tour radio : en cours (Levie) » ou « disponible ».
	static void StatusLines(notnull array<string> lines)
	{
		lines.Clear();
		for (int kind = 0; kind < KIND_COUNT; kind++)
		{
			HAM_SideMission mission = Find(kind);
			if (mission)
				lines.Insert(string.Format("%1 : en cours (%2)", KindName(kind), mission.m_sZone));
			else
				lines.Insert(string.Format("%1 : disponible", KindName(kind)));
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Zones visables : maîtres et esclaves ennemis du front. Jamais un dépôt, jamais deux fois le même nom.
	//! Une entrée « nom|libellé » par zone, pour la liste et la recherche du tableau.
	static void Targets(notnull array<string> entries)
	{
		entries.Clear();
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!HAM_Mission.IsFront(cell))
				continue;

			bool held = cell.IsMasterPlayer();
			if (!held)
			{
				string kind = "Maître";
				if (cell.m_bEnemyHq)
					kind = "QG ennemi";

				string entry = string.Format("%1|%1 — %2", cell.m_sName, kind);
				if (entries.Find(entry) < 0)
					entries.Insert(entry);
			}

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (slave.m_bPlayer)
					continue;

				if (slave.m_Def.m_eType == HAM_EMasterType.SUPPLY_VANILLA || slave.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
					continue;

				string entry = string.Format("%1|%1 — esclave de %2", slave.m_Def.m_sName, cell.m_sName);
				if (entries.Find(entry) < 0)
					entries.Insert(entry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Zone visée retrouvée par son nom parmi les cibles du front : sa cellule et sa position.
	protected static bool ResolveZone(string zoneName, out HAM_Cell cell, out vector center)
	{
		array<string> entries = {};
		Targets(entries);
		bool listed = false;
		foreach (string entry : entries)
		{
			if (entry.StartsWith(zoneName + "|"))
				listed = true;
		}

		if (!listed)
			return false;

		foreach (HAM_Cell candidate : HAM_Mission.s_aCells)
		{
			if (candidate.m_sName == zoneName)
			{
				cell = candidate;
				center = candidate.Pos();
				return true;
			}

			foreach (HAM_Point slave : candidate.m_aSlaves)
			{
				if (slave.m_Def.m_sName == zoneName)
				{
					cell = candidate;
					center = slave.Pos();
					return true;
				}
			}

			foreach (HAM_Deposit deposit : candidate.m_aDeposits)
			{
				if (deposit.m_Def.m_sName == zoneName)
				{
					cell = candidate;
					center = deposit.Pos();
					return true;
				}
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool Launch(HAM_EMissionKind kind, string baseName, string zoneName, int playerId)
	{
		Ensure();
		if (!HAM_Mission.IsAlliedBase(baseName))
			return false;

		if (Find(kind))
		{
			HAM_Net.NotifyPlayer(playerId, "Missions", KindName(kind) + " : une mission de ce type est déjà en cours.", 6);
			return false;
		}

		array<string> targets = {};
		Targets(targets);
		if (targets.IsEmpty())
		{
			HAM_Net.NotifyPlayer(playerId, "Missions", "Aucune zone ennemie sur la ligne de front.", 6);
			return false;
		}

		if (zoneName.IsEmpty())
		{
			string picked = targets.GetRandomElement();
			zoneName = picked.Substring(0, picked.IndexOf("|"));
		}

		HAM_Cell cell;
		vector center;
		if (!ResolveZone(zoneName, cell, center))
		{
			HAM_Net.NotifyPlayer(playerId, "Missions", zoneName + " n'est plus une zone ennemie du front.", 6);
			return false;
		}

		HAM_SideMission mission = new HAM_SideMission();
		mission.m_eKind = kind;
		mission.m_sBase = baseName;
		mission.m_sCell = cell.m_sName;
		mission.m_sZone = zoneName;
		mission.m_vZone = center;
		mission.m_fStarted = HAM_Util.Now();

		bool ok = false;
		switch (kind)
		{
			case HAM_EMissionKind.TOWER:
				ok = SetupTower(mission);
				break;
			case HAM_EMissionKind.CHECKPOINT:
				ok = SetupCheckpoint(mission);
				break;
			case HAM_EMissionKind.OFFICER:
				ok = SetupOfficer(mission);
				break;
			case HAM_EMissionKind.HOSTAGE:
				ok = SetupHostage(mission);
				break;
			case HAM_EMissionKind.CACHE:
				ok = SetupCache(mission);
				break;
			case HAM_EMissionKind.IED:
				ok = SetupIedZone(mission);
				break;
		}

		if (!ok)
		{
			Cleanup(mission);
			HAM_Net.NotifyPlayer(playerId, "Missions", KindName(kind) + " : aucun emplacement valable près de " + zoneName + ".", 6);
			return false;
		}

		s_aMissions.Insert(mission);
		CreateTask(mission);
		TrackTransient(mission);
		HAM_Util.Info(string.Format("Mission %1 lancée depuis %2 : %3, objectif à %4", KindName(kind), baseName, zoneName, mission.m_vObjective.ToString()));
		HAM_Mission.Notify("Mission secondaire", string.Format("%1 lancée depuis %2, cible : %3. Marqueur sur la carte.", KindName(kind), baseName, zoneName), 12);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static string ObjectiveText(HAM_EMissionKind kind)
	{
		switch (kind)
		{
			case HAM_EMissionKind.TOWER:
				return "Détruire la tour radio ennemie.";
			case HAM_EMissionKind.CHECKPOINT:
				return "Neutraliser tous les gardes du checkpoint.";
			case HAM_EMissionKind.OFFICER:
				return "Abattre le gradé ennemi, retranché dans un bâtiment.";
			case HAM_EMissionKind.HOSTAGE:
				return "Libérer l'otage retenu dans un bâtiment.";
			case HAM_EMissionKind.CACHE:
				return "Détruire la cache d'armes ennemie.";
			case HAM_EMissionKind.IED:
				return "Désamorcer les quatre IED posés sur les bonbonnes.";
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Tâche du jeu, comme les missions du Conflit : marqueur sur la carte et dans la liste des tâches de la faction.
	protected static void CreateTask(HAM_SideMission mission)
	{
		SCR_TaskSystem taskSystem = SCR_TaskSystem.GetInstance();
		if (!taskSystem)
		{
			HAM_Util.Info("Mission : système de tâches absent, pas de marqueur");
			return;
		}

		string taskId = string.Format("HAM_MISSION_%1_%2", mission.m_eKind, Math.Round(mission.m_fStarted));
		string title = string.Format("%1 — %2", KindName(mission.m_eKind), mission.m_sZone);
		string description = ObjectiveText(mission.m_eKind);
		if (!mission.m_bPinned)
			description = description + string.Format(" Temps limite : %1 min.", HAM_Util.FormatInt(HAM_Settings.Get().m_fMissionTimeoutMinutes));
		SCR_Task task = taskSystem.CreateTask(TASK_PREFAB, taskId, title, description, mission.m_vObjective);
		if (!task)
		{
			HAM_Util.Info("Mission : tâche non créée, pas de marqueur");
			return;
		}

		taskSystem.SetTaskOwnership(task, SCR_ETaskOwnership.FACTION);
		taskSystem.SetTaskVisibility(task, SCR_ETaskVisibility.FACTION);
		taskSystem.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		taskSystem.AddTaskFaction(task, HAM_Units.PLAYER_FACTION);
		HAM_Util.Forget(task);
		mission.m_Task = task;
	}

	//------------------------------------------------------------------------------------------------
	protected static void EndTask(HAM_SideMission mission, SCR_ETaskState state)
	{
		if (!mission.m_Task)
			return;

		if (mission.m_Task.GetTaskState() != state)
			mission.m_Task.SetTaskState(state);

		GetGame().GetCallqueue().CallLater(DeleteTask, 20000, false, mission.m_Task);
		mission.m_Task = null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void DeleteTask(SCR_Task task)
	{
		if (!Replication.IsServer() || !task)
			return;

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
	static bool Cancel(HAM_EMissionKind kind, int playerId)
	{
		HAM_SideMission mission = Find(kind);
		if (!mission)
			return false;

		Close(mission, false, "annulée depuis le tableau", true);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector PickSpot(vector center, float radius)
	{
		for (int attempt = 0; attempt < 10; attempt++)
		{
			vector candidate = HAM_Util.RandomAround(center, 80, radius);
			if (HAM_Util.IsWater(candidate))
				continue;

			vector spot = HAM_Util.FreeSpot(candidate, 30);
			if (!HAM_Util.IsWater(spot))
				return spot;
		}

		return HAM_Util.DryAround(center, 0, 60);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Guards(HAM_SideMission mission, vector pos, int groups)
	{
		for (int g = 0; g < groups; g++)
		{
			SCR_AIGroup group = HAM_Util.SpawnFullGroup(HAM_Units.AreaGroup(), HAM_Util.DryAround(pos, 8, 30), 0);
			if (!group)
				continue;

			group.SetCustomName(TASK_MARK, 0);
			HAM_Util.Order(group, HAM_Units.WP_DEFEND, pos, 30);
			mission.m_aGroups.Insert(group);
			HAM_Garrison.RegisterEnemy(group);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void MissionLav(HAM_SideMission mission)
	{
		if (!mission)
			return;

		vector at = mission.m_vObjective;
		vector road;
		if (HAM_Util.RoadPoint(at, 120, road) && !HAM_Util.IsNearWater(road))
			at = road;

		Vehicle lav = HAM_Garrison.SpawnCrewedVehicle(HAM_Units.VEHICLE_LAV, at, Math.RandomFloat(0, 360), false, null, HAM_Units.WP_DEFEND, 40, mission.m_aGroups, true, ResourceName.Empty);
		if (lav)
			mission.m_aProps.Insert(lav);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupTower(HAM_SideMission mission)
	{
		mission.m_vObjective = PickSpot(mission.m_vZone, 450);
		mission.m_Target = HAM_Util.Spawn(HAM_Units.MISSION_TOWER, mission.m_vObjective, Math.RandomFloat(0, 360));
		if (!mission.m_Target)
			return false;

		Guards(mission, mission.m_vObjective, 1);
		MissionLav(mission);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupCheckpoint(HAM_SideMission mission)
	{
		vector roadPoint;
		bool found = false;
		for (int attempt = 0; attempt < 8 && !found; attempt++)
		{
			found = HAM_Util.RoadPoint(HAM_Util.RandomAround(mission.m_vZone, 150, 600), 120, roadPoint);
		}

		if (!found)
			return false;

		mission.m_vObjective = roadPoint;
		IEntity checkpoint = HAM_Util.Spawn(HAM_Units.MISSION_CHECKPOINT, roadPoint, Math.RandomFloat(0, 360));
		if (checkpoint)
			mission.m_aProps.Insert(checkpoint);

		Guards(mission, roadPoint, 2);
		MissionLav(mission);
		return !mission.m_aGroups.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupOfficer(HAM_SideMission mission)
	{
		ResourceName officerPrefab = HAM_Units.EnemyOfficer();
		if (officerPrefab.IsEmpty())
			return false;

		mission.m_vObjective = IndoorSpot(mission.m_vZone);
		mission.m_Target = HAM_Util.Spawn(officerPrefab, mission.m_vObjective, Math.RandomFloat(0, 360));
		if (!mission.m_Target)
			return false;

		Guards(mission, mission.m_vObjective, 2);
		MissionLav(mission);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupHostage(HAM_SideMission mission)
	{
		mission.m_vObjective = IndoorSpot(mission.m_vZone);
		mission.m_Target = HAM_Util.Spawn(HAM_Units.MISSION_HOSTAGE, mission.m_vObjective, Math.RandomFloat(0, 360));
		if (!mission.m_Target)
			return false;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(mission.m_Target.FindComponent(CharacterControllerComponent));
		if (controller)
			controller.SetCanMove(false);

		Guards(mission, mission.m_vObjective, 2);
		MissionLav(mission);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupIedZone(HAM_SideMission mission)
	{
		vector spot;
		if (!HAM_Util.FindIedClearing(mission.m_vZone, spot))
			return false;

		IEntity zone = HAM_Util.SpawnLevelled(HAM_Units.IED_ZONE, spot, Math.RandomFloat(0, 360));
		if (!zone)
			return false;

		mission.m_Target = zone;
		mission.m_vObjective = zone.GetOrigin();
		PlaceIedsOnTanks(zone, mission);
		int missing = 4 - mission.m_aProps.Count();
		for (int i = 0; i < missing; i++)
		{
			float angle = Math.PI2 * i / 4;
			vector at = zone.GetOrigin() + Vector(Math.Cos(angle) * 2.4, 0, Math.Sin(angle) * 2.4);
			IEntity mine = HAM_Devices.SpawnBareIed(at);
			if (mine)
				mission.m_aProps.Insert(mine);
		}

		return mission.m_aProps.Count() >= 4;
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceIedsOnTanks(IEntity node, HAM_SideMission mission)
	{
		if (!node || !mission)
			return;

		string prefabName = string.Empty;
		if (node.GetPrefabData())
			prefabName = node.GetPrefabData().GetPrefabName();

		prefabName.ToLower();
		if (prefabName.Contains("gastank"))
		{
			IEntity mine = HAM_Devices.SpawnBareIed(node.GetOrigin());
			if (mine)
				mission.m_aProps.Insert(mine);
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			PlaceIedsOnTanks(child, mission);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickIedZone(HAM_SideMission mission)
	{
		int left = 0;
		foreach (IEntity mine : mission.m_aProps)
		{
			if (mine && !HAM_Util.IsDead(mine))
				left++;
		}

		if (left > 0)
			return;

		foreach (IEntity mine : mission.m_aProps)
		{
			HAM_Devices.ForgetBareIed(mine);
			if (mine && !HAM_Util.IsDead(mine))
				HAM_Util.Delete(mine);
		}

		mission.m_aProps.Clear();
		HAM_Util.Delete(mission.m_Target);
		mission.m_Target = null;
		Close(mission, true, "IED désamorcés");
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SetupCache(HAM_SideMission mission)
	{
		vector spot;
		if (!FindClosedCacheSpot(mission.m_vZone, spot))
			return false;

		mission.m_vObjective = spot;
		mission.m_Target = HAM_Util.Spawn(HAM_Units.MISSION_CACHE, mission.m_vObjective, Math.RandomFloat(0, 360));
		if (!mission.m_Target)
			return false;

		Guards(mission, mission.m_vObjective, 1);
		MissionLav(mission);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Pièce fermée, pas un grenier. Sinon un sol sec près d'un bâtiment du rayon.
	protected static bool FindClosedCacheSpot(vector center, out vector spot)
	{
		spot = vector.Zero;
		array<SCR_DestructibleBuildingEntity> buildings = {};
		HAM_Interior.CollectBuildings(center, 500, buildings);
		int count = buildings.Count();
		for (int attempt = 0; attempt < 24 && count > 0; attempt++)
		{
			int index = attempt;
			while (index >= count)
				index = index - count;

			SCR_DestructibleBuildingEntity house = buildings[index];
			vector inside;
			if (!house || !HAM_Interior.FindSpot(house, null, inside) || HAM_Util.IsWater(inside))
				continue;

			if (!HAM_CQBInterior.IsEnclosed(house, inside) || HAM_CQBInterior.IsUnderRoof(house, inside))
				continue;

			spot = inside;
			return true;
		}

		foreach (SCR_DestructibleBuildingEntity house : buildings)
		{
			if (!house)
				continue;

			vector outside = HAM_Util.DryAround(house.GetOrigin(), 6, 16);
			if (HAM_Util.IsWater(outside) || HAM_CQBInterior.IsRaisedFloor(outside))
				continue;

			spot = outside;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector IndoorSpot(vector center)
	{
		array<SCR_DestructibleBuildingEntity> buildings = {};
		HAM_Interior.CollectBuildings(center, 400, buildings);
		for (int attempt = 0; attempt < 8 && !buildings.IsEmpty(); attempt++)
		{
			vector spot;
			if (HAM_Interior.FindSpot(buildings.GetRandomElement(), null, spot) && !HAM_Util.IsWater(spot))
				return spot;
		}

		return PickSpot(center, 400);
	}

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		Ensure();
		float timeout = HAM_Settings.Get().m_fMissionTimeoutMinutes * 60;
		float now = HAM_Util.Now();
		array<IEntity> bodies = HAM_Mission.s_aPlayerBodies;

		for (int i = s_aMissions.Count() - 1; i >= 0; i--)
		{
			HAM_SideMission mission = s_aMissions[i];
			if (!mission.m_bContact && HAM_Util.AnyNear(bodies, mission.m_vObjective, CONTACT_RADIUS))
				mission.m_bContact = true;

			if (!mission.m_bPinned && now - mission.m_fStarted > timeout)
			{
				Close(mission, false, "temps écoulé");
				continue;
			}

			switch (mission.m_eKind)
			{
				case HAM_EMissionKind.TOWER:
					TickDestroy(mission, "tour détruite", "tour disparue");
					break;
				case HAM_EMissionKind.CACHE:
					TickDestroy(mission, "cache détruite", "cache disparue");
					break;
				case HAM_EMissionKind.CHECKPOINT:
					if (mission.m_bContact && HAM_Util.AliveCountAll(mission.m_aGroups) == 0)
						Close(mission, true, "checkpoint vidé");
					break;
				case HAM_EMissionKind.OFFICER:
					TickOfficer(mission);
					break;
				case HAM_EMissionKind.HOSTAGE:
					TickHostage(mission);
					break;
				case HAM_EMissionKind.IED:
					TickIedZone(mission);
					break;
			}
		}

		for (int c = s_aClosing.Count() - 1; c >= 0; c--)
		{
			HAM_SideMission closing = s_aClosing[c];
			if (HAM_Util.AnyNear(bodies, closing.m_vObjective, HAM_Settings.Get().m_fDespawnDistance))
				continue;

			Cleanup(closing);
			s_aClosing.Remove(c);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickDestroy(HAM_SideMission mission, string successReason, string goneReason)
	{
		if (!mission.m_Target)
		{
			bool witnessed = HAM_Util.AnyNear(HAM_Mission.s_aPlayerBodies, mission.m_vObjective, CONTACT_RADIUS);
			Close(mission, witnessed, goneReason);
			return;
		}

		DamageManagerComponent damage = DamageManagerComponent.Cast(mission.m_Target.FindComponent(DamageManagerComponent));
		if (damage && damage.GetState() == EDamageState.DESTROYED)
			Close(mission, true, successReason);
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickOfficer(HAM_SideMission mission)
	{
		if (!mission.m_Target)
		{
			Close(mission, false, "gradé disparu");
			return;
		}

		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(mission.m_Target.FindComponent(SCR_CharacterDamageManagerComponent));
		if (damage && damage.GetState() == EDamageState.DESTROYED)
			Close(mission, true, "gradé abattu");
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickHostage(HAM_SideMission mission)
	{
		if (mission.m_bFreed)
		{
			Close(mission, true, "otage libéré");
			return;
		}

		if (!mission.m_Target || HAM_Util.IsDead(mission.m_Target))
			Close(mission, false, "otage mort");
	}

	//------------------------------------------------------------------------------------------------
	//! Action « Libérer l'otage » (serveur).
	static void FreeHostage(IEntity action, int playerId)
	{
		if (!Replication.IsServer() || !action)
			return;

		Ensure();
		IEntity body = HAM_ActionUtil.CivilianBody(action);
		foreach (HAM_SideMission mission : s_aMissions)
		{
			if (!mission || mission.m_eKind != HAM_EMissionKind.HOSTAGE || mission.m_bFreed)
				continue;

			if (mission.m_Target != body && mission.m_HostageAction != action)
				continue;

			if (!mission.m_Target || HAM_Util.IsDead(mission.m_Target))
				return;

			CharacterControllerComponent controller = CharacterControllerComponent.Cast(mission.m_Target.FindComponent(CharacterControllerComponent));
			if (controller)
				controller.SetCanMove(true);

			ActionsManagerComponent manager = ActionsManagerComponent.Cast(mission.m_Target.FindComponent(ActionsManagerComponent));
			if (manager)
			{
				array<BaseUserAction> actions = {};
				manager.GetActionsList(actions);
				foreach (BaseUserAction listed : actions)
				{
					if (HAM_HostageFreeUserAction.Cast(listed))
						listed.SetActionEnabled_S(false);
				}
			}

			if (mission.m_HostageAction)
				HAM_Util.Delete(mission.m_HostageAction);

			mission.m_HostageAction = null;
			mission.m_bFreed = true;
			HAM_Net.NotifyPlayer(playerId, "Otage", "Otage libéré.", 5);
			return;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Close(HAM_SideMission mission, bool success, string reason, bool cancelled = false)
	{
		s_aMissions.RemoveItem(mission);
		if (success)
		{
			EndTask(mission, SCR_ETaskState.COMPLETED);
			if (mission.m_bNoReward)
				HAM_Mission.Notify("Famille", "La personne est libre.", 8);
			else
				Reward(mission);
		}
		else
		{
			if (cancelled)
				EndTask(mission, SCR_ETaskState.CANCELLED);
			else
				EndTask(mission, SCR_ETaskState.FAILED);

			HAM_Mission.Notify("Mission échouée", string.Format("%1 (%2) : %3.", KindName(mission.m_eKind), mission.m_sZone, reason), 10);
		}

		Print(string.Format("[13RDPA H&M] Mission %1 sur %2 terminée : %3", KindName(mission.m_eKind), mission.m_sZone, reason), LogLevel.NORMAL);
		s_aClosing.Insert(mission);
	}

	//------------------------------------------------------------------------------------------------
	//! Fournitures au QG désigné, et un versement d'eau, de nourriture ou de munitions à la base qui a lancé.
	protected static void Reward(HAM_SideMission mission)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Mission.PaySupplies(settings.m_iMissionRewardSupplies, "mission " + KindName(mission.m_eKind));

		string baseName = mission.m_sBase;
		if (!HAM_Mission.IsAlliedBase(baseName))
			baseName = HAM_Mission.HQ_NAME;

		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		HAM_EResource resource = Math.RandomInt(0, 3);
		if (stock)
		{
			stock.Add(resource, settings.m_fMissionRewardResource);
			HAM_Mission.RefreshBaseProp(baseName);
			HAM_Cell cell = HAM_Mission.FindCell(baseName);
			if (cell)
				HAM_Logistics.CheckHold(cell);
		}

		HAM_Mission.AddGlobalRep(settings.m_iRepMissionBonus, "mission réussie");
		HAM_Mission.MarkDirty();
		HAM_Mission.Notify("Mission réussie", string.Format("%1 (%2) : +%3 fournitures au QG, +%4 %5 à %6.", KindName(mission.m_eKind), mission.m_sZone, settings.m_iMissionRewardSupplies, HAM_Util.FormatInt(settings.m_fMissionRewardResource), HAM_Stock.Label(resource), baseName), 12);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Cleanup(HAM_SideMission mission)
	{
		HAM_Util.DeleteGroups(mission.m_aGroups);
		foreach (IEntity prop : mission.m_aProps)
		{
			HAM_Devices.ForgetBareIed(prop);
			HAM_Util.Delete(prop);
		}

		mission.m_aProps.Clear();
		HAM_Util.Delete(mission.m_HostageAction);
		if (mission.m_Target && !HAM_Util.IsDead(mission.m_Target))
			HAM_Util.Delete(mission.m_Target);
	}

	//------------------------------------------------------------------------------------------------
	static void OnCellChanged(HAM_Cell cell)
	{
	}
}
