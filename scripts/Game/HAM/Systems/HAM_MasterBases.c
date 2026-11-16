//------------------------------------------------------------------------------------------------
// Base Conflit d'un maître : la même que le QG ennemi, sans ses props ni ses véhicules.
// Poste de commandement du Conflit et panneau de construction. La radio n'est pas une source :
// la capture Conflit (barre) ne démarre que si le réseau joueur l'atteint, et si la cellule est découverte.
//------------------------------------------------------------------------------------------------

class HAM_MasterLinkComponentClass : ScriptComponentClass
{
}

class HAM_MasterLinkComponent : ScriptComponent
{
	protected string m_sCell;
	protected bool m_bHooked;
	protected IEntity m_Service;
	protected ref array<IEntity> m_aProps;
	// Répliqué : le client n'a pas les cellules, et le descripteur de carte est local.
	[RplProp(onRplName: "OnMapShown")]
	protected bool m_bMapShown;

	//------------------------------------------------------------------------------------------------
	void Setup(string cellName, int radius)
	{
		m_sCell = cellName;
		SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(GetOwner().FindComponent(SCR_CampaignSeizingComponent));
		if (seizing)
			seizing.HAM_SetRadius(radius);

		if (seizing)
			seizing.HAM_SetDuration(HAM_Settings.Get().m_fCaptureSeconds);

		if (!m_bHooked && seizing)
		{
			m_bHooked = true;
			seizing.GetOnCaptureFinish().Insert(OnCaptured);
		}

		bool shown = false;
		HAM_Cell cell = HAM_Mission.FindCell(m_sCell);
		if (cell && (cell.m_bRevealed || cell.IsMasterPlayer()))
			shown = true;

		SetMapShown(shown);
	}

	//------------------------------------------------------------------------------------------------
	bool IsMapShown()
	{
		return m_bMapShown;
	}

