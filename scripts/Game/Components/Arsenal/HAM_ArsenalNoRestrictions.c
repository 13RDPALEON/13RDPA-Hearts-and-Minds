//------------------------------------------------------------------------------------------------
// 13RDPA — Tous les arsenaux, comme sur Training : objets de toutes les factions, de tous les types
// et de tous les mods (filtrés par le tri), tenue enregistrable sans restriction.
//------------------------------------------------------------------------------------------------

modded class SCR_ArsenalComponent
{
	override protected void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		HAM_AlignWithBuiltArsenal();
		GetGame().GetCallqueue().CallLater(HAM_RefreshArsenalDelayed, 1000, false);
		GetGame().GetCallqueue().CallLater(HAM_RegisterIfPlaced, 1000, false);

		if (!Replication.IsServer())
			return;

		// Pendant EOnInit, le RplComponent n'est pas enregistré : le setter vanilla enverrait un RPC en erreur.
		// La valeur est posée en local, le rafraîchissement différé la diffuse.
		HAM_ApplyNoRestrictionsSaveType();
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		HAM_ArsenalBoardStore.UnregisterArsenal(this);
		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Bacon range ses pages avec le masque du prefab. Le mettre à -1 mélange munitions, soins et grenades dans un seul cache.
	protected void HAM_AlignWithBuiltArsenal()
	{
		m_eSupplyCostType = SCR_EArsenalSupplyCostType.DEFAULT;
		if (GetArsenalType() != SCR_EArsenalTypes.VEHICLES)
			return;

		// Le camion arsenal et le camion munitions n'autorisent pas les tenues. On ouvre tous les types d'objets.
		m_eSupportedArsenalItemTypes = HAM_AllItemTypes();
		m_eSupportedArsenalItemModes = HAM_AllItemModes();
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_EArsenalItemType HAM_AllItemTypes()
	{
		SCR_EArsenalItemType types = SCR_EArsenalItemType.RIFLE;
		types = types | SCR_EArsenalItemType.PISTOL;
		types = types | SCR_EArsenalItemType.LETHAL_THROWABLE;
		types = types | SCR_EArsenalItemType.ROCKET_LAUNCHER;
		types = types | SCR_EArsenalItemType.MACHINE_GUN;
		types = types | SCR_EArsenalItemType.HEAL;
		types = types | SCR_EArsenalItemType.BACKPACK;
		types = types | SCR_EArsenalItemType.SNIPER_RIFLE;
		types = types | SCR_EArsenalItemType.NON_LETHAL_THROWABLE;
		types = types | SCR_EArsenalItemType.HEADWEAR;
		types = types | SCR_EArsenalItemType.TORSO;
		types = types | SCR_EArsenalItemType.VEST_AND_WAIST;
		types = types | SCR_EArsenalItemType.LEGS;
		types = types | SCR_EArsenalItemType.FOOTWEAR;
		types = types | SCR_EArsenalItemType.RADIO_BACKPACK;
		types = types | SCR_EArsenalItemType.EQUIPMENT;
		types = types | SCR_EArsenalItemType.WEAPON_ATTACHMENT;
		types = types | SCR_EArsenalItemType.EXPLOSIVES;
		types = types | SCR_EArsenalItemType.HANDWEAR;
		types = types | SCR_EArsenalItemType.MORTARS;
		return types;
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_EArsenalItemMode HAM_AllItemModes()
	{
		SCR_EArsenalItemMode modes = SCR_EArsenalItemMode.DEFAULT;
		modes = modes | SCR_EArsenalItemMode.WEAPON;
		modes = modes | SCR_EArsenalItemMode.WEAPON_VARIANTS;
		modes = modes | SCR_EArsenalItemMode.AMMUNITION;
		modes = modes | SCR_EArsenalItemMode.CONSUMABLE;
		modes = modes | SCR_EArsenalItemMode.ATTACHMENT;
		modes = modes | SCR_EArsenalItemMode.SUPPORT_STATION;
		modes = modes | SCR_EArsenalItemMode.PYLON;
		return modes;
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_ApplyNoRestrictionsSaveType()
	{
		if (GetArsenalSaveType() == SCR_EArsenalSaveType.NO_RESTRICTIONS)
			return;

		RPL_SetArsenalSaveType(SCR_EArsenalSaveType.NO_RESTRICTIONS);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_RefreshArsenalDelayed()
	{
		if (!GetOwner())
			return;

		RefreshArsenal(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Le tableau de tri rafraîchit les caisses enregistrées quand un objet change.
	protected void HAM_RegisterIfPlaced()
	{
		if (!HAM_IsPlacedArsenal())
			return;

		HAM_ArsenalBoardStore.RegisterArsenal(this);
		if (GetArsenalType() == SCR_EArsenalTypes.VEHICLES && GetOwner())
			RefreshArsenal(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Une caisse réelle est répliquée ; l'aperçu local que Bacon fabrique pour chaque objet ne l'est pas.
	//! La caisse d'un camion n'a souvent pas son propre identifiant : celui du véhicule parent suffit.
	protected bool HAM_IsPlacedArsenal()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return false;

		bool seenVehicle = false;
		IEntity node = owner;
		int guard = 0;
		while (node && guard < 8)
		{
			if (Vehicle.Cast(node))
				seenVehicle = true;

			RplId id = HAM_RplUtil.IdOf(node);
			if (id.IsValid() && Replication.FindItem(id))
			{
				if (node == owner)
					return true;

				return seenVehicle;
			}

			node = node.GetParent();
			guard++;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Seule une caisse posée reçoit la liste : un aperçu qui la recevrait classerait tous les objets et viderait les pages.
	override bool GetFilteredArsenalItems(out notnull array<SCR_ArsenalItem> filteredArsenalItems, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		if (!HAM_IsPlacedArsenal())
		{
			filteredArsenalItems.Clear();
			return false;
		}

		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalog)
			return false;

		array<SCR_ArsenalItem> gathered = {};
		catalog.GetAllArsenalItems(gathered, -1, -1, -1, requiresDisplayType);

		filteredArsenalItems.Clear();
		foreach (SCR_ArsenalItem item : gathered)
		{
			if (item)
				filteredArsenalItems.Insert(item);
		}

		return !filteredArsenalItems.IsEmpty();
	}
}

//------------------------------------------------------------------------------------------------
//! L'inventaire vanilla crée une fiche pour chaque objet à chaque page. On ne le fait qu'une fois.
class HAM_ArsenalPreview
{
	protected static ref array<ResourceName> s_aNames;
	protected static ref map<ResourceName, IEntity> s_mEntity;
	protected static ref set<ResourceName> s_aFailed;

	static void ForgetList()
	{
		s_aNames = null;
	}

	static void Names(SCR_ArsenalComponent arsenal, notnull array<ResourceName> outNames)
	{
		if (!s_aNames)
		{
			array<ResourceName> fresh = {};
			if (arsenal)
				arsenal.GetAvailablePrefabs(fresh);

			if (fresh.IsEmpty())
				return;

			s_aNames = {};
			foreach (ResourceName name : fresh)
			{
				if (!name.IsEmpty())
					s_aNames.Insert(name);
			}
		}

		foreach (ResourceName kept : s_aNames)
			outNames.Insert(kept);
	}

	static IEntity Entity(ItemPreviewManagerEntity manager, ResourceName name)
	{
		if (!manager || name.IsEmpty())
			return null;

		if (!s_mEntity)
			s_mEntity = new map<ResourceName, IEntity>();

		IEntity found;
		if (s_mEntity.Find(name, found))
			return found;

		if (!s_aFailed)
			s_aFailed = new set<ResourceName>();

		if (s_aFailed.Contains(name))
			return null;

		found = manager.ResolvePreviewEntityForPrefab(name);
		if (!found)
		{
			s_aFailed.Insert(name);
			return null;
		}

		s_mEntity.Set(name, found);
		return found;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_InventoryOpenedStorageArsenalUI
{
	override void GetAllItems(out notnull array<IEntity> pItemsInStorage, BaseInventoryStorageComponent pStorage = null)
	{
		if (pStorage)
		{
			super.GetAllItems(pItemsInStorage, pStorage);
			return;
		}

		if (!m_Storage)
			return;

		IEntity owner = m_Storage.GetOwner();
		if (!owner)
			return;

		SCR_ArsenalComponent arsenal = SCR_ArsenalComponent.Cast(owner.FindComponent(SCR_ArsenalComponent));
		if (!arsenal)
			return;

		ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
		if (!world)
			return;

		ItemPreviewManagerEntity manager = world.GetItemPreviewManager();
		if (!manager)
			return;

		array<ResourceName> names = {};
		HAM_ArsenalPreview.Names(arsenal, names);
		foreach (ResourceName name : names)
		{
			IEntity preview = HAM_ArsenalPreview.Entity(manager, name);
			if (preview)
				pItemsInStorage.Insert(preview);
		}
	}
}
