// Mallette et poste de soin : plus aucune consommation de fournitures SUPPLIES.
// Le poste de base demande 1 fourniture par soin (HealSupportStation), et une réserve
// vide force encore un coût de 1 même quand le préfab affiche 0. Le réassort de la
// mallette paie le coût d'arsenal de l'objet. Ici les deux ignorent le stock.
// Les seuils d'arrêt cardiaque ACE ne sont pas touchés.
//------------------------------------------------------------------------------------------------
modded class SCR_HealSupportStationComponent
{
	override bool AreSuppliesEnabled()
	{
		return false;
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_ResupplyMedicalGadgetSupportStationComponent
{
	override bool AreSuppliesEnabled()
	{
		return false;
	}
}
