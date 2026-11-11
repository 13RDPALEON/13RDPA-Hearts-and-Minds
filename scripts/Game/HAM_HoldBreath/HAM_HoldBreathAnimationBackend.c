// Independent backend: scales vanilla breathing additive animation, not native aim offsets.
class HAM_HoldBreathAnimationBackend
{
 protected CharacterAnimationComponent m_Animation;
 protected TAnimGraphVariable m_Amplitude = -1;
 protected bool m_Attempted;
 protected bool m_HasApplied;
 protected float m_LastAmplitude;
 bool Bind(CharacterAnimationComponent animation)
 {
  if (animation != m_Animation)
  {
   Restore();
   m_Animation = animation;
   m_Attempted = false;
   m_Amplitude = -1;
   m_HasApplied = false;
  }
  // Le graphe vanilla du personnage n'a pas cette variable.
  // On ne la lie pas : la chercher plantait l'animation. La visée passe par le modificateur d'arme.
  m_Attempted = true;
  return true;
 }
 void Apply(float amplitude)
 {
  if (!m_Animation || m_Amplitude < 0)
   return;
  amplitude = Math.Clamp(amplitude, 0.01, 1);
  if (m_HasApplied && m_LastAmplitude == amplitude)
   return;
  m_Animation.SetVariableFloat(m_Amplitude, amplitude);
  m_LastAmplitude = amplitude;
  m_HasApplied = true;
 }
 void Restore() { Apply(1); }
}
