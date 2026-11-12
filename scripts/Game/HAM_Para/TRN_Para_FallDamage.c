//! The parachute component grades actual descent once on the state authority.
//! Native character falling physics can accumulate a different velocity on dedicated.
modded class SCR_CharacterDamageManagerComponent
{
	override void OnHandleFallDamage(EFallDamageType fallDamageType, vector velocityVector)
	{
		IEntity owner = GetOwner();
		HAM_Para_FreeFallComponent chute = HAM_Para_FreeFallComponent.Cast(owner.FindComponent(HAM_Para_FreeFallComponent));
		if (chute && chute.HAM_Para_HandlesParachuteFallDamage())
		{
			Print(string.Format("[HAM_Para_FreeFall] native fall damage replaced; engine velocity=%1", velocityVector), LogLevel.NORMAL);
			return;
		}

		// TP de sortie avion : le moteur calcule une chute fatale avant la voile.
		if (HAM_ParadropComponent.IsDropProtected(owner))
			return;

		super.OnHandleFallDamage(fallDamageType, velocityVector);
	}
}