	//------------------------------------------------------------------------------------------------
	void SetMapShown(bool shown)
	{
		m_bMapShown = shown;
		Replication.BumpMe();
		OnMapShown();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnMapShown()
	{
		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(GetOwner().FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (base)
			HAM_MasterBases.ApplyMapVisibility(base, m_bMapShown);
	}

	//------------------------------------------------------------------------------------------------
	//! +1000 fournitures dans un dépôt vanilla porté par ce maître. Faux s'il n'y en a pas.
	bool PourOwnedSupplies(int amount)
	{
		if (HAM_Mission.AddVanillaSupplies(GetOwner(), amount))
			return true;

		if (!m_aProps)
			return false;

		foreach (IEntity prop : m_aProps)
		{
			if (HAM_Mission.AddVanillaSupplies(prop, amount))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Remplit le dépôt sans relever son plafond. Rend la quantité réellement posée.
	int PourCappedSupplies(int amount)
	{
		if (amount <= 0)
			return 0;

		int left = amount - HAM_Credit.PourStorage(GetOwner(), amount);
		if (left <= 0 || !m_aProps)
			return amount - left;

		foreach (IEntity prop : m_aProps)
		{
			left -= HAM_Credit.PourStorage(prop, left);
			if (left <= 0)
				break;
		}

		return amount - left;
	}

	//------------------------------------------------------------------------------------------------
	IEntity BarracksProp()
	{
		if (!m_aProps)
			return null;

		for (int i = 0; i < m_aProps.Count(); i++)
		{
			if (HAM_MasterBases.IsBarracksProp(m_aProps[i]))
				return m_aProps[i];
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	string CellName()
	{
		return m_sCell;
	}

	//------------------------------------------------------------------------------------------------
	void RememberService(IEntity service)
	{
		m_Service = service;
	}

	//------------------------------------------------------------------------------------------------
	void RememberProp(IEntity prop)
	{
		if (!prop)
			return;

		if (!m_aProps)
			m_aProps = {};

		m_aProps.Insert(prop);
	}

	//------------------------------------------------------------------------------------------------
	bool OwnsProp(IEntity prop)
	{
		if (!prop || !m_aProps)
			return false;

		return m_aProps.Find(prop) >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Camp joueur : la construction du panneau se débloque. Camp ennemi : elle se referme.
	void SetCamp(string factionKey)
	{
		SCR_CampaignFaction faction = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(GetOwner().FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (base && faction)
			base.SetFaction(faction);

		HAM_MasterLinkComponent.SetEntityFaction(m_Service, factionKey);
		if (!m_aProps)
			return;

		bool player = factionKey == HAM_Units.PLAYER_FACTION;
		for (int i = 0; i < m_aProps.Count(); i++)
		{
			IEntity prop = m_aProps[i];
			if (HAM_MasterBases.IsFuelProp(prop) || HAM_MasterBases.IsBarracksProp(prop))
			{
				HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
				HAM_MasterBases.OpenBuilt(prop, factionKey);
			}
			else if (HAM_MasterBases.IsCommandPost(prop))
			{
				HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
				HAM_MasterBases.OpenBuilt(prop, factionKey);
				HAM_MasterBases.KeepSingleDeployPoint(prop, m_sCell);
			}
			else
			{
				HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
				HAM_MasterBases.OpenBuilt(prop, factionKey);
			}

			HAM_MasterBases.ScheduleBarracksCap(m_aProps[i], player);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void SetEntityFaction(IEntity ent, string factionKey)
	{
		if (!ent)
			return;

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
		if (affiliation)
			affiliation.SetAffiliatedFactionByKey(factionKey);

		SCR_ServicePointMapDescriptorComponent serviceIcon = SCR_ServicePointMapDescriptorComponent.Cast(ent.FindComponent(SCR_ServicePointMapDescriptorComponent));
		if (serviceIcon)
		{
			Faction faction = GetGame().GetFactionManager().GetFactionByKey(factionKey);
			if (faction)
				serviceIcon.SetServiceMarker(faction, true);
		}

		IEntity child = ent.GetChildren();
		while (child)
		{
			SetEntityFaction(child, factionKey);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnCaptured(SCR_Faction faction, SCR_SeizingComponent seizing)
	{
		if (!Replication.IsServer() || m_sCell.IsEmpty())
			return;

		if (!faction || faction.GetFactionKey() != HAM_Units.PLAYER_FACTION)
			return;

		HAM_Cell cell = HAM_Mission.FindCell(m_sCell);
		if (!cell || !cell.m_Master || cell.m_Master.m_bPlayer)
			return;

		HAM_Front.Capture(cell.m_Master);
	}
}

//------------------------------------------------------------------------------------------------
class HAM_MasterBases
{
	protected static const int CAMP_MEN = 8;
	protected static const int CAMP_DEPLOY_GROUPS = 4;
	protected static const ResourceName BASE = "{D17A6C4E90B2538F}Prefabs/HAM/Bases/HAM_MasterBase.et";
	protected static const ResourceName COMMAND_POST = "{16AF2FE47A6C297B}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatSmall/E_Headquarters_S_Conflict_US_01.et";
	protected static const ResourceName COMMAND_POST_PLAYER = "{D70BF977E6D6300C}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatSmall/E_Headquarters_S_Conflict_USSR_01.et";
	protected static const ResourceName BUILDING = "{7649AC4689618998}Prefabs/Props/Military/Compositions/US/BuildingService_US.et";
	protected static const ResourceName FUEL = "{6D2A35DA74224780}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatSmall/E_FuelStorage_S_US_01.et";
	protected static const ResourceName FUEL_PLAYER = "{C2D040BB21F15A20}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatSmall/E_FuelStorage_S_USSR_01.et";
	protected static const ResourceName ANTENNA = "{CF4F7AEE0BFA7B25}Prefabs/Props/Military/Compositions/US/Antenna_01_US.et";
	protected static const ResourceName BARRACKS = "{3C2CFF03FF61BE7A}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatLarge/E_LivingArea_L_Conflict_US_01.et";
	protected static const ResourceName BARRACKS_PLAYER = "{D44C687485600B72}PrefabsEditable/Auto/Compositions/Slotted/SlotFlatLarge/E_LivingArea_L_Conflict_USSR_01.et";
	protected static const ResourceName GARAGE_LIGHT = "{5753634EAFFBA2F6}Prefabs/Props/Military/Compositions/US/VehicleService_US.et";
	protected static const ResourceName GARAGE_HEAVY = "{2FE44206E527DA4C}Prefabs/Props/Military/Compositions/US/VehicleService_US_Medium.et";
	protected static const ResourceName REPAIR_RAMP = "{74FAABE8512145EF}Prefabs/Structures/Industrial/Repair/RampVehicle_01/RampVehicle_01_metal.et";
	protected static const float PREFAB_SEARCH = 200;
	protected static ref map<string, float> s_mExplainAt;

	//------------------------------------------------------------------------------------------------
	//! Prefab posé par H&M (caserne, carburant, QG). Pas une construction du joueur.
	static bool IsSystemProp(IEntity ent)
	{
		if (!ent || !HAM_Mission.s_aCells)
			return false;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell)
				continue;

			if (cell.m_Master && ListedProp(cell.m_Master.m_aTypeProps, ent))
				return true;

			if (cell.m_aSlaves)
			{
				foreach (HAM_Point slave : cell.m_aSlaves)
				{
					if (slave && ListedProp(slave.m_aTypeProps, ent))
						return true;
				}
			}

			SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), 160);
			if (!base || !base.GetOwner())
				continue;

			HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
			if (link && link.OwnsProp(ent))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ListedProp(array<IEntity> props, IEntity ent)
	{
		if (!props || !ent)
			return false;

		return props.Find(ent) >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Le préfab système est déjà reposé. Une copie sauvée au même endroit ferait doublon.
	static bool SystemPrefabNear(string prefab, vector pos, float radius)
	{
		if (prefab.IsEmpty() || !HAM_Mission.s_aCells)
			return false;

		float limit = radius * radius;
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell)
				continue;

			if (cell.m_Master && PrefabNear(cell.m_Master.m_aTypeProps, prefab, pos, limit))
				return true;

			if (!cell.m_aSlaves)
				continue;

			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				if (slave && PrefabNear(slave.m_aTypeProps, prefab, pos, limit))
					return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PrefabNear(array<IEntity> props, string prefab, vector pos, float limitSq)
	{
		if (!props)
			return false;

		foreach (IEntity prop : props)
		{
			if (!prop || !prop.GetPrefabData())
				continue;

			if (prop.GetPrefabData().GetPrefabName() != prefab)
				continue;

			if (vector.DistanceSqXZ(prop.GetOrigin(), pos) <= limitSq)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static void PlaceAll()
	{
		if (!HAM_Mission.s_aCells)
			return;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell.m_Master)
				continue;

			HAM_MasterLinkComponent existingLink = LinkOf(cell);
			if (existingLink)
			{
				HAM_Mission.CountZonePrefab(false);
				if (existingLink.CellName().IsEmpty())
					existingLink.Setup(cell.m_sName, Math.Round(cell.m_Master.Radius()));
				else
					existingLink.SetMapShown(cell.m_bRevealed || cell.IsMasterPlayer());

				if (cell.m_Master.m_bPlayer)
					Adopt(cell, true);

				KeepZoneSpawns(cell, existingLink.GetOwner());
				continue;
			}

			SCR_CampaignMilitaryBaseComponent foreign = HAM_Mission.FindBaseNear(cell.Pos(), cell.m_Master.Radius());
			if (foreign && foreign.GetOwner() && !HAM_MasterLinkComponent.Cast(foreign.GetOwner().FindComponent(HAM_MasterLinkComponent)))
				SilenceForeignBase(foreign);

			Place(cell);
		}

		PlaceSlaves();
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceSlaves()
	{
		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			foreach (HAM_Point slave : cell.m_aSlaves)
			{
				PlaceSlave(slave);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Hommes d'un camp, versés à la prise. Le réglage remplace l'ancien compte fixe.
	static int CampMen()
	{
		int men = HAM_Settings.Get().m_iCampMen;
		if (men < 0)
			return CAMP_MEN;

		return men;
	}

	//------------------------------------------------------------------------------------------------
	//! Après la prise, la base et son panneau passent au camp joueur. À la perte, ils redeviennent ennemis.
	static void Adopt(HAM_Cell cell, bool player)
	{
		HAM_MasterLinkComponent link = LinkOf(cell);
		if (!link)
			return;

		if (player)
			link.SetCamp(HAM_Units.PLAYER_FACTION);
		else
			link.SetCamp(HAM_Units.ENEMY_FACTION);
	}

	//------------------------------------------------------------------------------------------------
	static bool UsesSeizing(HAM_Cell cell)
	{
		return cell && LinkOf(cell) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! La planque ne se prend qu'une fois sa cache détruite. Sans cache posée, rien ne bloque.
	static bool CacheStanding(HAM_Point point)
	{
		if (!point || !point.m_Cache || point.m_Def.m_eType != HAM_EMasterType.HIDEOUT)
			return false;

		return !HAM_Util.IsDead(point.m_Cache);
	}

	//------------------------------------------------------------------------------------------------
	static bool RadioLinked(HAM_Cell cell)
	{
		if (!cell)
			return false;

		SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), 120);
		if (!base)
			return false;

		SCR_CampaignFaction player = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(HAM_Units.PLAYER_FACTION));
		if (!player)
			return false;

		return base.IsHQRadioTrafficPossible(player);
	}

	//------------------------------------------------------------------------------------------------
	//! Distance du QG et portée actuelle, pour dire pourquoi la barre ne part pas.
	static string RadioGap(HAM_Cell cell)
	{
		if (!cell)
			return "pas de radio";

		SCR_CampaignMilitaryBaseComponent hq = HAM_Mission.s_HqBase;
		if (!hq || !hq.GetOwner())
			return "le QG n'a pas de radio";

		float dist = vector.DistanceXZ(hq.GetOwner().GetOrigin(), cell.Pos());
		float range = 0;
		SCR_CoverageRadioComponent radio = SCR_CoverageRadioComponent.Cast(hq.GetOwner().FindComponent(SCR_CoverageRadioComponent));
		if (radio && radio.GetTransceiver(0))
			range = radio.GetTransceiver(0).GetRange();

		return string.Format("radio trop courte (%1 m, %2 est à %3 m du QG). Posez une antenne entre les deux.", Math.Round(range), cell.m_sName, Math.Round(dist));
	}

	//------------------------------------------------------------------------------------------------
	static void ExplainBlock(HAM_Point point)
	{
		if (!point || !point.m_Cell)
			return;

		float now = HAM_Util.Now();
		if (!s_mExplainAt)
			s_mExplainAt = new map<string, float>();

		string key = point.m_Def.m_sName;
		if (s_mExplainAt.Contains(key) && now - s_mExplainAt.Get(key) < 25)
			return;

		HAM_Cell cell = point.m_Cell;
		string why;
		if (CacheStanding(point))
			why = "la cache de la planque est encore debout";
		else if (!cell.m_bRevealed)
			why = "la zone n'est pas découverte";
		else if (!HAM_Mission.IsFront(cell) && !cell.IsMasterPlayer())
			why = "elle n'est pas sur la ligne de front";
		else if (HAM_Garrison.EnemyAliveNear(point.Pos(), point.Radius()))
			why = "il reste des ennemis dans la zone";
		else if (!RadioLinked(cell))
			why = RadioGap(cell);
		else
			return;

		s_mExplainAt.Set(key, now);
		HAM_Mission.Notify("Capture", string.Format("%1 : %2.", point.m_Def.m_sName, why), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! Base du jeu déjà sur le maître : on cache son icône et ses points, la nôtre porte la capture.
	protected static void SilenceForeignBase(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner())
			return;

		ApplyMapVisibility(base, false);
		array<SCR_SpawnPoint> points = {};
		CollectBaseSpawns(base.GetOwner(), points);
		foreach (SCR_SpawnPoint point : points)
		{
			if (point)
				point.SetSpawnPointEnabled_S(false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Le poste de commandement contient parfois sa propre base. Elle ferait un second point sur le maître.
	protected static void RemoveNestedBases(IEntity post)
	{
		if (!post)
			return;

		array<IEntity> drop = {};
		CollectNestedBases(post, post, drop);
		foreach (IEntity extra : drop)
			HAM_Util.Delete(extra);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectNestedBases(IEntity node, IEntity root, notnull array<IEntity> drop)
	{
		if (!node)
			return;

		if (node != root && SCR_CampaignMilitaryBaseComponent.Cast(node.FindComponent(SCR_CampaignMilitaryBaseComponent)))
		{
			drop.Insert(node);
			return;
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			CollectNestedBases(child, root, drop);
			child = next;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Le point du Conflit est l'enfant SCR_CampaignSpawnPointGroup de la base.
	//! Le poste de commandement en apporte un second (E_SpawnPoint) : on le retire.
	static void KeepSingleDeployPoint(IEntity root, string name)
	{
		NameDeployPoints(root, name);
	}

	//------------------------------------------------------------------------------------------------
	//! Base déjà en jeu (reprise) : un seul point, celui du Conflit, même si le poste est à part.
	protected static void KeepZoneSpawns(HAM_Cell cell, IEntity baseEntity)
	{
		if (!cell)
			return;

		NameDeployPoints(baseEntity, cell.m_sName);

		array<IEntity> posts = {};
		CollectCommandPosts(cell.Pos(), posts);
		foreach (IEntity post : posts)
			SchedulePostStrip(post, cell.m_sName);
	}

	//------------------------------------------------------------------------------------------------
	//! Le préfab éditable fait naître son point après la pose. On repasse quand il est là.
	protected static void SchedulePostStrip(IEntity post, string name)
	{
		NameDeployPoints(post, name);
		GetGame().GetCallqueue().CallLater(NameDeployPoints, 1600, false, post, name);
		GetGame().GetCallqueue().CallLater(NameDeployPoints, 4000, false, post, name);
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aCommandPosts;

	//------------------------------------------------------------------------------------------------
	protected static bool SeeCommandPost(IEntity ent)
	{
		if (IsCommandPost(ent))
			s_aCommandPosts.Insert(ent);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectCommandPosts(vector pos, notnull array<IEntity> posts)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		s_aCommandPosts = {};
		world.QueryEntitiesBySphere(pos, 120, SeeCommandPost);
		foreach (IEntity post : s_aCommandPosts)
		{
			if (post)
				posts.Insert(post);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyCaptureDuration()
	{
		float seconds = HAM_Settings.Get().m_fCaptureSeconds;
		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return;

		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		foreach (SCR_MilitaryBaseComponent base : bases)
		{
			if (!base || !base.GetOwner())
				continue;

			HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
			if (!link || link.CellName().IsEmpty())
				continue;

			SCR_CampaignSeizingComponent seizing = SCR_CampaignSeizingComponent.Cast(base.GetOwner().FindComponent(SCR_CampaignSeizingComponent));
			if (seizing)
				seizing.HAM_SetDuration(seconds);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_MasterLinkComponent LinkOf(HAM_Cell cell)
	{
		if (!cell || !cell.m_Master)
			return null;

		SCR_MilitaryBaseSystem system = SCR_MilitaryBaseSystem.GetInstance();
		if (!system)
			return null;

		float radius = cell.m_Master.Radius();
		if (radius < 80)
			radius = 80;

		float limitSq = radius * radius;
		array<SCR_MilitaryBaseComponent> bases = {};
		system.GetBases(bases);
		foreach (SCR_MilitaryBaseComponent listed : bases)
		{
			SCR_CampaignMilitaryBaseComponent campaign = SCR_CampaignMilitaryBaseComponent.Cast(listed);
			if (!campaign || !campaign.GetOwner())
				continue;

			if (vector.DistanceSqXZ(campaign.GetOwner().GetOrigin(), cell.Pos()) > limitSq)
				continue;

			HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(campaign.GetOwner().FindComponent(HAM_MasterLinkComponent));
			if (!link)
				continue;

			if (link.CellName().IsEmpty() || link.CellName() == cell.m_sName)
				return link;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aSamePrefab;
	protected static string s_sSameToken;

	//------------------------------------------------------------------------------------------------
	protected static string PrefabFile(ResourceName prefab)
	{
		string path = prefab;
		int slash = path.LastIndexOf("/");
		if (slash >= 0 && slash + 1 < path.Length())
			return path.Substring(slash + 1, path.Length() - slash - 1);

		return path;
	}

	//------------------------------------------------------------------------------------------------
	//! L'héritier posé dans le monde n'a pas le préfixe E_ du préfab éditable.
	protected static string CompositionToken(ResourceName prefab)
	{
		string file = PrefabFile(prefab);
		if (file.Length() > 2 && file.Substring(0, 2) == "E_")
			return file.Substring(2, file.Length() - 2);

		return file;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeeSamePrefab(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData() || s_sSameToken.IsEmpty())
			return true;

		if (ent.GetPrefabData().GetPrefabName().Contains(s_sSameToken))
			s_aSamePrefab.Insert(ent);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Prefab déjà posé dans le rayon de la zone. On garde le plus proche du centre.
	//! Un second exemplaire n'est retiré que s'il est collé à celui qu'on garde (doublon de persistance).
	protected static IEntity TakeExisting(ResourceName prefab, vector pos, float radius)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		s_sSameToken = CompositionToken(prefab);
		s_aSamePrefab = {};
		world.QueryEntitiesBySphere(pos, radius, SeeSamePrefab);
		if (!s_aSamePrefab || s_aSamePrefab.IsEmpty())
			return null;

		IEntity best = null;
		float bestSq = 0;
		foreach (IEntity ent : s_aSamePrefab)
		{
			if (!ent)
				continue;

			float distSq = vector.DistanceSqXZ(ent.GetOrigin(), pos);
			if (!best || distSq < bestSq)
			{
				best = ent;
				bestSq = distSq;
			}
		}

		if (!best)
			return null;

		HAM_Util.Forget(best);
		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Place(HAM_Cell cell)
	{
		vector pos = cell.Pos();
		int radius = Math.Round(cell.m_Master.Radius());
		IEntity post = KeepComposition(COMMAND_POST, COMMAND_POST_PLAYER, pos, pos, 0, 16, "poste " + cell.m_sName);
		IEntity logic = FindPrefabBase(post);
		if (logic)
			HAM_Util.Delete(logic);

		RemoveNestedBases(post);

		OpenBuilt(post);

		vector zone = pos;
		if (post)
			zone = post.GetOrigin();

		IEntity baseEntity = HAM_Util.Spawn(BASE, zone, 0);
		if (!baseEntity)
		{
			HAM_Mission.FailZonePrefab("base " + cell.m_sName);
			return;
		}

		HAM_Mission.CountZonePrefab(true);

		string factionKey = HAM_Units.ENEMY_FACTION;
		if (cell.m_Master.m_bPlayer)
			factionKey = HAM_Units.PLAYER_FACTION;

		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(baseEntity.FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (base)
			base.HAM_SetName(cell.m_sName);

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(baseEntity.FindComponent(FactionAffiliationComponent));
		if (affiliation)
			affiliation.SetAffiliatedFactionByKey(factionKey);

		HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(baseEntity.FindComponent(HAM_MasterLinkComponent));
		if (link)
		{
			link.Setup(cell.m_sName, radius);
			link.RememberProp(post);
		}

		HAM_Mission.DisableBasePatrols(base);
		PlaceType(cell, link);
		if (base)
		{
			base.Initialize();
			ApplyMapVisibility(base, cell.m_bRevealed);
			SCR_CampaignFaction owner = SCR_CampaignFaction.Cast(GetGame().GetFactionManager().GetFactionByKey(factionKey));
			if (owner)
				base.SetFaction(owner);
		}

		HAM_MasterLinkComponent.SetEntityFaction(post, factionKey);
		NameDeployPoints(baseEntity, cell.m_sName);
		if (post)
			SchedulePostStrip(post, cell.m_sName);

		Print(string.Format("[13RDPA H&M] Base maître posée : %1 (%2)", cell.m_sName, cell.m_Master.m_Def.TypeLabel()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul point de réapparition : le SCR_CampaignSpawnPointGroup de la base Conflit.
	//! Le menu liste les points par faction. Couper « enabled » les laisse affichés.
	//! Le poste ajoute le sien (E_SpawnPoint). On lui retire faction et affiliation,
	//! sans supprimer la pièce : le préfab éditable et sa sauvegarde la réfèrent.
	protected static void NameDeployPoints(IEntity root, string name)
	{
		if (!root || root.IsDeleted())
			return;

		if (!name.IsEmpty())
			NameDeployNode(root, name);

		SCR_CampaignMilitaryBaseComponent base = SCR_CampaignMilitaryBaseComponent.Cast(root.FindComponent(SCR_CampaignMilitaryBaseComponent));
		SCR_SpawnPoint kept;
		if (base)
			kept = base.GetSpawnPoint();

		array<SCR_SpawnPoint> points = {};
		CollectBaseSpawns(root, points);

		bool sawCampaign = false;
		array<SCR_SpawnPoint> extra = {};
		foreach (SCR_SpawnPoint point : points)
		{
			if (!point || point.IsDeleted())
				continue;

			if (point == kept || SCR_CampaignSpawnPointGroup.Cast(point))
			{
				sawCampaign = true;
				point.SetSpawnPointEnabled_S(true);
				continue;
			}

			extra.Insert(point);
		}

		foreach (SCR_SpawnPoint point : extra)
		{
			if (!point || point.IsDeleted())
				continue;

			point.SetSpawnPointEnabled_S(false);
			FactionAffiliationComponent mark = FactionAffiliationComponent.Cast(point.FindComponent(FactionAffiliationComponent));
			if (mark)
				mark.SetAffiliatedFaction(null);

			point.SetFactionKey(string.Empty);
		}

		if (base && !kept && !sawCampaign && !name.IsEmpty())
			Print(string.Format("[13RDPA H&M] %1 : la base Conflit n'a pas de point de réapparition.", name), LogLevel.WARNING);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectBaseSpawns(IEntity node, notnull array<SCR_SpawnPoint> points)
	{
		if (!node)
			return;

		SCR_SpawnPoint spawn = SCR_SpawnPoint.Cast(node);
		if (spawn)
			points.Insert(spawn);

		IEntity child = node.GetChildren();
		while (child)
		{
			CollectBaseSpawns(child, points);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void NameDeployNode(IEntity node, string name)
	{
		if (!node)
			return;

		SCR_CampaignMilitaryBaseComponent named = SCR_CampaignMilitaryBaseComponent.Cast(node.FindComponent(SCR_CampaignMilitaryBaseComponent));
		if (named)
			named.HAM_SetName(name);

		IEntity child = node.GetChildren();
		while (child)
		{
			NameDeployNode(child, name);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! La base incluse dans le poste de commandement. C'est elle qui porte la zone.
	protected static IEntity FindPrefabBase(IEntity node)
	{
		if (!node)
			return null;

		SCR_MilitaryBaseComponent military = SCR_MilitaryBaseComponent.Cast(node.FindComponent(SCR_MilitaryBaseComponent));
		if (military && !SCR_CampaignMilitaryBaseComponent.Cast(military))
			return node;

		IEntity child = node.GetChildren();
		while (child)
		{
			IEntity found = FindPrefabBase(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Vrai tant que ce maître H&M n'est pas découvert. Les QG sans lien H&M restent visibles.
	//! Côté client, les cellules ne sont pas là : le drapeau répliqué du lien fait foi.
	static bool MapHidden(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner() || !HAM_Mission.IsActive())
			return false;

		HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
		if (!link)
			return false;

		if (link.IsMapShown())
			return false;

		HAM_Cell cell = HAM_Mission.FindCell(link.CellName());
		if (cell)
			return !cell.m_bRevealed && !cell.IsMasterPlayer();

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void ShowDiscovered(HAM_Cell cell)
	{
		if (!cell)
			return;

		SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(cell.Pos(), 120);
		if (!base || !base.GetOwner())
			return;

		HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
		if (link)
			link.SetMapShown(true);
		else
			ApplyMapVisibility(base, true);
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyMapVisibility(SCR_CampaignMilitaryBaseComponent base, bool visible)
	{
		if (!base)
			return;

		SCR_CampaignMilitaryBaseMapDescriptorComponent descriptor = base.GetMapDescriptor();
		if (descriptor && descriptor.Item())
		{
			MapItem item = descriptor.Item();
			item.SetVisible(visible);
			if (visible)
			{
				item.SetDisplayName(base.GetBaseName());
			}
			else
			{
				MapDescriptorProps props = item.GetProps();
				if (props)
				{
					props.SetIconVisible(false);
					props.SetTextVisible(false);
					item.SetProps(props);
				}
			}
		}

		SCR_CampaignMapUIBase icon = base.GetMapUI();
		if (!icon)
			return;

		icon.SetVisible(visible);
	}

	//------------------------------------------------------------------------------------------------
	//! 900 fournitures sur la caserne capturée, pour le déploiement. Un stock plus haut n'est pas abaissé.
	static void PourBarracks(HAM_Point point)
	{
		IEntity barracks = BarracksOf(point);
		if (!barracks)
			return;

		GetGame().GetCallqueue().CallLater(PourBarracksLater, 1500, false, barracks, 0);
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity BarracksOf(HAM_Point point)
	{
		if (!point || !point.m_Def || point.m_Def.m_eType != HAM_EMasterType.CAMP)
			return null;

		if (point.IsMaster())
		{
			SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(point.Pos(), 120);
			if (!base || !base.GetOwner())
				return null;

			HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
			if (!link)
				return null;

			return link.BarracksProp();
		}

		if (!point.m_aTypeProps)
			return null;

		for (int i = 0; i < point.m_aTypeProps.Count(); i++)
		{
			if (IsBarracksProp(point.m_aTypeProps[i]))
				return point.m_aTypeProps[i];
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void PourBarracksLater(IEntity barracks, int tries)
	{
		if (!barracks)
			return;

		int amount = HAM_Settings.Get().m_iCampSupplies;
		if (amount < 0)
			amount = 0;

		SCR_ResourceContainer container = SuppliesContainer(barracks);
		if (!container)
		{
			if (tries < 10)
			{
				GetGame().GetCallqueue().CallLater(PourBarracksLater, 1000, false, barracks, tries + 1);
				return;
			}

			Print("[13RDPA H&M] Caserne sans conteneur de fournitures", LogLevel.WARNING);
			HAM_Credit.CreditBarracks(barracks, amount, "prise de caserne");
			return;
		}

		HAM_Credit.CreditBarracks(barracks, amount, "prise de caserne");
		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();
		Print(string.Format("[13RDPA H&M] Caserne : fournitures prévues %1", amount), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ResourceContainer SuppliesContainer(IEntity node)
	{
		if (!node)
			return null;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (resources)
		{
			SCR_ResourceContainer container = resources.GetContainer(EResourceType.SUPPLIES);
			if (container)
				return container;
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			SCR_ResourceContainer found = SuppliesContainer(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void PlaceType(HAM_Cell cell, HAM_MasterLinkComponent link)
	{
		vector pos = cell.Pos();
		HAM_EMasterType type = cell.m_Master.m_Def.m_eType;
		if (type == HAM_EMasterType.FUEL)
		{
			IEntity fuel = KeepComposition(FUEL, FUEL_PLAYER, pos, pos + Vector(16, 0, 0), 90, 8, cell.m_sName);
			if (link)
				link.RememberProp(fuel);

			OpenBuilt(fuel, FactionKeyOf(cell.m_Master.m_bPlayer));
		}
		else if (type == HAM_EMasterType.CAMP)
		{
			IEntity barracks = KeepComposition(BARRACKS, BARRACKS_PLAYER, pos, pos + Vector(22, 0, 14), 0, 18, cell.m_sName);
			if (link)
				link.RememberProp(barracks);

			OpenBuilt(barracks, FactionKeyOf(cell.m_Master.m_bPlayer));
			ScheduleBarracksCap(barracks, cell.m_Master.m_bPlayer);
		}
		else if (type == HAM_EMasterType.RADIO)
			PlaceProp(link, ANTENNA, pos, pos + Vector(14, 0, 6), 0, 6);
		else if (type == HAM_EMasterType.HIDEOUT)
			cell.m_Master.m_Cache = PlaceHideout(cell.Pos(), cell.m_Master.Radius(), cell.m_sName);
		else if (type == HAM_EMasterType.BASE)
		{
			PlaceProp(link, GARAGE_LIGHT, pos, pos + Vector(16, 0, 8), 90, 8);
			PlaceProp(link, GARAGE_HEAVY, pos, pos + Vector(16, 0, -10), 90, 12);
			PlaceProp(link, REPAIR_RAMP, pos, pos + Vector(28, 0, 0), 0, 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul préfab pour les deux camps, cherché autour du centre de zone.
	//! Le double de l'autre camp n'est jeté que s'il est collé à celui qu'on garde.
	protected static IEntity KeepComposition(ResourceName prefab, ResourceName other, vector zone, vector pos, float yaw, float footprint, string label)
	{
		IEntity kept = TakeExisting(prefab, zone, PREFAB_SEARCH);
		IEntity twin = TakeExisting(other, zone, PREFAB_SEARCH);
		if (kept && twin && vector.DistanceSqXZ(twin.GetOrigin(), zone) < vector.DistanceSqXZ(kept.GetOrigin(), zone))
			kept = twin;
		else if (!kept)
		{
			kept = twin;
		}

		bool spawned = false;
		if (!kept)
		{
			kept = HAM_Util.Spawn(prefab, pos, yaw);
			spawned = true;
		}

		if (!kept)
		{
			HAM_Mission.FailZonePrefab(label);
			return null;
		}

		HAM_Mission.CountZonePrefab(spawned);
		return kept;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity PlaceProp(HAM_MasterLinkComponent link, ResourceName prefab, vector zone, vector pos, float yaw, float footprint)
	{
		bool spawned = false;
		IEntity prop = TakeExisting(prefab, zone, PREFAB_SEARCH);
		if (!prop)
		{
			prop = HAM_Util.Spawn(prefab, pos, yaw);
			spawned = true;
		}

		if (!prop)
			HAM_Mission.FailZonePrefab(HAM_Util.PrefabFileName(prefab));
		else
			HAM_Mission.CountZonePrefab(spawned);

		if (link)
			link.RememberProp(prop);

		return prop;
	}

	//------------------------------------------------------------------------------------------------
	//! Carburant, camp, planque ou base sur un esclave.
	//! La radio est déjà posée dans le monde : on branche celle-là, sans en poser une seconde.
	protected static void PlaceSlave(HAM_Point slave)
	{
		vector pos = slave.Pos();
		HAM_EMasterType type = slave.m_Def.m_eType;
		if (type == HAM_EMasterType.FUEL)
		{
			IEntity fuel = KeepComposition(FUEL, FUEL_PLAYER, pos, pos + Vector(16, 0, 0), 90, 8, slave.m_Def.m_sName);
			if (fuel)
				slave.m_aTypeProps.Insert(fuel);

			OpenBuilt(fuel, FactionKeyOf(slave.m_bPlayer));
		}
		else if (type == HAM_EMasterType.CAMP)
		{
			IEntity barracks = KeepComposition(BARRACKS, BARRACKS_PLAYER, pos, pos + Vector(22, 0, 14), 0, 18, slave.m_Def.m_sName);
			if (barracks)
				slave.m_aTypeProps.Insert(barracks);

			OpenBuilt(barracks, FactionKeyOf(slave.m_bPlayer));
			ScheduleBarracksCap(barracks, slave.m_bPlayer);
		}
		else if (type == HAM_EMasterType.RADIO)
			HookSlaveRadio(slave);
		else if (type == HAM_EMasterType.HIDEOUT)
			slave.m_Cache = PlaceHideout(pos, slave.Radius(), slave.m_Def.m_sName);
		else if (type == HAM_EMasterType.BASE)
		{
			RememberSlave(slave, GARAGE_LIGHT, pos, pos + Vector(16, 0, 8), 90, 8);
			RememberSlave(slave, GARAGE_HEAVY, pos, pos + Vector(16, 0, -10), 90, 12);
			RememberSlave(slave, REPAIR_RAMP, pos, pos + Vector(28, 0, 0), 0, 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<IEntity> s_aRadioHits;
	protected static ref array<HAM_Point> s_aRadioWait;
	protected static ref map<string, int> s_mRadioTries;
	protected static bool s_bRadioRetry;

	//------------------------------------------------------------------------------------------------
	//! L'antenne du calque (Antenna_S_US) entre dans la couverture, comme une antenne construite.
	protected static void HookSlaveRadio(HAM_Point slave)
	{
		if (!slave || !slave.m_Def)
			return;

		IEntity radio = FindPlacedRadio(slave.Pos());
		if (!radio)
		{
			WaitForSlaveRadio(slave);
			return;
		}

		HAM_RadioLink.RememberAntenna(radio);
		HAM_RadioLink.Apply(radio);
		HAM_Util.Info(string.Format("Radio esclave %1 : antenne déjà posée branchée", slave.m_Def.m_sName));
	}

	//------------------------------------------------------------------------------------------------
	protected static void WaitForSlaveRadio(HAM_Point slave)
	{
		if (!slave || !slave.m_Def)
			return;

		if (!s_mRadioTries)
			s_mRadioTries = new map<string, int>();

		string name = slave.m_Def.m_sName;
		int tries = 0;
		if (s_mRadioTries.Contains(name))
			tries = s_mRadioTries.Get(name);

		tries++;
		s_mRadioTries.Set(name, tries);
		if (tries >= 6)
		{
			Print(string.Format("[13RDPA H&M] Pas de radio déjà posée pour %1 : esclave ignoré", name), LogLevel.WARNING);
			return;
		}

		if (!s_aRadioWait)
			s_aRadioWait = {};

		if (s_aRadioWait.Find(slave) < 0)
			s_aRadioWait.Insert(slave);

		if (s_bRadioRetry || !GetGame() || !GetGame().GetCallqueue())
			return;

		s_bRadioRetry = true;
		GetGame().GetCallqueue().CallLater(RetrySlaveRadios, 2000, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void RetrySlaveRadios()
	{
		s_bRadioRetry = false;
		if (!s_aRadioWait)
			return;

		array<HAM_Point> waiting = {};
		foreach (HAM_Point slave : s_aRadioWait)
		{
			if (slave)
				waiting.Insert(slave);
		}

		s_aRadioWait.Clear();
		foreach (HAM_Point slave : waiting)
			HookSlaveRadio(slave);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SeePlacedRadio(IEntity ent)
	{
		if (!ent || IsRelay(ent))
			return true;

		if (HasRadio(ent) || PrefabHasAntenna(ent))
			s_aRadioHits.Insert(ent);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity FindPlacedRadio(vector pos)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return null;

		s_aRadioHits = {};
		world.QueryEntitiesBySphere(pos, 50, SeePlacedRadio);

		IEntity best;
		float bestSq = 0;
		IEntity antenna;
		float antennaSq = 0;
		foreach (IEntity ent : s_aRadioHits)
		{
			if (!ent)
				continue;

			IEntity node = RadioNode(ent);
			if (node)
			{
				float distSq = vector.DistanceSqXZ(node.GetOrigin(), pos);
				if (!best || distSq < bestSq)
				{
					best = node;
					bestSq = distSq;
				}

				continue;
			}

			if (!PrefabHasAntenna(ent))
				continue;

			float placedSq = vector.DistanceSqXZ(ent.GetOrigin(), pos);
			if (!antenna || placedSq < antennaSq)
			{
				antenna = ent;
				antennaSq = placedSq;
			}
		}

		if (best && vector.DistanceXZ(best.GetOrigin(), pos) <= 80)
			return best;

		if (antenna && vector.DistanceXZ(antenna.GetOrigin(), pos) <= 80)
			return antenna;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity RadioNode(IEntity ent)
	{
		if (!ent || IsRelay(ent))
			return null;

		if (HasRadio(ent))
			return ent;

		IEntity child = ent.GetChildren();
		while (child)
		{
			IEntity found = RadioNode(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HasRadio(IEntity ent)
	{
		if (!ent || IsRelay(ent))
			return false;

		if (ent.FindComponent(BaseRadioComponent))
			return true;

		if (ent.FindComponent(SCR_CoverageRadioComponent))
			return true;

		if (ent.FindComponent(SCR_AntennaServicePointComponent))
			return true;

		SCR_ServicePointComponent service = SCR_ServicePointComponent.Cast(ent.FindComponent(SCR_ServicePointComponent));
		return service && service.GetType() == SCR_EServicePointType.RADIO_ANTENNA;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool PrefabHasAntenna(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return false;

		return ent.GetPrefabData().GetPrefabName().Contains("Antenna_S_US");
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsRelay(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return false;

		return ent.GetPrefabData().GetPrefabName().Contains("HAM_RadioRelay");
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberSlave(HAM_Point slave, ResourceName prefab, vector zone, vector pos, float yaw, float footprint)
	{
		bool spawned = false;
		IEntity prop = TakeExisting(prefab, zone, PREFAB_SEARCH);
		if (!prop)
		{
			prop = HAM_Util.Spawn(prefab, pos, yaw);
			spawned = true;
		}

		if (!prop)
		{
			string label = HAM_Util.PrefabFileName(prefab);
			if (slave && slave.m_Def)
				label = slave.m_Def.m_sName + " " + label;

			HAM_Mission.FailZonePrefab(label);
		}
		else
			HAM_Mission.CountZonePrefab(spawned);

		if (prop)
			slave.m_aTypeProps.Insert(prop);
	}

	//------------------------------------------------------------------------------------------------
	static void AdoptSlave(HAM_Point slave, bool player)
	{
		if (!slave)
			return;

		string factionKey = HAM_Units.ENEMY_FACTION;
		if (player)
			factionKey = HAM_Units.PLAYER_FACTION;

		for (int i = 0; i < slave.m_aTypeProps.Count(); i++)
		{
			IEntity prop = slave.m_aTypeProps[i];
			HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
			OpenBuilt(prop, factionKey);

			if (slave.m_Def.m_eType == HAM_EMasterType.CAMP)
				ScheduleBarracksCap(slave.m_aTypeProps[i], player);
		}

		if (slave.m_Def.m_eType == HAM_EMasterType.SUPPLY_VANILLA && slave.m_Prop)
		{
			HAM_MasterLinkComponent.SetEntityFaction(slave.m_Prop, factionKey);
			OpenBuilt(slave.m_Prop, factionKey);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Table de la caserne : quatre groupes une fois le camp pris. La garnison ennemie ne change pas.
	static void ScheduleBarracksCap(IEntity prop, bool player)
	{
		if (!prop)
			return;

		if (player)
			GetGame().GetCallqueue().CallLater(ApplyBarracksCapPlayer, 3000, false, prop);
		else
			GetGame().GetCallqueue().CallLater(ApplyBarracksCapEnemy, 3000, false, prop);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyBarracksCapPlayer(IEntity prop)
	{
		WalkBarracks(prop, true);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ApplyBarracksCapEnemy(IEntity prop)
	{
		WalkBarracks(prop, false);
	}

	//------------------------------------------------------------------------------------------------
	protected static void WalkBarracks(IEntity ent, bool player)
	{
		if (!ent)
			return;

		SCR_ServicePointComponent service = SCR_ServicePointComponent.Cast(ent.FindComponent(SCR_ServicePointComponent));
		SCR_CampaignBuildingProviderComponent provider = SCR_CampaignBuildingProviderComponent.Cast(ent.FindComponent(SCR_CampaignBuildingProviderComponent));
		if (service && provider && service.GetType() == SCR_EServicePointType.BARRACKS)
			HAM_BarracksCap.Apply(provider, player, CAMP_DEPLOY_GROUPS);

		IEntity child = ent.GetChildren();
		while (child)
		{
			WalkBarracks(child, player);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsBarracksProp(IEntity prop)
	{
		if (!prop || !prop.GetPrefabData())
			return false;

		string name = prop.GetPrefabData().GetPrefabName();
		return name.Contains("LivingArea_L_Conflict");
	}

	//------------------------------------------------------------------------------------------------
	static bool IsCommandPost(IEntity prop)
	{
		if (!prop || !prop.GetPrefabData())
			return false;

		return prop.GetPrefabData().GetPrefabName().Contains("Headquarters_S_Conflict");
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	static string FactionKeyOf(bool player)
	{
		if (player)
			return HAM_Units.PLAYER_FACTION;

		return HAM_Units.ENEMY_FACTION;
	}

	//------------------------------------------------------------------------------------------------
	//! Copie stable : GetPrefabName() peut partager un tampon avec la ResourceName suivante.
	protected static string PrefabNameOf(IEntity prop)
	{
		if (!prop || !prop.GetPrefabData())
			return string.Empty;

		string name = prop.GetPrefabData().GetPrefabName();
		int length = name.Length();
		if (length < 1)
			return string.Empty;

		return name.Substring(0, length);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool NameHas(string name, string token)
	{
		if (name.IsEmpty())
			return false;

		return name.Contains(token);
	}

	//------------------------------------------------------------------------------------------------
	//! Le préfab USSR contient la chaîne « US » : on teste USSR d'abord.
	protected static bool IsPlayerComposition(IEntity prop)
	{
		string name = PrefabNameOf(prop);
		return NameHas(name, "Conflict_USSR") || NameHas(name, "_USSR_");
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsEnemyComposition(IEntity prop)
	{
		if (IsPlayerComposition(prop))
			return false;

		string name = PrefabNameOf(prop);
		return NameHas(name, "Conflict_US") || NameHas(name, "_US_");
	}

	//------------------------------------------------------------------------------------------------
	static IEntity SwapBarracks(IEntity prop, bool player)
	{
		ResourceName prefab = BARRACKS;
		if (player)
			prefab = BARRACKS_PLAYER;

		return ReplaceComposition(prop, prefab, player);
	}

	//------------------------------------------------------------------------------------------------
	static IEntity SwapCommandPost(IEntity prop, bool player)
	{
		ResourceName prefab = COMMAND_POST;
		if (player)
			prefab = COMMAND_POST_PLAYER;

		IEntity next = ReplaceComposition(prop, prefab, player);
		IEntity logic = FindPrefabBase(next);
		if (logic)
			HAM_Util.Delete(logic);

		return next;
	}

	//------------------------------------------------------------------------------------------------
	//! Remplace une composition par celle du camp, au même endroit, puis la branche sur la base.
	//! Le nouveau préfab est posé avant d'effacer l'ancien : un préfab introuvable ne laisse pas le trou.
	static IEntity ReplaceComposition(IEntity prop, ResourceName prefab, bool player)
	{
		string factionKey = FactionKeyOf(player);
		bool already = IsEnemyComposition(prop);
		if (player)
			already = IsPlayerComposition(prop);

		if (already)
		{
			HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
			OpenBuilt(prop, factionKey);
			return prop;
		}

		vector pos = vector.Zero;
		float yaw = 0;
		if (prop)
		{
			vector transform[4];
			prop.GetWorldTransform(transform);
			pos = prop.GetOrigin();
			yaw = Math3D.MatrixToAngles(transform)[0];
		}

		IEntity next = HAM_Util.Spawn(prefab, pos, yaw);
		if (!next)
		{
			Print(string.Format("[13RDPA H&M] Composition %1 introuvable, l'ancienne reste en place", prefab), LogLevel.WARNING);
			if (prop)
			{
				HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
				OpenBuilt(prop, factionKey);
			}

			return prop;
		}

		if (prop)
		{
			HAM_Util.Forget(prop);
			HAM_Util.Delete(prop);
		}

		HAM_MasterLinkComponent.SetEntityFaction(next, factionKey);
		OpenBuilt(next, factionKey);
		if (IsBarracksProp(next))
			Print(string.Format("[13RDPA H&M] Caserne passée au camp %1", factionKey), LogLevel.NORMAL);

		return next;
	}

	//------------------------------------------------------------------------------------------------
	//! Composition éditable du jeu : interactions, services et zone de construction, reliée à la base la plus proche.
	//! La faction est reposée après la recréation des pièces : sinon le préfab respawn garde le camp US.
	static void OpenBuilt(IEntity ent, string factionKey = "")
	{
		if (!ent)
			return;

		GetGame().GetCallqueue().CallLater(OpenBuiltLater, 1200, false, ent, factionKey);
		if (!factionKey.IsEmpty())
			GetGame().GetCallqueue().CallLater(ReinforceCamp, 2800, false, ent, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	protected static void OpenBuiltLater(IEntity ent, string factionKey)
	{
		if (!ent)
			return;

		DropBakedDuplicates(ent);
		RemoveNestedBases(ent);
		SCR_CampaignBuildingCompositionComponent composition = SCR_CampaignBuildingCompositionComponent.Cast(ent.FindComponent(SCR_CampaignBuildingCompositionComponent));
		if (composition)
			composition.SetInteractionLockServer(false);

		SCR_CampaignMilitaryBaseComponent base = HAM_Mission.FindBaseNear(ent.GetOrigin(), 2500);
		if (base)
		{
			IEntity provider = ProviderNear(base.GetOwner().GetOrigin());
			if (!provider)
				provider = base.GetOwner();

			if (composition)
				composition.SetProviderEntity(provider);

			BindServices(ent, base);
		}

		ReinforceCamp(ent, factionKey);
	}

	//------------------------------------------------------------------------------------------------
	//! Deuxième passage : les pièces du préfab éditable naissent après le premier SetEntityFaction.
	protected static void ReinforceCamp(IEntity ent, string factionKey)
	{
		if (!ent || factionKey.IsEmpty())
			return;

		HAM_MasterLinkComponent.SetEntityFaction(ent, factionKey);
		bool player = factionKey == HAM_Units.PLAYER_FACTION;
		WalkBarracks(ent, player);
		if (IsCommandPost(ent))
			NameDeployPoints(ent, string.Empty);
	}

	//------------------------------------------------------------------------------------------------
	//! Le préfab éditable recrée ses pièces. Les copies déjà cuites dans le parent feraient doublon.
	protected static void DropBakedDuplicates(IEntity ent)
	{
		SCR_EditorLinkComponent link = SCR_EditorLinkComponent.Cast(ent.FindComponent(SCR_EditorLinkComponent));
		if (!link || !link.IsSpawned())
			return;

		array<IEntity> linked = link.GetLinkedChildren();
		if (!linked || linked.IsEmpty())
			return;

		array<IEntity> drop = {};
		IEntity child = ent.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			bool keep = linked.Contains(child);
			if (!keep && !SCR_CampaignBuildingLayoutComponent.Cast(child.FindComponent(SCR_CampaignBuildingLayoutComponent)) && SamePrefabAsLinked(child, linked))
				drop.Insert(child);

			child = next;
		}

		foreach (IEntity old : drop)
			HAM_Util.Delete(old);
	}

	//------------------------------------------------------------------------------------------------
	//! Une pièce recréée par le préfab éditable a le même prefab qu'une pièce déjà liée.
	//! Une tente ou un bunker ajouté dans l'héritier n'a pas ce double : on le garde.
	protected static bool SamePrefabAsLinked(IEntity child, array<IEntity> linked)
	{
		if (!child || !child.GetPrefabData())
			return false;

		string name = child.GetPrefabData().GetPrefabName();
		foreach (IEntity live : linked)
		{
			if (!live || live.IsDeleted() || live == child || !live.GetPrefabData())
				continue;

			if (live.GetPrefabData().GetPrefabName() == name)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity s_ProviderFound;

	//------------------------------------------------------------------------------------------------
	protected static bool SeeProvider(IEntity ent)
	{
		if (ent && ent.FindComponent(SCR_CampaignBuildingProviderComponent))
			s_ProviderFound = ent;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity ProviderNear(vector pos)
	{
		s_ProviderFound = null;
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 120, SeeProvider);
		return s_ProviderFound;
	}

	//------------------------------------------------------------------------------------------------
	protected static void BindServices(IEntity ent, SCR_CampaignMilitaryBaseComponent base)
	{
		if (!ent || !base)
			return;

		SCR_ServicePointComponent service = SCR_ServicePointComponent.Cast(ent.FindComponent(SCR_ServicePointComponent));
		if (service)
		{
			base.RegisterLogicComponent(service);
			service.RegisterBase(base);
			service.SetServiceOnline();
		}

		IEntity child = ent.GetChildren();
		while (child)
		{
			BindServices(child, base);
			child = child.GetSibling();
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsFuelProp(IEntity prop)
	{
		if (!prop || !prop.GetPrefabData())
			return false;

		string name = prop.GetPrefabData().GetPrefabName();
		return name.Contains("FuelStorage_S_US_01") || name.Contains("FuelStorage_S_USSR_01") || name.Contains("HAM_FuelStation");
	}

	//------------------------------------------------------------------------------------------------
	//! Le stockage US devient le stockage USSR à la prise, et l'inverse si la zone retombe.
	static IEntity SwapFuelProp(IEntity prop, string factionKey)
	{
		ResourceName prefab = FUEL;
		if (factionKey == HAM_Units.PLAYER_FACTION)
			prefab = FUEL_PLAYER;

		if (!prop)
			return null;

		string name = string.Empty;
		if (prop.GetPrefabData())
			name = prop.GetPrefabData().GetPrefabName();

		bool already;
		if (factionKey == HAM_Units.PLAYER_FACTION)
			already = name.Contains("FuelStorage_S_USSR_01");
		else
			already = name.Contains("FuelStorage_S_US_01") && !name.Contains("FuelStorage_S_USSR_01");

		if (already)
		{
			HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
			OpenBuilt(prop, factionKey);
			return prop;
		}

		vector transform[4];
		prop.GetWorldTransform(transform);
		vector pos = prop.GetOrigin();
		float yaw = Math3D.MatrixToAngles(transform)[0];
		IEntity next = HAM_Util.Spawn(prefab, pos, yaw);
		if (!next)
		{
			Print(string.Format("[13RDPA H&M] Stockage %1 introuvable, l'ancien reste en place", prefab), LogLevel.WARNING);
			HAM_MasterLinkComponent.SetEntityFaction(prop, factionKey);
			OpenBuilt(prop, factionKey);
			return prop;
		}

		HAM_Util.Forget(prop);
		HAM_Util.Delete(prop);
		HAM_MasterLinkComponent.SetEntityFaction(next, factionKey);
		OpenBuilt(next, factionKey);
		return next;
	}

	//------------------------------------------------------------------------------------------------
	//! Caisse dans une pièce du rez-de-chaussée. Jamais dans un mur, jamais au grenier.
	//! Sans pièce, dehors, à moins de 40 m du centre.
	protected static IEntity PlaceHideout(vector pos, float radius, string name)
	{
		array<SCR_DestructibleBuildingEntity> buildings = {};
		HAM_Interior.CollectBuildings(pos, radius, buildings);

		bool alive;
		vector saved;
		if (HAM_Save.CachePlan(name, alive, saved))
		{
			if (!alive)
			{
				Print(string.Format("[13RDPA H&M] Cache de planque détruite, non reposée : %1", name), LogLevel.NORMAL);
				return null;
			}

			IEntity restored = SpawnHideout(saved, name, "reprise");
			if (restored)
				return restored;
		}

		for (int attempt = 0; attempt < 24 && !buildings.IsEmpty(); attempt++)
		{
			SCR_DestructibleBuildingEntity house = buildings.GetRandomElement();
			vector spot;
			if (!HAM_Interior.FindSpot(house, null, spot))
				continue;

			if (!HideoutRoom(house, spot))
				continue;

			IEntity cache = SpawnHideout(spot, name, "pièce");
			if (cache)
				return cache;
		}

		IEntity outside = PlaceHideoutOutside(pos, buildings, name);
		if (outside)
			return outside;

		Print(string.Format("[13RDPA H&M] Planque %1 : aucune pièce ni sol proche pour la cache", name), LogLevel.WARNING);
		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Dehors, pas dans une pièce, pas plus loin que 40 m du centre.
	protected static IEntity PlaceHideoutOutside(vector pos, array<SCR_DestructibleBuildingEntity> buildings, string name)
	{
		float maxDist = 40;
		for (int outsideTry = 0; outsideTry < 16; outsideTry++)
		{
			vector outside = HAM_Util.DryAround(pos, 6, maxDist);
			if (!HideoutOutsideOk(pos, outside, buildings, maxDist))
				continue;

			IEntity cache = SpawnHideout(outside, name, "dehors");
			if (cache)
				return cache;
		}

		for (int loose = 0; loose < 12; loose++)
		{
			vector outside = HAM_Util.DryAround(pos, 4, maxDist);
			if (vector.DistanceXZ(pos, outside) > maxDist)
				continue;

			if (!HideoutGroundOk(outside))
				continue;

			IEntity cache = SpawnHideout(outside, name, "sol proche");
			if (cache)
				return cache;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HideoutRoom(SCR_DestructibleBuildingEntity house, vector spot)
	{
		if (!house)
			return false;

		if (!HideoutGroundOk(spot) || HAM_CQBInterior.IsUnderRoof(house, spot))
			return false;

		return HAM_CQBInterior.IsEnclosed(house, spot);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool HideoutOutsideOk(vector center, vector spot, array<SCR_DestructibleBuildingEntity> buildings, float maxDist)
	{
		if (vector.DistanceXZ(center, spot) > maxDist)
			return false;

		if (!HideoutGroundOk(spot))
			return false;

		SCR_DestructibleBuildingEntity house = HideoutHouse(spot, buildings);
		if (!house)
			return true;

		if (HAM_CQBInterior.IsEnclosed(house, spot) || HAM_CQBInterior.IsUnderRoof(house, spot))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_DestructibleBuildingEntity HideoutHouse(vector spot, array<SCR_DestructibleBuildingEntity> buildings)
	{
		if (!buildings)
			return null;

		foreach (SCR_DestructibleBuildingEntity house : buildings)
		{
			if (!house)
				continue;

			vector mins;
			vector maxs;
			house.GetBounds(mins, maxs);
			vector local = house.CoordToLocal(spot);
			if (local[0] < mins[0] || local[0] > maxs[0])
				continue;

			if (local[2] < mins[2] || local[2] > maxs[2])
				continue;

			return house;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity SpawnHideout(vector spot, string name, string where)
	{
		IEntity cache = HAM_Util.Spawn(HAM_Units.MISSION_CACHE, spot, Math.RandomFloat(0, 360));
		if (!cache)
			return null;

		Print(string.Format("[13RDPA H&M] Cache de planque %1 : %2", where, name), LogLevel.NORMAL);
		return cache;
	}

	//------------------------------------------------------------------------------------------------
	//! Sol, pas dans l'eau, pas à l'étage, pas déjà dans un mur. Le test se fait avant la pose :
	//! après, la trace touche la caisse elle-même et la ferait supprimer.
	protected static bool HideoutGroundOk(vector spot)
	{
		if (HAM_Util.IsWater(spot) || HAM_CQBInterior.IsRaisedFloor(spot))
			return false;

		return !HAM_Interior.SpotEmbedded(spot);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignMilitaryBaseComponent
{
	[RplProp()]
	protected string m_sHAM_ZoneName;

	// Portée plancher imposée par H&M. RecalculateRadioRange ne doit pas la redescendre.
	protected float m_fHAM_RadioFloor;

	// La clé posée par SetEncryptionKey ne part pas toujours au client. On la réplique à part.
	[RplProp(onRplName: "HAM_ApplyRadioKey")]
	protected string m_sHAM_RadioKey;

	void HAM_SetName(string name)
	{
		m_sBaseName = name;
		m_sBaseNameUpper = name;
		m_sBaseNameUpper.ToUpper();
		m_sHAM_ZoneName = name;
		Replication.BumpMe();

		SCR_CampaignMapUIBase icon = GetMapUI();
		if (icon)
			icon.HAM_ApplyZoneName(name);
	}

	//------------------------------------------------------------------------------------------------
	override string GetBaseName()
	{
		if (!m_sHAM_ZoneName.IsEmpty())
			return m_sHAM_ZoneName;

		return super.GetBaseName();
	}

	//------------------------------------------------------------------------------------------------
	override string GetBaseNameUpperCase()
	{
		if (!m_sHAM_ZoneName.IsEmpty())
		{
			string upper = m_sHAM_ZoneName;
			upper.ToUpper();
			return upper;
		}

		return super.GetBaseNameUpperCase();
	}

	//------------------------------------------------------------------------------------------------
	//! EOnInit force l'icône visible. On la recache tout de suite si le maître n'est pas découvert.
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (HAM_MasterBases.MapHidden(this))
			HAM_MasterBases.ApplyMapVisibility(this, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Étend la radio de la base et envoie portée + clé aux clients. Ne raccourcit jamais.
	void HAM_PublishRadio(float range, string key)
	{
		if (!HAM_Mission.IsActive() || !Replication.IsServer())
			return;

		if (range < m_fRadioRange)
			range = m_fRadioRange;

		if (range < m_fHAM_RadioFloor)
			range = m_fHAM_RadioFloor;

		if (range < 100)
			return;

		m_fHAM_RadioFloor = range;
		if (m_RadioComponent)
		{
			m_RadioComponent.SetPower(true);
			if (!key.IsEmpty())
			{
				m_RadioComponent.SetEncryptionKey(key);
				m_sHAM_RadioKey = key;
			}

			BaseTransceiver transceiver = m_RadioComponent.GetTransceiver(0);
			if (transceiver && transceiver.GetRange() < range)
				transceiver.SetRange(range);
		}

		bool rangeChanged = m_fRadioRange != range;
		m_fRadioRange = range;
		Replication.BumpMe();
		if (rangeChanged)
			OnRadioRangeChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Côté client : la portée du préfab (1 km) restait en place, le QG à 3 km ne suffisait pas
	//! pour répondre, et la base passait « hors de portée ».
	void HAM_ApplyRadioKey()
	{
		if (!m_RadioComponent)
			return;

		if (!m_sHAM_RadioKey.IsEmpty())
			m_RadioComponent.SetEncryptionKey(m_sHAM_RadioKey);

		if (m_fRadioRange <= 0)
			return;

		BaseTransceiver transceiver = m_RadioComponent.GetTransceiver(0);
		if (transceiver && transceiver.GetRange() < m_fRadioRange)
			transceiver.SetRange(m_fRadioRange);

		m_RadioComponent.SetPower(true);
	}

	//------------------------------------------------------------------------------------------------
	override void OnRadioRangeChanged()
	{
		if (HAM_Mission.IsActive())
			HAM_ApplyRadioKey();

		super.OnRadioRangeChanged();
	}

	//------------------------------------------------------------------------------------------------
	//! Repose le plancher. Une valeur plus basse est acceptée : les relais encore posés ne sont pas touchés ici.
	void HAM_SetRadioFloor(float floor)
	{
		if (!HAM_Mission.IsActive() || !Replication.IsServer())
			return;

		if (floor < 0)
			floor = 0;

		m_fHAM_RadioFloor = floor;
		if (m_fRadioRange > floor)
			m_fRadioRange = floor;

		if (m_RadioComponent)
		{
			BaseTransceiver current = m_RadioComponent.GetTransceiver(0);
			if (current && current.GetRange() > floor)
				current.SetRange(floor);
		}

		RecalculateRadioRange();

		float applied = m_fRadioRange;
		if (applied < floor)
			applied = floor;

		if (applied > m_fHAM_RadioFloor)
			m_fHAM_RadioFloor = applied;

		m_fRadioRange = applied;
		if (m_RadioComponent)
		{
			BaseTransceiver transceiver = m_RadioComponent.GetTransceiver(0);
			if (transceiver)
				transceiver.SetRange(applied);

			m_RadioComponent.SetPower(true);
		}

		Replication.BumpMe();
		OnRadioRangeChanged();
	}

	//------------------------------------------------------------------------------------------------
	override void RecalculateRadioRange()
	{
		float floor = 0;
		if (HAM_Mission.IsActive())
			floor = m_fHAM_RadioFloor;

		super.RecalculateRadioRange();

		if (floor <= m_fRadioRange)
			return;

		Faction faction = GetFaction();
		if (!faction || !HAM_Units.IsPlayerFaction(faction.GetFactionKey()))
			return;

		HAM_PublishRadio(floor, m_sHAM_RadioKey);
	}

	//------------------------------------------------------------------------------------------------
	protected static ref array<SCR_CampaignMilitaryBaseComponent> s_aHAM_RetainSpawn;

	//------------------------------------------------------------------------------------------------
	//! Ancien QG encore tenu : le point reste au camp joueur même sans le passe-droit « est QG ».
	static void HAM_RetainSpawn(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base)
			return;

		if (!s_aHAM_RetainSpawn)
			s_aHAM_RetainSpawn = {};

		if (s_aHAM_RetainSpawn.Find(base) < 0)
			s_aHAM_RetainSpawn.Insert(base);
	}

	//------------------------------------------------------------------------------------------------
	override protected void HandleSpawnPointFaction()
	{
		super.HandleSpawnPointFaction();

		if (!HAM_Mission.IsActive() || !m_SpawnPoint || m_bIsHQ)
			return;

		if (!s_aHAM_RetainSpawn || s_aHAM_RetainSpawn.Find(this) < 0)
			return;

		SCR_CampaignFaction holder = GetCampaignFaction();
		if (!holder || holder.GetFactionKey() != HAM_Units.PLAYER_FACTION)
			return;

		if (m_eCaptureState != SCR_EBaseCaptureState.NONE)
			return;

		if (!GetOwner())
			return;

		ChimeraWorld world = GetOwner().GetWorld();
		if (world && world.GetServerTimestamp().Less(m_fRespawnAvailableSince))
			return;

		if (m_SpawnPoint.GetFactionKey() != HAM_Units.PLAYER_FACTION)
			m_SpawnPoint.SetFactionKey(HAM_Units.PLAYER_FACTION);

		if (!m_SpawnPoint.IsSpawnPointEnabled())
			m_SpawnPoint.SetSpawnPointEnabled_S(true);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignSeizingComponent
{
	void HAM_SetRadius(int radius)
	{
		if (radius > 0)
			m_iRadius = radius;
	}

	//------------------------------------------------------------------------------------------------
	void HAM_SetDuration(float seconds)
	{
		if (seconds < 5)
			return;

		m_fMaximumSeizingTime = seconds;
		m_fMinimumSeizingTime = seconds;
	}

	//------------------------------------------------------------------------------------------------
	protected override SCR_Faction EvaluateEntityFaction(IEntity ent)
	{
		SCR_Faction faction = super.EvaluateEntityFaction(ent);
		if (!faction)
			return null;

		if (faction.GetFactionKey() == HAM_Units.CIVILIAN_FACTION)
			return null;

		return faction;
	}

	//------------------------------------------------------------------------------------------------
	override protected void EvaluatePrevailingFaction()
	{
		HAM_MasterLinkComponent link = HAM_MasterLinkComponent.Cast(GetOwner().FindComponent(HAM_MasterLinkComponent));
		if (link)
		{
			HAM_Cell cell = HAM_Mission.FindCell(link.CellName());
			if (!cell || HAM_MasterBases.CacheStanding(cell.m_Master) || !cell.m_bRevealed || !HAM_Mission.IsFront(cell) || !HAM_MasterBases.RadioLinked(cell))
			{
				if (cell && cell.m_Master && HAM_Mission.s_aPlayerBodies && HAM_Util.AnyNear(HAM_Mission.s_aPlayerBodies, cell.m_Master.Pos(), cell.m_Master.Radius()))
					HAM_MasterBases.ExplainBlock(cell.m_Master);

				// 1.9 finit la saisie dans EOnFrame dès que l'horodatage est posé.
				// Ne pas appeler super laisse ce timer courir : on l'interrompt, la règle de blocage reste la même.
				HAM_HaltBlockedSeizure();
				GetGame().GetCallqueue().CallLater(EvaluatePrevailingFaction, 2000, false);
				return;
			}
		}

		super.EvaluatePrevailingFaction();
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_HaltBlockedSeizure()
	{
		bool timerLive = true;
		if (m_fSeizingEndTimestamp == 0)
			timerLive = false;

		if (!m_PrevailingFaction && !timerLive && m_eCaptureState == SCR_EBaseCaptureState.NONE)
			return;

		if (m_PrevailingFaction || timerLive)
		{
			m_PrevailingFactionPrevious = m_PrevailingFaction;
			m_PrevailingFaction = null;
			m_iSeizingCharacters = 0;
			OnPrevailingFactionChanged();
		}

		if (m_eCaptureState != SCR_EBaseCaptureState.NONE)
		{
			m_eCaptureState = SCR_EBaseCaptureState.NONE;
			Rpc(RpcDo_OnCaptureStateChanged);
			RpcDo_OnCaptureStateChanged();
		}
	}
}
