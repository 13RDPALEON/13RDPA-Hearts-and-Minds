modded class SCR_EditorManagerEntity
{
    protected int m_iHAM_EditorLog;
    protected string m_sHAM_LastEditorLog;

    override bool HasMode(EEditorMode mode)
    {
        if (HAM_IsPhotoModeDenied(mode))
            return false;

        return super.HasMode(mode);
    }

    override void Open(bool showErrorNotification = true)
    {
        if (HAM_IsPhotoModeDenied(GetCurrentMode()))
        {
            if (IsAuthority())
                HAM_LogEditor(string.Format("[13RDPA H&M] Éditeur PHOTO refusé, joueur %1", HAM_Util.PlayerTag(GetPlayerID())));

            return;
        }

        if (IsAuthority() && !IsOpened())
            HAM_LogEditor(string.Format("[13RDPA H&M] Éditeur ouverture, joueur %1, mode %2", HAM_Util.PlayerTag(GetPlayerID()), typename.EnumToString(EEditorMode, GetCurrentMode())));

        super.Open(showErrorNotification);
    }

    override void Close(bool showErrorNotification = true)
    {
        if (IsOpened())
            HAM_LogEditor(string.Format("[13RDPA H&M] Éditeur fermeture, joueur %1, mode %2", HAM_Util.PlayerTag(GetPlayerID()), typename.EnumToString(EEditorMode, GetCurrentMode())));

        super.Close(showErrorNotification);
    }

    override void SetCurrentMode(EEditorMode mode)
    {
        EEditorMode previous = GetCurrentMode();
        super.SetCurrentMode(mode);
        if (previous == mode)
            return;

        HAM_LogEditor(string.Format("[13RDPA H&M] Éditeur mode, joueur %1, %2 -> %3", HAM_Util.PlayerTag(GetPlayerID()), typename.EnumToString(EEditorMode, previous), typename.EnumToString(EEditorMode, mode)));
    }

    //! Même texte ignoré pendant 3 s : Open peut être redemandé tant que le RPC n'a pas basculé l'état.
    protected void HAM_LogEditor(string text)
    {
        int now = System.GetTickCount();
        if (text == m_sHAM_LastEditorLog)
        {
            int since = now - m_iHAM_EditorLog;
            if (m_iHAM_EditorLog != 0 && since >= 0 && since < 3000)
                return;
        }

        m_sHAM_LastEditorLog = text;
        m_iHAM_EditorLog = now;
        Print(text, LogLevel.NORMAL);
    }

    protected bool HAM_IsPhotoModeDenied(EEditorMode mode)
    {
        return mode == EEditorMode.PHOTO && IsLimited() && HAM_MilsimSettingsComponent.HAM_BlockPhotoMode();
    }
}
