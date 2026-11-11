//------------------------------------------------------------------------------------------------
// Garages et hélipads du Conflit : « Demander un véhicule » ouvre le mode construction, pas les
// actions du générateur. Ici : véhicules de toutes les factions (pas d'étiquette de faction ajoutée
// au navigateur), prix et grade du tableau de tri véhicules à l'aperçu et au paiement, véhicules
// masqués au tableau retirés du navigateur.
// Le navigateur et les budgets ne donnent que la fiche (UIInfo) : le véhicule est retrouvé par son nom.
//------------------------------------------------------------------------------------------------

class HAM_VehicleBuildTri
{
	protected static ref map<string, ref array<string>> s_mPrefabsByName;
	protected static ref map<string, string> s_mNameByPrefab;

	//------------------------------------------------------------------------------------------------
	//! Serveur, au démarrage : pour chaque véhicule réglé au tableau, ce que le garage appliquera.
	static void LogSelfTest(notnull array<string> prefabs)
	{
		Ensure();
		foreach (string prefab : prefabs)
		{
			string name;
			if (!s_mNameByPrefab.Find(prefab, name))
			{
				HAM_Util.Info(string.Format("Tri véhicules : %1 absent des catalogues, réglage sans effet au garage", HAM_Util.PrefabFileName(prefab)));
				continue;
			}

			array<ref SCR_EntityBudgetValue> budgets = {};
			ApplyBudget(name, budgets);
			string applied = string.Empty;
			foreach (SCR_EntityBudgetValue budget : budgets)
			{
				applied = applied + string.Format(" %1=%2", typename.EnumToString(EEditableEntityBudget, budget.GetBudgetType()), budget.GetBudgetValue());
			}

			HAM_Util.Info(string.Format("Tri véhicules : %1 (%2) au garage :%3", HAM_Util.PrefabFileName(prefab), name, applied));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (s_mPrefabsByName)
			return;

		s_mPrefabsByName = new map<string, ref array<string>>();
		s_mNameByPrefab = new map<string, string>();
		SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
		if (catalogManager)
			Index(catalogManager.GetEntityCatalogOfType(EEntityCatalogType.VEHICLE));

		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		array<Faction> factions = {};
		factionManager.GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (scrFaction)
				Index(scrFaction.GetFactionEntityCatalogOfType(EEntityCatalogType.VEHICLE, false));
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Index(SCR_EntityCatalog catalog)
	{
		if (!catalog)
			return;

		array<SCR_EntityCatalogEntry> entries = {};
		catalog.GetEntityList(entries);
		foreach (SCR_EntityCatalogEntry entry : entries)
		{
			if (!entry || !entry.GetEntityUiInfo())
				continue;

			string name = entry.GetEntityUiInfo().GetName();
			string prefab = entry.GetPrefab();
			if (name.IsEmpty() || prefab.IsEmpty())
				continue;

			array<string> prefabs;
			if (!s_mPrefabsByName.Find(name, prefabs))
			{
				prefabs = {};
				s_mPrefabsByName.Set(name, prefabs);
			}

			if (prefabs.Find(prefab) == -1)
				prefabs.Insert(prefab);

			s_mNameByPrefab.Set(prefab, name);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> PrefabsNamed(string uiName)
	{
		if (uiName.IsEmpty() || !HAM_Mission.IsActive())
			return null;

		Ensure();
		array<string> prefabs;
		s_mPrefabsByName.Find(uiName, prefabs);
		return prefabs;
	}

	//------------------------------------------------------------------------------------------------
	//! Masqué si tous les véhicules de ce nom le sont au tableau.
	static bool IsHiddenName(string uiName)
	{
		array<string> prefabs = PrefabsNamed(uiName);
		if (!prefabs || prefabs.IsEmpty())
			return false;

		foreach (string prefab : prefabs)
		{
			if (!HAM_VehicleBoardStore.IsHidden(prefab))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Prix du tableau pour ce véhicule, -1 s'il n'en a pas.
	static int CostOf(string uiName)
	{
		array<string> prefabs = PrefabsNamed(uiName);
		if (!prefabs)
			return -1;

		foreach (string prefab : prefabs)
		{
			if (HAM_VehicleBoardStore.HasCostOverride(prefab))
				return HAM_VehicleBoardStore.BoardPrice(prefab);
		}

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	//! Remplace le coût CAMPAIGN et le grade par ceux du tableau. Vrai si la liste a changé.
	static bool ApplyBudget(string uiName, notnull array<ref SCR_EntityBudgetValue> budgets)
	{
		array<string> prefabs = PrefabsNamed(uiName);
		if (!prefabs)
			return false;

		bool changed = false;
		int cost = CostOf(uiName);
		if (cost >= 0)
		{
			bool costFound = false;
			for (int i = 0; i < budgets.Count(); i++)
			{
				if (budgets[i].GetBudgetType() != EEditableEntityBudget.CAMPAIGN)
					continue;

				budgets[i] = new SCR_EntityBudgetValue(EEditableEntityBudget.CAMPAIGN, cost);
				costFound = true;
			}

			if (!costFound)
				budgets.Insert(new SCR_EntityBudgetValue(EEditableEntityBudget.CAMPAIGN, cost));

			changed = true;
		}

		foreach (string prefab : prefabs)
		{
			if (!HAM_VehicleBoardStore.HasRankOverride(prefab))
				continue;

			for (int r = budgets.Count() - 1; r >= 0; r--)
			{
				EEditableEntityBudget type = budgets[r].GetBudgetType();
				if (type >= EEditableEntityBudget.RANK_RENEGADE && type <= EEditableEntityBudget.RANK_GENERAL)
					budgets.Remove(r);
			}

			SCR_ECharacterRank rank = HAM_VehicleBoardStore.GetRankOverride(prefab, SCR_ECharacterRank.PRIVATE);
			if (rank >= SCR_ECharacterRank.RENEGADE && rank <= SCR_ECharacterRank.GENERAL)
			{
				// RANK_RENEGADE = 100, puis un cran de 10 par grade jusqu'à RANK_GENERAL = 180.
				EEditableEntityBudget rankBudget = EEditableEntityBudget.RANK_RENEGADE + rank * 10;
				budgets.Insert(new SCR_EntityBudgetValue(rankBudget, 0));
			}

			changed = true;
			break;
		}

		return changed;
	}

	//------------------------------------------------------------------------------------------------
	//! Le fournisseur du mode construction est-il un garage ou un hélipad ?
	static bool IsVehicleProvider(IEntity provider)
	{
		if (!provider || !HAM_Mission.IsActive())
			return false;

		SCR_CatalogEntitySpawnerComponent spawner = SCR_CatalogEntitySpawnerComponent.Cast(provider.FindComponent(SCR_CatalogEntitySpawnerComponent));
		return spawner && spawner.HAM_IsVehicleList();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingEditorComponent
{
	//! Au garage, pas de filtre de faction : les véhicules de toutes les factions sont proposés.
	override protected void AddRemoveFactionLabel(SCR_Faction faction, bool addLabel)
	{
		if (HAM_VehicleBuildTri.IsVehicleProvider(GetProviderEntity()))
			return;

		super.AddRemoveFactionLabel(faction, addLabel);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingPlacingEditorComponent
{
	//! Le serveur refuse un véhicule sans l'étiquette de faction du garage : au garage et à l'hélipad,
	//! toutes factions. Le type du fournisseur (voitures, camions, hélicoptères) et la liste noire restent exigés.
	override protected bool AreLabelsMatching(notnull array<EEditableEntityLabel> entityLabels)
	{
		IEntity provider = m_Provider;
		if (m_CampaignBuildingComponent && m_CampaignBuildingComponent.GetProviderEntity())
			provider = m_CampaignBuildingComponent.GetProviderEntity();

		if (!HAM_VehicleBuildTri.IsVehicleProvider(provider))
			return super.AreLabelsMatching(entityLabels);

		SCR_CampaignBuildingProviderComponent providerComp = SCR_CampaignBuildingProviderComponent.Cast(provider.FindComponent(SCR_CampaignBuildingProviderComponent));
		if (!providerComp)
			return false;

		bool matchingLabel = false;
		foreach (EEditableEntityLabel providerLabel : providerComp.GetAvailableTraits())
		{
			if (!entityLabels.Contains(providerLabel))
				continue;

			matchingLabel = true;
			break;
		}

		if (!matchingLabel)
		{
			HAM_Util.Info(string.Format("Garage : pose refusée, le véhicule n'a aucun type de ce garage (%1)", provider.GetPrefabData().GetPrefabName()));
			return false;
		}

		SCR_ContentBrowserEditorComponent browserComp = SCR_ContentBrowserEditorComponent.Cast(m_Owner.FindComponent(SCR_ContentBrowserEditorComponent));
		if (!browserComp)
			return false;

		array<EEditableEntityLabel> validBlackListLabels = {};
		browserComp.GetValidBlackListedLabels(validBlackListLabels);
		foreach (EEditableEntityLabel blacklistedLabel : validBlackListLabels)
		{
			if (entityLabels.Contains(blacklistedLabel))
				return false;
		}

		return true;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_ContentBrowserEditorComponent
{
	//! En mode construction, un véhicule masqué au tableau de tri disparaît du navigateur.
	override void FilterEntries()
	{
		super.FilterEntries();

		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager || editorManager.GetCurrentMode() != EEditorMode.BUILDING || !HAM_Mission.IsActive())
			return;

		bool removed = false;
		for (int i = m_aFilteredPrefabIDs.Count() - 1; i >= 0; i--)
		{
			SCR_EditableEntityUIInfo info = GetInfo(m_aFilteredPrefabIDs[i]);
			if (!info || !HAM_VehicleBuildTri.IsHiddenName(info.GetName()))
				continue;

			m_aFilteredPrefabIDs.RemoveOrdered(i);
			removed = true;
		}

		if (!removed)
			return;

		m_iFilteredPrefabIDsCount = m_aFilteredPrefabIDs.Count();
		Event_OnBrowserEntriesFiltered.Invoke();
	}
}
