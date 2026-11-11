// ====================================================================================
// HAM_CharacterCameraHandlerComponent.c — 13RDPA (CLIENT)
//
// Toggles GM « 1re personne forcée » séparés à pied / en véhicule (tourelles
// incluses — elles passent par IsInVehicle()).
//
// Deux points d'application :
//   - OnCameraSwitchPressed : bloque le passage 1re → 3e personne dans un contexte
//     interdit (le retour 3e → 1re reste toujours possible).
//   - CameraSelector (évalué en continu) : rappel à l'ordre pour les cas que le
//     switch ne couvre pas — joueur déjà en 3e personne quand le GM active le
//     réglage, ou transition de contexte (ex. 3e personne à pied autorisée, entrée
//     dans un véhicule où elle est interdite).
//
// La caméra de l'éditeur GM et le mode photo sont des systèmes séparés → non
// affectés. Aucun autre mod FOF ne modde SCR_CharacterCameraHandlerComponent.
//
// ⚠️ Le fichier vanilla s'appelle CharacterCameraHandlerComponent.c mais la classe
// SCRIPT est SCR_CharacterCameraHandlerComponent (elle étend la classe NATIVE
// CameraHandlerComponent, non moddable — « Unknown type » au Rebuild sinon).
// ====================================================================================
modded class SCR_CharacterCameraHandlerComponent
{
    protected bool HAM_IsThirdPersonBlocked()
    {
        if (m_OwnerCharacter && m_OwnerCharacter.IsInVehicle())
            return HAM_MilsimSettingsComponent.HAM_ForceFirstPersonVehicle();

        return HAM_MilsimSettingsComponent.HAM_ForceFirstPersonFoot();
    }

    override void OnCameraSwitchPressed()
    {
        if (!IsInThirdPerson() && HAM_IsThirdPersonBlocked())
            return;

        super.OnCameraSwitchPressed();
    }

    override int CameraSelector()
    {
        if (IsInThirdPerson() && HAM_IsThirdPersonBlocked())
            SetThirdPerson(false);

        return super.CameraSelector();
    }
}
