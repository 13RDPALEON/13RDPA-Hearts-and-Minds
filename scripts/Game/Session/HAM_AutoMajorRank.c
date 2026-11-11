//------------------------------------------------------------------------------------------------
// 13RDPA : grade Major pour tout joueur dès la connexion (avant le lobby), comme sur 13RDPA Conflict.
// Même source que HasPlayerRequiredRank : SCR_PlayerXPHandlerComponent.
//------------------------------------------------------------------------------------------------

class HAM_AutoMajorRank
{
	protected static const SCR_ECharacterRank TARGET_RANK = SCR_ECharacterRank.MAJOR;
	protected static const int MAX_ATTEMPTS = 8;

	//------------------------------------------------------------------------------------------------
	static bool Apply(int playerId)
	{
		if (!Replication.IsServer())
			return false;

		PlayerController pc = GetGame().GetPlayerManager().GetPlayerController(playerId);
		if (!pc)
			return false;

		SCR_PlayerXPHandlerComponent handlerXP = SCR_PlayerXPHandlerComponent.Cast(pc.FindComponent(SCR_PlayerXPHandlerComponent));
		if (!handlerXP)
			return false;

		BaseGameMode gameMode = GetGame().GetGameMode();
		if (!gameMode)
			return false;

		SCR_XPHandlerComponent xpComp = SCR_XPHandlerComponent.Cast(gameMode.FindComponent(SCR_XPHandlerComponent));
		if (!xpComp)
			return false;

		SCR_FactionManager factionManager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		if (!factionManager)
			return false;

		SCR_RankContainer ranks = factionManager.GetFactionRanks(playerId);
		if (!ranks)
			return false;

		if (handlerXP.GetPlayerRankByXP() < TARGET_RANK)
		{
			int missingXp = ranks.GetRequiredRankXP(TARGET_RANK) - handlerXP.GetPlayerXP();
			if (missingXp > 0)
				xpComp.AwardXP(playerId, SCR_EXPRewards.STARTING_RANK, 1, false, missingXp);
		}

		handlerXP.UpdatePlayerRank(false);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Le contrôleur et la faction du joueur n'existent pas toujours au premier appel.
	static void ApplyWithRetry(int playerId, int attempt)
	{
		if (Apply(playerId) || attempt >= MAX_ATTEMPTS)
			return;

		GetGame().GetCallqueue().CallLater(ApplyWithRetry, 1000, false, playerId, attempt + 1);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_GameModeCampaign
{
	override protected void ApplyClientData(int playerId)
	{
		super.ApplyClientData(playerId);

		if (Replication.IsServer())
			HAM_AutoMajorRank.ApplyWithRetry(playerId, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! Un joueur arrivé après le chargement des réglages doit aussi recevoir son statut de liste blanche.
	override void OnPlayerAuditSuccess(int iPlayerID)
	{
		super.OnPlayerAuditSuccess(iPlayerID);

		if (Replication.IsServer())
			HAM_GroupAccessHelper.UpdatePlayerWhitelistStatusWithRetry(iPlayerID);
	}
}
