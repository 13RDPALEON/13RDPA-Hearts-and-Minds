//------------------------------------------------------------------------------------------------
// Fusionne, une fois par partie, les catalogues qui ne sont sur aucune faction vanilla
// (US, URSS, FIA, CIV) dans le catalogue commun. UK est injecté ici : ce n'est pas
// une faction de la partie. Serveur et clients : ces listes ne sont pas répliquées.
//
// Modèle : R.O.P.E.S charge par BaseContainerTools.LoadContainer, instancie, puis
// MergeCatalogs. On ne modde pas SCR_ArsenalItem. SCR_EntityCatalog ne reçoit ici
// aucun attribut : seulement de quoi retirer les entrées hors sujet et remplir
// m_mPrefabIndexes, que MergeCatalogs laisse inchangé. Sans cet index,
// GetEntryWithPrefab ne voit pas l'objet et le serveur refuse l'achat.
//------------------------------------------------------------------------------------------------

class HAM_CatalogMerge
{
	protected static ref map<ResourceName, int> s_mSkip;

	static void ClearSeen()
	{
		s_mSkip = new map<ResourceName, int>();
	}

	static void See(ResourceName prefab)
	{
		if (prefab.IsEmpty())
			return;

		if (!s_mSkip)
			ClearSeen();

		s_mSkip.Set(prefab, 1);
	}

	static bool Seen(ResourceName prefab)
	{
		if (!s_mSkip || prefab.IsEmpty())
			return false;

		int ignored = 0;
		return s_mSkip.Find(prefab, ignored);
	}

	static bool Wanted(SCR_EntityCatalogEntry entry, typename requiredData)
	{
		if (!entry || !entry.IsEnabled())
			return false;

		if (!entry.HasEntityDataOfType(requiredData))
			return false;

		ResourceName prefab = entry.GetPrefab();
		if (prefab.IsEmpty())
			return false;

		if (Seen(prefab))
			return false;

		return true;
	}
}

