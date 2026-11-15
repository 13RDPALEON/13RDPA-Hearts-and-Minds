//------------------------------------------------------------------------------------------------
// Points d'accroche sur le Conflit vanilla, actifs seulement avec le composant H&M sur le mode de jeu :
// victoire décidée par le front H&M, une seule faction jouable, construction triée,
// pas de renforts vanilla au QG ennemi, filtre des loadouts.
//------------------------------------------------------------------------------------------------

modded class SCR_GameModeCampaign
{
	//------------------------------------------------------------------------------------------------
	//! La victoire vanilla par points de contrôle est coupée : la fin vient de la prise du QG ennemi.
	override protected void CheckForWinner()
	{
		if (HAM_Mission.IsActive())
			return;

		super.CheckForWinner();
	}

	//------------------------------------------------------------------------------------------------
	void HAM_EndWithVictory(int factionIndex)
	{
		GetGame().GetCallqueue().Remove(CheckForWinner);
		RPC_DoEndMatch(factionIndex);
		Rpc(RPC_DoEndMatch, factionIndex);
		EndGameMode(SCR_GameModeEndData.CreateSimple(EGameOverTypes.ENDREASON_SCORELIMIT, -1, factionIndex));
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_Faction
{
	//------------------------------------------------------------------------------------------------
	//! Seule la faction joueur de HAM_Units est jouable.
	override bool IsPlayable()
	{
		if (HAM_Mission.IsActive())
			return GetFactionKey() == HAM_Units.PLAYER_FACTION;

		return super.IsPlayable();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_AmbientPatrolSpawnPointComponent
{
	//------------------------------------------------------------------------------------------------
	//! La base ennemie est tenue par la garnison H&M : pas de patrouille vanilla en plus.
	override void SpawnPatrol()
	{
		if (!HAM_Mission.IsActive())
		{
			super.SpawnPatrol();
			return;
		}

		if (HAM_Mission.IsNearEnemyHqBase(GetOwner().GetOrigin()) || HAM_BlocksVanillaPatrol(GetOwner()))
			return;

		super.SpawnPatrol();
	}

	//------------------------------------------------------------------------------------------------
	//! Les bases maîtres H&M ont leurs propres garnisons. Les points FIA du prefab Conflit ne partent pas.
	protected bool HAM_BlocksVanillaPatrol(IEntity node)
	{
		while (node)
		{
			if (HAM_MasterLinkComponent.Cast(node.FindComponent(HAM_MasterLinkComponent)))
				return true;

			node = node.GetParent();
		}

		return false;
	}
}

//------------------------------------------------------------------------------------------------
//! Une antenne posée (GM ou construction) entre dans le réseau joueur.
//! Au QG, elle porte sa portée propre (2 km pour la grande antenne).
//! Plus loin, mais déjà dans le réseau, elle relaie 2 km autour d'elle.
class HAM_RadioLink
{
	protected static const ResourceName RELAY = "{C3A91E5704B82D20}Prefabs/HAM/Props/HAM_RadioRelay.et";
	protected static ref array<IEntity> s_aLinked;
	protected static ref array<IEntity> s_aRelays;
	protected static bool s_bAnswerQueued;

	static void Apply(IEntity antenna)
	{
		if (!antenna || !Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		if (s_aLinked && s_aLinked.Contains(antenna))
			return;

		SCR_CampaignMilitaryBaseComponent base = NearestPlayerBase(antenna.GetOrigin());
		if (!base || !base.GetOwner())
			return;

		float dist = vector.DistanceXZ(base.GetOwner().GetOrigin(), antenna.GetOrigin());
		float range = AntennaRange(antenna);
		if (range < 3000)
			range = 3000;

		ForceAntennaRange(antenna, range);
		float current = BaseRange(base);
		bool atBase = dist <= base.GetRadius() + 40;
		if (!atBase && dist > current + 40)
			return;

		if (!s_aLinked)
			s_aLinked = {};

		s_aLinked.Insert(antenna);
		float want = range;
		if (!atBase)
			want = dist + 50;

		SetBaseRange(base, want);
		// RecalculateRadioRange éteint la radio de l'antenne. On la rallume :
		// sinon, à côté d'elle, il n'y a plus de signal.
		ForceAntennaRange(antenna, range);
		PowerRadio(antenna, true);
		if (!atBase)
			SpawnForwarder(antenna, range);

		ScheduleAnswer();

		HAM_Cell master = NearestEnemyMaster(antenna.GetOrigin());
		string extra = "Aucun maître ennemi.";
		if (master)
		{
			float masterDist = vector.DistanceXZ(antenna.GetOrigin(), master.Pos());
			float cover = range;
			if (atBase)
				cover = BaseRange(base);

			if (masterDist <= cover)
				extra = master.m_sName + " est à portée.";
			else
				extra = string.Format("%1 est à %2 m de l'antenne, hors des %3 m. Posez-en une entre les deux.", master.m_sName, Math.Round(masterDist), Math.Round(cover));
		}

		HAM_Mission.Notify("Radio", string.Format("Antenne reliée. %1", extra), 10);
		HAM_Util.Info(string.Format("Radio : antenne à %1 m du QG, portée QG %2 m, antenne %3 m", Math.Round(dist), Math.Round(BaseRange(base)), Math.Round(range)));
	}

	protected static float AntennaRange(IEntity antenna)
	{
		BaseRadioComponent radio = BaseRadioComponent.Cast(antenna.FindComponent(BaseRadioComponent));
		if (!radio)
			return 3000;

		float best = 0;
		for (int i = 0; i < radio.TransceiversCount(); i++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(i);
			if (transceiver && transceiver.GetRange() > best)
				best = transceiver.GetRange();
		}

		if (best < 100)
			return 3000;

		return best;
	}

	protected static void ForceAntennaRange(IEntity antenna, float range)
	{
		BaseRadioComponent radio = BaseRadioComponent.Cast(antenna.FindComponent(BaseRadioComponent));
		if (!radio)
			return;

		for (int i = 0; i < radio.TransceiversCount(); i++)
		{
			BaseTransceiver transceiver = radio.GetTransceiver(i);
			if (transceiver && transceiver.GetRange() < range)
				transceiver.SetRange(range);
		}
	}

	protected static float BaseRange(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner())
			return 0;

		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(base.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		if (!radio || !radio.GetTransceiver(0))
			return 0;

		return radio.GetTransceiver(0).GetRange();
	}

	protected static void SetBaseRange(SCR_CampaignMilitaryBaseComponent base, float range)
	{
		string key = PlayerKey();
		// Publie la portée : SetRange seul reste sur le serveur, le client garde le préfab (1 km).
		base.HAM_PublishRadio(range, key);

		array<SCR_ServicePointComponent> antennas = {};
		base.GetServicesByType(antennas, SCR_EServicePointType.RADIO_ANTENNA);
		foreach (SCR_ServicePointComponent service : antennas)
		{
			if (!service || !service.GetOwner())
				continue;

			PowerRadio(service.GetOwner(), true);
		}
	}

	protected static string PlayerKey()
	{
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(HAM_Units.PLAYER_FACTION));
		if (!faction)
			return "";

		return faction.GetFactionRadioEncryptionKey();
	}

	protected static void SpawnForwarder(IEntity antenna, float range)
	{
		IEntity relay = HAM_Util.Spawn(RELAY, antenna.GetOrigin(), 0);
		if (!relay)
			return;

		antenna.AddChild(relay, -1);
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		relay.SetLocalTransform(mat);
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(relay.FindComponent(SCR_CoverageRadioComponent));
		if (!radio)
			return;

		radio.SetIsSource(true);
		SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(HAM_Units.PLAYER_FACTION));
		if (faction)
			radio.SetEncryptionKey(faction.GetFactionRadioEncryptionKey());

		BaseTransceiver transceiver = radio.GetTransceiver(0);
		if (transceiver)
			transceiver.SetRange(range);

		radio.SetPower(true);
		RememberRelay(relay);
		ScheduleAnswer();
	}

	static void RememberRelay(IEntity relay)
	{
		if (!relay)
			return;

		if (!s_aRelays)
			s_aRelays = {};

		if (s_aRelays.Find(relay) < 0)
			s_aRelays.Insert(relay);
	}

	//------------------------------------------------------------------------------------------------
	//! La réapparition exige les deux sens. Étendre seulement le QG laisse la base à 1 km :
	//! elle reçoit, mais ne répond pas, et le dédié affiche « hors de portée ».
	static void ScheduleAnswer()
	{
		if (!Replication.IsServer() || !HAM_Mission.IsActive() || !GetGame() || !GetGame().GetCallqueue())
			return;

		if (s_bAnswerQueued)
			return;

		s_bAnswerQueued = true;
		GetGame().GetCallqueue().CallLater(RunAnswer, 500, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Deux passages : 30 s puis 45 s. Déjà relié, on ne change rien. Sinon la radio est rerallumée.
	static void ScheduleReconnect(int playerId)
	{
		if (!Replication.IsServer() || !HAM_Mission.IsActive() || playerId <= 0)
			return;

		if (!GetGame() || !GetGame().GetCallqueue())
			return;

		GetGame().GetCallqueue().CallLater(ReconnectPlayer, 30000, false, playerId);
		GetGame().GetCallqueue().CallLater(ReconnectPlayer, 45000, false, playerId);
	}

	protected static void ReconnectPlayer(int playerId)
	{
		if (!Replication.IsServer() || !HAM_Mission.IsActive() || playerId <= 0)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players || !players.IsPlayerConnected(playerId))
			return;

		ScheduleAnswer();
		HAM_RadioChannels.Reconnect(playerId);
	}

	protected static void RunAnswer()
	{
		s_bAnswerQueued = false;
		Answer();
	}

	protected static void Answer()
	{
		if (!Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		SCR_CampaignMilitaryBaseComponent hq = HAM_Mission.s_HqBase;
		if (!hq || !hq.GetOwner())
			return;

		string key = PlayerKey();
		float hqRange = BaseRange(hq);
		if (hqRange < 100)
			hqRange = 1000;

		hq.HAM_PublishRadio(hqRange, key);
		hqRange = BaseRange(hq);
		vector hqPos = hq.GetOwner().GetOrigin();

		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		int linked = 0;
		foreach (SCR_MilitaryBaseComponent listed : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(listed);
			if (!campaignBase || campaignBase == hq || !campaignBase.GetOwner())
				continue;

			Faction faction = campaignBase.GetFaction();
			if (!faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
				continue;

			vector pos = campaignBase.GetOwner().GetOrigin();
			float need = ReplyDistance(pos, hqPos, hqRange);
			if (need <= 0)
				continue;

			campaignBase.HAM_PublishRadio(need, key);
			linked++;
		}

		SCR_RadioCoverageSystem.UpdateAll(true);
		GetGame().GetCallqueue().CallLater(SCR_CoverageRadioComponent.HAM_PushAll, 1500, false);
		HAM_Util.Info(string.Format("Radio : %1 base(s) répondent au QG (portée QG %2 m)", linked, Math.Round(hqRange)));
	}

	protected static float ReplyDistance(vector pos, vector hqPos, float hqRange)
	{
		float need = -1;
		float distHq = vector.DistanceXZ(pos, hqPos);
		if (distHq <= hqRange)
			need = distHq + 50;

		if (!s_aRelays)
			return need;

		foreach (IEntity relay : s_aRelays)
		{
			if (!relay)
				continue;

			SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(relay.FindComponent(SCR_CoverageRadioComponent));
			if (!radio || !radio.IsPowered() || !radio.GetTransceiver(0))
				continue;

			float relayRange = radio.GetTransceiver(0).GetRange();
			float relayHq = vector.DistanceXZ(relay.GetOrigin(), hqPos);
			float relayBase = vector.DistanceXZ(relay.GetOrigin(), pos);
			if (relayRange < relayHq || relayRange < relayBase)
				continue;

			float hop = relayBase + 50;
			if (need < 0 || hop < need)
				need = hop;
		}

		return need;
	}

	protected static void PowerRadio(IEntity antenna, bool on)
	{
		BaseRadioComponent radio = BaseRadioComponent.Cast(antenna.FindComponent(BaseRadioComponent));
		if (radio)
			radio.SetPower(on);
	}

	protected static SCR_CampaignMilitaryBaseComponent NearestPlayerBase(vector pos)
	{
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return HAM_Mission.s_HqBase;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		SCR_CampaignMilitaryBaseComponent nearest;
		float bestSq = -1;
		foreach (SCR_MilitaryBaseComponent listed : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaignBase = SCR_CampaignMilitaryBaseComponent.Cast(listed);
			if (!campaignBase || !campaignBase.GetOwner())
				continue;

			Faction faction = campaignBase.GetFaction();
			if (!faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
				continue;

			float distSq = vector.DistanceSqXZ(pos, campaignBase.GetOwner().GetOrigin());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			nearest = campaignBase;
		}

		if (!nearest)
			return HAM_Mission.s_HqBase;

		return nearest;
	}

	protected static HAM_Cell NearestEnemyMaster(vector pos)
	{
		HAM_Cell nearest;
		float bestSq = -1;
		if (!HAM_Mission.s_aCells)
			return null;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || cell.IsMasterPlayer())
				continue;

			float distSq = vector.DistanceSqXZ(pos, cell.Pos());
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			nearest = cell;
		}

		return nearest;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_AntennaServicePointComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!Replication.IsServer() || !GetGame() || !GetGame().GetCallqueue())
			return;

		GetGame().GetCallqueue().CallLater(HAM_RadioLink.Apply, 2000, false, owner);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_BaseDeployableInventoryItemComponent
{
	protected static const ResourceName HAM_RELAY = "{C3A91E5704B82D20}Prefabs/HAM/Props/HAM_RadioRelay.et";

	override void Deploy(IEntity userEntity = null, bool reload = false)
	{
		super.Deploy(userEntity, reload);
		if (!HAM_Mission.IsActive() || !Replication.IsServer() || !GetOwner())
			return;

		if (!GetOwner().FindComponent(BaseRadioComponent))
			return;

		HAM_ReachDeployedRadio(GetOwner().GetOrigin());
		IEntity relay = HAM_Util.Spawn(HAM_RELAY, GetOwner().GetOrigin(), 0);
		if (!relay)
			return;

		GetOwner().AddChild(relay, -1);
		vector mat[4];
		Math3D.MatrixIdentity4(mat);
		relay.SetLocalTransform(mat);
		SCR_CoverageRadioComponent relayRadio = SCR_CoverageRadioComponent.Cast(relay.FindComponent(SCR_CoverageRadioComponent));
		if (relayRadio)
		{
			relayRadio.SetIsSource(true);
			relayRadio.SetPower(true);
			SCR_Faction faction = SCR_Faction.Cast(GetGame().GetFactionManager().GetFactionByKey(HAM_Units.PLAYER_FACTION));
			if (faction)
				relayRadio.SetEncryptionKey(faction.GetFactionRadioEncryptionKey());

			BaseTransceiver relayTx = relayRadio.GetTransceiver(0);
			if (relayTx)
				relayTx.SetRange(20000);
		}

		HAM_RadioLink.RememberRelay(relay);
		HAM_RadioLink.ScheduleAnswer();
		HAM_Util.Info("Radio posée : relais de 20 km branché sur le réseau joueur");
	}

	//------------------------------------------------------------------------------------------------
	//! Le QG doit atteindre la radio posée, sinon le relais ne reçoit jamais le réseau.
	protected void HAM_ReachDeployedRadio(vector radioPos)
	{
		SCR_CampaignMilitaryBaseComponent hq = HAM_Mission.s_HqBase;
		if (!hq || !hq.GetOwner())
			return;

		SCR_CoverageRadioComponent hqRadio = SCR_CoverageRadioComponent.Cast(hq.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		if (!hqRadio)
			return;

		float needed = vector.DistanceXZ(hq.GetOwner().GetOrigin(), radioPos) + 50;
		hq.HAM_PublishRadio(needed, "");
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CoverageRadioComponent
{
	protected static ref array<SCR_CoverageRadioComponent> s_aHAM_Radios;

	override void OnInit(IEntity owner)
	{
		super.OnInit(owner);
		if (!s_aHAM_Radios)
			s_aHAM_Radios = {};

		if (s_aHAM_Radios.Find(this) < 0)
			s_aHAM_Radios.Insert(this);

		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().CallLater(HAM_PushIfGame, 2000, false);
	}

	override void OnDelete(IEntity owner)
	{
		super.OnDelete(owner);
		if (s_aHAM_Radios)
			s_aHAM_Radios.RemoveItem(this);
	}

	//------------------------------------------------------------------------------------------------
	//! Le premier BumpMe part pendant l'init, avant l'enregistrement réplication.
	//! Sur un dédié il est jeté (« unregistred item ») et le client reste hors de portée.
	protected void HAM_PushIfGame()
	{
		if (!HAM_Mission.IsActive() || !Replication.IsServer())
			return;

		Replication.BumpMe();
	}

	static void HAM_PushAll()
	{
		if (!HAM_Mission.IsActive() || !Replication.IsServer() || !s_aHAM_Radios)
			return;

		foreach (SCR_CoverageRadioComponent radio : s_aHAM_Radios)
		{
			if (radio)
				radio.HAM_PushIfGame();
		}
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_LoadoutManager
{
	//------------------------------------------------------------------------------------------------
	//! HAM_Units.s_aPlayerSpawnSlots limite les loadouts proposés. Liste vide : tous.
	override int GetPlayerLoadoutsByFaction(Faction faction, out notnull array<ref SCR_BasePlayerLoadout> outLoadouts)
	{
		int count = super.GetPlayerLoadoutsByFaction(faction, outLoadouts);
		if (count == 0 || !faction || !HAM_Mission.IsActive() || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
			return count;

		HAM_Units.Init();
		array<ref SCR_BasePlayerLoadout> kept = {};
		foreach (SCR_BasePlayerLoadout loadout : outLoadouts)
		{
			if (loadout && HAM_Units.IsSpawnSlotAllowed(loadout.GetLoadoutResource()))
				kept.Insert(loadout);
		}

		if (kept.IsEmpty())
			return count;

		outLoadouts.Clear();
		foreach (SCR_BasePlayerLoadout keep : kept)
		{
			outLoadouts.Insert(keep);
		}

		return outLoadouts.Count();
	}
}

//------------------------------------------------------------------------------------------------
//! Prefabs 13RDPA du menu de construction des QG (ceux de Conflict, adaptés à H&M).
//! L'ordre ne doit pas changer : l'identifiant de placement d'un prefab en dépend.
class HAM_Buildables
{
	protected static ref array<ResourceName> s_aPrefabs;

	//------------------------------------------------------------------------------------------------
	static array<ResourceName> List()
	{
		if (!s_aPrefabs)
		{
			s_aPrefabs = {
				"{635F701501D22321}Prefabs/HAM/Props/HAM_ArsenalBoard.et",
				"{27B7149CE0DDD07C}Prefabs/HAM/Props/HAM_VehicleDepot.et",
				"{206886BC31AA4225}Prefabs/HAM/Props/HAM_PlayerTeleport.et",
				"{8270892E8A106C8D}Prefabs/HAM/Props/HAM_LightTower.et",
				"{125CF206731B064B}Prefabs/HAM/Props/HAM_RampVehicle.et",
				"{4976BB3090029F06}Prefabs/HAM/Props/HAM_MissionPost.et",
				"{BB8841A3A731852A}Prefabs/HAM/Props/HAM_LoadoutMannequin.et",
				"{6AA33470EE100010}Prefabs/Props/TRN_ParadropBoard.et"
			};
		}

		return s_aPrefabs;
	}
}

//------------------------------------------------------------------------------------------------
//! Le mode construction ne pose que les prefabs de sa liste de placement : le catalogue seul ne suffit pas.
modded class SCR_CampaignBuildingPlacingEditorComponentClass
{
	override int CountPrefabs()
	{
		return super.CountPrefabs() + HAM_Buildables.List().Count();
	}

	//------------------------------------------------------------------------------------------------
	override ResourceName GetPrefab(int index)
	{
		int vanillaCount = super.CountPrefabs();
		array<ResourceName> buildables = HAM_Buildables.List();
		if (index >= vanillaCount && index < vanillaCount + buildables.Count())
			return buildables[index - vanillaCount];

		return super.GetPrefab(index);
	}

	//------------------------------------------------------------------------------------------------
	override int GetPrefabID(ResourceName prefab)
	{
		int id = super.GetPrefabID(prefab);
		if (id != -1)
			return id;

		int index = HAM_Buildables.List().Find(prefab);
		if (index == -1)
			return -1;

		return super.CountPrefabs() + index;
	}

	//------------------------------------------------------------------------------------------------
	override int GetPrefabs(out notnull array<ResourceName> outPrefabs, bool onlyExposed = false)
	{
		super.GetPrefabs(outPrefabs, onlyExposed);
		foreach (ResourceName prefab : HAM_Buildables.List())
		{
			if (outPrefabs.Find(prefab) == -1)
				outPrefabs.Insert(prefab);
		}

		return outPrefabs.Count();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_EntityCatalogManagerComponent
{
	//------------------------------------------------------------------------------------------------
	//! Construction : masque les lignes triées et ajoute les prefabs 13RDPA. Garages et hélipads :
	//! tous les véhicules des catalogues, comme sur Training, même ceux réservés au Game Master.
	protected void HAM_AdjustBuildables(EEntityCatalogType catalogType, EEditorMode editorMode, notnull array<ResourceName> list, bool unfiltered)
	{
		if (editorMode != EEditorMode.BUILDING || !HAM_Mission.IsActive())
			return;

		HAM_Tri.Init();
		for (int i = list.Count() - 1; i >= 0; i--)
		{
			if (HAM_Tri.IsBuildHidden(list[i]))
				list.Remove(i);
		}

		if (catalogType == EEntityCatalogType.VEHICLE && unfiltered)
		{
			array<string> vehicles = {};
			HAM_VehicleCatalog.Collect(vehicles);
			foreach (string vehicle : vehicles)
			{
				if (list.Find(vehicle) == -1)
					list.Insert(vehicle);
			}

			return;
		}

		if (catalogType != EEntityCatalogType.COMPOSITION && catalogType != EEntityCatalogType.OBJECT)
			return;

		foreach (ResourceName prefab : HAM_Buildables.List())
		{
			if (list.Find(prefab) == -1 && !HAM_Tri.IsBuildHidden(prefab))
				list.Insert(prefab);
		}
	}

	//------------------------------------------------------------------------------------------------
	override int GetFilteredEditorPrefabs(EEntityCatalogType catalogType, EEditorMode editorMode, SCR_Faction faction, notnull out array<ResourceName> filteredPrefabsList, array<EEditableEntityLabel> includedLabels = null, array<EEditableEntityLabel> excludedLabels = null, bool needsAllIncludedLabels = true)
	{
		super.GetFilteredEditorPrefabs(catalogType, editorMode, faction, filteredPrefabsList, includedLabels, excludedLabels, needsAllIncludedLabels);
		HAM_AdjustBuildables(catalogType, editorMode, filteredPrefabsList, HAM_NoLabels(includedLabels, excludedLabels));
		return filteredPrefabsList.Count();
	}

	//------------------------------------------------------------------------------------------------
	override int GetFilteredEditorPrefabsOfAllFactions(EEntityCatalogType catalogType, EEditorMode editorMode, notnull out array<ResourceName> filteredPrefabsList, array<EEditableEntityLabel> includedLabels = null, array<EEditableEntityLabel> excludedLabels = null, bool needsAllIncludedLabels = true, bool getFactionLessPrefabs = false)
	{
		super.GetFilteredEditorPrefabsOfAllFactions(catalogType, editorMode, filteredPrefabsList, includedLabels, excludedLabels, needsAllIncludedLabels, getFactionLessPrefabs);
		HAM_AdjustBuildables(catalogType, editorMode, filteredPrefabsList, HAM_NoLabels(includedLabels, excludedLabels));
		return filteredPrefabsList.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! Liste demandée sans filtre d'étiquettes : celle du registre du mode construction.
	protected bool HAM_NoLabels(array<EEditableEntityLabel> includedLabels, array<EEditableEntityLabel> excludedLabels)
	{
		return (!includedLabels || includedLabels.IsEmpty()) && (!excludedLabels || excludedLabels.IsEmpty());
	}
}
