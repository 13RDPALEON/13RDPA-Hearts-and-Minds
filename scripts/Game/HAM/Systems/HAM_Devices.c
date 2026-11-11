//------------------------------------------------------------------------------------------------
// IED et mines, posés à l'approche d'un joueur dans une cellule ennemie du front.
// Les IED restent en place. Les mines AP reviennent à chaque sortie puis entrée, et après
// un redémarrage, tant que ni l'esclave ni le maître ne sont aux joueurs. La reprise du maître les réactive.
// Les engins ne sautent pas sous l'IA ennemie ni sous un civil.
//  - Chaque maître, esclave et dépôt ennemi : IED sur les routes de son rayon (pas les sentiers),
//    mines AP M14 entre mineMinDistance et mineMaxDistance autour du point, espacées de 14 m et
//    chacune signalée par un drapeau, comme le Conflit. Nombres dans le JSON.
//  - Les lignes ied / mines de la carte en ajoutent, si elles existent.
// Une zone prise (esclave ou maître) ne reçoit plus de mines. Celles déjà posées restent jusqu'à la perte du maître.
//------------------------------------------------------------------------------------------------

//! Engins d'un maître, d'un esclave ou d'un dépôt.
class HAM_DeviceZone
{
	HAM_Cell m_Cell;
	HAM_Point m_Point;
	HAM_Deposit m_Deposit;
	bool m_bActive;
	bool m_bSpent;
	bool m_bMinesPlaced;
	bool m_bIedsLocked;
	ref array<IEntity> m_aDevices = {};
	ref array<IEntity> m_aProps = {};

	//------------------------------------------------------------------------------------------------
	vector Pos()
	{
		if (m_Point)
			return m_Point.Pos();

		return m_Deposit.Pos();
	}

	//------------------------------------------------------------------------------------------------
	float Radius()
	{
		if (m_Point)
			return m_Point.Radius();

		return m_Deposit.m_Def.m_fRadius;
	}

	//------------------------------------------------------------------------------------------------
	string Name()
	{
		if (m_Point)
			return m_Point.m_Def.m_sName;

		return m_Deposit.m_Def.m_sName;
	}

	//------------------------------------------------------------------------------------------------
	//! Encore ennemie et sur la ligne de front.
	bool IsEligible()
	{
		if (!m_Cell || m_Cell.IsComplete() || !HAM_Mission.IsFront(m_Cell))
			return false;

		if (m_Point)
			return !m_Point.m_bPlayer;

		return !m_Cell.IsMasterPlayer();
	}

	//------------------------------------------------------------------------------------------------
	float ActivationDistance(HAM_Settings settings)
	{
		if (m_Point)
			return settings.ActivationOf(m_Point.m_Def);

		if (m_Deposit)
			return settings.ActivationOf(m_Deposit.m_Def);

		return settings.m_fActivationDistance;
	}

	//------------------------------------------------------------------------------------------------
	float DespawnDistance(HAM_Settings settings)
	{
		if (m_Point)
			return settings.DespawnOf(m_Point.m_Def);

		if (m_Deposit)
			return settings.DespawnOf(m_Deposit.m_Def);

		return settings.m_fDespawnDistance;
	}

	//------------------------------------------------------------------------------------------------
	bool UsesDepositThreat()
	{
		if (m_Deposit)
			return true;

		if (!m_Point)
			return false;

		return m_Point.m_Def.m_eType == HAM_EMasterType.SUPPLY_VANILLA || m_Point.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL;
	}

	//------------------------------------------------------------------------------------------------
	//! Le QG ennemi compte comme un maître.
	void Counts(HAM_Settings settings, out int ieds, out int mines)
	{
		HAM_MapPoint def = null;
		if (m_Deposit)
			def = m_Deposit.m_Def;
		else if (m_Point)
			def = m_Point.m_Def;

		if (!def)
		{
			ieds = 0;
			mines = 0;
			return;
		}

		ieds = settings.IedsOf(def);
		mines = settings.MinesOf(def);
	}
}

class HAM_SavedIed
{
	string m_sZone;
	bool m_bLine;
	string m_sMine;
	vector m_vMine;
	string m_sCover;
	vector m_vCover;
	float m_fCoverYaw;
}

class HAM_DeviceJob
{
	HAM_DeviceZone m_Zone;
	HAM_Line m_Line;
	int m_iIedsLeft;
	int m_iMinesLeft;
	int m_iIedPlaced;
	int m_iMinePlaced;
	int m_iFlags;
	string m_sName;
	vector m_vCenter;
	float m_fRadius;
	float m_fMin;
	float m_fMax;
	float m_fRoad;
	ref array<IEntity> m_aDevices;
	ref array<IEntity> m_aProps;
	bool m_bMines;
}

//------------------------------------------------------------------------------------------------
class HAM_ApMark
{
	IEntity m_Mine;
	IEntity m_Flag;
	vector m_vPos;
}

class HAM_Devices
{
	protected static const float MINE_SPACING = 14;
	// IED du Conflit : un joueur qui bouge à plus de 1,5 m/s à 8 m de la mine la fait sauter.
	protected static const float IED_TRIGGER_RADIUS = 8;
	protected static const float IED_TRIGGER_SPEED = 1.5;
	protected static const float IED_DISARM_BLAST_CHANCE = 0.15;
	protected static const int IED_PULSE_MS = 400;

	protected static ref map<IEntity, bool> s_mDevices;
	protected static ref array<ref HAM_DeviceZone> s_aZones;
	protected static int s_iIedProp;

