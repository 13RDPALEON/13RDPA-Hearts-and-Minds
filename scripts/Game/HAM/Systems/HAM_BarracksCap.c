//------------------------------------------------------------------------------------------------
// Caserne du camp pris : la table ne déploie que le nombre de groupes demandé.
// Un groupe coûte 10 de budget IA. Le plafond est lu au moment de chaque achat.
// Si un autre système réécrit le maximum stocké, ces 4 groupes restent en vigueur.
//------------------------------------------------------------------------------------------------

class HAM_BarracksCap
{
	static void Apply(SCR_CampaignBuildingProviderComponent provider, bool player, int groups)
	{
		if (!provider)
			return;

		SCR_CampaignBuildingMaxValueBudgetToEvaluateData budget = SCR_CampaignBuildingMaxValueBudgetToEvaluateData.Cast(provider.GetBudgetData(EEditableEntityBudget.AI));
		if (!budget)
			return;

		if (player && groups > 0)
			budget.HAM_OwnGroupCap(groups * 10);
		else
			budget.HAM_ClearGroupCap();
	}
}

modded class SCR_CampaignBuildingMaxValueBudgetToEvaluateData
{
	protected bool m_bHAM_Capped;
	protected int m_iHAM_Cap;

	void HAM_OwnGroupCap(int budget)
	{
		m_bUseMasterProviderBudget = false;
		m_bHAM_Capped = true;
		m_iHAM_Cap = budget;
		m_iMaxValue = budget;
	}

	void HAM_ClearGroupCap()
	{
		m_bHAM_Capped = false;
		m_iHAM_Cap = 0;
		m_bUseMasterProviderBudget = true;
	}

	override int GetMaxValue()
	{
		if (m_bHAM_Capped && m_iHAM_Cap > 0)
			return m_iHAM_Cap;

		return super.GetMaxValue();
	}
}
