// ====================================================================================
// HAM_SCR_GarbageSystem.c — 13RDPA (SERVEUR)
//
// Toggles GM « Conserver les épaves » / « Conserver les corps » : retourner un
// lifetime <= 0 depuis OnInsertRequested empêche l'insertion au nettoyage (doc
// officielle GarbageSystem générée). Sinon on laisse la logique vanilla (bonus de
// lifetime des véhicules de supply) via super.
//
// Les entités déjà en file avant l'activation gardent leur timer (toggle actif dès
// le boot par défaut → cas marginal).
// ====================================================================================
modded class SCR_GarbageSystem
{
    override protected float OnInsertRequested(IEntity entity, float lifetime)
    {
        if (HAM_MilsimSettingsComponent.HAM_KeepWrecks() && Vehicle.Cast(entity))
            return -1;

        if (HAM_MilsimSettingsComponent.HAM_KeepCorpses() && ChimeraCharacter.Cast(entity))
            return -1;

        return super.OnInsertRequested(entity, lifetime);
    }
}
