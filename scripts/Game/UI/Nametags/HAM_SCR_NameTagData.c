// ====================================================================================
// HAM_SCR_NameTagData.c — 13RDPA
//
// Toggle GM « Masquer l'icône inconscient » : point de passage unique — l'état
// UNCONSCIOUS n'est jamais activé (init du tag ET OnLifeStateChanged passent ici).
// Indépendant du toggle nametags : l'icône disparaît même quand les noms restent.
// ====================================================================================
modded class SCR_NameTagData
{
    override void ActivateEntityState(ENameTagEntityState state)
    {
        if (state == ENameTagEntityState.UNCONSCIOUS && HAM_MilsimSettingsComponent.HAM_HideUnconsciousIcon())
            return;

        super.ActivateEntityState(state);
    }
}
