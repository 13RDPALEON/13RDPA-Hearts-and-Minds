//------------------------------------------------------------------------------------------------
// Moteur d'avion, en boucle, uniquement pour le joueur qui est dans le C-130.
//------------------------------------------------------------------------------------------------

void HAM_ParadropAudioTick()
{
	HAM_ParadropAudio.Tick();
}

class HAM_ParadropAudio
{
	protected static bool s_bOn;
	protected static AudioHandle s_Handle;
	protected static const ResourceName SAMPLE = "{6AA33470EE000001}Sounds/TRN_Para/TRN_PlaneEngine.wav";

	static void Set(bool on)
	{
		s_bOn = on;
		if (!on)
			Stop();
	}

	static void Tick()
	{
		IEntity body = SCR_PlayerController.GetLocalControlledEntity();
		if (s_bOn && body && body.GetOrigin()[1] < 800)
			Set(false);

		if (!s_bOn)
			return;

		if (s_Handle != AudioHandle.Invalid && AudioSystem.IsSoundPlayed(s_Handle))
			return;

		Resource resource = Resource.Load(SAMPLE);
		if (!resource || !resource.IsValid())
			return;

		s_Handle = AudioSystem.PlaySound(SAMPLE);
	}

	protected static void Stop()
	{
		if (s_Handle != AudioHandle.Invalid)
			AudioSystem.TerminateSoundFadeOut(s_Handle, true, 0.1);

		s_Handle = AudioHandle.Invalid;
	}
}
