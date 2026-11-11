//------------------------------------------------------------------------------------------------
// Filtre catalogue + coût / rank + volume sac à la prise arsenal.
//------------------------------------------------------------------------------------------------

modded class SCR_EntityCatalogManagerComponent
{
	protected static bool s_bTRNEntrySearch;

	// L'achat vanilla ne cherche que le catalogue de la faction de la caisse (US).
	// L'interface montre toutes les factions : sans ce repli, le serveur refuse l'objet.
	override SCR_EntityCatalogEntry GetEntryWithPrefabFromFactionCatalog(EEntityCatalogType catalogType, ResourceName prefabToFind, notnull SCR_Faction faction)
	{
		SCR_EntityCatalogEntry entry = super.GetEntryWithPrefabFromFactionCatalog(catalogType, prefabToFind, faction);
		if (entry || s_bTRNEntrySearch || catalogType != EEntityCatalogType.ITEM)
			return entry;

		s_bTRNEntrySearch = true;
		entry = GetEntryWithPrefabFromAnyCatalog(catalogType, prefabToFind, faction, false);
		s_bTRNEntrySearch = false;
		return entry;
	}

	override int GetAllArsenalItems(out array<SCR_ArsenalItem> allArsenalItems, SCR_EArsenalItemType typeFilter = -1, SCR_EArsenalItemMode modeFilter = -1, SCR_EArsenalGameModeType arsenalGameModeType = -1, EArsenalItemDisplayType requiresDisplayType = -1)
	{
		array<SCR_ArsenalItem> local = {};
		if (m_FactionManager)
			super.GetAllArsenalItems(local, typeFilter, modeFilter, arsenalGameModeType, requiresDisplayType);

		// Copie avant le filtre : le vanilla peut renvoyer son tableau interne.
		array<SCR_ArsenalItem> copy = {};
		if (local)
		{
			foreach (SCR_ArsenalItem item : local)
			{
				if (item)
					copy.Insert(item);
			}
		}

		HAM_ArsenalBoardStore.FilterList(copy);
		if (!allArsenalItems)
			allArsenalItems = {};

		allArsenalItems.Clear();
		array<string> seen = {};
		foreach (SCR_ArsenalItem item : copy)
		{
			if (!item)
				continue;

			string prefab = item.GetItemResourceName();
			int prefabLength = prefab.Length();
			if (prefabLength > 0)
				prefab = prefab.Substring(0, prefabLength);

			// Un nom vide n'identifie pas l'objet : les fusionner n'en laisserait qu'un.
			if (!prefab.IsEmpty())
			{
				if (seen.Contains(prefab))
					continue;

				seen.Insert(prefab);
			}

			allArsenalItems.Insert(item);
		}

		return allArsenalItems.Count();
	}
}

