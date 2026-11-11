//------------------------------------------------------------------------------------------------
// Réseau H&M : notifications, état global et marqueurs envoyés aux clients, téléportation de
// sanction, requêtes du tableau. Les marqueurs sont locaux à chaque client : seules les cellules
// révélées y figurent, le QG toujours.
//------------------------------------------------------------------------------------------------

enum HAM_EMarkerKind
{
	HQ,
	MASTER,
	ENEMY_HQ,
	SLAVE,
	DEPOSIT,
	SUPPLY,
	PATROL,
	CQB,
	IED,
	MINES,
	CIVILIANS
}

//------------------------------------------------------------------------------------------------
//! Copie client de l'état utile aux actions et au tableau.
class HAM_ClientState
{
	static float s_fGlobalRep;
	static int s_iCivilianThreshold = 300;
	static int s_iMenThreshold = 4;
	static int s_iMenMax = 40;
	static float s_fWaterThreshold = 50;
	static float s_fFoodThreshold = 50;
	static float s_fAmmoThreshold = 50;
	static float s_fDepositWaterMax = 300;
	static float s_fDepositFoodMax = 300;
	static float s_fDepositAmmoMax = 300;
	static string s_sDesignatedHq = "QG";
	static bool s_bReceived;

	//------------------------------------------------------------------------------------------------
	static void ApplyGlobals(notnull array<int> ints, notnull array<float> floats, string designated)
	{
		if (ints.Count() >= 3)
		{
			s_fGlobalRep = ints[0];
			s_iCivilianThreshold = ints[1];
			s_iMenThreshold = ints[2];
		}

		if (ints.Count() >= 4)
			s_iMenMax = ints[3];

		if (floats.Count() >= 3)
		{
			s_fWaterThreshold = floats[0];
			s_fFoodThreshold = floats[1];
			s_fAmmoThreshold = floats[2];
		}

		if (floats.Count() >= 6)
		{
			s_fDepositWaterMax = floats[3];
			s_fDepositFoodMax = floats[4];
			s_fDepositAmmoMax = floats[5];
		}

		if (floats.Count() >= 7)
			s_fGlobalRep = floats[6];

		s_sDesignatedHq = designated;
		s_bReceived = true;
		HAM_ResourceBoardMenu.NoteReputation();
	}
}

//------------------------------------------------------------------------------------------------
//! Copie client des zones visibles. La carte du Conflit les dessine (HAM_MapZoneUI).
class HAM_ClientMarkers
{
	//! Par zone : eau, nourriture, munitions, hommes, secondes de grâce ; -1 = inconnu.
	static const int STATS_PER_ZONE = 5;
	//! Le QG et un QG désigné sont des bases Conflit posées sur la zone, à moins de cette distance.
	static const float CONFLICT_BASE_RADIUS = 150;

	protected static int s_iCount;
	protected static ref array<string> s_aLabels;
	protected static ref array<int> s_aKinds;
	protected static ref array<float> s_aXs;
	protected static ref array<float> s_aZs;
	protected static ref array<int> s_aFlags;
	protected static ref array<string> s_aSubtitles;
	protected static ref array<int> s_aStats;
	protected static float s_fStateAsk;

	//------------------------------------------------------------------------------------------------
	static void Release()
	{
		HAM_Callsigns.Release();
		s_iCount = 0;
		s_aLabels = null;
		s_aKinds = null;
		s_aXs = null;
		s_aZs = null;
		s_aFlags = null;
		s_aSubtitles = null;
		s_aStats = null;
	}

