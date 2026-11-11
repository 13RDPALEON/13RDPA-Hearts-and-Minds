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
				if (existingLink.CellName().IsEmpty())
					existingLink.Setup(cell.m_sName, Math.Round(cell.m_Master.Radius()));
				else
					existingLink.SetMapShown(cell.m_bRevealed || cell.IsMasterPlayer());

				if (cell.m_Master.m_bPlayer)
					Adopt(cell, true);

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
	//! Les 8 hommes d'un camp, versés à la prise du maître.
	static int CampMen()
	{
		return CAMP_MEN;
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
	//! Le préfab de poste a déjà son point de déploiement. Celui de la base, sans nom, est en trop.
	protected static void DropBaseSpawn(IEntity baseEntity)
	{
		if (!baseEntity)
			return;

		array<IEntity> drop = {};
		IEntity child = baseEntity.GetChildren();
		while (child)
		{
			IEntity next = child.GetSibling();
			if (child.FindComponent(SCR_SpawnPoint))
				drop.Insert(child);

			child = next;
		}

		foreach (IEntity spawn : drop)
			HAM_Util.Delete(spawn);
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
	//! La persistance du jeu a déjà remis ce prefab. On garde le plus proche et on le retire de sa sauvegarde.
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

		IEntity best = s_aSamePrefab[0];
		float bestSq = vector.DistanceSqXZ(best.GetOrigin(), pos);
		for (int i = 1; i < s_aSamePrefab.Count(); i++)
		{
			IEntity ent = s_aSamePrefab[i];
			if (!ent)
				continue;

			float distSq = vector.DistanceSqXZ(ent.GetOrigin(), pos);
			if (distSq < bestSq)
			{
				HAM_Util.Forget(best);
				HAM_Util.Delete(best);
				best = ent;
				bestSq = distSq;
			}
			else
			{
				HAM_Util.Forget(ent);
				HAM_Util.Delete(ent);
			}
		}

		HAM_Util.Forget(best);
		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Place(HAM_Cell cell)
	{
		vector pos = cell.Pos();
		int radius = Math.Round(cell.m_Master.Radius());
		IEntity post = KeepComposition(COMMAND_POST, COMMAND_POST_PLAYER, pos, 0, 16, "poste " + cell.m_sName);
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
			return;

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
		DropBaseSpawn(baseEntity);
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
		{
			NameDeployPoints(post, cell.m_sName);
			// Le poste recrée ses pièces après la pose. Le point de réapparition n'existe qu'alors.
			GetGame().GetCallqueue().CallLater(NameDeployPoints, 1600, false, post, cell.m_sName);
			GetGame().GetCallqueue().CallLater(NameDeployPoints, 4000, false, post, cell.m_sName);
		}

		Print(string.Format("[13RDPA H&M] Base maître posée : %1 (%2)", cell.m_sName, cell.m_Master.m_Def.TypeLabel()), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul point de réapparition par base maître : celui du poste de commandement,
	//! le plus proche de l'origine du poste s'il y en a plusieurs. La caserne et le reste
	//! de cette base sont coupés. Les points hors de cette hiérarchie ne sont pas touchés.
	protected static void NameDeployPoints(IEntity root, string name)
	{
		if (!root || name.IsEmpty())
			return;

		NameDeployNode(root, name);

		array<SCR_SpawnPoint> points = {};
		CollectBaseSpawns(root, points);
		if (points.IsEmpty())
			return;

		vector post = HeadquartersOrigin(root);
		SCR_SpawnPoint kept;
		float best = -1;
		foreach (SCR_SpawnPoint point : points)
		{
			if (!point || !SpawnInHeadquarters(point))
				continue;

			float dist = vector.Distance(point.GetOrigin(), post);
			if (best >= 0 && dist >= best)
				continue;

			best = dist;
			kept = point;
		}

		foreach (SCR_SpawnPoint point : points)
		{
			if (!point)
				continue;

			if (point == kept)
			{
				point.SetSpawnPointEnabled_S(true);
				point.SetSpawnPointName(name);
				continue;
			}

			point.SetSpawnPointEnabled_S(false);
		}

		if (!kept)
			Print(string.Format("[13RDPA H&M] %1 : aucun point de réapparition dans le poste de commandement.", name), LogLevel.WARNING);
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
	protected static vector HeadquartersOrigin(IEntity root)
	{
		IEntity post = FindPrefabNode(root, "Headquarters_");
		if (post)
			return post.GetOrigin();

		return root.GetOrigin();
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity FindPrefabNode(IEntity node, string token)
	{
		if (!node)
			return null;

		if (node.GetPrefabData() && node.GetPrefabData().GetPrefabName().Contains(token))
			return node;

		IEntity child = node.GetChildren();
		while (child)
		{
			IEntity found = FindPrefabNode(child, token);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpawnInHeadquarters(SCR_SpawnPoint point)
	{
		IEntity node = point;
		while (node)
		{
			if (node.GetPrefabData() && node.GetPrefabData().GetPrefabName().Contains("Headquarters_"))
				return true;

			node = node.GetParent();
		}

		return false;
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

		SCR_ResourceContainer container = SuppliesContainer(barracks);
		if (!container)
		{
			if (tries < 10)
				GetGame().GetCallqueue().CallLater(PourBarracksLater, 1000, false, barracks, tries + 1);
			else
				Print("[13RDPA H&M] Caserne sans conteneur de fournitures", LogLevel.WARNING);

			return;
		}

		float current = container.GetResourceValue();
		if (current <= 0.01)
		{
			if (container.GetMaxResourceValue() < 900)
				container.SetMaxResourceValue(900, true);

			container.SetResourceValue(900, true);
		}
		else
		{
			float next = current + 900;
			if (container.GetMaxResourceValue() < next)
				container.SetMaxResourceValue(next, true);

			container.IncreaseResourceValue(900, true);
		}

		HAM_Mission.MarkDirty();
		HAM_Mission.SaveNow();
		Print(string.Format("[13RDPA H&M] Caserne : fournitures %1", Math.Round(container.GetResourceValue())), LogLevel.NORMAL);
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
			ClearCustomFuel(pos);
			IEntity fuel = KeepComposition(FUEL, FUEL_PLAYER, pos + Vector(16, 0, 0), 90, 8, cell.m_sName);
			if (link)
				link.RememberProp(fuel);

			OpenBuilt(fuel, FactionKeyOf(cell.m_Master.m_bPlayer));
		}
		else if (type == HAM_EMasterType.CAMP)
		{
			IEntity barracks = KeepComposition(BARRACKS, BARRACKS_PLAYER, pos + Vector(22, 0, 14), 0, 18, cell.m_sName);
			if (link)
				link.RememberProp(barracks);

			OpenBuilt(barracks, FactionKeyOf(cell.m_Master.m_bPlayer));
			ScheduleBarracksCap(barracks, cell.m_Master.m_bPlayer);
		}
		else if (type == HAM_EMasterType.RADIO)
			PlaceProp(link, ANTENNA, pos + Vector(14, 0, 6), 0, 6);
		else if (type == HAM_EMasterType.HIDEOUT)
			cell.m_Master.m_Cache = PlaceHideout(cell.Pos(), cell.m_Master.Radius(), cell.m_sName);
		else if (type == HAM_EMasterType.BASE)
		{
			PlaceProp(link, GARAGE_LIGHT, pos + Vector(16, 0, 8), 90, 8);
			PlaceProp(link, GARAGE_HEAVY, pos + Vector(16, 0, -10), 90, 12);
			PlaceProp(link, REPAIR_RAMP, pos + Vector(28, 0, 0), 0, 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Un seul préfab pour les deux camps. S'il y en a déjà un, US ou USSR, on le garde et on jette le double.
	protected static IEntity KeepComposition(ResourceName prefab, ResourceName other, vector pos, float yaw, float footprint, string label)
	{
		IEntity kept = TakeExisting(prefab, pos, 40);
		IEntity twin = TakeExisting(other, pos, 40);
		if (kept && twin)
		{
			HAM_Util.Forget(twin);
			HAM_Util.Delete(twin);
		}
		else if (!kept)
		{
			kept = twin;
		}

		if (!kept)
			kept = HAM_Util.SpawnLevelled(prefab, Fit(pos, footprint, label), yaw);

		return kept;
	}

	//------------------------------------------------------------------------------------------------
	protected static IEntity PlaceProp(HAM_MasterLinkComponent link, ResourceName prefab, vector pos, float yaw, float footprint)
	{
		IEntity prop = TakeExisting(prefab, pos, 40);
		if (!prop)
			prop = HAM_Util.SpawnLevelled(prefab, Fit(pos, footprint, prefab), yaw);
		if (link)
			link.RememberProp(prop);

		return prop;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector Fit(vector preferred, float footprint, string label)
	{
		vector spot;
		if (HAM_Util.FindSite(preferred, footprint, spot))
			return spot;

		Print(string.Format("[13RDPA H&M] Pas de sol plat pour %1, posé sur le moins pentu", label), LogLevel.WARNING);
		return HAM_Util.Flattest(preferred, footprint);
	}

	//------------------------------------------------------------------------------------------------
	//! Carburant, camp, planque ou base sur un esclave. La radio reste sur le maître.
	protected static void PlaceSlave(HAM_Point slave)
	{
		vector pos = slave.Pos();
		HAM_EMasterType type = slave.m_Def.m_eType;
		if (type == HAM_EMasterType.FUEL)
		{
			ClearCustomFuel(pos);
			IEntity fuel = KeepComposition(FUEL, FUEL_PLAYER, pos + Vector(16, 0, 0), 90, 8, slave.m_Def.m_sName);
			if (fuel)
				slave.m_aTypeProps.Insert(fuel);

			OpenBuilt(fuel, FactionKeyOf(slave.m_bPlayer));
		}
		else if (type == HAM_EMasterType.CAMP)
		{
			IEntity barracks = KeepComposition(BARRACKS, BARRACKS_PLAYER, pos + Vector(22, 0, 14), 0, 18, slave.m_Def.m_sName);
			if (barracks)
				slave.m_aTypeProps.Insert(barracks);

			OpenBuilt(barracks, FactionKeyOf(slave.m_bPlayer));
			ScheduleBarracksCap(barracks, slave.m_bPlayer);
		}
		else if (type == HAM_EMasterType.HIDEOUT)
			slave.m_Cache = PlaceHideout(pos, slave.Radius(), slave.m_Def.m_sName);
		else if (type == HAM_EMasterType.BASE)
		{
			RememberSlave(slave, GARAGE_LIGHT, pos + Vector(16, 0, 8), 90, 8);
			RememberSlave(slave, GARAGE_HEAVY, pos + Vector(16, 0, -10), 90, 12);
			RememberSlave(slave, REPAIR_RAMP, pos + Vector(28, 0, 0), 0, 14);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberSlave(HAM_Point slave, ResourceName prefab, vector pos, float yaw, float footprint)
	{
		IEntity prop = TakeExisting(prefab, pos, 40);
		if (!prop)
			prop = HAM_Util.SpawnLevelled(prefab, Fit(pos, footprint, slave.m_Def.m_sName), yaw);
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

		IEntity next = HAM_Util.SpawnLevelled(prefab, pos, yaw);
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
			if (!live || live == child || !live.GetPrefabData())
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
		IEntity next = HAM_Util.SpawnLevelled(prefab, pos, yaw);
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
	protected static ref array<IEntity> s_aOldFuel;

	//------------------------------------------------------------------------------------------------
	protected static bool SeeCustomFuel(IEntity ent)
	{
		if (!ent || !ent.GetPrefabData())
			return true;

		if (ent.GetPrefabData().GetPrefabName().Contains("HAM_FuelStation"))
			s_aOldFuel.Insert(ent);

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearCustomFuel(vector pos)
	{
		s_aOldFuel = {};
		GetGame().GetWorld().QueryEntitiesBySphere(pos, 40, SeeCustomFuel);
		foreach (IEntity old : s_aOldFuel)
			HAM_Util.Delete(old);
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

			SCR_DestructibleBuildingEntity savedHouse = HideoutHouse(saved, buildings);
			if (savedHouse && HideoutRoom(savedHouse, saved))
			{
				IEntity restored = SpawnHideout(saved, name, "reprise");
				if (restored)
					return restored;
			}

			Print(string.Format("[13RDPA H&M] Cache de planque %1 reprise hors d'une pièce, on la repose", name), LogLevel.WARNING);
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