modded class SCR_UniversalInventoryStorageComponent
{
	protected float m_fTRNDesired;
	protected float m_fTRNBaseVolume;
	protected float m_fTRNBaseWeight;
	protected bool m_bTRNWeightScaled;
	protected bool m_bTRNTrunkClosed;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		GetGame().GetCallqueue().CallLater(HAM_ApplySavedVolume, 100, false);
		GetGame().GetCallqueue().CallLater(HAM_ApplySavedVolume, 1000, false);
	}

	void HAM_ApplySavedVolume()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return;

		float desired = HAM_ArsenalBoardStore.DesiredBagVolumeForEntity(owner);
		if (desired > 0)
		{
			HAM_SetDesiredVolume(desired);
			return;
		}

		bool trunk;
		float cargo = HAM_VehicleBoardStore.TrunkDesired(owner, trunk);
		if (!trunk)
			return;

		if (cargo <= 0)
		{
			m_bTRNTrunkClosed = true;
			m_fTRNDesired = 0;
			return;
		}

		m_bTRNTrunkClosed = false;
		HAM_SetDesiredVolume(cargo);
	}

	void HAM_SetDesiredVolume(float desired)
	{
		if (desired <= 0)
			return;

		if (!m_bTRNWeightScaled)
		{
			m_fTRNBaseVolume = GetMaxVolumeCapacity();
			m_fTRNBaseWeight = m_fMaxWeight;
			if (m_fTRNBaseVolume < 1)
				return;

			m_bTRNWeightScaled = true;
		}

		if (m_fTRNBaseVolume > 1 && desired > m_fTRNBaseVolume && m_fTRNBaseWeight > 0)
			m_fMaxWeight = m_fTRNBaseWeight * (desired / m_fTRNBaseVolume);

		m_fTRNDesired = desired;
		HAM_RescaleContents();
	}

	// Le moteur refuse de changer MaxCumulativeVolume. On réduit le volume
	// compté des objets dans le sac pour que le plafond d'origine représente
	// le volume demandé.
	protected bool HAM_CanEditItem(IEntity item)
	{
		if (!item)
			return false;

		RplComponent rpl = RplComponent.Cast(item.FindComponent(RplComponent));
		if (!rpl)
			return true;

		return rpl.IsMaster();
	}

	void HAM_FitItemToCapacity(IEntity item)
	{
		if (!HAM_CanEditItem(item) || m_fTRNDesired <= 0 || m_fTRNBaseVolume < 1)
			return;

		InventoryItemComponent comp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!comp)
			return;

		float baseVolume = comp.GetTotalVolume() - comp.GetAdditionalVolume();
		float baseWeight = comp.GetTotalWeight() - comp.GetAdditionalWeight();
		if (baseVolume < 0.001 && baseWeight < 0.001)
			return;

		float ratio = m_fTRNBaseVolume / m_fTRNDesired;
		if (baseVolume >= 0.001)
			comp.SetAdditionalVolume(baseVolume * (ratio - 1));

		// Le plafond de poids du prefab (40 kg sur l'ALICE) reste celui du moteur.
		// On compte les objets plus légers dans la même proportion que le volume.
		if (baseWeight >= 0.001 && m_fTRNDesired > m_fTRNBaseVolume)
			comp.SetAdditionalWeight(baseWeight * (ratio - 1));
		else
			comp.SetAdditionalWeight(0);
	}

	void HAM_ResetItemVolume(IEntity item)
	{
		if (!HAM_CanEditItem(item))
			return;

		InventoryItemComponent comp = InventoryItemComponent.Cast(item.FindComponent(InventoryItemComponent));
		if (!comp)
			return;

		comp.SetAdditionalVolume(0);
		comp.SetAdditionalWeight(0);
	}

	protected void HAM_RescaleContents()
	{
		array<IEntity> items = {};
		GetAll(items);
		foreach (IEntity item : items)
			HAM_FitItemToCapacity(item);
	}

	void HAM_PrepareIncoming(IEntity item)
	{
		IEntity owner = GetOwner();
		float bag = 0;
		if (owner)
			bag = HAM_ArsenalBoardStore.DesiredBagVolumeForEntity(owner);

		if (bag > 0)
		{
			m_bTRNTrunkClosed = false;
			if (m_fTRNDesired != bag)
				HAM_SetDesiredVolume(bag);

			HAM_FitItemToCapacity(item);
			return;
		}

		bool trunk;
		float cargo = 0;
		if (owner)
			cargo = HAM_VehicleBoardStore.TrunkDesired(owner, trunk);

		if (trunk)
		{
			if (cargo <= 0)
			{
				m_bTRNTrunkClosed = true;
				m_fTRNDesired = 0;
				return;
			}

			m_bTRNTrunkClosed = false;
			if (m_fTRNDesired != cargo)
				HAM_SetDesiredVolume(cargo);

			HAM_FitItemToCapacity(item);
			return;
		}

		m_bTRNTrunkClosed = false;
		if (m_fTRNDesired > 0)
			HAM_FitItemToCapacity(item);
	}

	bool HAM_IsTrunkClosed()
	{
		return m_bTRNTrunkClosed;
	}

	override bool CanStoreItem(IEntity item, int slotID)
	{
		HAM_PrepareIncoming(item);
		if (m_bTRNTrunkClosed)
			return false;

		return super.CanStoreItem(item, slotID);
	}

	override bool CanReplaceItem(IEntity nextItem, int slotID)
	{
		HAM_PrepareIncoming(nextItem);
		if (m_bTRNTrunkClosed)
			return false;

		return super.CanReplaceItem(nextItem, slotID);
	}

	override void OnAddedToSlot(IEntity item, int slotID)
	{
		HAM_PrepareIncoming(item);
		super.OnAddedToSlot(item, slotID);
	}

	override void OnRemovedFromSlot(IEntity item, int slotID)
	{
		HAM_ResetItemVolume(item);
		super.OnRemovedFromSlot(item, slotID);
	}
}