modded class SCR_EntityCatalog
{
	protected static ref SCR_EntityCatalog s_HAMEmpty;

	//------------------------------------------------------------------------------------------------
	//! Catalogue vide : l'inventaire cherche les caisses de fournitures même quand la partie n'en a pas.
	static SCR_EntityCatalog HAM_EmptyCatalog()
	{
		if (s_HAMEmpty)
			return s_HAMEmpty;

		s_HAMEmpty = new SCR_EntityCatalog();
		s_HAMEmpty.m_aEntityEntryList = {};
		if (!s_HAMEmpty.m_mPrefabIndexes)
			s_HAMEmpty.m_mPrefabIndexes = new map<ResourceName, int>();

		return s_HAMEmpty;
	}

	//------------------------------------------------------------------------------------------------
	//! Ne garde, dans la liste plate, que les entrées de .conf déjà dotées de la donnée demandée.
	void HAM_RetainEntries(typename requiredData)
	{
		if (!m_aEntityEntryList)
			m_aEntityEntryList = {};

		ref array<ref SCR_EntityCatalogEntry> kept = {};
		foreach (SCR_EntityCatalogEntry entry : m_aEntityEntryList)
		{
			if (!HAM_CatalogMerge.Wanted(entry, requiredData))
				continue;

			kept.Insert(entry);
			HAM_CatalogMerge.See(entry.GetPrefab());
		}

		m_aEntityEntryList = kept;
	}

	//------------------------------------------------------------------------------------------------
	//! Recopie les entrées des multi-listes dans la liste plate pour MergeCatalogs.
	void HAM_FlattenMultiLists()
	{
		SCR_EntityCatalogMultiList multi = SCR_EntityCatalogMultiList.Cast(this);
		if (!multi)
			return;

		if (!m_aEntityEntryList)
			m_aEntityEntryList = {};

		array<SCR_EntityCatalogMultiListEntry> lists = {};
		multi.GetMultiList(lists);
		foreach (SCR_EntityCatalogMultiListEntry list : lists)
		{
			if (!list || !list.m_aEntities)
				continue;

			foreach (SCR_EntityCatalogEntry entry : list.m_aEntities)
			{
				if (!entry)
					continue;

				string prefab = entry.GetPrefab();
				if (prefab.IsEmpty())
					continue;

				bool known = false;
				foreach (SCR_EntityCatalogEntry existing : m_aEntityEntryList)
				{
					if (existing && existing.GetPrefab() == prefab)
					{
						known = true;
						break;
					}
				}

				if (known)
					continue;

				m_aEntityEntryList.Insert(entry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Les entrées viennent d'un .conf : InitEntry une seule fois, puis l'index d'achat.
	int HAM_IndexFrom(int first)
	{
		if (!m_aEntityEntryList)
			return 0;

		int count = m_aEntityEntryList.Count();
		int added = 0;
		SCR_EntityCatalogEntry entry;
		for (int i = first; i < count; i++)
		{
			entry = m_aEntityEntryList[i];
			if (!entry)
				continue;

			if (entry.GetCatalogIndex() == -1)
				entry.InitEntry(this, i);

			HAM_RememberPrefab(entry, i);
			added = added + 1;
		}

		for (int j = first; j < count; j++)
		{
			entry = m_aEntityEntryList[j];
			if (!entry)
				continue;

			entry.PostInitEntry(this);
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	//! Entrées déjà dans la liste (fusion R.O.P.E.S comprise) mais absentes de l'index d'achat.
	//! Deux GUID pour le même fichier : on garde la première entrée, celle du catalogue d'origine.
	void HAM_DropDuplicatePaths()
	{
		if (!m_aEntityEntryList)
			return;

		ref array<ref SCR_EntityCatalogEntry> kept = {};
		ref set<string> seen = new set<string>();
		foreach (SCR_EntityCatalogEntry entry : m_aEntityEntryList)
		{
			if (!entry)
				continue;

			string prefab = entry.GetPrefab();
			string path = prefab;
			int brace = prefab.IndexOf("}");
			if (brace >= 0 && brace + 1 < prefab.Length())
				path = prefab.Substring(brace + 1, prefab.Length() - brace - 1);

			if (!path.IsEmpty() && seen.Contains(path))
				continue;

			if (!path.IsEmpty())
				seen.Insert(path);

			kept.Insert(entry);
		}

		m_aEntityEntryList = kept;
		if (!m_mPrefabIndexes)
			m_mPrefabIndexes = new map<ResourceName, int>();
		else
			m_mPrefabIndexes.Clear();

		for (int i = 0; i < kept.Count(); i++)
		{
			SCR_EntityCatalogEntry keptEntry = kept[i];
			if (!keptEntry)
				continue;

			ResourceName keptPrefab = keptEntry.GetPrefab();
			if (!keptPrefab.IsEmpty())
				m_mPrefabIndexes.Set(keptPrefab, i);
		}
	}

	//------------------------------------------------------------------------------------------------
	void HAM_IndexUnmapped()
	{
		if (!m_aEntityEntryList)
			return;

		int count = m_aEntityEntryList.Count();
		SCR_EntityCatalogEntry entry;
		for (int i = 0; i < count; i++)
		{
			entry = m_aEntityEntryList[i];
			if (!entry)
				continue;

			ResourceName prefab = entry.GetPrefab();
			if (prefab.IsEmpty() || GetEntryWithPrefab(prefab))
				continue;

			bool fresh = entry.GetCatalogIndex() == -1;
			if (fresh)
				entry.InitEntry(this, i);

			m_mPrefabIndexes.Set(prefab, i);

			if (fresh)
				entry.PostInitEntry(this);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_RememberPrefab(notnull SCR_EntityCatalogEntry entry, int index)
	{
		ResourceName prefab = entry.GetPrefab();
		if (prefab.IsEmpty())
			return;

		if (GetEntryWithPrefab(prefab))
			return;

		m_mPrefabIndexes.Set(prefab, index);
	}
}

modded class SCR_Faction
{
	override void Init(IEntity owner)
	{
		super.Init(owner);

		SCR_EntityCatalogManagerComponent manager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!manager)
			return;

		manager.HAM_ScheduleCatalogMerge();
	}
}

modded class SCR_EntityCatalogManagerComponent
{
	protected static bool s_bHAMCatalogMergeDone;
	protected static int s_iHAMMergeAttempts;
	protected int m_iHAMOwnLoaded;
	protected int m_iHAMCopyLoaded;
	protected int m_iHAMCatalogFailed;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		HAM_ScheduleCatalogMerge();
	}

	//------------------------------------------------------------------------------------------------
	//! Prendre la clé de réparation ouvre l'inventaire, qui cherche ce catalogue. Sans lui, la VM s'arrête.
	override SCR_EntityCatalog GetEntityCatalogOfType(EEntityCatalogType catalogType, bool printNotFound = true)
	{
		SCR_EntityCatalog catalog = super.GetEntityCatalogOfType(catalogType, false);
		if (catalog)
			return catalog;

		if (catalogType == EEntityCatalogType.SUPPLY_CONTAINER_ITEM)
			return SCR_EntityCatalog.HAM_EmptyCatalog();

		if (printNotFound)
			Print(string.Format("'SCR_EntityCatalogManagerComponent' trying to get entity list of type '%1' but there is no catalog with that type.", typename.EnumToString(EEntityCatalogType, catalogType)), LogLevel.WARNING);

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! R.O.P.E.S relit son catalogue à chaque ouverture d'arsenal. Ce fichier est un
	//! SCR_EntityCatalog, et cette classe est moddée ici : LoadContainer répond alors
	//! « Unknown class » et l'essai recommence. On lui donne la copie lisible.
	protected override SCR_EntityCatalog UniHook_LoadItemCatalog()
	{
		return HAM_LoadCatalog("{A13D0F0100000021}Configs/EntityCatalog/HAM/UniHook_InventoryItems.conf");
	}

	//------------------------------------------------------------------------------------------------
	//! Repousse la fusion après le dernier Init de faction, une seule fois.
	void HAM_ScheduleCatalogMerge()
	{
		if (s_bHAMCatalogMergeDone)
			return;

		if (SCR_Global.IsEditMode())
			return;

		GetGame().GetCallqueue().Remove(HAM_MergeCatalogsOnce);
		GetGame().GetCallqueue().CallLater(HAM_MergeCatalogsOnce, 100, false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_MergeCatalogsOnce()
	{
		if (s_bHAMCatalogMergeDone)
			return;

		if (SCR_Global.IsEditMode())
			return;

		SCR_EntityCatalog items = GetEntityCatalogOfType(EEntityCatalogType.ITEM, false);
		SCR_EntityCatalog vehicles = GetEntityCatalogOfType(EEntityCatalogType.VEHICLE, false);
		if (!items && !vehicles)
		{
			s_iHAMMergeAttempts = s_iHAMMergeAttempts + 1;
			if (s_iHAMMergeAttempts > 20)
			{
				s_bHAMCatalogMergeDone = true;
				Print("[HAM] Fusion catalogues abandonnée : catalogues communs absents.", LogLevel.WARNING);
				return;
			}

			GetGame().GetCallqueue().CallLater(HAM_MergeCatalogsOnce, 100, false);
			return;
		}

		s_bHAMCatalogMergeDone = true;

		array<ResourceName> ownItems = {};
		array<ResourceName> copyItems = {};
		array<ResourceName> ownVehicles = {};
		array<ResourceName> copyVehicles = {};
		HAM_FillOwnItems(ownItems);
		HAM_FillCopyItems(copyItems);
		HAM_FillOwnVehicles(ownVehicles);
		HAM_FillCopyVehicles(copyVehicles);

		HAM_CatalogMerge.ClearSeen();
		int itemCount = HAM_MergeGroup(ownItems, EEntityCatalogType.ITEM, SCR_ArsenalItem, true);
		itemCount = itemCount + HAM_MergeGroup(copyItems, EEntityCatalogType.ITEM, SCR_ArsenalItem, false);

		HAM_CatalogMerge.ClearSeen();
		int vehicleCount = HAM_MergeGroup(ownVehicles, EEntityCatalogType.VEHICLE, SCR_EntityCatalogSpawnerData, true);
		vehicleCount = vehicleCount + HAM_MergeGroup(copyVehicles, EEntityCatalogType.VEHICLE, SCR_EntityCatalogSpawnerData, false);

		if (items)
			items.HAM_DropDuplicatePaths();
		if (vehicles)
			vehicles.HAM_DropDuplicatePaths();

		HAM_VehicleBuildTri.ForgetIndex();

		Print("[HAM] Fusion catalogues : " + itemCount + " objets, " + vehicleCount + " véhicules. GUID propres chargés : " + m_iHAMOwnLoaded + ". Copies chargées : " + m_iHAMCopyLoaded + ". Illisibles : " + m_iHAMCatalogFailed + ".", LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected int HAM_MergeGroup(notnull array<ResourceName> sources, EEntityCatalogType catalogType, typename requiredData, bool ownGuid)
	{
		SCR_EntityCatalog dest = GetEntityCatalogOfType(catalogType, false);
		if (!dest)
			return 0;

		dest.HAM_IndexUnmapped();
		HAM_CollectPrefabs(dest);
		HAM_CollectFactionPrefabs(catalogType);

		int added = 0;
		foreach (ResourceName path : sources)
		{
			SCR_EntityCatalog source = HAM_LoadCatalog(path);
			if (!source)
			{
				m_iHAMCatalogFailed = m_iHAMCatalogFailed + 1;
				Print("[HAM] Catalogue illisible : " + path, LogLevel.WARNING);
				continue;
			}

			if (ownGuid)
				m_iHAMOwnLoaded = m_iHAMOwnLoaded + 1;
			else
				m_iHAMCopyLoaded = m_iHAMCopyLoaded + 1;

			source.HAM_RetainEntries(requiredData);
			HAM_RetainMultiLists(source, requiredData);
			// MergeCatalogs ne lit que la liste plate. Le MultiList s'aplatit ici,
			// parce que ce chargement n'appelle pas InitCatalog.
			source.HAM_FlattenMultiLists();

			array<SCR_EntityCatalogEntry> existing = {};
			int before = dest.GetEntityList(existing);
			dest.MergeCatalogs(source);
			added = added + dest.HAM_IndexFrom(before);
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_RetainMultiLists(SCR_EntityCatalog source, typename requiredData)
	{
		SCR_EntityCatalogMultiList multi = SCR_EntityCatalogMultiList.Cast(source);
		if (!multi)
			return;

		array<SCR_EntityCatalogMultiListEntry> lists = {};
		multi.GetMultiList(lists);
		foreach (SCR_EntityCatalogMultiListEntry list : lists)
		{
			if (!list || !list.m_aEntities)
				continue;

			ref array<ref SCR_EntityCatalogEntry> kept = {};
			foreach (SCR_EntityCatalogEntry entry : list.m_aEntities)
			{
				if (!HAM_CatalogMerge.Wanted(entry, requiredData))
					continue;

				kept.Insert(entry);
				HAM_CatalogMerge.See(entry.GetPrefab());
			}

			list.m_aEntities.Clear();
			foreach (SCR_EntityCatalogEntry keptEntry : kept)
			{
				list.m_aEntities.Insert(keptEntry);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_CollectFactionPrefabs(EEntityCatalogType catalogType)
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return;

		HAM_CollectFactionKey(manager, "US", catalogType);
		HAM_CollectFactionKey(manager, "USSR", catalogType);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_CollectFactionKey(notnull FactionManager manager, string key, EEntityCatalogType catalogType)
	{
		Faction faction = manager.GetFactionByKey(key);
		if (!faction)
			return;

		SCR_Faction scr = SCR_Faction.Cast(faction);
		if (!scr)
			return;

		HAM_CollectPrefabs(scr.GetFactionEntityCatalogOfType(catalogType, false));
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_CollectPrefabs(SCR_EntityCatalog catalog)
	{
		if (!catalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			if (!entry)
				continue;

			HAM_CatalogMerge.See(entry.GetPrefab());
		}
		// Ne pas See les multi-listes ici : sinon les catalogues sans faction
		// sont exclus de la fusion vers le catalogue commun (achat / panneaux).
	}

	//------------------------------------------------------------------------------------------------
	//! Même appel que UniHook_LoadItemCatalog.
	protected SCR_EntityCatalog HAM_LoadCatalog(ResourceName path)
	{
		Resource res = BaseContainerTools.LoadContainer(path);
		if (!res || !res.IsValid())
			return null;

		BaseContainer container = res.GetResource().ToBaseContainer();
		if (!container)
			return null;

		return SCR_EntityCatalog.Cast(BaseContainerTools.CreateInstanceFromContainer(container));
	}

	protected void HAM_FillOwnItems(notnull array<ResourceName> list)
	{
	}

	protected void HAM_FillOwnVehicles(notnull array<ResourceName> list)
	{
	}

	// Catalogues sans faction vanilla (US, URSS, FIA, CIV). UK n'est pas une faction de la partie.
	// Les mods qui posent déjà leurs objets sur une de ces factions ne sont pas recopiés ici.
	protected void HAM_FillCopyItems(notnull array<ResourceName> list)
	{
		list.Insert("{A13D0F0100000021}Configs/EntityCatalog/HAM/UniHook_InventoryItems.conf");
		list.Insert("{A13D0F0100000031}Configs/EntityCatalog/HAM/Extra_Items.conf");
		list.Insert("{880572D0AEB4F212}Configs/EntityCatalog/HAM/Wrap_ACECaptivesDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
		list.Insert("{6AFCEEE5F8E93B1C}Configs/EntityCatalog/HAM/Wrap_ACEExplosivesDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
		list.Insert("{4E6C7FAEDF5BE64D}Configs/EntityCatalog/HAM/Wrap_ACEFacepaintDev_ACE_InventoryItems_EntityCatalog_Equipment_UK.conf");
		list.Insert("{CCFF6D339FDA7D60}Configs/EntityCatalog/HAM/ACEFacepaintDev_UK_InventoryItems.conf");
		list.Insert("{721FE08EEEC1DEB5}Configs/EntityCatalog/HAM/Wrap_ACEMedicalBreathingDev_ACE_InventoryItems_EntityCatalog_Medical.conf");
		list.Insert("{B1239FEB47B8D56C}Configs/EntityCatalog/HAM/Wrap_ACEMedicalCirculationDev_ACE_InventoryItems_EntityCatalog_Medical.conf");
		list.Insert("{9408BA23C1972C26}Configs/EntityCatalog/HAM/Wrap_ACEMedicalCoreDev_ACE_InventoryItems_EntityCatalog_Medical.conf");
		list.Insert("{7BAF8A8FD15CFAB4}Configs/EntityCatalog/HAM/Wrap_ACERadioDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
		list.Insert("{D0884FDADC248091}Configs/EntityCatalog/HAM/Wrap_ACETacticalLadderDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
		list.Insert("{876FFABB1B1BACB9}Configs/EntityCatalog/HAM/Wrap_ACETacticalPeriscopeDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
		list.Insert("{D5F716ABA505FDE5}Configs/EntityCatalog/HAM/Wrap_ACETrenchesDev_ACE_InventoryItems_EntityCatalog_Equipment_UK.conf");
		list.Insert("{9E4D33E8FBD40580}Configs/EntityCatalog/HAM/Wrap_ACEWeatherDev_ACE_InventoryItems_EntityCatalog_Equipment.conf");
	}

	protected void HAM_FillCopyVehicles(notnull array<ResourceName> list)
	{
	}
}
