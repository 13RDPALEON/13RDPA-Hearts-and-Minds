//------------------------------------------------------------------------------------------------
// 13RDPA — menu clic-droit GM : dump, copier stuff, coller stuff.
//------------------------------------------------------------------------------------------------
modded class SCR_ContextActionsEditorComponent
{
	protected ref HAM_DumpMannequinContextAction m_HAM_DumpMannequinAction;
	protected ref HAM_CopyLoadoutContextAction m_HAM_CopyLoadoutAction;
	protected ref HAM_PasteLoadoutContextAction m_HAM_PasteLoadoutAction;
	protected ref HAM_AIGarrisonBuildingContextAction m_HAM_GarrisonBuildingAction;

	//------------------------------------------------------------------------------------------------
	override int GetActions(out notnull array<SCR_BaseEditorAction> actions)
	{
		super.GetActions(actions);

		if (!m_HAM_DumpMannequinAction)
		{
			m_HAM_DumpMannequinAction = new HAM_DumpMannequinContextAction();

			SCR_UIInfo info = new SCR_UIInfo();
			info.SetName("Dump stuff");
			info.SetDescription("Exporte la tenue Bacon + le détail des slots. Fichiers dans 13RDPA_HeartsAndMinds/Debug : HAM_MannequinDump.txt et HAM_LoadoutDump.json");
			m_HAM_DumpMannequinAction.HAM_InitInfo(info);
		}

		if (!m_HAM_CopyLoadoutAction)
		{
			m_HAM_CopyLoadoutAction = new HAM_CopyLoadoutContextAction();

			SCR_UIInfo copyInfo = new SCR_UIInfo();
			copyInfo.SetName("Copier stuff");
			copyInfo.SetDescription("Mémorise la tenue de ce personnage (même format que Bacon).");
			m_HAM_CopyLoadoutAction.HAM_InitInfo(copyInfo);
		}

		if (!m_HAM_PasteLoadoutAction)
		{
			m_HAM_PasteLoadoutAction = new HAM_PasteLoadoutContextAction();

			SCR_UIInfo pasteInfo = new SCR_UIInfo();
			pasteInfo.SetName("Coller stuff");
			pasteInfo.SetDescription("Applique la tenue copiée sur ce personnage.");
			m_HAM_PasteLoadoutAction.HAM_InitInfo(pasteInfo);
		}

		if (!m_HAM_GarrisonBuildingAction)
		{
			m_HAM_GarrisonBuildingAction = new HAM_AIGarrisonBuildingContextAction();
			SCR_UIInfo garrisonInfo = new SCR_UIInfo();
			garrisonInfo.SetName("#HAM-Action_Garrison_Name");
			garrisonInfo.SetDescription("#HAM-Action_Garrison_Desc");
			m_HAM_GarrisonBuildingAction.HAM_InitInfo(garrisonInfo);
		}

		actions.Insert(m_HAM_DumpMannequinAction);
		actions.Insert(m_HAM_CopyLoadoutAction);
		actions.Insert(m_HAM_PasteLoadoutAction);
		actions.Insert(m_HAM_GarrisonBuildingAction);
		return actions.Count();
	}
}