modded class SCR_InventoryStorageManagerComponent
{
	override bool CanInsertItemInActualStorage(IEntity item, BaseInventoryStorageComponent storage, int slotID = -1)
	{
		SCR_UniversalInventoryStorageComponent bag = SCR_UniversalInventoryStorageComponent.Cast(storage);
		if (bag)
		{
			bag.HAM_PrepareIncoming(item);
			if (bag.HAM_IsTrunkClosed())
				return false;
		}

		return super.CanInsertItemInActualStorage(item, storage, slotID);
	}
}

class HAM_ArsenalBoardBagApply
{
	protected static string s_sPrefab;
	protected static IEntity s_Requester;

	static void Queue(ResourceName prefab, IEntity requester)
	{
		s_sPrefab = prefab;
		s_Requester = requester;
		GetGame().GetCallqueue().CallLater(ApplyDelayed, 250, false);
		GetGame().GetCallqueue().CallLater(ApplyDelayed, 1000, false);
	}

	protected static void ApplyDelayed()
	{
		if (!Replication.IsServer() || !s_Requester || s_sPrefab.IsEmpty())
			return;

		array<IEntity> nodes = {};
		CollectNodes(s_Requester, nodes);
		foreach (IEntity node : nodes)
		{
			if (!node || !node.GetPrefabData())
				continue;

			string prefab = node.GetPrefabData().GetPrefabName();
			if (prefab != s_sPrefab && !PrefabPathEquals(prefab, s_sPrefab))
				continue;

			HAM_ArsenalBoardStore.ApplyBagVolumeToEntity(node);
		}
	}

	protected static bool PrefabPathEquals(string a, string b)
	{
		int braceA = a.IndexOf("}");
		int braceB = b.IndexOf("}");
		if (braceA >= 0 && braceA + 1 < a.Length())
			a = a.Substring(braceA + 1, a.Length() - braceA - 1);

		if (braceB >= 0 && braceB + 1 < b.Length())
			b = b.Substring(braceB + 1, b.Length() - braceB - 1);

		return a == b;
	}

	protected static void CollectNodes(IEntity ent, notnull array<IEntity> nodes)
	{
		if (!ent)
			return;

		nodes.Insert(ent);
		IEntity child = ent.GetChildren();
		while (child)
		{
			CollectNodes(child, nodes);
			child = child.GetSibling();
		}
	}

	protected static bool s_bListening;

	static void EnsureListener()
	{
		if (!Replication.IsServer() || s_bListening)
			return;

		s_bListening = true;
		SCR_ResourcePlayerControllerInventoryComponent.GetOnArsenalItemRequested().Insert(OnArsenalItemRequested);
	}

	protected static void OnArsenalItemRequested(SCR_ResourceComponent resourceComponent, ResourceName resourceName, IEntity requesterEntity, BaseInventoryStorageComponent inventoryStorageComponent, EResourceType resourceType, int resourceValue)
	{
		Queue(resourceName, requesterEntity);
	}
}

// Le tableau vanilla passe par l'emplacement d'inventaire. Bacon lit GetSupplyCost une fois
// et range le nombre dans un cache sealed : on ne modde pas ce cache, on y écrit le champ public.
class HAM_SupplyRules
{
	static bool GlobalSuppliesEnabled()
	{
		SCR_BaseGameMode mode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!mode)
			return true;

		return mode.IsResourceTypeEnabled(EResourceType.SUPPLIES);
	}
}

modded class SCR_ArsenalInventorySlotUI
{
	protected string HAM_Prefab()
	{
		if (!m_pItem || !m_pItem.GetOwner() || !m_pItem.GetOwner().GetPrefabData())
			return string.Empty;

		return m_pItem.GetOwner().GetPrefabData().GetPrefabName();
	}

	override float GetTotalResources()
	{
		if (!HAM_SupplyRules.GlobalSuppliesEnabled())
			return 0;

		float shown = super.GetTotalResources();
		if (shown < 0)
			return shown;

		string prefab = HAM_Prefab();
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.HasCostOverride(prefab))
			return shown;

		int custom = HAM_ArsenalBoardStore.BoardPrice(prefab);
		m_fSupplyCost = custom;
		if (m_ArsenalResourceComponent)
		{
			SCR_ResourceConsumer consumer = m_ArsenalResourceComponent.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
			if (consumer)
				m_fSupplyCost = custom * consumer.GetBuyMultiplier();
		}

		return m_fSupplyCost;
	}

	override SCR_ECharacterRank GetRequiredRank()
	{
		SCR_ECharacterRank rank = super.GetRequiredRank();
		string prefab = HAM_Prefab();
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.HasRankOverride(prefab))
			return rank;

		return HAM_ArsenalBoardStore.GetRankOverride(prefab, rank);
	}

	// Le mode H&M laisse le verrou vanilla coupé (noMinimumRank des véhicules).
	// Un grade posé au tableau s'affiche et verrouille quand même la case.
	override protected void SetItemRank()
	{
		super.SetItemRank();

		string prefab = HAM_Prefab();
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.HasRankOverride(prefab))
			return;

		m_eRequiredRank = HAM_ArsenalBoardStore.GetRankOverride(prefab, m_eRequiredRank);
		CheckRequiredRank();
	}
}

