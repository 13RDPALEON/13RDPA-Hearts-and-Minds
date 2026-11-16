modded class SCR_CameraEditorComponent
{
	protected float m_fSNK_NextSendTimeMs;
	protected vector m_vSNK_LastSentPosition;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		if (!m_Camera)
			return;

		SCR_EditorManagerEntity editorManager = GetEditorManagerEntity();
		if (!editorManager || !editorManager.IsOpened() || !editorManager.IsOwner())
			return;

		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return;

		float nowMs = world.GetWorldTime();
		if (nowMs < m_fSNK_NextSendTimeMs)
			return;

		vector cameraPosition = m_Camera.GetOrigin();
		if (vector.Distance(cameraPosition, m_vSNK_LastSentPosition) < 0.5)
			return;

		m_fSNK_NextSendTimeMs = nowMs + 100;
		m_vSNK_LastSentPosition = cameraPosition;
		editorManager.SNK_PushCameraPosition(cameraPosition);
	}
}
