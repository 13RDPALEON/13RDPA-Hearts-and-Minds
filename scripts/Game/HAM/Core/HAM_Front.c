//------------------------------------------------------------------------------------------------
// Ligne de front : révélation, sanction derrière le front, prise maître puis esclaves,
// cellule entière, reprise ennemie de n'importe quelle zone tenue, fin de partie.
//------------------------------------------------------------------------------------------------

class HAM_Front
{
	protected static ref map<int, float> s_mForbiddenSince;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_mForbiddenSince = null;
	}

	//------------------------------------------------------------------------------------------------
	static void Tick(float dt)
	{
		array<int> ids = HAM_Mission.s_aPlayerIds;
		array<IEntity> bodies = HAM_Mission.s_aPlayerBodies;
		if (!s_mForbiddenSince)
			s_mForbiddenSince = new map<int, float>();

		for (int i = 0; i < ids.Count(); i++)
		{
			IEntity body = bodies[i];
			if (!body)
				continue;

			CheckPlayer(ids[i], body, HAM_Mission.CellAt(body.GetOrigin()));
		}

		foreach (HAM_Point point : HAM_Mission.s_aPoints)
		{
			if (point.m_bPlayer)
				UpdateRetake(point, dt, bodies);
			else
				UpdateCapture(point, dt, bodies);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void CheckPlayer(int playerId, IEntity body, HAM_Cell cell)
	{
		bool forbidden = cell && !cell.IsMasterPlayer() && !HAM_Mission.IsFront(cell);
		if (!forbidden)
		{
			s_mForbiddenSince.Remove(playerId);
			return;
		}

		HAM_Settings settings = HAM_Settings.Get();
		if (!settings.m_bSanctionEnabled || IsExempt(playerId))
			return;

		float now = HAM_Util.Now();
		float since;
		if (!s_mForbiddenSince.Find(playerId, since))
		{
			s_mForbiddenSince.Set(playerId, now);
			int grace = Math.Round(settings.m_fSanctionGraceSeconds);
			HAM_Net.NotifyPlayer(playerId, "Ligne de front", string.Format("Tu es derrière la ligne de front. Fais demi-tour : retour forcé au QG dans %1 s.", grace), 8);
			return;
		}

		if (now - since < settings.m_fSanctionGraceSeconds)
			return;

		s_mForbiddenSince.Remove(playerId);
		Sanction(playerId);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsExempt(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;

		return players.HasPlayerRole(playerId, EPlayerRole.GAME_MASTER);
	}

	//------------------------------------------------------------------------------------------------
	protected static void Sanction(int playerId)
	{
		HAM_Settings settings = HAM_Settings.Get();
		vector spot = HAM_Util.FreeSpot(HAM_Mission.DesignatedHqPos() + Vector(0, 0, 12), 25);
		HAM_Net.TeleportPlayer(playerId, spot);
		HAM_Mission.AddGlobalRep(-settings.m_iRepFrontlinePenalty, "passage du front");

		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		HAM_Mission.Notify("Ligne de front", name + " est passé derrière la ligne de front : renvoyé au QG, la réputation baisse.", 8);
	}

	//------------------------------------------------------------------------------------------------
	static void Reveal(HAM_Cell cell, string reason)
	{
		if (!cell || cell.m_bRevealed)
			return;

		if (!HAM_Mission.IsFront(cell))
		{
			Print(string.Format("[13RDPA H&M] Cellule %1 non révélée : elle n'est pas sur la ligne de front (%2)", cell.m_sName, reason), LogLevel.NORMAL);
			return;
		}

		cell.m_bRevealed = true;
		HAM_Mission.MarkDirty();
		HAM_Mission.Notify("Renseignement", string.Format("Zone découverte : %1 (%2).", cell.m_sName, cell.TypeLabel()), 8);
		Print(string.Format("[13RDPA H&M] Cellule %1 révélée : %2", cell.m_sName, reason), LogLevel.NORMAL);
		HAM_Intel.OnCellRevealed(cell);
	}

	//------------------------------------------------------------------------------------------------
	//! Révèle une cellule encore cachée de la ligne de front, la plus proche de near si donné.
	static HAM_Cell RevealFrontCell(string reason, vector near, bool useNear)
	{
		HAM_Cell best;
		float bestSq = float.MAX;
		array<HAM_Cell> candidates = {};
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (cell.m_bRevealed || !HAM_Mission.IsFront(cell))
				continue;

			candidates.Insert(cell);
			if (!useNear)
				continue;

			float distSq = vector.DistanceSqXZ(cell.Pos(), near);
			if (distSq < bestSq)
			{
				bestSq = distSq;
				best = cell;
			}
		}

		if (candidates.IsEmpty())
			return null;

		if (!best)
			best = candidates.GetRandomElement();

		Reveal(best, reason);
		return best;
	}

	//------------------------------------------------------------------------------------------------
	static void ShowCaptureBar(array<IEntity> bodies, HAM_Point point, float seconds)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !point)
			return;

		if (!point.m_aBarPlayers)
			point.m_aBarPlayers = {};
		else
			point.m_aBarPlayers.Clear();

		foreach (IEntity body : bodies)
		{
			if (!body || vector.DistanceXZ(body.GetOrigin(), point.Pos()) > point.Radius())
				continue;

			int playerId = players.GetPlayerIdFromControlledEntity(body);
			SCR_PlayerController controller = HAM_Net.ControllerOf(playerId);
			if (!controller)
				continue;

			controller.HAM_SendCaptureBar(point.m_Def.m_sName, seconds);
			if (point.m_aBarPlayers.Find(playerId) < 0)
				point.m_aBarPlayers.Insert(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void HideCaptureBar(array<IEntity> bodies, HAM_Point point)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !point)
			return;

		HideCaptureBarLeavers(bodies, point);

		float radius = point.Radius();
		foreach (IEntity body : bodies)
		{
			if (!body || vector.DistanceXZ(body.GetOrigin(), point.Pos()) > radius)
				continue;

			int playerId = players.GetPlayerIdFromControlledEntity(body);
			SCR_PlayerController controller = HAM_Net.ControllerOf(playerId);
			if (controller)
				controller.HAM_SendCaptureBar(string.Empty, 0);
		}

		if (point.m_aBarPlayers)
			point.m_aBarPlayers.Clear();
	}

	//------------------------------------------------------------------------------------------------
	//! Celui qui était dans le rayon au tick précédent et qui en sort reçoit le masquage.
	protected static void HideCaptureBarLeavers(array<IEntity> bodies, HAM_Point point)
	{
		if (!point || !point.m_aBarPlayers)
			return;

		array<int> outside = {};
		foreach (int playerId : point.m_aBarPlayers)
		{
			if (PlayerStillInside(bodies, point, playerId))
				continue;

			outside.Insert(playerId);
		}

		foreach (int playerId : outside)
		{
			SCR_PlayerController controller = HAM_Net.ControllerOf(playerId);
			if (controller)
				controller.HAM_SendCaptureBar(string.Empty, 0);

			point.m_aBarPlayers.RemoveItem(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Joueur arrivé en cours, ou entré dans la zone après le début : il n'a pas reçu la barre.
	protected static void ShowCaptureBarNewcomers(array<IEntity> bodies, HAM_Point point, float seconds)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !point || !bodies)
			return;

		if (!point.m_aBarPlayers)
			point.m_aBarPlayers = {};

		float radius = point.Radius();
		foreach (IEntity body : bodies)
		{
			if (!body || vector.DistanceXZ(body.GetOrigin(), point.Pos()) > radius)
				continue;

			int playerId = players.GetPlayerIdFromControlledEntity(body);
			if (playerId <= 0 || point.m_aBarPlayers.Find(playerId) >= 0)
				continue;

			SCR_PlayerController controller = HAM_Net.ControllerOf(playerId);
			if (!controller)
				continue;

			controller.HAM_SendCaptureBar(point.m_Def.m_sName, seconds);
			point.m_aBarPlayers.Insert(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PlayerStillInside(array<IEntity> bodies, HAM_Point point, int playerId)
	{
		if (!bodies || !point)
			return false;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return false;

		float radius = point.Radius();
		vector pos = point.Pos();
		foreach (IEntity body : bodies)
		{
			if (!body || players.GetPlayerIdFromControlledEntity(body) != playerId)
				continue;

			return vector.DistanceXZ(body.GetOrigin(), pos) <= radius;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void UpdateCapture(HAM_Point point, float dt, array<IEntity> bodies)
	{
		HAM_Cell cell = point.m_Cell;
		if (point.IsMaster() && HAM_MasterBases.UsesSeizing(cell))
			return;

		bool allowed = !HAM_MasterBases.CacheStanding(point) && cell.m_bRevealed && HAM_MasterBases.RadioLinked(cell) && (HAM_Mission.IsFront(cell) || cell.IsMasterPlayer());
		if (!allowed)
		{
			if (HAM_Util.AnyNear(bodies, point.Pos(), point.Radius()) && !HAM_Garrison.EnemyAliveNear(point.Pos(), point.Radius()))
				HAM_MasterBases.ExplainBlock(point);

			if (point.m_bCaptureBar)
			{
				point.m_bCaptureBar = false;
				HAM_Front.HideCaptureBar(bodies, point);
			}

			point.m_fCaptureProgress = 0;
			return;
		}

		float radius = point.Radius();
		if (!HAM_Util.AnyNear(bodies, point.Pos(), radius))
		{
			if (point.m_bCaptureBar)
			{
				point.m_bCaptureBar = false;
				HAM_Front.HideCaptureBar(bodies, point);
			}

			point.m_fCaptureProgress = Math.Max(0, point.m_fCaptureProgress - dt);
			return;
		}

		if (HAM_Garrison.EnemyAliveNear(point.Pos(), radius))
		{
			if (point.m_bCaptureBar)
			{
				point.m_bCaptureBar = false;
				HAM_Front.HideCaptureBar(bodies, point);
			}

			point.m_fCaptureProgress = 0;
			return;
		}

		if (!point.m_bCaptureBar)
		{
			point.m_bCaptureBar = true;
			float left = Math.Max(1, HAM_Settings.Get().m_fCaptureSeconds - point.m_fCaptureProgress);
			HAM_Front.ShowCaptureBar(bodies, point, left);
		}
		else
		{
			HideCaptureBarLeavers(bodies, point);
			float left = Math.Max(1, HAM_Settings.Get().m_fCaptureSeconds - point.m_fCaptureProgress);
			ShowCaptureBarNewcomers(bodies, point, left);
		}

		point.m_fCaptureProgress += dt;
		if (point.m_fCaptureProgress >= HAM_Settings.Get().m_fCaptureSeconds)
			Capture(point);
	}

	//------------------------------------------------------------------------------------------------
	//! Zone tenue sans joueur ni garnison amie, avec des ennemis dessus : elle repasse à l'ennemi.
	//! Maître, esclave, dépôt, camp, planque : le point compté est celui où ils se tiennent.
	protected static void UpdateRetake(HAM_Point point, float dt, array<IEntity> bodies)
	{
		float radius = point.Radius();
		bool enemy = HAM_Garrison.EnemyAliveNear(point.Pos(), radius);
		bool held = HAM_Util.AnyNear(bodies, point.Pos(), radius) || HAM_Garrison.FriendlyAliveNear(point.m_Cell, point.Pos(), radius);
		if (!enemy || held)
		{
			point.m_fRetakeProgress = 0;
			return;
		}

		if (point.m_fRetakeProgress == 0)
			HAM_Mission.Notify("Attaque", string.Format("%1 est attaqué et n'a plus de défenseurs !", point.m_Def.m_sName), 10);

		point.m_fRetakeProgress += dt;
		if (point.m_fRetakeProgress < HAM_Settings.Get().m_fRetakeSeconds)
			return;

		if (point.IsMaster())
			LoseMaster(point.m_Cell, "ennemis sur la zone");
		else
			LoseSlave(point, "ennemis sur la zone");
	}

	//------------------------------------------------------------------------------------------------
	static void Capture(HAM_Point point)
	{
		if (!point || point.m_bPlayer)
			return;

		HAM_Settings settings = HAM_Settings.Get();
		HAM_Cell cell = point.m_Cell;
		point.m_bPlayer = true;
		point.m_bCaptureBar = false;
		HAM_Front.HideCaptureBar(HAM_Mission.s_aPlayerBodies, point);
		point.m_fCaptureProgress = 0;
		point.m_fRetakeProgress = 0;
		point.m_iEnemyLeft = 0;
		point.m_fCapturedAt = HAM_Util.Now();
		HAM_Garrison.OnPointCaptured(point);

		if (point.IsMaster())
		{
			HAM_MasterBases.Adopt(cell, true);
			cell.m_bRevealed = true;
			cell.m_fCapturedAt = point.m_fCapturedAt;
			cell.m_Stock.Clear();
			cell.m_Stock.Add(HAM_EResource.WATER, 100);
			cell.m_Stock.Add(HAM_EResource.FOOD, 100);
			cell.m_Stock.Add(HAM_EResource.AMMO, 100);
			if (point.m_Def.m_eType == HAM_EMasterType.CAMP)
				cell.m_Stock.Add(HAM_EResource.MEN, HAM_MasterBases.CampMen());

			foreach (HAM_Point held : cell.m_aSlaves)
			{
				if (held.m_bPlayer && held.m_Def.m_eType == HAM_EMasterType.CAMP)
					cell.m_Stock.Add(HAM_EResource.MEN, HAM_MasterBases.CampMen());
			}

			cell.m_fGraceLeft = settings.m_fGraceMinutes * 60;
			cell.m_fCycleLeft = settings.m_fCycleMinutes * 60;
			int graceMinutes = Math.Round(settings.m_fGraceMinutes);
			string message = string.Format("Maître de %1 pris. Prenez ses esclaves et livrez eau, nourriture et munitions avant %2 min.", cell.m_sName, graceMinutes);
			if (cell.m_aSlaves.IsEmpty())
				message = string.Format("Maître de %1 pris. Livrez eau, nourriture et munitions avant %2 min.", cell.m_sName, graceMinutes);

			HAM_Mission.Notify("Zone prise", message, 12);
		}
		else
		{
			HAM_MasterBases.AdoptSlave(point, true);
			string taken = string.Format("%1 (%2) est aux joueurs.", point.m_Def.m_sName, cell.m_sName);
			if (point.m_Def.m_eType == HAM_EMasterType.CAMP)
			{
				cell.m_Stock.Add(HAM_EResource.MEN, HAM_MasterBases.CampMen());
				taken = string.Format("%1 (%2) est aux joueurs. La caserne verse %3 hommes.", point.m_Def.m_sName, cell.m_sName, HAM_MasterBases.CampMen());
			}

			HAM_Mission.Notify("Point pris", taken, 8);
		}

		if (point.IsMaster())
		{
			HAM_MasterBases.ShowDiscovered(cell);
			HAM_Mission.PourMasterSupplies(cell);
		}

		if (point.m_Def.m_eType == HAM_EMasterType.CAMP)
			HAM_MasterBases.PourBarracks(point);

		Print(string.Format("[13RDPA H&M] Point %1 pris (cellule %2)", point.m_Def.m_sName, cell.m_sName), LogLevel.NORMAL);
		HAM_Mission.AddLocalRep(cell, settings.m_iRepLocalCaptureBonus);
		HAM_Mission.RefreshZoneProps(cell);
		HAM_Mission.RefreshMatchScore();
		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();

		if (cell.IsComplete())
			OnCellComplete(cell);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OnCellComplete(HAM_Cell cell)
	{
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Mission.AddGlobalRep(settings.m_iRepCaptureBonus, "cellule prise");
		HAM_Garrison.OnCellComplete(cell);
		HAM_Devices.OnCellComplete(cell);
		HAM_Intel.OnCellComplete(cell);
		HAM_SideMissions.OnCellChanged(cell);
		HAM_Mission.RefreshZoneProps(cell);
		Print(string.Format("[13RDPA H&M] Cellule %1 entière aux joueurs", cell.m_sName), LogLevel.NORMAL);

		if (cell.m_bEnemyHq)
		{
			HAM_Mission.Victory();
			return;
		}

		HAM_Mission.Notify("Cellule prise", string.Format("%1 est entière aux joueurs : plus de renforts ennemis, la ligne de front avance.", cell.m_sName), 12);
		HAM_Mission.SaveNow();
	}

	//------------------------------------------------------------------------------------------------
	//! Le maître repasse à l'ennemi : ses esclaves et son dépôt avec lui, les renforts reprennent.
	static void LoseMaster(HAM_Cell cell, string reason)
	{
		if (!cell || !cell.IsMasterPlayer())
			return;

		cell.m_Master.m_bPlayer = false;
		cell.m_Master.m_iEnemyLeft = -1;
		cell.m_Master.m_fRetakeProgress = 0;
		foreach (HAM_Point slave : cell.m_aSlaves)
		{
			slave.m_bPlayer = false;
			slave.m_iEnemyLeft = -1;
			slave.m_fCaptureProgress = 0;
			slave.m_fCapturedAt = -1;
			HAM_MasterBases.AdoptSlave(slave, false);
		}

		cell.m_Master.m_fCapturedAt = -1;
		HAM_MasterBases.Adopt(cell, false);
		cell.m_Stock.Clear();
		cell.m_fGraceLeft = -1;
		cell.m_fCapturedAt = -1;
		cell.m_fReinforceLeft = HAM_Settings.Get().m_fReinforcementMinutes * 60;

		HAM_Logistics.ClearDemand(cell);
		HAM_Garrison.OnMasterLost(cell);
		HAM_HqDesignation.OnCellLost(cell);
		HAM_SideMissions.OnCellChanged(cell);
		HAM_Mission.RefreshZoneProps(cell);
		HAM_Mission.RefreshMatchScore();
		HAM_Mission.Notify("Zone perdue", string.Format("%1 est repassée à l'ennemi (%2).", cell.m_sName, reason), 12);
		Print(string.Format("[13RDPA H&M] Cellule %1 perdue : %2", cell.m_sName, reason), LogLevel.NORMAL);
		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();
	}

	//------------------------------------------------------------------------------------------------
	//! Un esclave, un dépôt ou un camp tenu repasse seul à l'ennemi. Le maître reste aux joueurs.
	static void LoseSlave(HAM_Point point, string reason)
	{
		if (!point || point.IsMaster() || !point.m_bPlayer)
			return;

		HAM_Cell cell = point.m_Cell;
		point.m_bPlayer = false;
		point.m_iEnemyLeft = -1;
		point.m_fRetakeProgress = 0;
		point.m_fCaptureProgress = 0;
		point.m_fCapturedAt = -1;
		HAM_MasterBases.AdoptSlave(point, false);
		if (cell && point.m_Def.m_eType == HAM_EMasterType.CAMP)
			cell.m_Stock.Add(HAM_EResource.MEN, -HAM_MasterBases.CampMen());

		HAM_Garrison.OnSlaveLost(point);
		HAM_SideMissions.OnCellChanged(cell);
		HAM_Mission.RefreshZoneProps(cell);
		HAM_Mission.RefreshMatchScore();
		HAM_Mission.Notify("Zone perdue", string.Format("%1 est repassé à l'ennemi (%2).", point.m_Def.m_sName, reason), 12);
		Print(string.Format("[13RDPA H&M] Point %1 perdu : %2", point.m_Def.m_sName, reason), LogLevel.NORMAL);
		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();
	}
}