modded class SCR_ResourcePlayerControllerInventoryComponent
{
	protected int m_iTRNCustomCost;
	protected bool m_bTRNReplaceBuy;

	override protected void RpcAsk_ArsenalRequestItem_(RplId resourceComponentRplId, RplId storageComponentRplId, ResourceName resourceNameItem, EResourceType resourceType)
	{
		if (!HAM_RankAllows(resourceNameItem))
			return;

		if (HAM_ArsenalBoardStore.IsHidden(resourceNameItem))
			return;

		HAM_BeginCost(resourceNameItem, true);
		super.RpcAsk_ArsenalRequestItem_(resourceComponentRplId, storageComponentRplId, resourceNameItem, resourceType);
		HAM_EndCost();
	}

	override protected void RpcAsk_ArsenalRefundItem_(RplId resourceComponentRplId, RplId inventoryItemRplId, EResourceType resourceType)
	{
		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(Replication.FindItem(resourceComponentRplId));
		HAM_RefundContext.Begin(resources);
		HAM_VehicleDepot.MarkDepotRefund(resources);
		super.RpcAsk_ArsenalRefundItem_(resourceComponentRplId, inventoryItemRplId, resourceType);
		HAM_VehicleDepot.SettleDepotRefund(resources);
		HAM_RefundContext.End();
		HAM_EndCost();
	}

	protected bool HAM_RankAllows(string prefab)
	{
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.HasRankOverride(prefab))
			return true;

		SCR_ECharacterRank required = HAM_ArsenalBoardStore.BoardRank(prefab);
		if (required < SCR_ECharacterRank.PRIVATE)
			return true;

		SCR_PlayerController controller = SCR_PlayerController.Cast(GetOwner());
		if (!controller)
			return false;

		IEntity character = controller.GetControlledEntity();
		if (!character)
			return false;

		return required <= SCR_CharacterRankComponent.GetCharacterRank(character);
	}

	protected void HAM_BeginCost(string prefab, bool buy)
	{
		HAM_EndCost();
		if (prefab.IsEmpty() || !HAM_ArsenalBoardStore.HasCostOverride(prefab))
			return;

		m_iTRNCustomCost = HAM_ArsenalBoardStore.BoardPrice(prefab);
		m_bTRNReplaceBuy = buy;
	}

	protected void HAM_EndCost()
	{
		m_iTRNCustomCost = 0;
		m_bTRNReplaceBuy = false;
	}

	override protected bool TryPerformResourceConsumption(notnull SCR_ResourceActor actor, float resourceValue, bool ignoreOnEmptyBehavior = false)
	{
		if (!HAM_SupplyRules.GlobalSuppliesEnabled())
			return true;

		if (m_bTRNReplaceBuy)
		{
			float mult = 1;
			SCR_ResourceConsumer consumer = SCR_ResourceConsumer.Cast(actor);
			if (consumer)
				mult = consumer.GetBuyMultiplier();

			resourceValue = m_iTRNCustomCost * mult;
		}

		return super.TryPerformResourceConsumption(actor, resourceValue, ignoreOnEmptyBehavior);
	}

	override protected bool TryPerformResourceGeneration(notnull SCR_ResourceActor actor, float resourceValue)
	{
		if (!HAM_SupplyRules.GlobalSuppliesEnabled())
			return true;

		return super.TryPerformResourceGeneration(actor, resourceValue);
	}
}

//------------------------------------------------------------------------------------------------
// Le remboursement vanilla lit GetSupplyRefundAmount (souvent un autre chiffre que le prix payé).
// Pendant RpcAsk_ArsenalRefundItem_, on ramène chaque pièce au prix d'achat, multiplicateur compris.
class HAM_RefundContext
{
	protected static bool s_bActive;
	protected static float s_fBuy;
	protected static float s_fGenerator;
	protected static float s_fSameFaction;
	protected static float s_fOtherFaction;
	protected static SCR_Faction s_Faction;