	//------------------------------------------------------------------------------------------------
	static void Rebuild(notnull array<string> labels, notnull array<int> kinds, notnull array<float> xs, notnull array<float> zs, notnull array<int> flags, notnull array<string> subtitles, notnull array<int> stats)
	{
		if (RplSession.Mode() == RplMode.Dedicated)
			return;

		s_aLabels = {};
		s_aLabels.Copy(labels);
		s_aKinds = {};
		s_aKinds.Copy(kinds);
		s_aXs = {};
		s_aXs.Copy(xs);
		s_aZs = {};
		s_aZs.Copy(zs);
		s_aFlags = {};
		s_aFlags.Copy(flags);
		s_aSubtitles = {};
		s_aSubtitles.Copy(subtitles);
		s_aStats = {};
		s_aStats.Copy(stats);

		s_iCount = Math.Min(labels.Count(), Math.Min(kinds.Count(), Math.Min(xs.Count(), Math.Min(zs.Count(), Math.Min(flags.Count(), subtitles.Count())))));
		s_iCount = Math.Min(s_iCount, stats.Count() / STATS_PER_ZONE);

		SCR_MapCampaignUI.HAM_RefreshOpenMap();
	}

	//------------------------------------------------------------------------------------------------
	static int Count()
	{
		return s_iCount;
	}

	//------------------------------------------------------------------------------------------------
	//! Le survol d'une base sans stats redemande l'état, au plus une fois toutes les 3 s.
	static void RequestIfEmpty()
	{
		if (s_iCount > 0)
			return;

		float now = HAM_Util.Now();
		if (now - s_fStateAsk < 3)
			return;

		s_fStateAsk = now;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_AskFrontState();
	}

	//------------------------------------------------------------------------------------------------
	static string Label(int index)
	{
		return s_aLabels[index];
	}

	//------------------------------------------------------------------------------------------------
	static string Subtitle(int index)
	{
		return s_aSubtitles[index];
	}

	//------------------------------------------------------------------------------------------------
	static HAM_EMarkerKind Kind(int index)
	{
		return s_aKinds[index];
	}

