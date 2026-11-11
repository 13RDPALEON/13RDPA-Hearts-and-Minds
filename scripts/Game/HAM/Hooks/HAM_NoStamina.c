//------------------------------------------------------------------------------------------------
// Pas d'endurance pour les joueurs en H&M : course, sprint, pentes et charge ne l'entament plus.
// Dans le jeu de base, le poids porté n'agit que sur l'endurance (encombrement) : plus de pénalité.
//------------------------------------------------------------------------------------------------

modded class SCR_CharacterStaminaComponent
{
	//! AddStamina doit tourner chez le propriétaire : le client du joueur pour son personnage.
	override event void OnStaminaDrain(float pDrain)
	{
		if (pDrain >= 0 || !HAM_Mission.IsActive())
			return;

		PlayerController controller = GetGame().GetPlayerController();
		if (!controller || controller.GetControlledEntity() != GetOwner())
			return;

		AddStamina(1);
	}
}
