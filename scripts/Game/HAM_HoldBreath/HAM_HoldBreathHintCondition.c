[BaseContainerProps()]
class HAM_HoldBreathHintCondition : SCR_AvailableActionCondition
{
 protected bool m_bWasVisible;
 override bool IsAvailable(notnull SCR_AvailableActionsConditionData data)
 {
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(data.GetCharacterController());
  PlayerController player = GetGame().GetPlayerController();
  bool visible = config && config.ShowControlHint && controller && player;
  if (visible)
   visible = player.GetControlledEntity() == controller.GetOwner() && controller.HAM_IsHoldBreathAvailable();
  if (m_bWasVisible != visible)
  {
   m_bWasVisible = visible;
   if (visible)
    HAM_HoldBreathWeaponUtils.Log("Control hint shown");
   else
    HAM_HoldBreathWeaponUtils.Log("Control hint hidden");
  }
  return GetReturnResult(visible);
 }
}