	//------------------------------------------------------------------------------------------------
	static vector Pos(int index)
	{
		return Vector(s_aXs[index], 0, s_aZs[index]);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsHeld(int index)
	{
		return (s_aFlags[index] & 1) != 0;
	}

	//------------------------------------------------------------------------------------------------
	//! -1 si la zone n'est pas un maître.
	static int MasterType(int index)
	{
		return (s_aFlags[index] >> 4) - 1;
	}

	//------------------------------------------------------------------------------------------------
	static int Stat(int index, int slot)
	{
		return s_aStats[index * STATS_PER_ZONE + slot];
	}

	//------------------------------------------------------------------------------------------------
	//! Le QG et les QG désignés ont déjà leur icône de base Conflit.
	static bool ShownByConflict(int index)
	{
		HAM_EMarkerKind kind = Kind(index);
		if (kind != HAM_EMarkerKind.HQ && kind != HAM_EMarkerKind.MASTER)
			return false;

		return HasCampaignBaseNear(Pos(index));
	}

	//------------------------------------------------------------------------------------------------
	//! Zone H&M posée sous une base Conflit du camp joueur, ou -1.
	static int ZoneAtBase(vector basePos)
	{
		// Même rayon que la recherche de base Conflit : un poste décalé reste rattaché.
		float limitSq = HAM_Mission.BASE_MATCH_RADIUS * HAM_Mission.BASE_MATCH_RADIUS;
		for (int i = 0; i < s_iCount; i++)
		{
			HAM_EMarkerKind kind = Kind(i);
			if (kind != HAM_EMarkerKind.HQ && kind != HAM_EMarkerKind.MASTER)
				continue;

			if (vector.DistanceSqXZ(Pos(i), basePos) <= limitSq)
				return i;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	static int ZoneByLabel(string name)
	{
		if (name.IsEmpty() || !s_aLabels)
			return -1;

		for (int i = 0; i < s_iCount; i++)
		{
			if (s_aLabels[i] == name)
				return i;
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasCampaignBaseNear(vector pos)
	{
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return false;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		float limitSq = CONFLICT_BASE_RADIUS * CONFLICT_BASE_RADIUS;
		foreach (SCR_MilitaryBaseComponent base : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(base);
			if (!campaignBase || !campaignBase.GetOwner())
				continue;

			if (vector.DistanceSqXZ(campaignBase.GetOwner().GetOrigin(), pos) <= limitSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasPlayerBaseNear(vector pos)
	{
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return false;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		float limitSq = CONFLICT_BASE_RADIUS * CONFLICT_BASE_RADIUS;
		foreach (SCR_MilitaryBaseComponent base : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(base);
			if (!campaignBase || !campaignBase.GetOwner() || !campaignBase.GetFaction())
				continue;

			if (campaignBase.GetFaction().GetFactionKey() != HAM_Units.PLAYER_FACTION)
				continue;

			if (vector.DistanceSqXZ(campaignBase.GetOwner().GetOrigin(), pos) <= limitSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Ligne des ressources de l'encadré, comme les fournitures d'une base Conflit.
	static string StockLine(int index)
	{
		int water = Stat(index, 0);
		int type = MasterType(index);
		if (type == HAM_EMasterType.SUPPLY_VANILLA && water >= 0)
			return string.Format("Fournitures %1/%2", water, Stat(index, 1));

		if (water < 0)
		{
			HAM_EMarkerKind hidden = Kind(index);
			if (hidden == HAM_EMarkerKind.PATROL)
				return "Patrouille";

			if (hidden == HAM_EMarkerKind.CQB)
				return "CQB";

			if (hidden == HAM_EMarkerKind.IED)
				return "IED";

			if (hidden == HAM_EMarkerKind.MINES)
				return "Mines";

			if (hidden == HAM_EMarkerKind.CIVILIANS)
				return "Civils";

			if (Kind(index) == HAM_EMarkerKind.SLAVE)
			{
				if (IsHeld(index))
					return "Point tenu";

				return "Point ennemi";
			}

			return "Ressources inconnues";
		}

		int food = Stat(index, 1);
		int ammo = Stat(index, 2);
		if (type == HAM_EMasterType.SUPPLY_PERSONAL || Kind(index) == HAM_EMarkerKind.DEPOSIT)
		{
			int waterMax = Math.Round(HAM_ClientState.s_fDepositWaterMax);
			int foodMax = Math.Round(HAM_ClientState.s_fDepositFoodMax);
			int ammoMax = Math.Round(HAM_ClientState.s_fDepositAmmoMax);
			return string.Format("Eau %1/%2 · Vivres %3/%4 · Munitions %5/%6", water, waterMax, food, foodMax, ammo, ammoMax);
		}

		int waterMin = Math.Round(HAM_ClientState.s_fWaterThreshold);
		int foodMin = Math.Round(HAM_ClientState.s_fFoodThreshold);
		int ammoMin = Math.Round(HAM_ClientState.s_fAmmoThreshold);
		return string.Format("Eau %1/%2 · Vivres %3/%4 · Munitions %5/%6", water, waterMin, food, foodMin, ammo, ammoMin);
	}

	//------------------------------------------------------------------------------------------------
	//! Hommes et délai de grâce ; vide quand ils ne s'appliquent pas.
	static string MenLine(int index)
	{
		int men = Stat(index, 3);
		if (men < 0)
			return string.Empty;

		string line = string.Format("Hommes %1/%2 · max %3", men, HAM_ClientState.s_iMenThreshold, HAM_ClientState.s_iMenMax);
		int grace = Stat(index, 4);
		if (grace >= 0)
		{
			int minutes = Math.Ceil(grace / 60.0);
			line = line + string.Format(" · Perte dans %1 min", minutes);
		}

		return line;
	}

	//------------------------------------------------------------------------------------------------
	//! Texte affiché au-dessus de l'icône : ressources et hommes, seulement quand les chiffres existent.
	static string InfoLine(int index)
	{
		if (!IsHeld(index))
			return string.Empty;

		HAM_EMarkerKind kind = Kind(index);
		if (kind != HAM_EMarkerKind.HQ && kind != HAM_EMarkerKind.MASTER && kind != HAM_EMarkerKind.ENEMY_HQ && kind != HAM_EMarkerKind.SLAVE && kind != HAM_EMarkerKind.DEPOSIT && kind != HAM_EMarkerKind.SUPPLY)
			return string.Empty;

		int water = Stat(index, 0);
		int men = Stat(index, 3);
		if (water < 0 && men < 0)
			return string.Empty;

		string line = string.Empty;
		if (water >= 0)
			line = StockLine(index);

		string menLine = MenLine(index);
		if (menLine.IsEmpty())
			return line;

		if (line.IsEmpty())
			return menLine;

		return line + "\n" + menLine;
	}
}

//------------------------------------------------------------------------------------------------
//! Côté serveur : construit et envoie l'état aux joueurs.
class HAM_Net
{
	protected static ref map<int, int> s_mSyncTries;

	//------------------------------------------------------------------------------------------------
	static SCR_PlayerController ControllerOf(int playerId)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return null;

		return SCR_PlayerController.Cast(players.GetPlayerController(playerId));
	}

	//------------------------------------------------------------------------------------------------
	static void NotifyPlayer(int playerId, string title, string message, int seconds)
	{
		if (playerId <= 0)
			return;

		SCR_PlayerController controller = ControllerOf(playerId);
		if (controller)
			controller.HAM_SendHint(title, message, seconds);
	}

	//------------------------------------------------------------------------------------------------
	static void NotifyAll(string title, string message, int seconds)
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			NotifyPlayer(playerId, title, message, seconds);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void TeleportPlayer(int playerId, vector spot)
	{
		SCR_PlayerController controller = ControllerOf(playerId);
		if (controller)
			controller.HAM_ForceTeleport(spot);
	}

	//------------------------------------------------------------------------------------------------
	static void BroadcastState()
	{
		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
		{
			SyncPlayer(playerId);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Renvoie l'état même si les essais précédents ont été abandonnés (carte ouverte, corps reçu).
	static void SyncPlayerNow(int playerId)
	{
		if (s_mSyncTries)
			s_mSyncTries.Set(playerId, 0);

		SyncPlayer(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static void SyncPlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0 || !HAM_Mission.IsActive())
			return;

		SCR_PlayerController controller = ControllerOf(playerId);
		if (!controller || !HAM_Mission.s_aCells)
		{
			int tries;
			if (s_mSyncTries)
				s_mSyncTries.Find(playerId, tries);

			if (tries >= 30)
				return;

			if (!s_mSyncTries)
				s_mSyncTries = new map<int, int>();

			s_mSyncTries.Set(playerId, tries + 1);
			GetGame().GetCallqueue().CallLater(SyncPlayer, 2000, false, playerId);
			return;
		}

		if (s_mSyncTries)
			s_mSyncTries.Set(playerId, 0);

		HAM_Settings settings = HAM_Settings.Get();
		array<int> ints = {Math.Round(HAM_Mission.s_fGlobalRep), settings.m_iRepCivilianTalkThreshold, settings.m_iMenThreshold, settings.m_iMenMax};
		array<float> floats = {settings.m_fWaterThreshold, settings.m_fFoodThreshold, settings.m_fAmmoThreshold, settings.m_fDepositWaterMax, settings.m_fDepositFoodMax, settings.m_fDepositAmmoMax, HAM_Mission.s_fGlobalRep};
		controller.HAM_SendGlobals(ints, floats, HAM_Mission.s_sDesignatedHq);

		array<string> labels = {};
		array<int> kinds = {};
		array<float> xs = {};
		array<float> zs = {};
		array<int> flags = {};
		array<string> subtitles = {};
		array<int> stats = {};
		BuildMarkers(labels, kinds, xs, zs, flags, subtitles, stats);
		controller.HAM_SendMarkers(labels, kinds, xs, zs, flags, subtitles, stats);
	}

	//------------------------------------------------------------------------------------------------
	//! flags : 1 = tenu par les joueurs, 2 = cellule complète, bits 4 et plus = type + 1 (maître ou esclave).
	protected static void BuildMarkers(notnull array<string> labels, notnull array<int> kinds, notnull array<float> xs, notnull array<float> zs, notnull array<int> flags, notnull array<string> subtitles, notnull array<int> stats)
	{
		vector hqPos = HAM_Mission.BasePos(HAM_Mission.HQ_NAME);
		string hqSubtitle = "QG de départ";
		if (HAM_Mission.s_sDesignatedHq == HAM_Mission.HQ_NAME)
			hqSubtitle = "QG actif";

		AddMarker(labels, kinds, xs, zs, flags, subtitles, HAM_Mission.HQ_NAME, hqSubtitle, HAM_EMarkerKind.HQ, hqPos, 1);
		AddStats(stats, HAM_Mission.s_HqStock, true, -1);

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell.m_bRevealed)
				continue;

			bool held = cell.IsMasterPlayer();
			int cellFlags = (cell.m_Master.m_Def.m_eType + 1) << 4;
			if (held)
				cellFlags = cellFlags | 1;

			if (cell.IsComplete())
				cellFlags = cellFlags | 2;

			string subtitle = cell.TypeLabel();
			if (HAM_Mission.s_sDesignatedHq == cell.m_sName)
				subtitle = subtitle + " — QG";

			HAM_EMarkerKind masterKind = HAM_EMarkerKind.MASTER;
			if (cell.m_bEnemyHq)
				masterKind = HAM_EMarkerKind.ENEMY_HQ;

			AddMarker(labels, kinds, xs, zs, flags, subtitles, cell.m_sName, subtitle, masterKind, cell.Pos(), cellFlags);
			if (held)
				AddStats(stats, cell.m_Stock, true, cell.m_fGraceLeft);
			else
				AddStats(stats, null, false, -1);

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				int slaveFlags = (slave.m_Def.m_eType + 1) << 4;
				if (slave.m_bPlayer)
					slaveFlags = slaveFlags | 1;

				string slaveSubtitle = "Esclave de " + cell.m_sName;
				HAM_EMasterType slaveType = slave.m_Def.m_eType;
				if (slaveType == HAM_EMasterType.FUEL || slaveType == HAM_EMasterType.CAMP || slaveType == HAM_EMasterType.HIDEOUT || slaveType == HAM_EMasterType.BASE || slaveType == HAM_EMasterType.SUPPLY_VANILLA || slaveType == HAM_EMasterType.SUPPLY_PERSONAL)
					slaveSubtitle = slave.m_Def.TypeLabel() + " de " + cell.m_sName;

				AddMarker(labels, kinds, xs, zs, flags, subtitles, slave.m_Def.m_sName, slaveSubtitle, HAM_EMarkerKind.SLAVE, slave.Pos(), slaveFlags);
				if (slaveType == HAM_EMasterType.SUPPLY_VANILLA)
				{
					int supplyValue;
					int supplyMax;
					HAM_Mission.VanillaSupplyStock(slave.m_Prop, supplyValue, supplyMax);
					AddPlain(stats, supplyValue, supplyMax, -1, -1, -1);
				}
				else if (slaveType == HAM_EMasterType.SUPPLY_PERSONAL)
				{
					HAM_Deposit deposit = HAM_Mission.FindDeposit(slave.m_Def.m_sName);
					if (deposit)
						AddStats(stats, deposit.m_Stock, false, -1);
					else
						AddStats(stats, null, false, -1);
				}
				else
				{
					AddStats(stats, null, false, -1);
				}
			}

		}

		if (!HAM_Settings.Get().m_bDebug || !HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || !line.m_Def)
				continue;

			HAM_EMarkerKind debugKind;
			string debugLabel;
			if (!DebugMarker(line.m_Def.m_eRole, debugKind, debugLabel))
				continue;

			AddMarker(labels, kinds, xs, zs, flags, subtitles, line.m_Def.m_sName, debugLabel, debugKind, line.Pos(), 0);
			AddStats(stats, null, false, -1);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool DebugMarker(HAM_ERole role, out HAM_EMarkerKind kind, out string label)
	{
		kind = HAM_EMarkerKind.PATROL;
		label = "Patrouille";
		if (role == HAM_ERole.PATROL)
			return true;

		if (role == HAM_ERole.CQB)
		{
			kind = HAM_EMarkerKind.CQB;
			label = "CQB";
			return true;
		}

		if (role == HAM_ERole.IED)
		{
			kind = HAM_EMarkerKind.IED;
			label = "IED";
			return true;
		}

		if (role == HAM_ERole.MINES)
		{
			kind = HAM_EMarkerKind.MINES;
			label = "Mines";
			return true;
		}

		if (role == HAM_ERole.CIVILIANS)
		{
			kind = HAM_EMarkerKind.CIVILIANS;
			label = "Civils";
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddMarker(array<string> labels, array<int> kinds, array<float> xs, array<float> zs, array<int> flags, array<string> subtitles, string label, string subtitle, HAM_EMarkerKind kind, vector pos, int markerFlags)
	{
		labels.Insert(label);
		kinds.Insert(kind);
		xs.Insert(pos[0]);
		zs.Insert(pos[2]);
		flags.Insert(markerFlags);
		subtitles.Insert(subtitle);
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddPlain(notnull array<int> stats, int water, int food, int ammo, int men, int grace)
	{
		stats.Insert(water);
		stats.Insert(food);
		stats.Insert(ammo);
		stats.Insert(men);
		stats.Insert(grace);
	}

	//------------------------------------------------------------------------------------------------
	//! Eau, nourriture, munitions, hommes, secondes de grâce ; -1 quand la valeur n'est pas connue.
	protected static void AddStats(notnull array<int> stats, HAM_Stock stock, bool withMen, float graceLeft)
	{
		if (stock)
		{
			stats.Insert(Math.Round(stock.m_fWater));
			stats.Insert(Math.Round(stock.m_fFood));
			stats.Insert(Math.Round(stock.m_fAmmo));
		}
		else
		{
			stats.Insert(-1);
			stats.Insert(-1);
			stats.Insert(-1);
		}

		if (stock && withMen)
			stats.Insert(stock.m_iMen);
		else
			stats.Insert(-1);

		if (graceLeft >= 0)
			stats.Insert(Math.Round(graceLeft));
		else
			stats.Insert(-1);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_PlayerController
{
	//------------------------------------------------------------------------------------------------
	override void OnControlledEntityChanged(IEntity from, IEntity to)
	{
		super.OnControlledEntityChanged(from, to);
		if (!Replication.IsServer() || !to)
			return;

		HAM_Net.SyncPlayerNow(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	//! Le client ouvre la carte : les stats du front partent quand le contrôleur est prêt.
	void HAM_AskFrontState()
	{
		if (!Replication.IsServer())
		{
			Rpc(HAM_RpcAskFrontState);
			return;
		}

		HAM_Net.SyncPlayerNow(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcAskFrontState()
	{
		HAM_Net.SyncPlayerNow(GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SendCaptureBar(string title, float seconds)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
		{
			HAM_RpcCaptureBar(title, seconds);
			return;
		}

		Rpc(HAM_RpcCaptureBar, title, seconds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcCaptureBar(string title, float seconds)
	{
		SCR_PopUpNotification popup = SCR_PopUpNotification.GetInstance();
		if (!popup)
			return;

		if (seconds <= 0)
		{
			popup.HideCurrentMsg();
			return;
		}

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		WorldTimestamp start = world.GetServerTimestamp();
		WorldTimestamp end = start.PlusSeconds(seconds);
		popup.PopupMsg(title, seconds, string.Empty, -1, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, string.Empty, SCR_EPopupMsgFilter.ALL, start, end);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SendHint(string title, string message, int seconds)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
		{
			HAM_RpcHint(title, message, seconds);
			return;
		}

		Rpc(HAM_RpcHint, title, message, seconds);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcHint(string title, string message, int seconds)
	{
		float duration = seconds;
		if (duration < 4)
			duration = 4;

		SCR_HintManagerComponent.ShowCustomHint(message, title, duration);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SendGlobals(array<int> ints, array<float> floats, string designated)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_RpcGlobals(ints, floats, designated);
		else
			Rpc(HAM_RpcGlobals, ints, floats, designated);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcGlobals(array<int> ints, array<float> floats, string designated)
	{
		if (ints && floats)
			HAM_ClientState.ApplyGlobals(ints, floats, designated);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SendMarkers(array<string> labels, array<int> kinds, array<float> xs, array<float> zs, array<int> flags, array<string> subtitles, array<int> stats)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_RpcMarkers(labels, kinds, xs, zs, flags, subtitles, stats);
		else
			Rpc(HAM_RpcMarkers, labels, kinds, xs, zs, flags, subtitles, stats);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcMarkers(array<string> labels, array<int> kinds, array<float> xs, array<float> zs, array<int> flags, array<string> subtitles, array<int> stats)
	{
		if (labels && kinds && xs && zs && flags && subtitles && stats)
			HAM_ClientMarkers.Rebuild(labels, kinds, xs, zs, flags, subtitles, stats);
	}

	//------------------------------------------------------------------------------------------------
	//! Sanction : retour au QG. À pied, le client déplace son personnage ; en véhicule, le serveur.
	void HAM_ForceTeleport(vector spot)
	{
		IEntity body = GetControlledEntity();
		if (!body)
			return;

		if (body.GetRootParent() != body)
		{
			SCR_Global.TeleportPlayer(GetPlayerId(), spot, SCR_EPlayerTeleportedReason.DEFAULT);
			return;
		}

		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_RpcTeleport(spot);
		else
			Rpc(HAM_RpcTeleport, spot);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcTeleport(vector spot)
	{
		SCR_Global.TeleportLocalPlayer(spot, SCR_EPlayerTeleportedReason.DEFAULT);
	}

	//------------------------------------------------------------------------------------------------
	//! Tableau : demande des données de la base où le tableau est posé.
	void HAM_AskBoard(RplId boardId)
	{
		Rpc(HAM_RpcAskBoard, boardId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcAskBoard(RplId boardId)
	{
		HAM_BoardServer.SendBoard(GetPlayerId(), boardId);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_AskBoardAction(RplId boardId, int operation, string target, int kindA, int kindB, float amount)
	{
		Rpc(HAM_RpcAskBoardAction, boardId, operation, target, kindA, kindB, amount);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcAskBoardAction(RplId boardId, int operation, string target, int kindA, int kindB, float amount)
	{
		HAM_BoardServer.DoAction(GetPlayerId(), boardId, operation, target, kindA, kindB, amount);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SendBoard(string baseName, array<int> ints, array<float> floats, array<string> bases, array<string> targets, array<string> missions)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
			HAM_RpcBoard(baseName, ints, floats, bases, targets, missions);
		else
			Rpc(HAM_RpcBoard, baseName, ints, floats, bases, targets, missions);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcBoard(string baseName, array<int> ints, array<float> floats, array<string> bases, array<string> targets, array<string> missions)
	{
		HAM_ResourceBoardMenu.OnBoardData(baseName, ints, floats, bases, targets, missions);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_RequestCivilianInfo(RplId civilianId)
	{
		if (Replication.IsServer())
			HAM_Intel.AskForInfo(HAM_Util.EntityOf(civilianId), GetPlayerId());
		else
			Rpc(HAM_RpcRequestCivilianInfo, civilianId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcRequestCivilianInfo(RplId civilianId)
	{
		HAM_Intel.AskForInfo(HAM_Util.EntityOf(civilianId), GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void HAM_RequestCivilianTalk(RplId civilianId)
	{
		if (Replication.IsServer())
			HAM_Intel.BeginInterrogate(HAM_Util.EntityOf(civilianId), GetPlayerId());
		else
			Rpc(HAM_RpcRequestCivilianTalk, civilianId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcRequestCivilianTalk(RplId civilianId)
	{
		HAM_Intel.BeginInterrogate(HAM_Util.EntityOf(civilianId), GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void HAM_OpenCivilianTalk(RplId civilianId, bool payOnly)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
		{
			HAM_RpcOpenCivilianTalk(civilianId, payOnly);
			return;
		}

		Rpc(HAM_RpcOpenCivilianTalk, civilianId, payOnly);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcOpenCivilianTalk(RplId civilianId, bool payOnly)
	{
		HAM_CivilianTalkUI.Open(civilianId, payOnly);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_CivilianAsk(RplId civilianId, int question)
	{
		if (Replication.IsServer())
			HAM_Intel.Answer(HAM_Util.EntityOf(civilianId), GetPlayerId(), question);
		else
			Rpc(HAM_RpcCivilianAsk, civilianId, question);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcCivilianAsk(RplId civilianId, int question)
	{
		HAM_Intel.Answer(HAM_Util.EntityOf(civilianId), GetPlayerId(), question);
	}

	//------------------------------------------------------------------------------------------------
	void HAM_CivilianTalkClosed(RplId civilianId)
	{
		if (Replication.IsServer())
			HAM_Intel.OnTalkClosed(HAM_Util.EntityOf(civilianId));
		else
			Rpc(HAM_RpcCivilianTalkClosed, civilianId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcCivilianTalkClosed(RplId civilianId)
	{
		HAM_Intel.OnTalkClosed(HAM_Util.EntityOf(civilianId));
	}

	//------------------------------------------------------------------------------------------------
	void HAM_CivilianPay(RplId civilianId)
	{
		if (Replication.IsServer())
			HAM_Intel.Pay(HAM_Util.EntityOf(civilianId), GetPlayerId());
		else
			Rpc(HAM_RpcCivilianPay, civilianId);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcCivilianPay(RplId civilianId)
	{
		HAM_Intel.Pay(HAM_Util.EntityOf(civilianId), GetPlayerId());
	}

	//------------------------------------------------------------------------------------------------
	void HAM_CivilianReply(string text, bool canPay, string confirm)
	{
		if (GetPlayerId() == SCR_PlayerController.GetLocalPlayerId())
		{
			HAM_RpcCivilianReply(text, canPay, confirm);
			return;
		}

		Rpc(HAM_RpcCivilianReply, text, canPay, confirm);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcCivilianReply(string text, bool canPay, string confirm)
	{
		HAM_CivilianTalkUI.ShowAnswer(text, canPay, confirm);
	}

	//------------------------------------------------------------------------------------------------
	//! Le client GM envoie la position de sa caméra. Le serveur la pose sur l'entité éditeur,
	//! c'est elle que la voix de proximité interroge.
	void HAM_PushGmVoice(vector origin)
	{
		if (Replication.IsServer())
		{
			HAM_ApplyGmVoice(origin);
			return;
		}

		Rpc(HAM_RpcGmVoice, origin);
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcGmVoice(vector origin)
	{
		HAM_ApplyGmVoice(origin);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_ApplyGmVoice(vector origin)
	{
		SCR_EditorManagerCore core = SCR_EditorManagerCore.Cast(SCR_EditorManagerCore.GetInstance(SCR_EditorManagerCore));
		if (!core)
			return;

		SCR_EditorManagerEntity editor = core.GetEditorManager(GetPlayerId());
		if (!editor || !editor.IsOpened())
			return;

		vector mat[4];
		editor.GetWorldTransform(mat);
		mat[3] = origin;
		editor.SetWorldTransform(mat);
	}
}
