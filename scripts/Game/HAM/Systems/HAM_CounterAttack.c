//------------------------------------------------------------------------------------------------
// Contre-attaque : une vague part d'une zone encore ennemie (maître, esclave, dépôt vanilla ou
// dépôt perso) collée à une zone joueur, et vise la zone joueur la plus proche dont le délai
// après prise est écoulé. Même cellule ou cellules voisines : c'est le voisinage. Jamais depuis
// une zone déjà joueur, jamais de l'arrière (une zone ennemie sans voisine joueur ne part pas).
// Sans pool ennemi, pas de vague.
//------------------------------------------------------------------------------------------------

class HAM_CounterWave
{
	string m_sFrom;
	string m_sTarget;
	float m_fStarted;
	bool m_bDefending;
	ref array<SCR_AIGroup> m_aGroups = {};
	ref array<Vehicle> m_aVehicles = {};
}

class HAM_CounterAttack
{
	static const float WAVE_TIMEOUT = 2700;
	static const float INFANTRY_SCATTER = 60;
	//! Vitesse des véhicules de la contre-attaque, en km/h. SetCruiseSpeed attend des km/h.
	static const float MARCH_KMH = 30;

	protected static ref array<ref HAM_CounterWave> s_aWaves;
	protected static float s_fNextLeft = -1;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aWaves = null;
		s_fNextLeft = -1;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aWaves)
			s_aWaves = {};
	}

	//------------------------------------------------------------------------------------------------
	static bool EnemyNear(vector pos, float radius)
	{
		Ensure();
		foreach (HAM_CounterWave wave : s_aWaves)
		{
			if (HAM_Util.AnyAliveNear(wave.m_aGroups, pos, radius))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void SlowTick(float dt)
	{
		Ensure();
		CleanWaves();

		HAM_Settings settings = HAM_Settings.Get();
		if (!settings.m_bCounterEnabled)
			return;

		if (s_fNextLeft < 0)
			s_fNextLeft = settings.m_fCounterMinutes * 60;

		s_fNextLeft -= dt;
		if (s_fNextLeft > 0)
			return;

		s_fNextLeft = settings.m_fCounterMinutes * 60;
		if (Math.RandomFloat01() > settings.m_fCounterChance)
			return;

		Launch(settings);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Launch(HAM_Settings settings)
	{
		if (HAM_Units.EnemyCounterGroup().IsEmpty() && HAM_Units.EnemyCounterVehicle().IsEmpty())
			return;

		HAM_Point origin;
		HAM_Point target;
		if (!PickPair(settings, origin, target))
		{
			HAM_Util.Debug("Contre-attaque : aucune zone ennemie voisine d'une zone joueur dont le délai de prise est écoulé");
			return;
		}

		HAM_CounterWave wave = new HAM_CounterWave();
		wave.m_sFrom = origin.m_Def.m_sName;
		wave.m_sTarget = target.m_Def.m_sName;
		wave.m_fStarted = HAM_Util.Now();

		vector targetPos = target.Pos();
		vector start;
		if (!Staging(origin, target, start))
		{
			Print(string.Format("[13RDPA H&M] Contre-attaque %1 annulée : pas de départ à %2 m des bases et %3 m des joueurs", origin.m_Def.m_sName, Math.Round(settings.m_fCounterMinFromBase), Math.Round(settings.m_fCounterMinFromPlayer)), LogLevel.NORMAL);
			return;
		}

		for (int g = 0; g < settings.m_iCounterInfantryGroups; g++)
		{
			ResourceName groupPrefab = HAM_Units.EnemyCounterGroup();
			if (groupPrefab.IsEmpty())
				break;

			SCR_AIGroup group = HAM_Util.SpawnFullGroup(HAM_Units.GROUP_LIGHT, SpawnSpot(start), 0);
			if (!group)
				continue;

			March(group);
			HAM_Util.Order(group, HAM_Units.WP_SEARCH_DESTROY, targetPos, target.Radius());
			HAM_Util.Order(group, HAM_Units.WP_DEFEND, targetPos, target.Radius() * 0.5);
			wave.m_aGroups.Insert(group);
			HAM_Garrison.RegisterEnemy(group);
		}

		for (int v = 0; v < settings.m_iCounterVehicles; v++)
		{
			ResourceName vehiclePrefab = HAM_Units.EnemyCounterVehicle();
			if (vehiclePrefab.IsEmpty())
				break;

			vector roadStart = RoadNear(start);
			vector roadEnd;
			if (!HAM_Util.RoadPoint(targetPos, 300, roadEnd))
				roadEnd = targetPos;

			array<vector> route = {roadEnd};
			vector heading = roadEnd - roadStart;
			Vehicle vehicle = HAM_Garrison.SpawnCrewedVehicle(vehiclePrefab, roadStart, heading.ToYaw(), false, route, HAM_Units.WP_SEARCH_DESTROY, 60, wave.m_aGroups, true, ResourceName.Empty);
			if (vehicle)
			{
				vehicle.HAM_SetCruiseSpeedOverride(MARCH_KMH);
				wave.m_aVehicles.Insert(vehicle);
			}
		}

		if (wave.m_aGroups.IsEmpty() && wave.m_aVehicles.IsEmpty())
			return;

		s_aWaves.Insert(wave);
		HAM_Mission.Notify("Contre-attaque", string.Format("Contre-attaque ennemie depuis %1 vers %2 !", origin.m_Def.m_sName, target.m_Def.m_sName), 12);
		Print(string.Format("[13RDPA H&M] Contre-attaque %1 -> %2", origin.m_Def.m_sName, target.m_Def.m_sName), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Zone ennemie (maître, esclave ou dépôt) + zone joueur voisine la plus proche, délai de prise écoulé.
	protected static bool PickPair(HAM_Settings settings, out HAM_Point origin, out HAM_Point target)
	{
		origin = null;
		target = null;
		float now = HAM_Util.Now();
		float minDelay = settings.m_fCounterDelayAfterCaptureMinutes * 60;
		array<HAM_Point> origins = {};
		if (!HAM_Mission.s_aCells)
			return false;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell)
				continue;

			if (cell.m_Master && !cell.m_Master.m_bPlayer)
				origins.Insert(cell.m_Master);

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (slave && !slave.m_bPlayer)
					origins.Insert(slave);
			}
		}

		while (!origins.IsEmpty())
		{
			int index = origins.GetRandomIndex();
			HAM_Point candidate = origins[index];
			origins.Remove(index);
			if (!candidate || candidate.m_bPlayer)
				continue;

			HAM_Point best;
			float bestSq = float.MAX;
			foreach (HAM_Cell cell : HAM_Mission.s_aCells)
			{
				if (!cell)
					continue;

				ConsiderTarget(candidate, cell.m_Master, now, minDelay, best, bestSq);
				foreach (HAM_Point slave : cell.m_aSlaves)
					ConsiderTarget(candidate, slave, now, minDelay, best, bestSq);
			}

			if (best)
			{
				origin = candidate;
				target = best;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ConsiderTarget(HAM_Point origin, HAM_Point candidate, float now, float minDelay, inout HAM_Point best, inout float bestSq)
	{
		if (!candidate || !candidate.m_bPlayer || !ZonesTouch(origin, candidate))
			return;

		if (!CaptureDelayOver(candidate, now, minDelay))
			return;

		float distSq = vector.DistanceSqXZ(candidate.Pos(), origin.Pos());
		if (distSq >= bestSq)
			return;

		bestSq = distSq;
		best = candidate;
	}

	//------------------------------------------------------------------------------------------------
	//! Même cellule, ou cellules citées comme voisines. Pas soi-même.
	protected static bool ZonesTouch(HAM_Point a, HAM_Point b)
	{
		if (!a || !b || !a.m_Cell || !b.m_Cell || !a.m_Def || !b.m_Def)
			return false;

		if (a.m_Cell == b.m_Cell && a.m_Def.m_sName == b.m_Def.m_sName)
			return false;

		if (a.m_Cell == b.m_Cell)
			return true;

		return a.m_Cell.m_aNeighbors.Find(b.m_Cell.m_sName) >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! -1 : prise non datée, le délai ne bloque pas. Toute autre valeur, même négative après un
	//! chargement (le temps du monde est reparti à zéro), est un écart réel : il finit par passer.
	protected static bool CaptureDelayOver(HAM_Point point, float now, float minDelay)
	{
		bool dated = false;
		float capturedAt = -1;
		if (point.m_fCapturedAt != -1)
		{
			capturedAt = point.m_fCapturedAt;
			dated = true;
		}
		else if (point.IsMaster() && point.m_Cell && point.m_Cell.m_fCapturedAt != -1)
		{
			capturedAt = point.m_Cell.m_fCapturedAt;
			dated = true;
		}

		if (!dated)
			return true;

		return now - capturedAt >= minDelay;
	}

	//------------------------------------------------------------------------------------------------
	//! Sur l'axe ennemi, à au moins 800 m de chaque base et 600 m de chaque joueur.
	protected static bool Staging(HAM_Point origin, HAM_Point target, out vector start)
	{
		vector from = origin.Pos();
		vector to = target.Pos();
		vector away = from - to;
		away[1] = 0;
		float length = away.Length();
		if (length < 1)
			away = Vector(0, 0, 1);
		else
			away.Normalize();

		float dist = length;
		float minimum = HAM_Settings.Get().m_fCounterMinFromBase + INFANTRY_SCATTER;
		if (dist < minimum)
			dist = minimum;

		for (int step = 0; step < 8; step++)
		{
			vector candidate = HAM_Util.Ground(to + away * (dist + step * 150));
			if (HAM_Util.IsWater(candidate))
				continue;

			if (!SpawnAllowed(candidate))
				continue;

			start = candidate;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector SpawnSpot(vector start)
	{
		for (int attempt = 0; attempt < 6; attempt++)
		{
			vector spot = HAM_Util.DryAround(start, 0, INFANTRY_SCATTER);
			if (SpawnAllowed(spot))
				return spot;
		}

		return start;
	}

	//------------------------------------------------------------------------------------------------
	//! Route de départ proche de l'infanterie, sous les mêmes distances.
	protected static vector RoadNear(vector start)
	{
		for (int attempt = 0; attempt < 6; attempt++)
		{
			vector sample = HAM_Util.RandomAround(start, 0, 120);
			vector road;
			if (!HAM_Util.RoadPoint(sample, 80, road))
				continue;

			if (!SpawnAllowed(road))
				continue;

			return road;
		}

		return start;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpawnAllowed(vector pos)
	{
		if (!ClearOfBases(pos))
			return false;

		if (HAM_Mission.s_aPlayerBodies && HAM_Util.AnyNear(HAM_Mission.s_aPlayerBodies, pos, HAM_Settings.Get().m_fCounterMinFromPlayer))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ClearOfBases(vector pos)
	{
		float minimum = HAM_Settings.Get().m_fCounterMinFromBase;
		if (HAM_Mission.s_HqPoint && vector.DistanceXZ(pos, HAM_Mission.s_HqPoint.m_vPos) < minimum)
			return false;

		if (HAM_Mission.s_HqBase && HAM_Mission.s_HqBase.GetOwner() && vector.DistanceXZ(pos, HAM_Mission.s_HqBase.GetOwner().GetOrigin()) < minimum)
			return false;

		if (!HAM_Mission.s_aCells)
			return true;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell)
				continue;

			if (cell.m_Master && vector.DistanceXZ(pos, cell.m_Master.Pos()) < minimum)
				return false;

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (!slave)
					continue;

				if (vector.DistanceXZ(pos, slave.Pos()) < minimum)
					return false;
			}

			foreach (HAM_Deposit deposit : cell.m_aDeposits)
			{
				if (!deposit)
					continue;

				if (vector.DistanceXZ(pos, deposit.Pos()) < minimum)
					return false;
			}
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void March(SCR_AIGroup group)
	{
		if (!group)
			return;

		AIGroupMovementComponent movement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (movement)
			movement.SetGroupCharactersWantedMovementType(EMovementType.WALK);

		SCR_AIGroupSettingsComponent settings = SCR_AIGroupSettingsComponent.Cast(group.FindComponent(SCR_AIGroupSettingsComponent));
		if (settings)
			settings.AddSetting(SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.SCENARIO, EMovementType.WALK), false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ReleaseMarch(SCR_AIGroup group)
	{
		if (!group)
			return;

		AIGroupMovementComponent movement = AIGroupMovementComponent.Cast(group.FindComponent(AIGroupMovementComponent));
		if (movement)
			movement.SetGroupCharactersWantedMovementType(EMovementType.RUN);

		SCR_AIGroupSettingsComponent settings = SCR_AIGroupSettingsComponent.Cast(group.FindComponent(SCR_AIGroupSettingsComponent));
		if (settings)
			settings.AddSetting(SCR_AIGroupCharactersMovementSpeedSetting.Create(SCR_EAISettingOrigin.SCENARIO, EMovementType.RUN), false, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Arrived(HAM_CounterWave wave)
	{
		HAM_Point target = HAM_Mission.FindPoint(wave.m_sTarget);
		if (!target)
			return false;

		float radius = target.Radius();
		vector pos = target.Pos();
		if (HAM_Util.AnyAliveNear(wave.m_aGroups, pos, radius))
			return true;

		foreach (Vehicle vehicle : wave.m_aVehicles)
		{
			if (vehicle && !HAM_Util.IsDead(vehicle) && vector.DistanceXZ(vehicle.GetOrigin(), pos) <= radius)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CleanWaves()
	{
		float now = HAM_Util.Now();
		for (int i = s_aWaves.Count() - 1; i >= 0; i--)
		{
			HAM_CounterWave wave = s_aWaves[i];
			HAM_Util.PruneGroups(wave.m_aGroups);
			if (!wave.m_bDefending && Arrived(wave))
			{
				wave.m_bDefending = true;
				foreach (SCR_AIGroup marching : wave.m_aGroups)
					ReleaseMarch(marching);

				foreach (Vehicle marchingVehicle : wave.m_aVehicles)
				{
					if (marchingVehicle)
						marchingVehicle.HAM_SetCruiseSpeedOverride(MARCH_KMH);
				}
			}
			bool vehiclesAlive = false;
			foreach (Vehicle vehicle : wave.m_aVehicles)
			{
				if (vehicle && !HAM_Util.IsDead(vehicle))
				{
					vehiclesAlive = true;
					break;
				}
			}

			bool pending = false;
			foreach (Vehicle crewVehicle : wave.m_aVehicles)
			{
				if (crewVehicle && HAM_Garrison.IsCrewPending(crewVehicle))
					pending = true;
			}

			bool beaten = wave.m_aGroups.IsEmpty() && !pending;
			vector watch = HAM_Mission.BasePos(wave.m_sTarget);
			HAM_Point watched = HAM_Mission.FindPoint(wave.m_sTarget);
			if (watched)
				watch = watched.Pos();

			bool expired = now - wave.m_fStarted > WAVE_TIMEOUT && !HAM_Util.AnyNear(HAM_Mission.s_aPlayerBodies, watch, HAM_Settings.Get().m_fDespawnDistance);
			if (!beaten && !expired)
				continue;

			if (beaten)
				HAM_Mission.Notify("Contre-attaque", string.Format("La contre-attaque sur %1 est repoussée.", wave.m_sTarget), 8);

			HAM_Util.DeleteGroups(wave.m_aGroups);
			if (!vehiclesAlive || expired)
			{
				foreach (Vehicle leftover : wave.m_aVehicles)
				{
					if (leftover && !HAM_Util.IsDead(leftover))
						HAM_Util.Delete(leftover);
				}
			}

			s_aWaves.Remove(i);
		}
	}
}
