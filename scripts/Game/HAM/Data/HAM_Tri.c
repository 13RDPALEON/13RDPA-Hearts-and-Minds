//------------------------------------------------------------------------------------------------
// Tri des prefabs : construction, arsenal, véhicules.
// Une ligne masque un prefab, fixe son coût en fournitures ou son grade.
// Sans ligne, le prix et le grade vanilla restent.
// Arsenal et véhicules : les lignes amorcent les tableaux Training au premier boot seulement ;
// ensuite le staff les règle depuis le tableau.
//------------------------------------------------------------------------------------------------

enum HAM_ETriKind
{
	BUILD,
	ARSENAL,
	VEHICLE
}

class HAM_TriEntry
{
	HAM_ETriKind m_eKind;
	ResourceName m_sPrefab;
	bool m_bHidden;
	int m_iCost = -1;
	int m_iRank = -1;
	string m_sUIName;
}

class HAM_Tri
{
	protected static ref array<ref HAM_TriEntry> s_aEntries;
	protected static ref map<string, int> s_mBuildByUIName;
	protected static bool s_bInit;

	//------------------------------------------------------------------------------------------------
	static void Init()
	{
		if (s_bInit)
			return;

		s_bInit = true;
		s_aEntries = {};
		s_mBuildByUIName = new map<string, int>();

		// Construction (prefabs PrefabsEditable du menu Conflit)
		// BuildHide("{GUID}PrefabsEditable/Auto/Compositions/....et");
		// BuildCost("{GUID}PrefabsEditable/Auto/Compositions/....et", 150);
		// BuildRank("{GUID}PrefabsEditable/Auto/Compositions/....et", EEditableEntityBudget.RANK_SERGEANT);

		// Arsenal
		// ArsenalHide("{GUID}Prefabs/Weapons/....et");
		// ArsenalParams("{GUID}Prefabs/Weapons/....et", 25, SCR_ECharacterRank.CORPORAL);

		// Véhicules
		// VehicleHide("{GUID}Prefabs/Vehicles/....et");
		// VehicleParams("{GUID}Prefabs/Vehicles/....et", 300, SCR_ECharacterRank.SERGEANT);

		ResolveBuildNames();
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_TriEntry Entry(HAM_ETriKind kind, ResourceName prefab)
	{
		foreach (HAM_TriEntry existing : s_aEntries)
		{
			if (existing.m_eKind == kind && existing.m_sPrefab == prefab)
				return existing;
		}

		HAM_TriEntry created = new HAM_TriEntry();
		created.m_eKind = kind;
		created.m_sPrefab = prefab;
		s_aEntries.Insert(created);
		return created;
	}

	//------------------------------------------------------------------------------------------------
	static void BuildHide(ResourceName prefab)
	{
		Entry(HAM_ETriKind.BUILD, prefab).m_bHidden = true;
	}

	static void BuildCost(ResourceName prefab, int supplies)
	{
		Entry(HAM_ETriKind.BUILD, prefab).m_iCost = supplies;
	}

	static void BuildRank(ResourceName prefab, EEditableEntityBudget rank)
	{
		Entry(HAM_ETriKind.BUILD, prefab).m_iRank = rank;
	}

	static void ArsenalHide(ResourceName prefab)
	{
		Entry(HAM_ETriKind.ARSENAL, prefab).m_bHidden = true;
	}

	static void ArsenalParams(ResourceName prefab, int cost, SCR_ECharacterRank rank)
	{
		HAM_TriEntry entry = Entry(HAM_ETriKind.ARSENAL, prefab);
		entry.m_iCost = cost;
		entry.m_iRank = rank;
	}

	static void VehicleHide(ResourceName prefab)
	{
		Entry(HAM_ETriKind.VEHICLE, prefab).m_bHidden = true;
	}

	static void VehicleParams(ResourceName prefab, int cost, SCR_ECharacterRank rank)
	{
		HAM_TriEntry entry = Entry(HAM_ETriKind.VEHICLE, prefab);
		entry.m_iCost = cost;
		entry.m_iRank = rank;
	}

	//------------------------------------------------------------------------------------------------
	//! Le coût de construction est lu sur l'UIInfo du prefab, qui ne connaît pas son chemin.
	//! On retrouve la ligne par le nom affiché.
	protected static void ResolveBuildNames()
	{
		foreach (int index, HAM_TriEntry entry : s_aEntries)
		{
			if (entry.m_eKind != HAM_ETriKind.BUILD)
				continue;

			if (entry.m_iCost < 0 && entry.m_iRank < 0)
				continue;

			if (HAM_ProfileFiles.IsProfilePath(entry.m_sPrefab))
			{
				Print("[13RDPA H&M] Tri : chemin profil ignoré " + entry.m_sPrefab, LogLevel.WARNING);
				continue;
			}

			Resource resource = Resource.Load(entry.m_sPrefab);
			if (!resource || !resource.IsValid())
			{
				Print("[13RDPA H&M] Tri : prefab introuvable " + entry.m_sPrefab, LogLevel.WARNING);
				continue;
			}

			IEntityComponentSource editableSource = SCR_EditableEntityComponentClass.GetEditableEntitySource(resource);
			if (!editableSource)
				continue;

			SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(SCR_EditableEntityComponentClass.GetInfo(editableSource));
			if (!info)
				continue;

			entry.m_sUIName = info.GetName();
			if (!entry.m_sUIName.IsEmpty())
				s_mBuildByUIName.Set(entry.m_sUIName, index);
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsBuildHidden(ResourceName prefab)
	{
		Init();
		if (!s_aEntries)
			return false;

		foreach (HAM_TriEntry entry : s_aEntries)
		{
			if (entry.m_eKind == HAM_ETriKind.BUILD && entry.m_bHidden && entry.m_sPrefab == prefab)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Coût en supplies d'une ligne de construction triée, -1 si la ligne ne le change pas.
	static int BuildCostOf(string uiName)
	{
		Init();
		if (!s_mBuildByUIName || s_mBuildByUIName.IsEmpty() || uiName.IsEmpty())
			return -1;

		int index;
		if (!s_mBuildByUIName.Find(uiName, index))
			return -1;

		return s_aEntries[index].m_iCost;
	}

	//------------------------------------------------------------------------------------------------
	//! Remplace le coût CAMPAIGN et le grade d'une ligne de construction triée. Vrai si la liste a changé.
	static bool ApplyBuildBudget(string uiName, notnull array<ref SCR_EntityBudgetValue> budgets)
	{
		Init();
		if (!s_mBuildByUIName || s_mBuildByUIName.IsEmpty() || uiName.IsEmpty())
			return false;

		int index;
		if (!s_mBuildByUIName.Find(uiName, index))
			return false;

		HAM_TriEntry entry = s_aEntries[index];
		if (entry.m_iCost >= 0)
		{
			bool costFound = false;
			for (int costIdx = 0; costIdx < budgets.Count(); costIdx++)
			{
				if (budgets[costIdx].GetBudgetType() != EEditableEntityBudget.CAMPAIGN)
					continue;

				budgets[costIdx] = new SCR_EntityBudgetValue(EEditableEntityBudget.CAMPAIGN, entry.m_iCost);
				costFound = true;
			}

			if (!costFound)
				budgets.Insert(new SCR_EntityBudgetValue(EEditableEntityBudget.CAMPAIGN, entry.m_iCost));
		}

		if (entry.m_iRank >= 0)
		{
			for (int rankIdx = budgets.Count() - 1; rankIdx >= 0; rankIdx--)
			{
				EEditableEntityBudget type = budgets[rankIdx].GetBudgetType();
				if (type >= EEditableEntityBudget.RANK_RENEGADE && type <= EEditableEntityBudget.RANK_GENERAL)
					budgets.Remove(rankIdx);
			}

			EEditableEntityBudget rankBudget = entry.m_iRank;
			budgets.Insert(new SCR_EntityBudgetValue(rankBudget, 0));
		}

		return entry.m_iCost >= 0 || entry.m_iRank >= 0;
	}

	//------------------------------------------------------------------------------------------------
	//! Construction (HAM_Tri.c) puis véhicules demandés au garage (tableau de tri véhicules).
	static bool ApplyAllBuildBudgets(SCR_EditableEntityUIInfo info, notnull array<ref SCR_EntityBudgetValue> budgets)
	{
		if (!info)
			return false;

		bool changed = ApplyBuildBudget(info.GetName(), budgets);
		if (HAM_VehicleBuildTri.ApplyBudget(info.GetName(), budgets))
			changed = true;

		return changed;
	}

	//------------------------------------------------------------------------------------------------
	//! Amorce les tableaux arsenal et véhicules. Appelé une seule fois par partie (drapeau dans la sauvegarde).
	static int SeedBoards()
	{
		if (!s_aEntries)
			return 0;

		int seeded = 0;
		foreach (HAM_TriEntry entry : s_aEntries)
		{
			if (entry.m_eKind == HAM_ETriKind.BUILD)
				continue;

			int cost = entry.m_iCost;
			if (cost < 0)
				cost = 0;

			int rank = entry.m_iRank;
			if (rank < 0)
				rank = SCR_ECharacterRank.RENEGADE;

			bool hasParams = entry.m_iCost >= 0 || entry.m_iRank >= 0;
			if (entry.m_eKind == HAM_ETriKind.ARSENAL)
			{
				if (entry.m_bHidden)
					HAM_ArsenalBoardStore.SetShown(entry.m_sPrefab, false);

				if (hasParams)
					HAM_ArsenalBoardStore.ApplyItemParams(entry.m_sPrefab, cost, rank, 0, false);
			}
			else
			{
				if (entry.m_bHidden)
					HAM_VehicleBoardStore.SetShown(entry.m_sPrefab, false);

				if (hasParams)
					HAM_VehicleBoardStore.ApplyVehicleParams(entry.m_sPrefab, cost, rank, -1);
			}

			seeded++;
		}

		return seeded;
	}
}

//------------------------------------------------------------------------------------------------
// SCR_EditableEntityUIInfo ne peut pas être moddée : son CopyFrom lit des champs private d'une autre instance.
// Le tri s'applique donc là où le coût est lu : aperçu/paiement du placement, puis coût de remboursement.
modded class SCR_BudgetEditorComponent
{
	override bool GetEntityPreviewBudgetCosts(SCR_EditableEntityUIInfo entityUIInfo, out notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		bool result = super.GetEntityPreviewBudgetCosts(entityUIInfo, budgetCosts);
		if (result && HAM_Tri.ApplyAllBuildBudgets(entityUIInfo, budgetCosts))
			FilterAvailableBudgets(budgetCosts);

		return result;
	}

	override bool GetEntityPreviewBudgetCostsDontDiscardCampaignBudget(SCR_EditableEntityUIInfo entityUIInfo, out notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		bool result = super.GetEntityPreviewBudgetCostsDontDiscardCampaignBudget(entityUIInfo, budgetCosts);
		if (result && HAM_Tri.ApplyAllBuildBudgets(entityUIInfo, budgetCosts))
			FilterAvailableBudgetsDontDiscardCampaignBudget(budgetCosts);

		return result;
	}
}

//------------------------------------------------------------------------------------------------
// La version Conflict remplace GetEntityPreviewBudgetCosts sans appeler la parente.
modded class SCR_CampaignBuildingBudgetEditorComponent
{
	override bool GetEntityPreviewBudgetCosts(SCR_EditableEntityUIInfo entityUIInfo, out notnull array<ref SCR_EntityBudgetValue> budgetCosts)
	{
		bool result = super.GetEntityPreviewBudgetCosts(entityUIInfo, budgetCosts);
		if (result && HAM_Tri.ApplyAllBuildBudgets(entityUIInfo, budgetCosts))
			FilterAvailableBudgets(budgetCosts);

		return result;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingCompositionComponent
{
	//! Le jeu n'inscrit une composition que si elle a un SCR_EditorLinkComponent.
	//! Les panneaux H&M (dépôt, téléport, tour, rampe, poste, mannequin, tableau) n'en ont pas :
	//! sans cet appel, la pose du joueur ne passe jamais par HAM_Save.NoteBuilt.
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		if (!owner || !GetGame().InPlayMode() || !Replication.IsServer() || !HAM_Mission.IsActive())
			return;

		if (SCR_EditorLinkComponent.Cast(owner.FindComponent(SCR_EditorLinkComponent)))
			return;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return;

		SCR_CampaignBuildingManagerComponent manager = SCR_CampaignBuildingManagerComponent.Cast(gameMode.FindComponent(SCR_CampaignBuildingManagerComponent));
		if (!manager)
			return;

		manager.RegisterComposition(this);
	}

	override protected void SetCompositionCost(notnull SCR_EditableEntityComponent editableEnt)
	{
		super.SetCompositionCost(editableEnt);

		SCR_EditableEntityUIInfo info = SCR_EditableEntityUIInfo.Cast(editableEnt.GetInfo());
		if (!info)
			return;

		int cost = HAM_Tri.BuildCostOf(info.GetName());
		if (cost < 0)
			cost = HAM_VehicleBuildTri.CostOf(info.GetName());

		if (cost >= 0)
			m_iCost = cost;
	}
}
