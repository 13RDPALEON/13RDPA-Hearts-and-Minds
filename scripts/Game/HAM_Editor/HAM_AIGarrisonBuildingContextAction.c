// Clic droit GM « Garnir le bâtiment ». Textes déjà dans HAM_AI_localization.
// Le filtre de bibliothèque est le config vanilla des groupes, pas un prefab nouveau.
//------------------------------------------------------------------------------------------------
class HAM_AIGarrisonBuildingContextAction : SCR_BaseContextAction
{
	protected SCR_DestructibleBuildingEntity m_Building;
	protected ref SCR_EditorContentBrowserDisplayConfig m_BrowserConfig;

	protected static const ResourceName BROWSER_CONFIG = "{9259B3DA5843CDB9}Configs/Editor/ContentBrowser/PassengerLargeGroupsOrLessLabelConfig.conf";

	void HAM_InitInfo(SCR_UIInfo info)
	{
		m_Info = info;
	}

	protected SCR_EditorContentBrowserDisplayConfig BrowserConfig()
	{
		if (m_BrowserConfig)
			return m_BrowserConfig;

		Resource container = BaseContainerTools.LoadContainer(BROWSER_CONFIG);
		if (!container || !container.IsValid())
			return null;

		m_BrowserConfig = SCR_EditorContentBrowserDisplayConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
		return m_BrowserConfig;
	}

	override bool CanBeShown(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		m_Building = null;
		SCR_CameraEditorComponent cameraManager = SCR_CameraEditorComponent.Cast(SCR_CameraEditorComponent.GetInstance(SCR_CameraEditorComponent));
		if (!cameraManager)
			return false;

		SCR_ManualCamera camera = cameraManager.GetCamera();
		if (!camera)
			return false;

		TraceParam trace = new TraceParam();
		trace.Start = camera.GetOrigin();
		trace.End = trace.Start + 1.1 * (cursorWorldPosition - trace.Start);
		trace.Flags = TraceFlags.ENTS;
		GetGame().GetWorld().TraceMove(trace, null);
		m_Building = SCR_DestructibleBuildingEntity.Cast(trace.TraceEnt);
		return m_Building != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags)
	{
		return CanBeShown(hoveredEntity, selectedEntities, cursorWorldPosition, flags);
	}

	override void Perform(SCR_EditableEntityComponent hoveredEntity, notnull set<SCR_EditableEntityComponent> selectedEntities, vector cursorWorldPosition, int flags, int param = -1)
	{
		if (!m_Building)
			return;

		SCR_PlacingEditorComponent placing = SCR_PlacingEditorComponent.Cast(SCR_PlacingEditorComponent.GetInstance(SCR_PlacingEditorComponent));
		if (!placing || placing.IsPlacing())
			return;

		SCR_ContentBrowserEditorComponent browser = SCR_ContentBrowserEditorComponent.Cast(SCR_ContentBrowserEditorComponent.GetInstance(SCR_ContentBrowserEditorComponent, true));
		if (!browser)
			return;

		placing.HAM_ArmGarrison(m_Building.GetOrigin());
		SCR_EditorContentBrowserDisplayConfig config = BrowserConfig();
		if (!config || !browser.OpenBrowserLabelConfig(config))
			return;

		vector transform[4];
		m_Building.GetWorldTransform(transform);
		vector groundPos = transform[3];
		groundPos[1] = GetGame().GetWorld().GetSurfaceY(groundPos[0], groundPos[2]);
		transform[3] = groundPos;
		SCR_EditorPreviewParams previewParams = SCR_EditorPreviewParams.CreateParams(transform, verticalMode: EEditorTransformVertical.TERRAIN);
		placing.SetInstantPlacing(previewParams);
	}
}
