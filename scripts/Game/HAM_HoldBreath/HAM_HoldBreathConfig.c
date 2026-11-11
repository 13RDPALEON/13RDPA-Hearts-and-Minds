[BaseContainerProps()]
class HAM_HoldBreathWeightOverride
{
 [Attribute("", UIWidgets.EditBox, "Exact prefab resource identifier; optional addon need not be a dependency")] string WeaponPrefab;
 [Attribute("0", UIWidgets.EditBox, "Override rifle weight in kilograms; 0 uses native weight")] float WeightKg;
}
[BaseContainerProps()]
class HAM_HoldBreathConfig
{
 [Attribute("20.0", UIWidgets.EditBox, "Maximum hold duration in seconds")] float MaxHoldTime;
 [Attribute("0", UIWidgets.EditBox, "Minimum normalized stamina")] float MinStamina;
 [Attribute("0.15", UIWidgets.EditBox, "Remaining sway; 1 is unchanged")] float HoldSwayMultiplier;
 [Attribute("0.30", UIWidgets.EditBox, "Seconds to blend into stabilization")] float BlendInTime;
 [Attribute("0.75", UIWidgets.EditBox, "Seconds to restore ordinary sway")] float BlendOutTime;
 [Attribute("1.5", UIWidgets.EditBox, "Cooldown and stamina recovery pause in seconds after holding stops")] float RecoveryTime;
 [Attribute("0.06", UIWidgets.EditBox, "Normalized stamina drain per second")] float StaminaDrainPerSecond;
 [Attribute("2.0", UIWidgets.EditBox, "Seconds with no aim compensation after a shot; minimum 2")] float RecoilProtectionTime;
 [Attribute("0.04", UIWidgets.EditBox, "Time constant in seconds for smooth additive aim compensation")] float AimCorrectionResponseTime;
 [Attribute("1")] bool EnableBreathSounds;
 [Attribute("1", UIWidgets.CheckBox, "Relay short positional breath sounds to other players")] bool EnableRemoteBreathSounds;
 [Attribute("2.5", UIWidgets.EditBox, "Minimum recovery audio window in seconds after releasing breath")] float BreathRecoveryMinTime;
 [Attribute("4.5", UIWidgets.EditBox, "Recovery audio window after a full hold; gameplay cooldown is unchanged")] float BreathRecoveryMaxTime;
 [Attribute("0.45", UIWidgets.EditBox, "Silence after the main exhale before recovery starts, seconds; minimum 0.1")] float BreathRecoveryInitialPause;
 [Attribute("0.30", UIWidgets.EditBox, "Silence after a recovery inhale, seconds; minimum 0.1")] float BreathRecoveryPauseAfterIn;
 [Attribute("0.55", UIWidgets.EditBox, "Silence after a recovery exhale, seconds; minimum 0.1")] float BreathRecoveryPauseAfterOut;
 [Attribute("0")] bool SniperOnly;
 [Attribute("1")] bool ShowControlHint;
 [Attribute("", UIWidgets.ResourceNamePicker, "Exact prefab resources; highest priority", "et")]
 ref array<ResourceName> DenyList;
 [Attribute("", UIWidgets.ResourceNamePicker, "Exact prefab resources; overrides WeaponType", "et")]
 ref array<ResourceName> AllowList;
 [Attribute("1")] bool UseWeaponWeight;
 [Attribute("3.75", UIWidgets.EditBox, "Reference rifle weight in kilograms")] float WeightReferenceKg;
 [Attribute("0.5", UIWidgets.EditBox, "Weight influence exponent; 0 keeps the reference multiplier")] float WeightExponent;
 [Attribute("0.10", UIWidgets.EditBox, "Minimum remaining sway with weight scaling")] float MinHoldSwayMultiplier;
 [Attribute("0.50", UIWidgets.EditBox, "Maximum remaining sway with weight scaling")] float MaxHoldSwayMultiplier;
 [Attribute()] ref array<ref HAM_HoldBreathWeightOverride> WeightOverrides;
 protected static ref HAM_HoldBreathConfig s_Config;
 protected static bool s_LoadAttempted;
 static HAM_HoldBreathConfig Get()
 {
  if (!s_LoadAttempted)
  {
   s_LoadAttempted = true;
   Resource resource = Resource.Load("{6EE8EF09B75313A0}Configs/HAM_HoldBreath/Settings.conf");
   if (resource && resource.IsValid())
    s_Config = HAM_HoldBreathConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(resource.GetResource().ToBaseContainer()));
  }
  return s_Config;
 }
 void HAM_ApplyValues(float maxHold, float minStamina, bool sounds, bool sniperOnly)
 {
  if (maxHold < 0.5)
   maxHold = 0.5;
  if (minStamina < 0)
   minStamina = 0;
  MaxHoldTime = maxHold;
  MinStamina = minStamina;
  EnableBreathSounds = sounds;
  SniperOnly = sniperOnly;
 }
 static void HAM_ApplyRuntime(float maxHold, float minStamina, bool sounds, bool sniperOnly)
 {
  HAM_HoldBreathConfig config = Get();
  if (!config)
   return;
  config.HAM_ApplyValues(maxHold, minStamina, sounds, sniperOnly);
 }
}