	protected static ref array<IEntity> s_aIedMines;
	protected static ref array<IEntity> s_aIedProps;
	protected static ref array<IEntity> s_aIedHeld;
	protected static ref map<int, vector> s_mPlayerPos;
	protected static ref map<int, float> s_mPlayerSpeed;
	protected static float s_fMotionAt;
	protected static bool s_bPulse;
	protected static ref array<ref HAM_DeviceJob> s_aJobs;
	protected static bool s_bDeviceDrain;
	protected static ref array<string> s_aIedLockZones;
	protected static ref array<string> s_aIedLockLines;
	protected static ref array<ref HAM_SavedIed> s_aSavedIeds;
	protected static ref array<ref HAM_ApMark> s_aApMarks;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_mDevices = null;
		s_aZones = null;
		s_aIedMines = null;
		s_aIedProps = null;
		s_aIedHeld = null;
		s_mPlayerPos = null;
		s_mPlayerSpeed = null;
		s_bPulse = false;
		s_aJobs = null;
		s_bDeviceDrain = false;
		s_aIedLockZones = null;
		s_aIedLockLines = null;
		s_aSavedIeds = null;
		s_aApMarks = null;
		if (GetGame())
			GetGame().GetCallqueue().Remove(PulseIed);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_mDevices)
			s_mDevices = new map<IEntity, bool>();

		if (!s_aIedMines)
		{
			s_aIedMines = {};
			s_aIedProps = {};
			s_aIedHeld = {};
			s_mPlayerPos = new map<int, vector>();
			s_mPlayerSpeed = new map<int, float>();
		}

		if (!s_bPulse && Replication.IsServer())
		{
			s_bPulse = true;
			GetGame().GetCallqueue().CallLater(PulseIed, IED_PULSE_MS, false);
		}

		if (s_aZones)
			return;

		s_aZones = {};
		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			HAM_DeviceZone zone = new HAM_DeviceZone();
			zone.m_Cell = point.m_Cell;
			zone.m_Point = point;
			s_aZones.Insert(zone);
		}

		foreach (HAM_Deposit deposit : HAM_Mission.s_aDeposits)
		{
			if (deposit.m_Def.m_eType == HAM_EMasterType.SUPPLY_PERSONAL)
				continue;

			HAM_DeviceZone depositZone = new HAM_DeviceZone();
			depositZone.m_Cell = deposit.m_Cell;
			depositZone.m_Deposit = deposit;
			s_aZones.Insert(depositZone);
		}
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	static void WriteSave(JsonSaveContext ctx)
	{
		array<string> zones = {};
		array<string> lines = {};
		array<ref HAM_SavedIed> ieds = {};
		if (s_aIedLockZones)
		{
			foreach (string lockedZone : s_aIedLockZones)
				zones.Insert(lockedZone);
		}

		if (s_aIedLockLines)
		{
			foreach (string lockedLine : s_aIedLockLines)
				lines.Insert(lockedLine);
		}

		if (s_aSavedIeds)
		{
			foreach (HAM_SavedIed saved : s_aSavedIeds)
			{
				if (saved)
					ieds.Insert(saved);
			}
		}

		CollectLiveIeds(zones, lines, ieds);

		ctx.WriteValue("iedZoneCount", zones.Count());
		for (int i = 0; i < zones.Count(); i++)
		{
			ctx.StartObject("iedZone" + i.ToString());
			ctx.WriteValue("name", zones[i]);
			ctx.EndObject();
		}

		ctx.WriteValue("iedLineCount", lines.Count());
		for (int j = 0; j < lines.Count(); j++)
		{
			ctx.StartObject("iedLine" + j.ToString());
			ctx.WriteValue("name", lines[j]);
			ctx.EndObject();
		}

		ctx.WriteValue("iedCount", ieds.Count());
		for (int k = 0; k < ieds.Count(); k++)
		{
			HAM_SavedIed ied = ieds[k];
			ctx.StartObject("ied" + k.ToString());
			ctx.WriteValue("zone", ied.m_sZone);
			ctx.WriteValue("line", ied.m_bLine);
			ctx.WriteValue("mine", ied.m_sMine);
			ctx.WriteValue("mx", ied.m_vMine[0]);
			ctx.WriteValue("my", ied.m_vMine[1]);
			ctx.WriteValue("mz", ied.m_vMine[2]);
			ctx.WriteValue("cover", ied.m_sCover);
			ctx.WriteValue("cx", ied.m_vCover[0]);
			ctx.WriteValue("cy", ied.m_vCover[1]);
			ctx.WriteValue("cz", ied.m_vCover[2]);
			ctx.WriteValue("cyaw", ied.m_fCoverYaw);
			ctx.EndObject();
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ReadSave(JsonLoadContext ctx)
	{
		s_aIedLockZones = {};
		s_aIedLockLines = {};
		s_aSavedIeds = {};

		int zoneCount;
		ctx.ReadValue("iedZoneCount", zoneCount);
		for (int i = 0; i < zoneCount; i++)
		{
			if (!ctx.StartObject("iedZone" + i.ToString()))
				continue;

			string name;
			ctx.ReadValue("name", name);
			ctx.EndObject();
			if (!name.IsEmpty())
				s_aIedLockZones.Insert(name);
		}

		int lineCount;
		ctx.ReadValue("iedLineCount", lineCount);
		for (int j = 0; j < lineCount; j++)
		{
			if (!ctx.StartObject("iedLine" + j.ToString()))
				continue;

			string name;
			ctx.ReadValue("name", name);
			ctx.EndObject();
			if (!name.IsEmpty())
				s_aIedLockLines.Insert(name);
		}

		int iedCount;
		ctx.ReadValue("iedCount", iedCount);
		for (int k = 0; k < iedCount; k++)
		{
			if (!ctx.StartObject("ied" + k.ToString()))
				continue;

			HAM_SavedIed ied = new HAM_SavedIed();
			ctx.ReadValue("zone", ied.m_sZone);
			ctx.ReadValue("line", ied.m_bLine);
			ctx.ReadValue("mine", ied.m_sMine);
			float x;
			float y;
			float z;
			if (ctx.ReadValue("mx", x) && ctx.ReadValue("my", y) && ctx.ReadValue("mz", z))
				ied.m_vMine = Vector(x, y, z);

			ctx.ReadValue("cover", ied.m_sCover);
			if (ctx.ReadValue("cx", x) && ctx.ReadValue("cy", y) && ctx.ReadValue("cz", z))
				ied.m_vCover = Vector(x, y, z);

			ctx.ReadValue("cyaw", ied.m_fCoverYaw);
			ctx.EndObject();
			s_aSavedIeds.Insert(ied);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ZoneIedsLocked(string name)
	{
		return s_aIedLockZones && s_aIedLockZones.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool LineIedsLocked(string name)
	{
		return s_aIedLockLines && s_aIedLockLines.Contains(name);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectLiveIeds(notnull array<string> zones, notnull array<string> lines, notnull array<ref HAM_SavedIed> ieds)
	{
		if (s_aZones)
		{
			foreach (HAM_DeviceZone zone : s_aZones)
			{
				if (!zone || (!zone.m_bSpent && !zone.m_bIedsLocked))
					continue;

				if (zones.Find(zone.Name()) < 0)
					zones.Insert(zone.Name());

				DropSaved(ieds, zone.Name(), false);
				AppendIeds(zone.Name(), false, zone.m_aDevices, ieds);
			}
		}

		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || !line.m_Def || line.m_Def.m_eRole != HAM_ERole.IED || !line.m_bSpawnedOnce)
				continue;

			if (lines.Find(line.m_Def.m_sName) < 0)
				lines.Insert(line.m_Def.m_sName);

			DropSaved(ieds, line.m_Def.m_sName, true);
			AppendIeds(line.m_Def.m_sName, true, line.m_aDevices, ieds);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void DropSaved(notnull array<ref HAM_SavedIed> ieds, string zone, bool line)
	{
		for (int i = ieds.Count() - 1; i >= 0; i--)
		{
			HAM_SavedIed ied = ieds[i];
			if (!ied || ied.m_sZone != zone || ied.m_bLine != line)
				continue;

			ieds.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AppendIeds(string zone, bool line, array<IEntity> devices, notnull array<ref HAM_SavedIed> ieds)
	{
		if (!devices || !s_aIedMines)
			return;

		foreach (IEntity device : devices)
		{
			if (!device || s_aIedMines.Find(device) < 0)
				continue;

			int index = s_aIedMines.Find(device);
			HAM_SavedIed ied = new HAM_SavedIed();
			ied.m_sZone = zone;
			ied.m_bLine = line;
			if (device.GetPrefabData())
				ied.m_sMine = device.GetPrefabData().GetPrefabName();

			ied.m_vMine = device.GetOrigin();
			IEntity cover = null;
			if (s_aIedProps && index >= 0 && index < s_aIedProps.Count())
				cover = s_aIedProps[index];

			if (cover)
			{
				if (cover.GetPrefabData())
					ied.m_sCover = cover.GetPrefabData().GetPrefabName();

				ied.m_vCover = cover.GetOrigin();
				vector angles = cover.GetYawPitchRoll();
				ied.m_fCoverYaw = angles[0];
			}

			ieds.Insert(ied);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void RestoreIeds(string zone, bool line, notnull array<IEntity> devices, notnull array<IEntity> props)
	{
		Ensure();
		if (!s_aSavedIeds)
			return;

		foreach (HAM_SavedIed ied : s_aSavedIeds)
		{
			if (!ied || ied.m_sZone != zone || ied.m_bLine != line || ied.m_sMine.IsEmpty())
				continue;

			IEntity mine = SpawnArmed(ied.m_sMine, ied.m_vMine);
			if (!mine)
				continue;

			s_aIedMines.Insert(mine);
			devices.Insert(mine);
			IEntity cover = null;
			if (!ied.m_sCover.IsEmpty())
				cover = HAM_Util.Spawn(ied.m_sCover, ied.m_vCover, ied.m_fCoverYaw);

			s_aIedProps.Insert(cover);
			if (cover)
				props.Insert(cover);
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsDevice(IEntity ent)
	{
		if (!s_mDevices || !ent)
			return false;

		return s_mDevices.Contains(ent);
	}

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		if (!HAM_Mission.s_aPoints || !HAM_Mission.s_aDeposits)
			return;

		Ensure();
		SweepSpentMines();
		SweepApMarks();
		HAM_Settings settings = HAM_Settings.Get();
		array<IEntity> bodies = HAM_Mission.s_aPlayerBodies;
		TickZones(settings, bodies);
		TickLines(settings, bodies);
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickZones(HAM_Settings settings, array<IEntity> bodies)
	{
		foreach (HAM_DeviceZone zone : s_aZones)
		{
			if (!settings.m_bZoneDevicesEnabled)
				continue;

			if (!zone.m_bIedsLocked && ZoneIedsLocked(zone.Name()))
			{
				RestoreIeds(zone.Name(), false, zone.m_aDevices, zone.m_aProps);
				zone.m_bIedsLocked = true;
				zone.m_bSpent = true;
			}

			float nearest = HAM_Util.NearestDistance(bodies, zone.Pos());
			if (MinesBlocked(zone))
			{
				CancelZoneMines(zone);
				continue;
			}

			if (nearest > zone.DespawnDistance(settings))
			{
				ClearApMines(zone.m_aDevices, zone.m_aProps);
				zone.m_bMinesPlaced = false;
				CancelZoneMines(zone);
				continue;
			}

			float reach = Math.Max(zone.Radius(), settings.m_fMineMaxDistance);
			if (nearest > Math.Min(zone.ActivationDistance(settings), reach + 500))
				continue;

			if (zone.m_bSpent || zone.m_bActive)
			{
				if (!zone.m_bMinesPlaced)
				{
					int mineFlags;
					int mineIeds;
					int mineCount;
					zone.Counts(settings, mineIeds, mineCount);
					zone.m_bMinesPlaced = true;
					PlaceMines(zone.Pos(), settings.m_fMineMinDistance, settings.m_fMineMaxDistance, mineCount, zone.m_aDevices, zone.m_aProps, mineFlags);
				}

				continue;
			}

			int ieds;
			int mines;
			zone.Counts(settings, ieds, mines);
			if (ZoneIedsLocked(zone.Name()))
			{
				RestoreIeds(zone.Name(), false, zone.m_aDevices, zone.m_aProps);
				zone.m_bIedsLocked = true;
				ieds = 0;
			}

			zone.m_bActive = true;
			zone.m_bSpent = true;
			zone.m_bMinesPlaced = true;
			HAM_DeviceJob job = new HAM_DeviceJob();
			job.m_Zone = zone;
			job.m_sName = zone.Name();
			job.m_iIedsLeft = ieds;
			job.m_iMinesLeft = mines;
			job.m_vCenter = zone.Pos();
			job.m_fRadius = zone.Radius();
			job.m_fMin = settings.m_fMineMinDistance;
			job.m_fMax = settings.m_fMineMaxDistance;
			job.m_fRoad = settings.m_fIedRoadMinWidth;
			job.m_aDevices = zone.m_aDevices;
			job.m_aProps = zone.m_aProps;
			EnqueueDevice(job);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickLines(HAM_Settings settings, array<IEntity> bodies)
	{
		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			HAM_ERole role = line.m_Def.m_eRole;
			if (role != HAM_ERole.IED && role != HAM_ERole.MINES)
				continue;

			if (role == HAM_ERole.MINES)
			{
				TickMineLine(line, bodies, settings);
				continue;
			}

			if (LineIedsLocked(line.m_Def.m_sName))
			{
				if (!line.m_bActive)
				{
					RestoreIeds(line.m_Def.m_sName, true, line.m_aDevices, line.m_aProps);
					line.m_bActive = true;
					line.m_bSpawnedOnce = true;
				}

				continue;
			}

			if (line.m_bActive || line.m_bSpawnedOnce)
				continue;

			if (HAM_Mission.SlaveHolds(line.m_Cell, line.Pos()))
			{
				line.m_bSpawnedOnce = true;
				continue;
			}

			float nearest = HAM_Util.NearestDistance(bodies, line.Pos());
			float activation = settings.m_fActivationDistance;
			if (line.m_Def.m_fRadius + 600 > activation)
				activation = line.m_Def.m_fRadius + 600;
			if (nearest > activation)
				continue;

			HAM_Cell cell = line.m_Cell;
			if (!cell || cell.IsComplete() || !HAM_Mission.IsFront(cell))
				continue;

			line.m_bActive = true;
			line.m_bSpawnedOnce = true;

			HAM_DeviceJob job = new HAM_DeviceJob();
			job.m_Line = line;
			job.m_sName = line.m_Def.m_sName;
			job.m_vCenter = line.Pos();
			job.m_fRadius = line.m_Def.m_fRadius;
			job.m_fRoad = settings.m_fIedRoadMinWidth;
			job.m_aDevices = line.m_aDevices;
			job.m_aProps = line.m_aProps;
			job.m_iIedsLeft = settings.IedsOf(line.m_Def);
			EnqueueDevice(job);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool MinesBlocked(HAM_DeviceZone zone)
	{
		if (!zone || !zone.m_Cell || !HAM_Mission.IsFront(zone.m_Cell) || zone.m_Cell.IsMasterPlayer())
			return true;

		if (!zone.m_Point)
			return false;

		return zone.m_Point.m_bPlayer;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CancelZoneMines(HAM_DeviceZone zone)
	{
		if (!s_aJobs || !zone)
			return;

		foreach (HAM_DeviceJob job : s_aJobs)
		{
			if (job && job.m_Zone == zone)
				job.m_iMinesLeft = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickMineLine(HAM_Line line, array<IEntity> bodies, HAM_Settings settings)
	{
		HAM_Cell cell = line.m_Cell;
		bool blocked = !cell || !HAM_Mission.IsFront(cell) || cell.IsMasterPlayer() || HAM_Mission.SlaveHolds(cell, line.Pos());
		if (blocked)
		{
			line.m_bSpawnedOnce = true;
			return;
		}

		float nearest = HAM_Util.NearestDistance(bodies, line.Pos());
		float activation = settings.m_fActivationDistance;
		if (line.m_Def.m_fRadius + 600 > activation)
			activation = line.m_Def.m_fRadius + 600;

		float despawn = settings.m_fDespawnDistance;
		if (despawn < activation + 100)
			despawn = activation + 100;

		if (nearest > despawn)
		{
			ClearApMines(line.m_aDevices, line.m_aProps);
			line.m_bActive = false;
			line.m_bSpawnedOnce = false;
			return;
		}

		if (nearest > activation || line.m_bSpawnedOnce)
			return;

		line.m_bSpawnedOnce = true;
		line.m_bActive = true;
		int flags;
		int mineCount = settings.MinesOf(line.m_Def);
		int placed = PlaceMines(line.Pos(), 0, line.m_Def.m_fRadius, mineCount, line.m_aDevices, line.m_aProps, flags);
		HAM_Util.Info(string.Format("Mines posées : %1, %2 sur %3 (%4 drapeaux)", line.m_Def.m_sName, placed, mineCount, flags));
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsApMine(IEntity ent)
	{
		return ent && ent.GetPrefabData() && ent.GetPrefabData().GetPrefabName().Contains("Mine_M14");
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsMineFlag(IEntity ent)
	{
		return ent && ent.GetPrefabData() && ent.GetPrefabData().GetPrefabName().Contains("MineFlag");
	}

	//------------------------------------------------------------------------------------------------
	//! La mine AP a été désamorcée ou a sauté : elle sort de la liste, et son drapeau avec elle.
	protected static void RetireApMine(IEntity mine)
	{
		if (!mine)
			return;

		vector pos = mine.GetOrigin();
		bool hadFlag = ReleaseMark(mine);
		PullDevice(mine);
		if (!hadFlag)
			DeleteFlagNear(pos);

		HAM_Util.Delete(mine);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberAp(IEntity mine, IEntity flag)
	{
		if (!mine)
			return;

		if (!s_aApMarks)
			s_aApMarks = {};

		HAM_ApMark mark = new HAM_ApMark();
		mark.m_Mine = mine;
		mark.m_Flag = flag;
		mark.m_vPos = mine.GetOrigin();
		s_aApMarks.Insert(mark);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ReleaseMark(IEntity mine)
	{
		if (!s_aApMarks || !mine)
			return false;

		for (int i = s_aApMarks.Count() - 1; i >= 0; i--)
		{
			HAM_ApMark mark = s_aApMarks[i];
			if (!mark || mark.m_Mine != mine)
				continue;

			if (mark.m_Flag)
			{
				PullFlag(mark.m_Flag);
				HAM_Util.Delete(mark.m_Flag);
			}

			s_aApMarks.Remove(i);
			return mark.m_Flag != null;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SweepApMarks()
	{
		if (!s_aApMarks)
			return;

		for (int i = s_aApMarks.Count() - 1; i >= 0; i--)
		{
			HAM_ApMark mark = s_aApMarks[i];
			if (!mark)
			{
				s_aApMarks.Remove(i);
				continue;
			}

			if (mark.m_Mine && !HAM_Util.IsDead(mark.m_Mine))
				continue;

			vector pos = mark.m_vPos;
			IEntity flag = mark.m_Flag;
			IEntity mine = mark.m_Mine;
			s_aApMarks.Remove(i);
			if (flag)
			{
				PullFlag(flag);
				HAM_Util.Delete(flag);
			}
			else
			{
				DeleteFlagNear(pos);
			}

			if (!mine)
				continue;

			PullDevice(mine);
			HAM_Util.Delete(mine);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PullFlag(IEntity flag)
	{
		if (!flag)
			return;

		if (s_aZones)
		{
			foreach (HAM_DeviceZone zone : s_aZones)
			{
				if (zone)
					PullOne(zone.m_aProps, flag);
			}
		}

		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (line)
				PullOne(line.m_aProps, flag);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SweepSpentMines()
	{
		if (s_aZones)
		{
			foreach (HAM_DeviceZone zone : s_aZones)
			{
				if (zone)
					SweepSpentList(zone.m_aDevices);
			}
		}

		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || !line.m_Def || line.m_Def.m_eRole != HAM_ERole.MINES)
				continue;

			SweepSpentList(line.m_aDevices);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SweepSpentList(array<IEntity> devices)
	{
		if (!devices)
			return;

		for (int i = devices.Count() - 1; i >= 0; i--)
		{
			IEntity mine = devices[i];
			if (!mine)
			{
				devices.Remove(i);
				continue;
			}

			if (!IsApMine(mine) || !HAM_Util.IsDead(mine))
				continue;

			RetireApMine(mine);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void PullDevice(IEntity mine)
	{
		if (s_aZones)
		{
			foreach (HAM_DeviceZone zone : s_aZones)
			{
				if (zone)
					PullOne(zone.m_aDevices, mine);
			}
		}

		if (HAM_Mission.s_aLines)
		{
			foreach (HAM_Line line : HAM_Mission.s_aLines)
			{
				if (line)
					PullOne(line.m_aDevices, mine);
			}
		}

		if (s_mDevices)
			s_mDevices.Remove(mine);
	}

	//------------------------------------------------------------------------------------------------
	protected static void PullOne(array<IEntity> list, IEntity mine)
	{
		if (!list || !mine)
			return;

		int index = list.Find(mine);
		if (index >= 0)
			list.Remove(index);
	}

	//------------------------------------------------------------------------------------------------
	//! Le drapeau est posé à 1,5–2 m. Les mines sont à 14 m : un seul drapeau peut être le sien.
	protected static void DeleteFlagNear(vector pos)
	{
		IEntity best;
		array<IEntity> bestList;
		float bestSq = 3.5 * 3.5;
		CollectNearestFlag(pos, s_aZones, best, bestList, bestSq);
		if (HAM_Mission.s_aLines)
		{
			foreach (HAM_Line line : HAM_Mission.s_aLines)
			{
				if (!line)
					continue;

				ConsiderFlags(line.m_aProps, pos, best, bestList, bestSq);
			}
		}

		if (!best || !bestList)
			return;

		int index = bestList.Find(best);
		if (index >= 0)
			bestList.Remove(index);

		HAM_Util.Delete(best);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectNearestFlag(vector pos, array<ref HAM_DeviceZone> zones, out IEntity best, out array<IEntity> bestList, out float bestSq)
	{
		if (!zones)
			return;

		foreach (HAM_DeviceZone zone : zones)
		{
			if (!zone)
				continue;

			ConsiderFlags(zone.m_aProps, pos, best, bestList, bestSq);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ConsiderFlags(array<IEntity> props, vector pos, out IEntity best, out array<IEntity> bestList, out float bestSq)
	{
		if (!props)
			return;

		for (int i = props.Count() - 1; i >= 0; i--)
		{
			IEntity flag = props[i];
			if (!flag)
			{
				props.Remove(i);
				continue;
			}

			if (!IsMineFlag(flag))
				continue;

			float distSq = vector.DistanceSqXZ(flag.GetOrigin(), pos);
			if (distSq > bestSq)
				continue;

			best = flag;
			bestList = props;
			bestSq = distSq;
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearApMines(array<IEntity> devices, array<IEntity> props)
	{
		if (devices)
		{
			for (int i = devices.Count() - 1; i >= 0; i--)
			{
				IEntity mine = devices[i];
				if (!mine)
				{
					devices.Remove(i);
					continue;
				}

				if (!IsApMine(mine))
					continue;

				devices.Remove(i);
				if (s_mDevices)
					s_mDevices.Remove(mine);

				ReleaseMark(mine);
				HAM_Util.Delete(mine);
			}
		}

		if (!props)
			return;

		for (int j = props.Count() - 1; j >= 0; j--)
		{
			IEntity flag = props[j];
			if (!flag)
			{
				props.Remove(j);
				continue;
			}

			if (!IsMineFlag(flag))
				continue;

			props.Remove(j);
			HAM_Util.Delete(flag);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void EnqueueDevice(HAM_DeviceJob job)
	{
		if (!s_aJobs)
			s_aJobs = {};

		s_aJobs.Insert(job);
		if (s_bDeviceDrain)
			return;

		s_bDeviceDrain = true;
		GetGame().GetCallqueue().CallLater(DrainDevice, 450, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void DrainDevice()
	{
		if (!s_aJobs || s_aJobs.IsEmpty())
		{
			s_bDeviceDrain = false;
			return;
		}

		HAM_DeviceJob job = s_aJobs[0];
		if (!DeviceJobAlive(job) || !StepDevice(job))
		{
			if (job && DeviceJobAlive(job))
			{
				HAM_Util.Info(string.Format("Engins posés : %1, IED %2, mines %3 (%4 drapeaux)", job.m_sName, job.m_iIedPlaced, job.m_iMinePlaced, job.m_iFlags));
				HAM_Mission.MarkDirty();
			}

			s_aJobs.Remove(0);
		}

		if (!s_aJobs.IsEmpty())
			GetGame().GetCallqueue().CallLater(DrainDevice, 450, false);
		else
			s_bDeviceDrain = false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool DeviceJobAlive(HAM_DeviceJob job)
	{
		if (!job)
			return false;

		if (job.m_Zone)
			return job.m_Zone.m_bActive;

		return job.m_Line && job.m_Line.m_bActive;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool StepDevice(HAM_DeviceJob job)
	{
		if (job.m_iIedsLeft > 0)
		{
			job.m_iIedsLeft--;
			if (PlaceSingleIed(job))
				job.m_iIedPlaced++;

			return job.m_iIedsLeft > 0 || job.m_iMinesLeft > 0;
		}

		if (job.m_iMinesLeft > 0)
		{
			job.m_iMinesLeft--;
			if (PlaceSingleMine(job))
				job.m_iMinePlaced++;

			return job.m_iMinesLeft > 0;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PlaceSingleIed(HAM_DeviceJob job)
	{
		Ensure();
		float limitSq = (job.m_fRadius + 10) * (job.m_fRadius + 10);
		for (int attempt = 0; attempt < 6; attempt++)
		{
			vector sample = HAM_Util.RandomAround(job.m_vCenter, 0, job.m_fRadius);
			vector roadPoint;
			if (!HAM_Util.RoadPoint(sample, 80, roadPoint, job.m_fRoad))
				continue;

			if (vector.DistanceSqXZ(roadPoint, job.m_vCenter) > limitSq || TooClose(job.m_aDevices, roadPoint, 25) || HAM_Util.IsNearWater(roadPoint) || !HAM_Util.OutOfBuildings(roadPoint))
				continue;

			vector minePos = MineBeside(roadPoint);
			if (HAM_Util.IsNearWater(minePos) || !HAM_Util.OutOfBuildings(minePos))
				continue;

			IEntity cover = HAM_Util.Spawn(NextIedProp(), roadPoint, Math.RandomFloat(0, 360));
			if (!cover)
				cover = HAM_Util.Spawn(HAM_Units.IED_COVER, roadPoint, Math.RandomFloat(0, 360));

			if (!cover)
				continue;

			IEntity mine = SpawnArmed(HAM_Units.MINE_AT, minePos);
			if (!mine)
			{
				HAM_Util.Delete(cover);
				continue;
			}

			job.m_aDevices.Insert(mine);
			job.m_aProps.Insert(cover);
			s_aIedMines.Insert(mine);
			s_aIedProps.Insert(cover);
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PlaceSingleMine(HAM_DeviceJob job)
	{
		float minDist = job.m_fMin;
		float maxDist = job.m_fMax;
		if (job.m_bMines)
		{
			minDist = 0;
			maxDist = job.m_fRadius;
		}

		for (int attempt = 0; attempt < 8; attempt++)
		{
			vector pos = HAM_Util.RandomAround(job.m_vCenter, minDist, maxDist);
			if (HAM_Util.IsWater(pos) || TooClose(job.m_aDevices, pos, MINE_SPACING))
				continue;

			IEntity mine = SpawnArmed(HAM_Units.MINE_AP, pos);
			if (!mine)
				continue;

			LiftOnGround(mine);
			job.m_aDevices.Insert(mine);
			IEntity flag = PlaceMineFlagNear(pos);
			if (flag)
			{
				job.m_aProps.Insert(flag);
				job.m_iFlags++;
			}

			RememberAp(mine, flag);
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Sur les routes du rayon seulement, cachés sous un tas de pneus.
	//! Champ posé sur un point déjà validé. 25 m : l'écart déjà utilisé entre deux IED, pour rester sur ce point.
	static bool PlaceLooseMines(vector pos, notnull array<IEntity> devices, notnull array<IEntity> props)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_MapPoint probe = new HAM_MapPoint();
		probe.m_eRole = HAM_ERole.MINES;
		int count = settings.MinesOf(probe);
		if (count < 1)
			return false;

		int flags;
		return PlaceMines(pos, 0, 25, count, devices, props, flags) > 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool PlaceLooseIeds(vector pos, notnull array<IEntity> devices, notnull array<IEntity> props)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_MapPoint probe = new HAM_MapPoint();
		probe.m_eRole = HAM_ERole.IED;
		int count = settings.IedsOf(probe);
		if (count < 1)
			return false;

		return PlaceIeds(pos, 25, count, settings.m_fIedRoadMinWidth, devices, props) > 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static int PlaceIeds(vector center, float radius, int count, float minRoadWidth, notnull array<IEntity> devices, notnull array<IEntity> props)
	{
		int placed = 0;
		float limitSq = (radius + 10) * (radius + 10);
		for (int attempt = 0; attempt < count * 6 && placed < count; attempt++)
		{
			vector sample = HAM_Util.RandomAround(center, 0, radius);
			vector roadPoint;
			if (!HAM_Util.RoadPoint(sample, 80, roadPoint, minRoadWidth))
				continue;

			if (vector.DistanceSqXZ(roadPoint, center) > limitSq || TooClose(devices, roadPoint, 25) || HAM_Util.IsNearWater(roadPoint) || !HAM_Util.OutOfBuildings(roadPoint))
				continue;

			// Comme le Conflit : l'accessoire sur la route, la mine M15 à côté.
			vector minePos = MineBeside(roadPoint);
			if (HAM_Util.IsNearWater(minePos) || !HAM_Util.OutOfBuildings(minePos))
				continue;

			IEntity cover = HAM_Util.Spawn(NextIedProp(), roadPoint, Math.RandomFloat(0, 360));
			if (!cover)
				cover = HAM_Util.Spawn(HAM_Units.IED_COVER, roadPoint, Math.RandomFloat(0, 360));

			if (!cover)
				continue;

			IEntity mine = SpawnArmed(HAM_Units.MINE_AT, minePos);
			if (!mine)
			{
				HAM_Util.Delete(cover);
				continue;
			}

			devices.Insert(mine);
			props.Insert(cover);
			s_aIedMines.Insert(mine);
			s_aIedProps.Insert(cover);
			placed++;
		}

		return placed;
	}

	//------------------------------------------------------------------------------------------------
	protected static ResourceName NextIedProp()
	{
		HAM_Units.Init();
		if (!HAM_Units.s_aIedProps || HAM_Units.s_aIedProps.IsEmpty())
			return HAM_Units.IED_COVER;

		int propCount = HAM_Units.s_aIedProps.Count();
		int propIndex = s_iIedProp - (s_iIedProp / propCount) * propCount;
		ResourceName prop = HAM_Units.s_aIedProps[propIndex];
		s_iIedProp++;
		return prop;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector MineBeside(vector spot)
	{
		vector dir = Vector(Math.RandomFloat(0, 360), 0, 0).AnglesToVector();
		return HAM_Util.Ground(spot + dir * Math.RandomFloat(1.3, 2.6));
	}

	//------------------------------------------------------------------------------------------------
	//! Comme le Conflit : M14 espacées de 14 m, posées sur la surface touchée, un drapeau à 1,5–2 m de chacune.
	protected static int PlaceMines(vector center, float minDist, float maxDist, int count, notnull array<IEntity> devices, notnull array<IEntity> props, out int flags)
	{
		int placed = 0;
		flags = 0;
		array<vector> spots = {};
		for (int attempt = 0; attempt < count * 10 && placed < count; attempt++)
		{
			vector pos = HAM_Util.RandomAround(center, minDist, maxDist);
			if (HAM_Util.IsNearWater(pos) || !HAM_Util.OutOfBuildings(pos) || TooCloseToList(spots, pos, MINE_SPACING) || TooClose(devices, pos, 4))
				continue;

			IEntity mine = SpawnArmed(HAM_Units.MINE_AP, pos);
			if (!mine)
				continue;

			LiftOnGround(mine);
			devices.Insert(mine);
			spots.Insert(pos);
			placed++;

			IEntity flag = PlaceMineFlagNear(pos);
			if (flag)
			{
				props.Insert(flag);
				flags++;
			}

			RememberAp(mine, flag);
		}

		return placed;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TooCloseToList(notnull array<vector> spots, vector pos, float spacing)
	{
		float spacingSq = spacing * spacing;
		foreach (vector spot : spots)
		{
			if (vector.DistanceSqXZ(spot, pos) < spacingSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Drapeau debout, au sec, orientation aléatoire.
	protected static IEntity PlaceMineFlagNear(vector minePos)
	{
		for (int attempt = 0; attempt < 4; attempt++)
		{
			vector dir = Vector(Math.RandomFloat(0, 360), 0, 0).AnglesToVector();
			vector spot = HAM_Util.Ground(minePos + dir * Math.RandomFloat(1.5, 2.0));
			if (HAM_Util.IsWater(spot))
				continue;

			IEntity flag = HAM_Util.Spawn(HAM_Units.MINE_FLAG, spot);
			if (!flag)
				return null;

			vector fwd = Vector(Math.RandomFloat(0, 360), 0, 0).AnglesToVector();
			vector mat[4];
			mat[0] = Vector(fwd[2], 0, -fwd[0]);
			mat[1] = Vector(0, 1, 0);
			mat[2] = fwd;
			mat[3] = spot;
			flag.SetWorldTransform(mat);
			return flag;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Sur ce qui est vraiment au sol (route, rocher, dalle), pas sous la surface du terrain.
	protected static void LiftOnGround(IEntity mine)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world || !mine)
			return;

		vector pos = mine.GetOrigin();
		TraceParam trace = new TraceParam();
		trace.Flags = TraceFlags.WORLD | TraceFlags.ENTS;
		trace.Exclude = mine;
		trace.Start = pos + Vector(0, 3, 0);
		trace.End = pos - Vector(0, 4, 0);
		float hit = world.TraceMove(trace, null);
		vector at = pos;
		if (hit > 0.001 && hit < 0.999)
			at = trace.Start + (trace.End - trace.Start) * hit;
		else
			at[1] = world.GetSurfaceY(pos[0], pos[2]);

		at[1] = at[1] + 0.08;
		vector mat[4];
		mine.GetWorldTransform(mat);
		mat[3] = at;
		mine.SetWorldTransform(mat);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool TooClose(notnull array<IEntity> devices, vector pos, float spacing)
	{
		float spacingSq = spacing * spacing;
		foreach (IEntity device : devices)
		{
			if (device && vector.DistanceSqXZ(device.GetOrigin(), pos) < spacingSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! IED de route, sans l'accessoire posé sur la chaussée.
	static IEntity SpawnBareIed(vector pos)
	{
		Ensure();
		IEntity mine = SpawnArmed(HAM_Units.MINE_AT, pos);
		if (!mine)
			return null;

		s_aIedMines.Insert(mine);
		s_aIedProps.Insert(null);
		return mine;
	}

	//------------------------------------------------------------------------------------------------
	static void ForgetBareIed(IEntity mine)
	{
		if (!mine || !s_aIedMines)
			return;

		int index = s_aIedMines.Find(mine);
		if (index >= 0)
		{
			s_aIedMines.Remove(index);
			if (s_aIedProps && index < s_aIedProps.Count())
				s_aIedProps.Remove(index);
		}

		if (s_mDevices)
			s_mDevices.Remove(mine);
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnArmed(ResourceName prefab, vector pos)
	{
		IEntity mine = HAM_Util.Spawn(prefab, HAM_Util.Ground(pos) + Vector(0, 0.02, 0), Math.RandomFloat(0, 360));
		if (!mine)
			return null;

		Arm(mine);
		IEntity child = mine.GetChildren();
		while (child)
		{
			Arm(child);
			child = child.GetSibling();
		}

		s_mDevices.Set(mine, true);
		return mine;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Arm(IEntity ent)
	{
		SCR_PressureTriggerComponent trigger = SCR_PressureTriggerComponent.Cast(ent.FindComponent(SCR_PressureTriggerComponent));
		if (!trigger)
			return;

		trigger.ActivateTrigger();
		if (trigger.GetArmingTime() > 0.0)
			trigger.ResetTimeout();
	}

	//------------------------------------------------------------------------------------------------
	//! Méthode du Conflit : la M15 cachée ne sent pas un fantassin, le script la fait sauter.
	protected static void PulseIed()
	{
		if (!s_bPulse || !s_aIedMines)
			return;

		ForgetGoneIeds();
		if (s_aIedMines.IsEmpty())
		{
			s_bPulse = false;
			return;
		}

		GetGame().GetCallqueue().CallLater(PulseIed, IED_PULSE_MS, false);

		SamplePlayerMotion();
		array<vector> moving = {};
		foreach (int playerId, float speed : s_mPlayerSpeed)
		{
			if (speed > IED_TRIGGER_SPEED && s_mPlayerPos.Contains(playerId))
				moving.Insert(s_mPlayerPos.Get(playerId));
		}

		if (moving.IsEmpty())
			return;

		float radiusSq = IED_TRIGGER_RADIUS * IED_TRIGGER_RADIUS;
		array<IEntity> blasts = {};
		foreach (IEntity mine : s_aIedMines)
		{
			if (!mine || s_aIedHeld.Contains(mine))
				continue;

			vector minePos = mine.GetOrigin();
			foreach (vector pos : moving)
			{
				if (vector.DistanceSqXZ(pos, minePos) <= radiusSq)
				{
					blasts.Insert(mine);
					break;
				}
			}
		}

		foreach (IEntity blast : blasts)
			Detonate(blast);
	}

	//------------------------------------------------------------------------------------------------
	//! Mine partie (explosion, zone vidée) : son accessoire disparaît avec elle.
	//! Une mine détruite compte pour la question civile, même si personne ne l'a désamorcée.
	protected static void ForgetGoneIeds()
	{
		for (int i = s_aIedMines.Count() - 1; i >= 0; i--)
		{
			IEntity mine = s_aIedMines[i];
			if (mine && !HAM_Util.IsDead(mine))
				continue;

			if (mine)
				HAM_Intel.OnIedCleared(mine);

			if (i < s_aIedProps.Count())
				HAM_Util.Delete(s_aIedProps[i]);

			s_aIedMines.Remove(i);
			if (i < s_aIedProps.Count())
				s_aIedProps.Remove(i);
		}

		for (int j = s_aIedHeld.Count() - 1; j >= 0; j--)
		{
			if (!s_aIedHeld[j])
				s_aIedHeld.Remove(j);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void SamplePlayerMotion()
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		BaseWorld world = GetGame().GetWorld();
		if (!playerManager || !world)
			return;

		float now = world.GetWorldTime();
		float dt = now - s_fMotionAt;
		if (dt < 50.0)
			dt = IED_PULSE_MS;
		s_fMotionAt = now;

		s_mPlayerSpeed.Clear();
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);
		foreach (int playerId : playerIds)
		{
			IEntity player = playerManager.GetPlayerControlledEntity(playerId);
			if (!HAM_Util.IsPlayerBody(player))
			{
				s_mPlayerPos.Remove(playerId);
				continue;
			}

			vector pos = player.GetOrigin();
			float speed = 0.0;
			if (s_mPlayerPos.Contains(playerId))
				speed = vector.DistanceXZ(pos, s_mPlayerPos.Get(playerId)) / (dt / 1000.0);

			s_mPlayerPos.Set(playerId, pos);
			s_mPlayerSpeed.Set(playerId, speed);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aBlastHandled;

	//------------------------------------------------------------------------------------------------
	protected static bool BlastHandled(IEntity mine)
	{
		if (!mine)
			return true;

		if (!s_aBlastHandled)
			s_aBlastHandled = {};

		if (s_aBlastHandled.Contains(mine))
			return true;

		s_aBlastHandled.Insert(mine);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Detonate(IEntity mine)
	{
		SCR_PressureTriggerComponent trigger = SCR_PressureTriggerComponent.Cast(mine.FindComponent(SCR_PressureTriggerComponent));
		if (!trigger || !trigger.IsActivated())
			return;

		if (BlastHandled(mine))
			return;

		HAM_Intel.OnIedCleared(mine);
		vector blastAt = mine.GetOrigin();
		trigger.TriggerManuallyServer();
		if (Replication.IsServer() && Math.RandomFloat01() < 0.3)
			HAM_Garrison.IedAttract(blastAt);
	}

	//------------------------------------------------------------------------------------------------
	//! Mine ou IED déjà armé, détruit par une explosion. Une seule fois. Le désamorçage ne passe pas ici.
	static void OnDeviceDestroyed(IEntity mine)
	{
		if (!Replication.IsServer() || !mine || BlastHandled(mine))
			return;

		if (IsIed(mine))
		{
			HAM_Intel.OnIedCleared(mine);
			if (Math.RandomFloat01() < 0.3)
				HAM_Garrison.IedAttract(mine.GetOrigin());

			return;
		}

		if (IsApMine(mine))
			RetireApMine(mine);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsIed(IEntity mine)
	{
		return mine && s_aIedMines && s_aIedMines.Contains(mine);
	}

	//------------------------------------------------------------------------------------------------
	//! Un joueur désamorce : l'IED ne saute pas sous lui tant qu'il travaille.
	static void HoldDisarm(IEntity mine)
	{
		if (!IsIed(mine) || s_aIedHeld.Contains(mine))
			return;

		s_aIedHeld.Insert(mine);
	}

	//------------------------------------------------------------------------------------------------
	static void ReleaseDisarm(IEntity mine)
	{
		if (s_aIedHeld && mine)
			s_aIedHeld.RemoveItem(mine);
	}

	//------------------------------------------------------------------------------------------------
	//! IED du seul point où se tient le civil. Une mine sans détonateur n'est pas désignée.
	static bool CollectZoneIeds(vector pos, notnull array<IEntity> outMines)
	{
		if (!s_aIedMines)
			return false;

		HAM_Point point = HAM_Mission.NearestPoint(pos);
		if (!point)
			return false;

		float radius = point.Radius();
		foreach (IEntity mine : s_aIedMines)
		{
			if (!mine || !IsIed(mine))
				continue;

			if (vector.DistanceXZ(mine.GetOrigin(), point.Pos()) > radius)
				continue;

			outMines.Insert(mine);
		}

		return !outMines.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	//! Désamorçage réussi : 15 % de chances que l'IED saute quand même. Dans les deux cas, la mine compte comme traitée pour le civil.
	protected static ref array<IEntity> s_aRepPaid;

	//------------------------------------------------------------------------------------------------
	protected static bool RepPaid(IEntity mine)
	{
		if (!mine)
			return true;

		if (!s_aRepPaid)
			s_aRepPaid = {};

		if (s_aRepPaid.Contains(mine))
			return true;

		s_aRepPaid.Insert(mine);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void DropDisarmed(IEntity mine)
	{
		if (!Replication.IsServer() || !IsDevice(mine))
			return;

		ReleaseDisarm(mine);
		HAM_Intel.OnIedCleared(mine);
		if (!IsIed(mine))
		{
			if (IsApMine(mine))
			{
				RetireApMine(mine);
				if (!RepPaid(mine))
					HAM_Mission.AddGlobalRep(2.5, "mine AP désamorcée");
			}

			return;
		}

		if (Math.RandomFloat01() < IED_DISARM_BLAST_CHANCE)
		{
			Detonate(mine);
			return;
		}

		if (!RepPaid(mine))
			HAM_Mission.AddGlobalRep(5, "IED désamorcé");

		int index = s_aIedMines.Find(mine);
		HAM_Util.Delete(s_aIedProps[index]);
		s_aIedMines.Remove(index);
		s_aIedProps.Remove(index);
		s_mDevices.Remove(mine);
		HAM_Util.Delete(mine);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsSlowPlayer(IEntity other)
	{
		if (!other || !s_mPlayerSpeed)
			return false;

		int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(other.GetRootParent());
		if (playerId <= 0)
			return false;

		return s_mPlayerSpeed.Get(playerId) <= IED_TRIGGER_SPEED;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Clear(notnull array<IEntity> devices, notnull array<IEntity> props)
	{
		foreach (IEntity device : devices)
		{
			if (!device)
				continue;

			s_mDevices.Remove(device);
			HAM_Util.Delete(device);
		}

		foreach (IEntity prop : props)
		{
			HAM_Util.Delete(prop);
		}

		devices.Clear();
		props.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! La prise de la cellule ne retire pas les engins déjà posés, et n'en repose pas.
	static void OnCellComplete(HAM_Cell cell)
	{
	}

	//------------------------------------------------------------------------------------------------
	//! Le maître est repris par l'ennemi : les lignes d'engins peuvent se poser une fois de plus.
	static void OnMasterLost(HAM_Cell cell)
	{
		if (!cell || !s_aZones)
			return;

		foreach (HAM_DeviceZone zone : s_aZones)
		{
			if (!zone || zone.m_Cell != cell)
				continue;

			Clear(zone.m_aDevices, zone.m_aProps);
			zone.m_bActive = false;
			zone.m_bSpent = false;
			zone.m_bMinesPlaced = false;
			zone.m_bIedsLocked = false;
			if (s_aIedLockZones)
				s_aIedLockZones.RemoveItem(zone.Name());
		}

		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || line.m_Cell != cell)
				continue;

			if (line.m_Def.m_eRole != HAM_ERole.IED && line.m_Def.m_eRole != HAM_ERole.MINES)
				continue;

			Clear(line.m_aDevices, line.m_aProps);
			line.m_bActive = false;
			line.m_bSpawnedOnce = false;
			if (s_aIedLockLines)
				s_aIedLockLines.RemoveItem(line.m_Def.m_sName);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un engin H&M ne saute pas sous un personnage ou un véhicule du camp ennemi.
	static bool IsHarmlessContact(IEntity mine, IEntity other)
	{
		if (!IsDevice(mine) || !other)
			return false;

		if (IsIed(mine) && (s_aIedHeld.Contains(mine) || IsSlowPlayer(other)))
			return true;

		Vehicle vehicle = Vehicle.Cast(other.GetRootParent());
		if (vehicle)
		{
			SCR_BaseCompartmentManagerComponent compartments = SCR_BaseCompartmentManagerComponent.Cast(vehicle.FindComponent(SCR_BaseCompartmentManagerComponent));
			if (!compartments)
				return false;

			array<IEntity> occupants = {};
			compartments.GetOccupants(occupants);
			if (occupants.IsEmpty())
				return false;

			foreach (IEntity occupant : occupants)
			{
				if (!HAM_Units.IsEnemyFaction(HAM_Util.FactionKey(occupant)))
					return false;
			}

			return true;
		}

		string factionKey = HAM_Util.FactionKey(other.GetRootParent());
		if (factionKey == HAM_Units.CIVILIAN_FACTION)
			return true;

		return HAM_Units.IsEnemyFaction(factionKey);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PressureTriggerComponent
{
	override void EOnContact(IEntity owner, IEntity other, Contact contact)
	{
		if (HAM_Devices.IsHarmlessContact(owner, other))
			return;

		super.EOnContact(owner, other, contact);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_DisarmMineAction
{
	override event void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_Devices.HoldDisarm(pOwnerEntity);
		super.PerformAction(pOwnerEntity, pUserEntity);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CharacterControllerComponent
{
	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (m_OnItemUseFinishedInvoker)
			m_OnItemUseFinishedInvoker.Insert(HAM_OnIedDisarmed);
		if (m_OnItemUseEndedInvoker)
			m_OnItemUseEndedInvoker.Insert(HAM_OnIedUseEnded);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity HAM_UsedMine(IEntity item, ItemUseParameters animParams)
	{
		IEntity mine = item;
		if (!mine && animParams)
			mine = animParams.GetEntity();
		if (!mine || !mine.FindComponent(SCR_PressureTriggerComponent))
			return null;

		return mine;
	}

	//------------------------------------------------------------------------------------------------
	void HAM_OnIedUseEnded(IEntity item, bool successful, ItemUseParameters animParams)
	{
		if (successful)
			return;

		IEntity mine = HAM_UsedMine(item, animParams);
		if (!mine)
			return;

		if (Replication.IsServer())
			HAM_Devices.ReleaseDisarm(mine);
		else
			Rpc(HAM_RpcReleaseIed, HAM_Util.IdOf(mine));
	}

	//------------------------------------------------------------------------------------------------
	void HAM_OnIedDisarmed(IEntity item, bool successful, ItemUseParameters animParams)
	{
		if (!successful)
			return;

		IEntity mine = HAM_UsedMine(item, animParams);
		if (!mine)
			return;

		if (Replication.IsServer())
			HAM_Devices.DropDisarmed(mine);
		else
			Rpc(HAM_RpcDropIed, HAM_Util.IdOf(mine));
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcReleaseIed(RplId id)
	{
		IEntity mine = HAM_Util.EntityOf(id);
		IEntity owner = GetOwner();
		if (!mine || !owner || vector.Distance(mine.GetOrigin(), owner.GetOrigin()) > 8.0)
			return;

		HAM_Devices.ReleaseDisarm(mine);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcDropIed(RplId id)
	{
		IEntity mine = HAM_Util.EntityOf(id);
		IEntity owner = GetOwner();
		if (!mine || !owner || vector.Distance(mine.GetOrigin(), owner.GetOrigin()) > 8.0)
			return;

		HAM_Devices.DropDisarmed(mine);
	}
}

//------------------------------------------------------------------------------------------------
// Grenade et explosif (EDamageType.EXPLOSIVE, FRAGMENTATION, PROCESSED_FRAGMENTATION).
// Pas les balles (KINETIC). La destruction passe par le SCR_MineDamageManager vanilla :
// une seule détonation via OnDamageStateChanged. Serveur seulement.
modded class SCR_MineDamageManager
{
	protected bool m_bHAM_BlastArmed;

	override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
	{
		if (super.HijackDamageHandling(damageContext))
			return true;

		if (!Replication.IsServer() || m_bHAM_BlastArmed)
			return false;

		EDamageType kind = damageContext.damageType;
		bool blast = kind == EDamageType.EXPLOSIVE || kind == EDamageType.FRAGMENTATION || kind == EDamageType.PROCESSED_FRAGMENTATION;
		if (!blast)
			return false;

		SCR_PressureTriggerComponent trigger = SCR_PressureTriggerComponent.Cast(GetOwner().FindComponent(SCR_PressureTriggerComponent));
		if (!trigger || !trigger.IsActivated())
			return false;

		m_bHAM_BlastArmed = true;
		damageContext.damageValue = 1000;
		return false;
	}

	override event protected void OnDamageStateChanged(EDamageState newState, EDamageState previousDamageState, bool isJIP)
	{
		super.OnDamageStateChanged(newState, previousDamageState, isJIP);
		if (newState != EDamageState.DESTROYED || !Replication.IsServer())
			return;

		HAM_Devices.OnDeviceDestroyed(GetOwner());
	}
}
