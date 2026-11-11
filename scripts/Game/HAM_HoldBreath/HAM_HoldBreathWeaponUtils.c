class HAM_HoldBreathWeaponUtils
{
 static bool IsAllowed(BaseWeaponComponent weapon, HAM_HoldBreathConfig config)
 {
  if (!weapon || !config)
   return false;
  IEntity entity = weapon.GetOwner();
  if (!entity || !entity.GetPrefabData())
   return false;
  ResourceName prefab = entity.GetPrefabData().GetPrefabName();
  if (config.DenyList && config.DenyList.Contains(prefab))
  {
   Log("Weapon blocked by DenyList");
   return false;
  }
  if (config.AllowList && config.AllowList.Contains(prefab))
  {
   Log("Weapon allowed by AllowList");
   return true;
  }
  if (weapon.GetWeaponType() == EWeaponType.WT_SNIPERRIFLE)
  {
   Log("Weapon allowed by WeaponType");
   return true;
  }
  // Opt-in extension for rifle-class DMRs. No explosives or launchers.
  if (!config.SniperOnly && weapon.GetWeaponType() == EWeaponType.WT_RIFLE)
   return true;
  Log("Unsupported weapon");
  return false;
 }
 static float GetWeaponWeightKg(BaseWeaponComponent weapon, HAM_HoldBreathConfig config)
 {
  if (!weapon || !config) return 0;
  IEntity entity = weapon.GetOwner();
  if (!entity) return 0;
  if (entity.GetPrefabData() && config.WeightOverrides)
  {
   ResourceName prefab = entity.GetPrefabData().GetPrefabName();
   foreach (HAM_HoldBreathWeightOverride entry : config.WeightOverrides)
   {
    if (entry && entry.WeaponPrefab == prefab && entry.WeightKg > 0)
     return entry.WeightKg;
   }
  }
  InventoryItemComponent item = InventoryItemComponent.Cast(entity.FindComponent(InventoryItemComponent));
  if (item) return Math.Max(0, item.GetTotalWeight());
  return 0;
 }
 static float GetWeightAdjustedMultiplier(float weightKg, HAM_HoldBreathConfig config)
 {
  if (!config) return 1;
  float baseline = Math.Clamp(config.HoldSwayMultiplier, 0.01, 1);
  if (!config.UseWeaponWeight || weightKg <= 0 || config.WeightReferenceKg <= 0)
   return baseline;
  float exponent = Math.Clamp(config.WeightExponent, 0, 2);
  float ratio = Math.Clamp(weightKg / config.WeightReferenceKg, 0.01, 100);
  float lower = Math.Clamp(config.MinHoldSwayMultiplier, 0.01, 1);
  float upper = Math.Clamp(config.MaxHoldSwayMultiplier, lower, 1);
  // Gameplay tuning: heavier rifles retain more native fluctuation while held.
  return Math.Clamp(baseline * Math.Pow(ratio, exponent), lower, upper);
 }
 static void Log(string message)
 {
  if (SCR_CharacterControllerComponent.DEBUG)
   Print("[HAM_HoldBreath] " + message);
 }
}
