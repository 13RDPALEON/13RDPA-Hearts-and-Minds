//------------------------------------------------------------------------------------------------
// Actions drapeau téléport : joueurs et/ou bases selon JSON Conflict (teleport.*).
//------------------------------------------------------------------------------------------------

class HAM_PlayerTeleportUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user && HAM_TeleportSettings.PlayerToPlayerEnabled();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!HAM_TeleportSettings.PlayerToPlayerEnabled())
			return;

		HAM_PlayerTeleportBoard.Toggle(pOwnerEntity, HAM_ETeleportBoardMode.PLAYERS);
	}
}

class HAM_BaseTeleportUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user && HAM_TeleportSettings.BetweenBasesEnabled();
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return CanBeShownScript(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		if (!HAM_TeleportSettings.BetweenBasesEnabled())
			return;

		HAM_PlayerTeleportBoard.Toggle(pOwnerEntity, HAM_ETeleportBoardMode.BASES);
	}
}