	static void Begin(SCR_ResourceComponent resources)
	{
		s_bActive = true;
		s_fBuy = 1;
		s_fGenerator = 1;
		s_fSameFaction = 1;
		s_fOtherFaction = 1;
		s_Faction = null;

		SCR_ArsenalManagerComponent arsenalManager;
		if (SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager))
		{
			s_fSameFaction = arsenalManager.HAM_RefundSameFaction();
			s_fOtherFaction = arsenalManager.HAM_RefundOtherFaction();
		}

		if (!resources)
			return;

		SCR_ResourceConsumer consumer = resources.GetConsumer(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
		if (consumer)
			s_fBuy = consumer.GetBuyMultiplier();

		SCR_ResourceGenerator generator = resources.GetGenerator(EResourceGeneratorID.DEFAULT, EResourceType.SUPPLIES);
		if (generator)
			s_fGenerator = generator.GetResourceMultiplier();

		IEntity owner = resources.GetOwner();
		if (!owner)
			return;

		SCR_ArsenalComponent arsenal = SCR_ArsenalComponent.FindArsenalComponent(owner);
		if (arsenal)
			s_Faction = arsenal.GetAssignedFaction();
	}

	static void End()
	{
		s_bActive = false;
	}

	static bool Active()
	{
		return s_bActive;
	}

	static int PurchaseRefund(SCR_ArsenalItem item, int unit)
	{
		if (!s_bActive)
			return unit;

		float refundMult = s_fGenerator * FactionMultiplier(item);
		float paid = unit * s_fBuy;
		if (refundMult > 0.0001)
			return Math.Round(paid / refundMult);

		return Math.Round(paid);
	}

	protected static float FactionMultiplier(SCR_ArsenalItem item)
	{
		if (!s_Faction || !item)
			return s_fSameFaction;

		SCR_EntityCatalogEntry entry = item.HAM_CatalogEntry();
		SCR_EntityCatalogManagerComponent catalogs = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!entry || !catalogs)
			return s_fOtherFaction;

		if (entry.GetCatalogParent() == catalogs.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM, s_Faction.GetFactionKey()))
			return s_fSameFaction;

		return s_fOtherFaction;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_ArsenalManagerComponent
{
	bool HAM_VanillaRankLock()
	{
		return m_bRankLockedItems;
	}

	float HAM_RefundSameFaction()
	{
		return m_fRefundSameFactionItemMultiplier;
	}

	float HAM_RefundOtherFaction()
	{
		return m_fRefundDifferentFactionItemMultiplier;
	}

	// Bacon ne teste le grade que si ce drapeau est vrai, et son cache sealed jette le rang sinon.
	// Le champ du préfab reste faux : les véhicules continuent de passer par noMinimumRank.
	// GetRequiredRank ne renvoie un grade bloquant que pour une ligne du tableau.
	override bool AreItemsRankLocked()
	{
		if (HAM_Mission.IsActive())
			return true;

		return super.AreItemsRankLocked();
	}
}

