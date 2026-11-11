
modded class SCR_BaseGameMode {
	[RplProp(onRplName: "HAM_GMBudget_OnBroadcastValueUpdated")]
	protected bool m_HAM_GMBudgets_BudgetsEnabled = true;
	
	static bool s_bTRN_GMBudgets_BudgetsEnabled = true;
	
	static ref set<EEditableEntityBudget> s_HAM_GMBudgets_Budgets;
	
	override void EOnInit(IEntity owner) {
		super.EOnInit(owner);
		
		s_HAM_GMBudgets_Budgets = new set<EEditableEntityBudget>;
		
		s_HAM_GMBudgets_Budgets.Insert(EEditableEntityBudget.PROPS);
		s_HAM_GMBudgets_Budgets.Insert(EEditableEntityBudget.AI);
		s_HAM_GMBudgets_Budgets.Insert(EEditableEntityBudget.VEHICLES);
		s_HAM_GMBudgets_Budgets.Insert(EEditableEntityBudget.WAYPOINTS);
		s_HAM_GMBudgets_Budgets.Insert(EEditableEntityBudget.SYSTEMS);
	};
	
	void HAM_GMBudget_SetBudgetsEnabled(bool enabled) {
		m_HAM_GMBudgets_BudgetsEnabled = enabled;
		Replication.BumpMe();
		
		HAM_GMBudget_OnBroadcastValueUpdated();
	};
	
	bool HAM_GMBudget_AreBudgetsEnabled() {
		return m_HAM_GMBudgets_BudgetsEnabled;
	};
	
	void HAM_GMBudget_OnBroadcastValueUpdated() {
		s_bTRN_GMBudgets_BudgetsEnabled = m_HAM_GMBudgets_BudgetsEnabled;
		
		SCR_BudgetEditorComponent budgetManager = SCR_BudgetEditorComponent.Cast(SCR_BudgetEditorComponent.GetInstance(SCR_BudgetEditorComponent, false, true));
		if (!budgetManager)
			return;

		// budgetManager.HAM_GMBudget_BudgetsUpdated(m_HAM_GMBudgets_BudgetsEnabled);
	};
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class HAM_GMBudget_BudgetsEnabledAttribute : SCR_BaseEditorAttribute
{	
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		//If opened in global attributes
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return null;
		
		return SCR_BaseEditorAttributeVar.CreateBool(gamemode.HAM_GMBudget_AreBudgetsEnabled());
	}
	
	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		if (!var) 
			return;
		
		SCR_BaseGameMode gamemode = SCR_BaseGameMode.Cast(item);
		if (!gamemode)
			return;
		
		gamemode.HAM_GMBudget_SetBudgetsEnabled(var.GetBool());
	}
};

modded class SCR_BudgetEditorComponent {
	protected ref map<EEditableEntityBudget, int> m_HAM_GMBudget_OriginalMaxBudgets = new map<EEditableEntityBudget, int>();
	
	override void EOnEditorInit() {
		super.EOnEditorInit();
		
		foreach	(SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			m_HAM_GMBudget_OriginalMaxBudgets.Set(maxBudget.GetBudgetType(), maxBudget.GetBudgetValue());
		};
		
		SCR_BaseGameMode game = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		if (!game || game.HAM_GMBudget_AreBudgetsEnabled())
			return;
			
		HAM_GMBudget_BudgetsUpdated(false);
	};

	void HAM_GMBudget_BudgetsUpdated(bool enabled) {
		foreach	(SCR_EntityBudgetValue maxBudget : m_MaxBudgets)
		{
			if (enabled)
				maxBudget.SetBudgetValue(m_HAM_GMBudget_OriginalMaxBudgets.Get(maxBudget.GetBudgetType()));
			else
				maxBudget.SetBudgetValue(m_HAM_GMBudget_OriginalMaxBudgets.Get(maxBudget.GetBudgetType()) * 500);
		};
	};
	
	override protected bool IsBudgetCapEnabled()
	{
		if (this.ClassName() == "SCR_CampaignBuildingBudgetEditorComponent")
			return super.IsBudgetCapEnabled();
		
		if (!SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled)
			return false;

		return super.IsBudgetCapEnabled();
	}
	
//	override bool IsBudgetCapEnabled()
//	{
//		if (!m_game) {
//			m_game = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
//			if (!m_game)
//				return super.IsBudgetCapEnabled();
//		};
//		
//		return m_game.HAM_GMBudget_AreBudgetsEnabled();
//	}
};

modded class SCR_PlacingEditorComponent {
	override bool IsThereEnoughBudgetToSpawn(IEntityComponentSource entitySource)
	{
		if (!SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled)
			return true;
		
		return super.IsThereEnoughBudgetToSpawn(entitySource);
	}
	
//	override protected bool CanPlaceEntityServer(IEntityComponentSource editableEntitySource, out EEditableEntityBudget blockingBudget, bool updatePreview, bool showNotification)
//	{
//		if (!SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled)
//			return true;
//
//		return super.CanPlaceEntityServer(editableEntitySource, blockingBudget, updatePreview, showNotification);
//	}
//	
    override void CheckBudgetOwner()
    {
		if (!SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled)
			return;
		
		super.CheckBudgetOwner();
    }

    override void OnBudgetMaxReached(EEditableEntityBudget entityBudget, bool maxReached)
    {
		if (SCR_BaseGameMode.s_HAM_GMBudgets_Budgets.Contains(entityBudget) && !SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled)
			return;
		
		super.OnBudgetMaxReached(entityBudget, maxReached);
    }
}

//modded class SCR_ContentBrowserEditorComponent {
//	override bool CanPlace(int prefabID, out notnull array<ref SCR_EntityBudgetValue> budgetCosts, out SCR_UIInfo blockingBudgetInfo, bool showNotification = false)
//	{
//		bool output = super.CanPlace(prefabID, budgetCosts, blockingBudgetInfo, showNotification);
//
//		if (!SCR_BaseGameMode.s_bTRN_GMBudgets_BudgetsEnabled) {
//			foreach (SCR_EntityBudgetValue budget : budgetCosts) {
//				if (!SCR_BaseGameMode.s_HAM_GMBudgets_Budgets.Contains(budget.GetBudgetValue()))
//					return true;
//			}
//			
//			return false;
//		}
//		
//		return output;
//	}
//}