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
	//! La fusion des catalogues arrive après le premier index : le garage doit le refaire.
	static void ForgetIndex()
	{
		s_mPrefabsByName = null;
		s_mNameByPrefab = null;
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
	//! 1 camion, 2 APC, 3 char lourd, 0 : on ne déplace pas (voiture, hélico, inconnu).
	//! Le garage moyen a trois onglets. Un T-14 porte à la fois camion, APC et blindage,
	//! un K-17 porte camion et APC : sans ce tri, le même véhicule est dans deux onglets.
	static int GarageKind(string prefab)
	{
		if (prefab.IsEmpty())
			return 0;

		string low = prefab;
		low.ToLower();
		int brace = low.IndexOf("}");
		if (brace >= 0 && brace + 1 < low.Length())
			low = low.Substring(brace + 1, low.Length() - brace - 1);

		if (low.Contains("helicopter") || low.Contains("/heli") || low.Contains("aircraft") || low.Contains("/plane") || low.Contains("/air/") || low.Contains("/boat"))
			return 0;

		bool apc = low.Contains("k17") || low.Contains("k4386") || low.Contains("k-4386") || low.Contains("lav25") || low.Contains("lav-25") || low.Contains("/lav") || low.Contains("vpk") || low.Contains("btr") || low.Contains("stryker") || low.Contains("typhoon") || low.Contains("brdm") || low.Contains("bmp") || low.Contains("/bmd") || low.Contains("bmd_") || low.Contains("m113") || low.Contains("bradley") || low.Contains("kurganets");
		if (apc)
			return 2;

		bool truck = low.Contains("ural") || low.Contains("m923") || low.Contains("m939") || low.Contains("kamaz") || low.Contains("/zil") || low.Contains("zil_") || low.Contains("gaz66") || low.Contains("gaz-66") || low.Contains("hemtt") || low.Contains("kraz") || low.Contains("/truck") || low.Contains("truck_");
		if (truck)
			return 1;

		if (low.Contains("/tracked/") || low.Contains("/t14") || low.Contains("t-14") || low.Contains("2s1") || low.Contains("2s3") || low.Contains("2s19") || low.Contains("m1a1") || low.Contains("m1a2") || low.Contains("abrams"))
			return 3;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool LooksLikeVehicle(string prefab)
	{
		if (prefab.IsEmpty())
			return false;

		string low = prefab;
		low.ToLower();
		return low.Contains("/vehicles/") || low.Contains("/vehicle/") || low.Contains("helicopter");
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
	//------------------------------------------------------------------------------------------------
	//! Chargement vanilla pour le GM. Le tri garage reste uniquement dans FilterEntries.
	protected bool HAM_OpeningVehicleDepot()
	{
		SCR_CampaignBuildingEditorComponent building = SCR_CampaignBuildingEditorComponent.Cast(FindEditorComponent(SCR_CampaignBuildingEditorComponent, false, true));
		if (!building)
			return false;

		return HAM_VehicleBuildTri.IsVehicleProvider(building.GetProviderEntity());
	}

	//! Au garage seulement : masquage tri + onglets camion / APC / char.
	override void FilterEntries()
	{
		super.FilterEntries();

		SCR_EditorManagerEntity editorManager = SCR_EditorManagerEntity.GetInstance();
		if (!editorManager || editorManager.GetCurrentMode() != EEditorMode.BUILDING || !HAM_Mission.IsActive())
			return;

		if (!HAM_OpeningVehicleDepot())
			return;

		bool changed = false;
		for (int i = m_aFilteredPrefabIDs.Count() - 1; i >= 0; i--)
		{
			string prefab = GetResourceNamePrefabID(m_aFilteredPrefabIDs[i]);
			SCR_EditableEntityUIInfo info = GetInfo(m_aFilteredPrefabIDs[i]);
			if (!info)
			{
				m_aFilteredPrefabIDs.RemoveOrdered(i);
				changed = true;
				continue;
			}

			if (HAM_VehicleBuildTri.IsHiddenName(info.GetName()) || !HAM_VehicleBuildTri.LooksLikeVehicle(prefab))
			{
				m_aFilteredPrefabIDs.RemoveOrdered(i);
				changed = true;
			}
		}

		int tab = HAM_ActiveGarageTab();
		if (tab != 0)
		{
			for (int j = m_aFilteredPrefabIDs.Count() - 1; j >= 0; j--)
			{
				string prefab = GetResourceNamePrefabID(m_aFilteredPrefabIDs[j]);
				int kind = HAM_VehicleBuildTri.GarageKind(prefab);
				if (kind == 0 || kind == tab)
					continue;

				m_aFilteredPrefabIDs.RemoveOrdered(j);
				changed = true;
			}

			if (tab == 3 && HAM_AddGarageKind(3))
				changed = true;
		}

		if (!changed)
			return;

		m_iFilteredPrefabIDsCount = m_aFilteredPrefabIDs.Count();
		Event_OnBrowserEntriesFiltered.Invoke();
	}

	//------------------------------------------------------------------------------------------------
	//! 1 camion, 2 APC, 3 char. 0 si l'onglet actif n'est pas l'un de ces trois.
	protected int HAM_ActiveGarageTab()
	{
		bool truck = IsLabelActive(EEditableEntityLabel.VEHICLE_TRUCK);
		bool apc = IsLabelActive(EEditableEntityLabel.VEHICLE_APC);
		EEditableEntityLabel tankLabel = typename.StringToEnum(EEditableEntityLabel, "VEHICLE_TANK");
		string tankName = typename.EnumToString(EEditableEntityLabel, tankLabel);
		bool tank = false;
		if (tankName == "VEHICLE_TANK")
			tank = IsLabelActive(tankLabel);

		int tabs = 0;
		if (truck)
			tabs++;
		if (apc)
			tabs++;
		if (tank)
			tabs++;

		if (tabs != 1)
			return 0;

		if (truck)
			return 1;

		if (apc)
			return 2;

		return 3;
	}

	//------------------------------------------------------------------------------------------------
	//! Les chars RHS n'ont pas l'étiquette VEHICLE_TANK : l'onglet char les laisse de côté.
	protected bool HAM_AddGarageKind(int kind)
	{
		bool added = false;
		int count = GetInfoCount();
		for (int i = 0; i < count; i++)
		{
			if (m_aFilteredPrefabIDs.Contains(i))
				continue;

			SCR_EditableEntityUIInfo info = GetInfo(i);
			if (!info || HAM_VehicleBuildTri.IsHiddenName(info.GetName()))
				continue;

			string prefab = GetResourceNamePrefabID(i);
			if (HAM_VehicleBuildTri.GarageKind(prefab) != kind)
				continue;

			if (!HAM_HasGroundVehicleLabel(info) || HAM_IsBlackListed(info) || !HAM_MatchesSearch(info.GetName()))
				continue;

			m_aFilteredPrefabIDs.Insert(i);
			added = true;
		}

		return added;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HAM_HasGroundVehicleLabel(notnull SCR_EditableEntityUIInfo info)
	{
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		if (labels.Contains(EEditableEntityLabel.VEHICLE_TRUCK) || labels.Contains(EEditableEntityLabel.VEHICLE_APC) || labels.Contains(EEditableEntityLabel.VEHICLE_CAR))
			return true;

		EEditableEntityLabel tankLabel = typename.StringToEnum(EEditableEntityLabel, "VEHICLE_TANK");
		string tankName = typename.EnumToString(EEditableEntityLabel, tankLabel);
		return tankName == "VEHICLE_TANK" && labels.Contains(tankLabel);
	}

	//------------------------------------------------------------------------------------------------
	protected bool HAM_IsBlackListed(notnull SCR_EditableEntityUIInfo info)
	{
		array<EEditableEntityLabel> labels = {};
		info.GetEntityLabels(labels);
		array<EEditableEntityLabel> black = {};
		GetValidBlackListedLabels(black);
		foreach (EEditableEntityLabel label : black)
		{
			if (labels.Contains(label))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected bool HAM_MatchesSearch(string name)
	{
		string search = GetCurrentSearch();
		if (search.IsEmpty())
			return true;

		array<string> keys = {};
		keys.Insert(name);
		array<int> hits = {};
		WidgetManager.SearchLocalized(search, keys, hits);
		return !hits.IsEmpty();
	}
}
