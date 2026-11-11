// Vanilla breath samples, played in 2D only by the locally controlled character.
enum HAM_EBreathCue
{
 NONE,
 INHALE,
 EXHALE,
 RECOVERY_IN,
 RECOVERY_OUT
}
class HAM_HoldBreathAudioSequence
{
 protected bool m_Holding;
 protected float m_RecoveryLeft;
 protected float m_PauseLeft;
 // All four referenced vanilla WAVs are <= 0.5 s; include 0.05 s for playback startup.
 protected float m_SampleLeft;
 protected bool m_NextIn = true;
 void Reset()
 {
  m_Holding = false;
  m_RecoveryLeft = 0;
  m_PauseLeft = 0;
  m_SampleLeft = 0;
  m_NextIn = true;
 }
 float GetRecoveryLeft() { return m_RecoveryLeft; }
 bool IsActive() { return m_Holding || m_RecoveryLeft > 0; }
 static string GetEvent(HAM_EBreathCue cue)
 {
  switch (cue)
  {
   case HAM_EBreathCue.INHALE: return "SOUND_BREATH_IN";
   case HAM_EBreathCue.EXHALE: return "SOUND_BREATH_OUT";
   case HAM_EBreathCue.RECOVERY_IN: return "SOUND_BREATH_REG_IN";
   case HAM_EBreathCue.RECOVERY_OUT: return "SOUND_BREATH_REG_OUT";
  }
  return string.Empty;
 }
 protected HAM_EBreathCue Emit(HAM_EBreathCue cue, float pause)
 {
  m_SampleLeft = 0.55;
  m_PauseLeft = Math.Max(0.1, pause);
  return cue;
 }
 HAM_EBreathCue Step(float dt, bool holding, float heldTime, bool soundPlaying, HAM_HoldBreathConfig config)
 {
  if (!config || !config.EnableBreathSounds)
  {
   Reset();
   return HAM_EBreathCue.NONE;
  }
  if (holding != m_Holding)
  {
   m_Holding = holding;
   m_NextIn = true;
   if (holding)
   {
    m_RecoveryLeft = 0;
    return Emit(HAM_EBreathCue.INHALE, 0);
   }
   float effort = Math.Clamp(heldTime / Math.Max(0.01, config.MaxHoldTime), 0, 1);
   m_RecoveryLeft = Math.Max(0, config.BreathRecoveryMinTime);
   m_RecoveryLeft += effort * Math.Max(0, config.BreathRecoveryMaxTime - m_RecoveryLeft);
   return Emit(HAM_EBreathCue.EXHALE, config.BreathRecoveryInitialPause);
  }
  if (dt <= 0) return HAM_EBreathCue.NONE;
  bool sampleGuarded = m_SampleLeft > 0;
  m_SampleLeft = Math.Max(0, m_SampleLeft - dt);
  if (holding || m_RecoveryLeft <= 0) return HAM_EBreathCue.NONE;
  m_RecoveryLeft = Math.Max(0, m_RecoveryLeft - dt);
  // Do not spend the silence budget in a frame that may still contain the clip.
  // Native playback can start asynchronously or briefly report false.
  if (sampleGuarded || soundPlaying) return HAM_EBreathCue.NONE;
  m_PauseLeft = Math.Max(0, m_PauseLeft - dt);
  if (m_RecoveryLeft <= 0 || m_PauseLeft > 0)
   return HAM_EBreathCue.NONE;
  m_NextIn = !m_NextIn;
  if (!m_NextIn)
   return Emit(HAM_EBreathCue.RECOVERY_IN, config.BreathRecoveryPauseAfterIn);
  return Emit(HAM_EBreathCue.RECOVERY_OUT, config.BreathRecoveryPauseAfterOut);
 }
}
class HAM_HoldBreathAudio
{
 protected ref HAM_HoldBreathAudioSequence m_Sequence = new HAM_HoldBreathAudioSequence();
 protected AudioHandle m_Handle = AudioHandle.Invalid;
 protected ref map<ResourceName, ref Resource> m_Samples = new map<ResourceName, ref Resource>();
 // Native IsSoundPlayed means the clip has finished (Workbench probe: false at start, true after 0.5 s).
 static bool IsPlaying(AudioHandle handle)
 {
  return handle != AudioHandle.Invalid && !AudioSystem.IsSoundPlayed(handle);
 }
 static ResourceName GetSample(HAM_EBreathCue cue)
 {
  switch (cue)
  {
   case HAM_EBreathCue.INHALE: return "{16F184F777D08F2C}Sounds/Character/Voice/Samples/Eng/Male1/Breath/Character_Voice_Breath_In_500_01.wav";
   case HAM_EBreathCue.EXHALE: return "{E0BD3430CD7E6505}Sounds/Character/Voice/Samples/Eng/Male1/Breath/Character_Voice_Breath_Out_500_01.wav";
   case HAM_EBreathCue.RECOVERY_IN: return "{D1D62BCA5A59C67B}Sounds/Character/Voice/Samples/Eng/Male1/Breath/Character_Voice_Breath_Regenerate_In_500_01.wav";
   case HAM_EBreathCue.RECOVERY_OUT: return "{65CFAAB12715D68D}Sounds/Character/Voice/Samples/Eng/Male1/Breath/Character_Voice_Breath_Regenerate_Out_500_01.wav";
  }
  return string.Empty;
 }
 void Reset()
 {
  if (IsPlaying(m_Handle))
   AudioSystem.TerminateSoundFadeOut(m_Handle, true, 0.05);
  m_Handle = AudioHandle.Invalid;
  m_Sequence.Reset();
 }
 HAM_EBreathCue Update(IEntity owner, float dt, HAM_HoldBreathStateMachine state, HAM_HoldBreathConfig config)
 {
  if (!owner || !state || !config || !config.EnableBreathSounds)
  {
   Reset();
   return HAM_EBreathCue.NONE;
  }
  bool holding = state.GetState() == HAM_EHoldBreathState.HOLDING;
  if (!holding && !m_Sequence.IsActive())
   return HAM_EBreathCue.NONE;
  bool playing = IsPlaying(m_Handle);
  HAM_EBreathCue cue = m_Sequence.Step(dt, holding, state.GetHoldTime(), playing, config);
  if (cue == HAM_EBreathCue.NONE)
   return HAM_EBreathCue.NONE;
  ResourceName sample = GetSample(cue);
  if (!m_Samples.Contains(sample))
  {
   Resource resource = Resource.Load(sample);
   if (!resource || !resource.IsValid()) return HAM_EBreathCue.NONE;
   m_Samples.Set(sample, resource);
  }
  if (IsPlaying(m_Handle))
   AudioSystem.TerminateSoundFadeOut(m_Handle, true, 0.05);
  m_Handle = AudioSystem.PlaySound(sample);
  return cue;
 }
 void ~HAM_HoldBreathAudio()
 {
  Reset();
 }
}
