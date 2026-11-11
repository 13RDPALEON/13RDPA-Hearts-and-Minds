// Native stamina balance for the existing local control update; no global callbacks.
class HAM_HoldBreathStamina
{
 static float Update(CharacterStaminaComponent stamina, HAM_HoldBreathStateMachine state, float current, float dt, bool eligible, bool pressed, HAM_HoldBreathConfig config, bool weaponChanged = false)
 {
  float recovered = state.GetStaminaRecoveryToCancel(current);
  float available = Math.Max(0, current - recovered);
  float drain = state.Step(dt, eligible, pressed, available, config, weaponChanged);
  float spent = recovered + drain;
  float after = available;
  if (stamina && spent > 0)
  {
   stamina.AddStamina(-spent);
   after = stamina.GetStamina();
  }
  state.RecordStamina(after);
  return after;
 }
}
