// Voix de proximité du Game Master : la position écoutée est la caméra, pas le point
// figé de l'entité éditeur. Code original, API vanilla (SCR_CameraEditorComponent,
// VoNComponent.GetEditorWorldLocation, ConnectEditorToVoNSystem).
// L'addon SNK GM Voice est sous licence APL-ND : son contenu n'est pas copié.
//------------------------------------------------------------------------------------------------
modded class SCR_VoNComponent
{
	override protected event vector GetEditorWorldLocation(int playerId)
	{
		if (playerId == SCR_PlayerController.GetLocalPlayerId())
		{
			SCR_ManualCamera camera = SCR_CameraEditorComponent.GetCameraInstance();
			if (camera)
				return camera.GetOrigin();
		}

		return super.GetEditorWorldLocation(playerId);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_CameraEditorComponent
{
	protected static float s_fHAM_NextVoicePush;
	protected static bool s_bHAM_EditorVoice;

	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		HAM_PushVoice();
	}

	protected void HAM_PushVoice()
	{
		SCR_EditorManagerEntity editor = GetEditorManagerEntity();
		if (!editor || !editor.IsOpened() || !m_Camera)
		{
			s_bHAM_EditorVoice = false;
			return;
		}

		float now = 0;
		if (GetGame().GetWorld())
			now = GetGame().GetWorld().GetWorldTime();

		if (!s_bHAM_EditorVoice)
		{
			SCR_VoNComponent von = SCR_VoNComponent.Cast(editor.FindComponent(SCR_VoNComponent));
			PlayerController controller = GetGame().GetPlayerController();
			if (von && controller)
				von.ConnectEditorToVoNSystem(controller.GetPlayerId());

			s_bHAM_EditorVoice = true;
		}

		if (now < s_fHAM_NextVoicePush)
			return;

		s_fHAM_NextVoicePush = now + 400;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
			return;

		player.HAM_PushGmVoice(m_Camera.GetOrigin());
	}
}
