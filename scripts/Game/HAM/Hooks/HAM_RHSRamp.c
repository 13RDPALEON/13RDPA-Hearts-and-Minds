//------------------------------------------------------------------------------------------------
// La rampe du K-17 relance PortalUpdate en boucle. Quand l'aperçu du garage est détruit,
// GetOwner() est null et la VM s'arrête à chaque frame. On retire le rappel.
//------------------------------------------------------------------------------------------------
modded class RHS_K17RampAction
{
	override void PortalUpdate()
	{
		if (!GetGame() || !GetOwner())
		{
			if (GetGame())
				GetGame().GetCallqueue().Remove(PortalUpdate);

			return;
		}

		super.PortalUpdate();
	}
}
