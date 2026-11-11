// Hold/recovery logic shared by the local controller and Workbench tests.
enum HAM_EHoldBreathState
{
 IDLE,
 HOLDING,
 RECOVERY
}
enum HAM_EHoldBreathStopReason
{
 NONE,
 RELEASED,
 CONDITIONS_LOST,
 WEAPON_CHANGED,
 LOW_STAMINA,
 MAX_HOLD_TIME
}
class HAM_HoldBreathStateMachine
{
 protected HAM_EHoldBreathState m_State;
 protected HAM_EHoldBreathStopReason m_Reason;
 protected float m_HoldTime;
 protected float m_RecoveryLeft;
 protected float m_Blend;
 protected bool m_RequireRelease;
 protected bool m_HasStaminaCeiling;
 protected float m_StaminaCeiling;
 HAM_EHoldBreathState GetState() { return m_State; }
 HAM_EHoldBreathStopReason GetStopReason() { return m_Reason; }
 float GetHoldTime() { return m_HoldTime; }
 float GetRecoveryLeft() { return m_RecoveryLeft; }
 bool IsStaminaRecoveryBlocked() { return m_State == HAM_EHoldBreathState.HOLDING || (m_State == HAM_EHoldBreathState.RECOVERY && m_RecoveryLeft > 0); }
 // Called before Step, so the preceding hold/recovery interval is reconciled first.
 float GetStaminaRecoveryToCancel(float current)
 {
  if (!IsStaminaRecoveryBlocked() || !m_HasStaminaCeiling)
   return 0;
  return Math.Max(0, current - m_StaminaCeiling);
 }
 void RecordStamina(float current)
 {
  m_HasStaminaCeiling = true;
  m_StaminaCeiling = Math.Clamp(current, 0, 1);
 }
 bool CanStart() { return m_State == HAM_EHoldBreathState.IDLE && !m_RequireRelease; }
 float GetSwayMultiplier(HAM_HoldBreathConfig config, float targetMultiplier = -1)
 {
  if (!config)
   return 1;
  if (targetMultiplier < 0) targetMultiplier = config.HoldSwayMultiplier;
  return 1 - m_Blend * (1 - Math.Clamp(targetMultiplier, 0.01, 1));
 }
 void Reset()
 {
  m_State = HAM_EHoldBreathState.IDLE;
  m_Reason = HAM_EHoldBreathStopReason.NONE;
  m_HoldTime = 0;
  m_RecoveryLeft = 0;
  m_Blend = 0;
  m_RequireRelease = false;
  m_HasStaminaCeiling = false;
  m_StaminaCeiling = 0;
 }
 protected void Blend(float dt, bool holding, HAM_HoldBreathConfig config)
 {
  if (holding)
  {
   if (config.BlendInTime <= 0)
    m_Blend = 1;
   else
    m_Blend = Math.Min(1, m_Blend + dt / config.BlendInTime);
  }
  else
  {
   if (config.BlendOutTime <= 0)
    m_Blend = 0;
   else
    m_Blend = Math.Max(0, m_Blend - dt / config.BlendOutTime);
  }
 }
 protected void Stop(HAM_EHoldBreathStopReason reason, HAM_HoldBreathConfig config)
 {
  m_State = HAM_EHoldBreathState.RECOVERY;
  m_Reason = reason;
  m_RecoveryLeft = Math.Max(0, config.RecoveryTime);
  m_RequireRelease = true;
 }
 protected void AdvanceRecovery(float dt, HAM_HoldBreathConfig config)
 {
  m_RecoveryLeft = Math.Max(0, m_RecoveryLeft - dt);
  Blend(dt, false, config);
  if (m_RecoveryLeft <= 0)
   m_State = HAM_EHoldBreathState.IDLE;
 }
 // Returns the stamina amount requested for this tick. Caller must apply it on the owner only.
 float Step(float dt, bool eligible, bool pressed, float stamina, HAM_HoldBreathConfig config, bool weaponChanged = false)
 {
  if (!config || dt <= 0)
   return 0;
  if (!pressed)
   m_RequireRelease = false;
  if (m_State == HAM_EHoldBreathState.RECOVERY)
  {
   AdvanceRecovery(dt, config);
   return 0;
  }
  float minimum = Math.Clamp(config.MinStamina, 0, 1);
  if (m_State == HAM_EHoldBreathState.IDLE)
  {
   if (!CanStart() || !eligible || !pressed || stamina < minimum || weaponChanged || config.MaxHoldTime <= 0)
   {
    Blend(dt, false, config);
    return 0;
   }
   m_State = HAM_EHoldBreathState.HOLDING;
   m_Reason = HAM_EHoldBreathStopReason.NONE;
   m_HoldTime = 0;
  }
  if (!pressed || !eligible || weaponChanged || stamina < minimum)
  {
   HAM_EHoldBreathStopReason reason = HAM_EHoldBreathStopReason.CONDITIONS_LOST;
   if (!pressed)
    reason = HAM_EHoldBreathStopReason.RELEASED;
   else if (weaponChanged)
    reason = HAM_EHoldBreathStopReason.WEAPON_CHANGED;
   else if (stamina < minimum)
    reason = HAM_EHoldBreathStopReason.LOW_STAMINA;
   Stop(reason, config);
   AdvanceRecovery(dt, config);
   return 0;
  }
  float activeTime = Math.Min(dt, Math.Max(0, config.MaxHoldTime - m_HoldTime));
  float rate = Math.Max(0, config.StaminaDrainPerSecond);
  float budget = Math.Max(0, stamina - minimum);
  if (rate > 0)
   activeTime = Math.Min(activeTime, budget / rate);
  float drained = activeTime * rate;
  m_HoldTime += activeTime;
  Blend(activeTime, true, config);
  if (m_HoldTime >= config.MaxHoldTime)
   Stop(HAM_EHoldBreathStopReason.MAX_HOLD_TIME, config);
  else if (rate > 0 && drained >= budget)
   Stop(HAM_EHoldBreathStopReason.LOW_STAMINA, config);
  if (m_State != HAM_EHoldBreathState.HOLDING)
   AdvanceRecovery(dt - activeTime, config);
  return drained;
 }
}
