// ====================================================================================
// HAM_SCR_DestructibleBuildingComponent.c — 13RDPA (autorité dégâts)
//
// Toggle GM « Bâtiments indestructibles » : HijackDamageHandling → true = les dégâts
// sont interceptés et ignorés, le bâtiment ne s'effondre jamais (méthode du mod
// DKG_DisableBuildingDestruction, rendue togglable).
// ====================================================================================
modded class SCR_DestructibleBuildingComponent
{
    override bool HijackDamageHandling(notnull BaseDamageContext damageContext)
    {
        if (HAM_MilsimSettingsComponent.HAM_IndestructibleBuildings())
            return true;

        return super.HijackDamageHandling(damageContext);
    }
}
