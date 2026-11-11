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
	}

	//------------------------------------------------------------------------------------------------
	//! Une caisse réelle est répliquée ; l'aperçu local que Bacon fabrique pour chaque objet ne l'est pas.
	protected bool HAM_IsPlacedArsenal()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return false;

		RplId id = HAM_RplUtil.IdOf(owner);
		if (!id.IsValid())
			return false;

		return Replication.FindItem(id) != null;
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
