modded class SCR_EditorManagerEntity
{
    override bool HasMode(EEditorMode mode)
    {
        if (HAM_IsPhotoModeDenied(mode))
            return false;

        return super.HasMode(mode);
    }

    override void Open(bool showErrorNotification = true)
    {
        if (HAM_IsPhotoModeDenied(GetCurrentMode()))
            return;

        super.Open(showErrorNotification);
    }

    protected bool HAM_IsPhotoModeDenied(EEditorMode mode)
    {
        return mode == EEditorMode.PHOTO && IsLimited() && HAM_MilsimSettingsComponent.HAM_BlockPhotoMode();
    }
}
