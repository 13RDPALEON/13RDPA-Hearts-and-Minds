//------------------------------------------------------------------------------------------------
// Panneau : ouvre la carte. Personnage : saute de l'avion.
//------------------------------------------------------------------------------------------------

class HAM_ParadropUserAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return user != null;
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return user != null;
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		HAM_ParadropPick.Begin();
	}
}

class HAM_ParadropJumpAction : ScriptedUserAction
{
	override bool HasLocalEffectOnlyScript()
	{
		return true;
	}

	override bool CanBeShownScript(IEntity user)
	{
		return HAM_ParadropComponent.IsInPlane(user);
	}

	override bool CanBePerformedScript(IEntity user)
	{
		return HAM_ParadropComponent.IsInPlane(user);
	}

	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_ParadropJump();
	}
}
