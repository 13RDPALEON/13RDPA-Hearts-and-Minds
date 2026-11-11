//------------------------------------------------------------------------------------------------
// Le Conflit cherche la faction ennemie parmi les factions jouables. En H&M, la faction ennemie
// ne l'est pas : sans ce repli, SetAsHQ plante au démarrage et la mise en place des QG s'arrête.
//------------------------------------------------------------------------------------------------

modded class SCR_CampaignFactionManager
{
	override SCR_CampaignFaction GetEnemyFaction(notnull SCR_CampaignFaction alliedFaction)
	{
		SCR_CampaignFaction enemy = super.GetEnemyFaction(alliedFaction);
		if (enemy || !HAM_Mission.IsActive())
			return enemy;

		string enemyKey = HAM_Units.ENEMY_FACTION;
		if (alliedFaction.GetFactionKey() == HAM_Units.ENEMY_FACTION)
			enemyKey = HAM_Units.PLAYER_FACTION;

		return SCR_CampaignFaction.Cast(GetFactionByKey(enemyKey));
	}
}
