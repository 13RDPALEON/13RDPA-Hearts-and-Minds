// Original additive weapon-space compensation. No camera/FOV or recoil setters.
class HAM_HoldBreathAimFilter
{
 protected vector m_CentreTranslation;
 protected vector m_CentreRotation;
 protected vector m_Translation;
 protected vector m_Rotation;
 protected bool m_Ready;
 void Reset()
 {
  m_Ready = false;
  m_Translation = vector.Zero;
  m_Rotation = vector.Zero;
 }
 void Calculate(vector nativeTranslation, vector nativeRotation, float multiplier, out vector translation, out vector rotation, float timeSlice = 0, float responseTime = 0)
 {
  float strength = 1 - Math.Clamp(multiplier, 0.01, 1);
  if (!m_Ready || strength <= 0)
  {
   m_CentreTranslation = nativeTranslation;
   m_CentreRotation = nativeRotation;
   m_Translation = vector.Zero;
   m_Rotation = vector.Zero;
   m_Ready = true;
  }
  vector targetTranslation = (m_CentreTranslation - nativeTranslation) * strength;
  vector targetRotation = (m_CentreRotation - nativeRotation) * strength;
  float alpha = 1;
  if (responseTime > 0)
   alpha = 1 - Math.Pow(2.718281828, -Math.Max(0, timeSlice) / responseTime);
  // Smooth only our additive correction, using elapsed time rather than frames.
  // The native aiming input, recoil and camera remain untouched.
  m_Translation += (targetTranslation - m_Translation) * alpha;
  m_Rotation += (targetRotation - m_Rotation) * alpha;
  translation = m_Translation;
  rotation = m_Rotation;
 }
}
class HAM_HoldBreathAimModifier : ScriptedWeaponAimModifier
{
 static const bool DIAGNOSTICS = false;
 protected IEntity m_Weapon;
 protected int m_DiagnosticSamples;
 protected float m_DiagnosticTime;
 protected SCR_CharacterControllerComponent m_Controller;
 override void OnInit(IEntity weaponEnt)
 {
  m_Weapon = weaponEnt;
  if (DIAGNOSTICS) Print(string.Format("[TRN aim] Initialized weapon=%1", weaponEnt));
 }
 protected ref HAM_HoldBreathAimFilter m_Filter;
 protected float m_RecoilGuard;
 override void OnActivated(IEntity weaponOwner)
 {
  if (System.IsConsoleApp())
   return;
  if (weaponOwner)
   m_Controller = SCR_CharacterControllerComponent.Cast(weaponOwner.FindComponent(CharacterControllerComponent));
  if (m_Filter) m_Filter.Reset();
  m_RecoilGuard = 0;
  m_DiagnosticSamples = 0;
  if (DIAGNOSTICS) Print(string.Format("[TRN aim] Activated parent=%1 controller=%2 weapon=%3", weaponOwner, m_Controller, m_Weapon));
 }
 override void OnDeactivated(IEntity weaponOwner)
 {
  m_Controller = null;
  if (m_Filter) m_Filter.Reset();
 }
 override void OnWeaponFired()
 {
  if (System.IsConsoleApp() || !m_Controller)
   return;
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  m_RecoilGuard = 2;
  if (config)
   m_RecoilGuard = Math.Max(2, config.RecoilProtectionTime);
  if (m_Filter) m_Filter.Reset();
 }
 override void OnCalculate(IEntity owner, WeaponAimModifierContext context, float timeSlice, out vector translation, out vector rotation, out vector turnOffset)
 {
  // These outputs hold THIS modifier's last contribution, not native sway.
  vector previousTranslation = translation;
  vector previousRotation = rotation;
  translation = vector.Zero;
  rotation = vector.Zero;
  turnOffset = vector.Zero;
  if (DIAGNOSTICS && m_DiagnosticSamples < 2)
  {
   Print(string.Format("[TRN aim] Calculate owner=%1 initializedWeapon=%2 controller=%3", owner, m_Weapon, m_Controller));
   m_DiagnosticSamples++;
  }
  if (!m_Controller)
   return;
  m_RecoilGuard = Math.Max(0, m_RecoilGuard - Math.Max(0, timeSlice));
  // Workbench 1.8 passes the character as callback owner; OnInit gives the weapon.
  float multiplier = m_Controller.HAM_GetWeaponSwayMultiplier(m_Weapon);
  // Remote characters and idle weapons contribute zero without allocating a filter.
  if (multiplier >= 1)
  {
   if (m_Filter) m_Filter.Reset();
   return;
  }
  CharacterAimingComponent aiming = m_Controller.GetAimingComponent();
  CharacterInputContext input = m_Controller.GetInputContext();
  bool trigger = input && input.WeaponIsPullingTrigger();
  vector recoilTranslation;
  if (aiming) recoilTranslation = aiming.GetCurrentRecoilTranslation();
  if (DIAGNOSTICS && m_Controller.IsWeaponADS())
  {
   m_DiagnosticTime += timeSlice;
   if (m_DiagnosticTime >= 0.5)
   {
    m_DiagnosticTime = 0;
    Print(string.Format("[TRN aim] ADS multiplier=%1 guard=%2 trigger=%3 recoilTranslation=%4", multiplier, m_RecoilGuard, trigger, recoilTranslation));
    if (aiming) Print(string.Format("[TRN aim] rawRotation=%1 previousCorrection=%2", aiming.GetRawAimingRotation(), previousRotation));
   }
  }
  if (!aiming || m_RecoilGuard > 0 || trigger || recoilTranslation.LengthSq() > 0.00000001)
  {
   if (m_Filter) m_Filter.Reset();
   return;
  }
  // Raw rotation is radians; modifier output is degrees. Raw sample contains
  // the preceding frame's own contribution, which must be removed first.
  vector nativeTranslation = aiming.GetRawAimingTranslation() - previousTranslation;
  vector nativeRotation = aiming.GetRawAimingRotation() * Math.RAD2DEG - previousRotation;
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  float responseTime = 0.04;
  if (config) responseTime = Math.Clamp(config.AimCorrectionResponseTime, 0.01, 0.2);
  if (!m_Filter)
   m_Filter = new HAM_HoldBreathAimFilter();
  m_Filter.Calculate(nativeTranslation, nativeRotation, multiplier, translation, rotation, timeSlice, responseTime);
 }
}