//------------------------------------------------------------------------------------------------
// GetSupplyCost seul, sans toucher InitData : les catalogues restent ceux du jeu.
// Bacon (menu et achat) appelle cette méthode au remplissage de son cache.
[BaseContainerProps(configRoot: true), BaseContainerCustomCheckIntTitleField("m_bEnabled", "Arsenal Data", "DISABLED - Arsenal Data", 1)]
modded class SCR_ArsenalItem
{
	protected static bool s_bHAMRawCost;
	protected static bool s_bHAMRawRank;

	static void HAM_SetRawCost(bool raw)
	{
		s_bHAMRawCost = raw;
	}

	static void HAM_SetRawRank(bool raw)
	{
		s_bHAMRawRank = raw;
	}

	SCR_EntityCatalogEntry HAM_CatalogEntry()
	{
		return m_EntryParent;
	}

	override SCR_ECharacterRank GetRequiredRank()
	{
		if (s_bHAMRawRank)
			return super.GetRequiredRank();

		string prefab = GetItemResourceName();
		if (!prefab.IsEmpty() && HAM_ArsenalBoardStore.HasRankOverride(prefab))
			return HAM_ArsenalBoardStore.GetRankOverride(prefab, super.GetRequiredRank());

		SCR_ArsenalManagerComponent arsenalManager;
		if (SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager) && !arsenalManager.HAM_VanillaRankLock())
			return SCR_ECharacterRank.RENEGADE;

		return super.GetRequiredRank();
	}

	override int GetSupplyRefundAmount(SCR_EArsenalSupplyCostType supplyCostType, bool addAdditionalCosts = true, IEntity item = null)
	{
		int unit = GetSupplyCost(supplyCostType, addAdditionalCosts);
		return HAM_RefundContext.PurchaseRefund(this, unit);
	}

	static int HAM_RawCost(string prefab)
	{
		SCR_EntityCatalogManagerComponent catalog = SCR_EntityCatalogManagerComponent.GetInstance();
		if (!catalog || prefab.IsEmpty())
			return -1;

		SCR_EntityCatalogEntry entry = catalog.GetEntryWithPrefabFromAnyCatalog(EEntityCatalogType.ITEM, prefab, null, false);
		if (!entry)
			return -1;

		SCR_ArsenalItem data = SCR_ArsenalItem.Cast(entry.GetEntityDataOfType(SCR_ArsenalItem));
		if (!data)
			return -1;

		return data.GetSupplyCost(SCR_EArsenalSupplyCostType.DEFAULT, false);
	}

	override int GetSupplyCost(SCR_EArsenalSupplyCostType supplyCostType, bool addAdditionalCosts = true)
	{
		if (!s_bHAMRawCost)
		{
			string prefab = GetItemResourceName();
			if (!prefab.IsEmpty() && HAM_ArsenalBoardStore.HasCostOverride(prefab))
				return HAM_ArsenalBoardStore.BoardPrice(prefab);
		}

		return super.GetSupplyCost(supplyCostType, addAdditionalCosts);
	}
}

//------------------------------------------------------------------------------------------------
class HAM_ArsenalPrices
{
	static void PushAll()
	{
		map<ResourceName, ref Bacon_GunBuilderUI_Cache_ArsenalItemDetails> prices = Bacon_GunBuilderUI_Helpers.m_ArsenalItemDetails;
		if (!prices)
			return;

		for (int i = 0; i < prices.Count(); i++)
			Write(prices.GetKey(i), prices.GetElement(i), true);
	}

	static void PushOne(string prefab)
	{
		map<ResourceName, ref Bacon_GunBuilderUI_Cache_ArsenalItemDetails> prices = Bacon_GunBuilderUI_Helpers.m_ArsenalItemDetails;
		if (!prices || prefab.IsEmpty())
			return;

		for (int i = 0; i < prices.Count(); i++)
		{
			ResourceName key = prices.GetKey(i);
			if (!SamePrefab(key, prefab))
				continue;

			Write(key, prices.GetElement(i), false);
		}
	}

	protected static void Write(ResourceName key, Bacon_GunBuilderUI_Cache_ArsenalItemDetails details, bool overridesOnly)
	{
		if (!details)
			return;

		string prefab = key;
		if (HAM_ArsenalBoardStore.HasCostOverride(prefab))
			details.supplyCost = HAM_ArsenalBoardStore.BoardPrice(prefab);

		SCR_ArsenalManagerComponent arsenalManager;
		bool vanillaRank = false;
		if (SCR_ArsenalManagerComponent.GetArsenalManager(arsenalManager))
			vanillaRank = arsenalManager.HAM_VanillaRankLock();

		if (HAM_ArsenalBoardStore.HasRankOverride(prefab))
			details.requiredRank = HAM_ArsenalBoardStore.BoardRank(prefab);
		else if (!vanillaRank)
			details.requiredRank = SCR_ECharacterRank.RENEGADE;

		if (HAM_ArsenalBoardStore.HasCostOverride(prefab))
			return;

		if (overridesOnly)
			return;

		SCR_ArsenalItem.HAM_SetRawCost(true);
		int raw = SCR_ArsenalItem.HAM_RawCost(prefab);
		SCR_ArsenalItem.HAM_SetRawCost(false);
		if (raw >= 0)
			details.supplyCost = raw;
	}

	protected static bool SamePrefab(string a, string b)
	{
		if (a == b)
			return true;

		return PathOf(a) == PathOf(b);
	}

	protected static string PathOf(string prefab)
	{
		int brace = prefab.IndexOf("}");
		if (brace < 0 || brace + 1 >= prefab.Length())
			return prefab;

		return prefab.Substring(brace + 1, prefab.Length() - brace - 1);
	}
}
