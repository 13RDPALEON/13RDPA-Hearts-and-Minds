//------------------------------------------------------------------------------------------------
// Aucun plafond de quantité en H&M : props, IA et véhicules construits par base, délai entre deux
// poses, bases établies par faction, recrues par groupe de joueur, délai entre deux demandes aux
// générateurs (dans HAM_SCR_CatalogEntitySpawnerComponent.c). Les fournitures et les grades restent dus.
// La limite d'IA actives du monde vient du JSON (ai.activeLimit), comme sur le Conflit.
//------------------------------------------------------------------------------------------------

class HAM_NoLimits
{
	//! Assez haut pour ne jamais être atteint, et lisible dans les jauges de l'éditeur (-1 ne l'est pas).
	static const int UNLIMITED = 100000;

	protected static int s_iEngineAiLimit = -1;

	//------------------------------------------------------------------------------------------------
	static bool IsUnlimitedBudget(EEditableEntityBudget budget)
	{
		if (!HAM_Mission.IsActive())
			return false;

		switch (budget)
		{
			case EEditableEntityBudget.PROPS: return true;
			case EEditableEntityBudget.AI: return true;
			case EEditableEntityBudget.VEHICLES: return true;
			case EEditableEntityBudget.SYSTEMS: return true;
			case EEditableEntityBudget.WAYPOINTS: return true;
			case EEditableEntityBudget.COOLDOWN: return true;
			case EEditableEntityBudget.AI_SERVER: return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! 0 rend la limite du jeu de base.
	static void ApplyActiveAiLimit()
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return;

		if (s_iEngineAiLimit < 0)
			s_iEngineAiLimit = aiWorld.GetLimitOfActiveAIs();

		int limit = HAM_Settings.Get().m_iActiveAiLimit;
		if (limit <= 0)
			limit = s_iEngineAiLimit;

		aiWorld.SetLimitOfActiveAIs(limit);
		HAM_Util.Info(string.Format("Limite d'IA actives : %1", limit));
	}

	//------------------------------------------------------------------------------------------------
	static int ActiveAiLimit()
	{
		AIWorld aiWorld = GetGame().GetAIWorld();
		if (!aiWorld)
			return 0;

		return aiWorld.GetLimitOfActiveAIs();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CampaignBuildingProviderComponent
{
	//------------------------------------------------------------------------------------------------
	override int GetMaxBudgetValue(EEditableEntityBudget budget)
	{
		if (HAM_NoLimits.IsUnlimitedBudget(budget))
			return HAM_NoLimits.UNLIMITED;

		return super.GetMaxBudgetValue(budget);
	}

	//------------------------------------------------------------------------------------------------
	override int GetMaxBudgetValueFromMasterIfNeeded(EEditableEntityBudget budget)
	{
		if (HAM_NoLimits.IsUnlimitedBudget(budget))
			return HAM_NoLimits.UNLIMITED;

		return super.GetMaxBudgetValueFromMasterIfNeeded(budget);
	}

	//------------------------------------------------------------------------------------------------
	//! Un budget non évalué ne bloque jamais la pose dans l'éditeur de construction.
	override bool IsBudgetToEvaluate(EEditableEntityBudget blockingBudget)
	{
		if (HAM_NoLimits.IsUnlimitedBudget(blockingBudget))
			return false;

		return super.IsBudgetToEvaluate(blockingBudget);
	}

	//------------------------------------------------------------------------------------------------
	override void SetPlayerCooldown(int playerId, int cooldownTime)
	{
		if (HAM_Mission.IsActive())
			return;

		super.SetPlayerCooldown(playerId, cooldownTime);
	}

	//------------------------------------------------------------------------------------------------
	override bool HasCooldownSet(int playerId)
	{
		if (HAM_Mission.IsActive())
			return false;

		return super.HasCooldownSet(playerId);
	}

	//------------------------------------------------------------------------------------------------
	//! 1.9 endort le groupe posé vers 800 m. Le suivi de budget reste. Le groupe déployé reste éveillé.
	override void TrackAIGroup(notnull SCR_AIGroup group)
	{
		super.TrackAIGroup(group);
		if (!HAM_Mission.IsActive())
			return;

		group.SetLifecyclePolicy(SCR_EAIGroupLifecyclePolicy.Manual);
		group.SetImportance(SCR_EAISpawnImportance.CRITICAL);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_GameModeCampaign
{
	//! -1 : seuls les indicatifs radio disponibles limitent encore les bases, une par indicatif.
	override int GetFactionEstablishBaseLimit()
	{
		if (HAM_Mission.IsActive())
			return -1;

		return super.GetFactionEstablishBaseLimit();
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CommandingManagerComponent
{
	//! Recrues de caserne et commande de recrutement : pas de plafond par groupe de joueur.
	override int GetMaxAIPerGroup()
	{
		if (HAM_Mission.IsActive())
			return HAM_NoLimits.UNLIMITED;

		return super.GetMaxAIPerGroup();
	}
}
