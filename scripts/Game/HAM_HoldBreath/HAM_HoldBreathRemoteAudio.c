// Cosmetic event relay only; existing character replication supplies ownership/relevancy.
class HAM_HoldBreathCueGate
{
 protected int m_Actor;
 protected int m_Sequence;
 protected float m_LastAt = -10000;
 protected float m_LastIn = -10000;
 protected HAM_EBreathCue m_LastCue;
 bool Accept(HAM_EBreathCue cue, int sequence, int actor, float nowMs)
 {
  if (cue < HAM_EBreathCue.INHALE || cue > HAM_EBreathCue.RECOVERY_OUT || sequence <= 0 || actor <= 0)
   return false;
  if (actor != m_Actor)
  {
   m_Actor = actor;
   m_Sequence = 0;
   m_LastAt = -10000;
   m_LastIn = -10000;
   m_LastCue = HAM_EBreathCue.NONE;
  }
  if (sequence <= m_Sequence) return false;
  m_Sequence = sequence;
  if (cue == HAM_EBreathCue.INHALE && nowMs - m_LastIn < 500) return false;
  if (cue == HAM_EBreathCue.EXHALE && m_LastCue != HAM_EBreathCue.INHALE && nowMs - m_LastAt < 550) return false;
  if (cue >= HAM_EBreathCue.RECOVERY_IN && nowMs - m_LastAt < 550) return false;
  m_LastAt = nowMs;
  if (cue == HAM_EBreathCue.INHALE) m_LastIn = nowMs;
  m_LastCue = cue;
  return true;
 }
}
class HAM_HoldBreathRemoteAudio
{
 static const ResourceName PROJECT = "{56B5752AF6AF19E6}Sounds/HAM_HoldBreath/BreathSpatial.acp";
 static const float RANGE = 8;
 protected ref SCR_AudioSource m_Source;
 protected float m_LastPlayAt = -10000;
 static string GetEvent(HAM_EBreathCue cue)
 {
  switch (cue)
  {
   case HAM_EBreathCue.INHALE: return "HAM_BREATH_IN";
   case HAM_EBreathCue.EXHALE: return "HAM_BREATH_OUT";
   case HAM_EBreathCue.RECOVERY_IN: return "HAM_BREATH_REG_IN";
   case HAM_EBreathCue.RECOVERY_OUT: return "HAM_BREATH_REG_OUT";
  }
  return string.Empty;
 }
 void Play(IEntity owner, HAM_EBreathCue cue)
 {
  if (!owner || System.IsConsoleApp() || AudioSystem.GetDistance(owner.GetOrigin()) > RANGE) return;
  float nowMs = owner.GetWorld().GetWorldTime();
  bool recovery = cue >= HAM_EBreathCue.RECOVERY_IN;
  if (recovery && (nowMs - m_LastPlayAt < 550 || (m_Source && HAM_HoldBreathAudio.IsPlaying(m_Source.m_AudioHandle)))) return;
  SCR_SoundManagerModule manager = SCR_SoundManagerModule.GetInstance(owner.GetWorld());
  if (!manager) return;
  SCR_AudioSourceConfiguration config = new SCR_AudioSourceConfiguration();
  config.m_sSoundProject = PROJECT;
  config.m_sSoundEventName = GetEvent(cue);
  // No interior query or per-event CIE update loop; the native manager follows the entity.
  config.m_eFlags = EAudioSourceConfigurationFlag.ExteriorSource;
  SCR_AudioSource source = manager.CreateAudioSource(owner, config);
  if (!source) return;
  if (m_Source) m_Source.Terminate(false);
  m_Source = source;
  m_LastPlayAt = nowMs;
  manager.PlayAudioSource(source);
 }
 void ~HAM_HoldBreathRemoteAudio()
 {
  if (m_Source) m_Source.Terminate(false);
 }
}
