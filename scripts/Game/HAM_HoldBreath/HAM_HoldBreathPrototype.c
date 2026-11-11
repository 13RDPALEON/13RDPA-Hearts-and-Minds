// Local ADS hold/release controller shared by animation and weapon aim compensation.
modded class SCR_CharacterControllerComponent
{
 static const bool DEBUG = false;
 protected bool m_bTRNLastPressed;
 protected bool m_bTRNReset = true;
 protected BaseWeaponComponent m_TRNCachedWeapon;
 protected PlayerController m_TRNLocalPlayer;
 protected RplComponent m_TRNReplication;
 protected bool m_bTRNReplicationCached;
 protected BaseWeaponManagerComponent m_TRNWeaponManager;
 protected CharacterStaminaComponent m_TRNStamina;
 protected ref HAM_HoldBreathAudio m_TRNBreathAudio;
 protected ref HAM_HoldBreathStateMachine m_TRNBreathState;
 protected ref HAM_HoldBreathAnimationBackend m_TRNBreathAnimation;
 protected bool m_bTRNEligible;
 protected bool m_bTRNAvailable;
 protected float m_TRNWeaponWeightKg;
 protected float m_TRNHoldTarget = 1;
 bool HAM_IsHoldBreathAvailable() { return m_bTRNAvailable; }
 float HAM_GetWeaponSwayMultiplier(IEntity weaponOwner)
 {
  if (!m_TRNCachedWeapon || m_TRNCachedWeapon.GetOwner() != weaponOwner || !m_TRNBreathState || !IsWeaponADS())
   return 1;
  return m_TRNBreathState.GetSwayMultiplier(HAM_HoldBreathConfig.Get(), m_TRNHoldTarget);
 }

 protected void HAM_UpdateWeightTarget(BaseWeaponComponent weapon, HAM_HoldBreathConfig config)
 {
  m_TRNWeaponWeightKg = HAM_HoldBreathWeaponUtils.GetWeaponWeightKg(weapon, config);
  m_TRNHoldTarget = HAM_HoldBreathWeaponUtils.GetWeightAdjustedMultiplier(m_TRNWeaponWeightKg, config);
 }
 protected void HAM_ResetHoldBreath()
 {
  if (m_bTRNReset)
   return;
  m_bTRNReset = true;
  if (m_TRNBreathAudio)
   m_TRNBreathAudio.Reset();
  if (m_TRNBreathAnimation)
   m_TRNBreathAnimation.Restore();
  if (m_TRNBreathState)
   m_TRNBreathState.Reset();
  m_TRNCachedWeapon = null;
  m_TRNWeaponWeightKg = 0;
  m_TRNHoldTarget = 1;
  m_bTRNLastPressed = false;
  m_bTRNEligible = false;
  m_bTRNAvailable = false;
 }


 protected int m_TRNBreathSequence;
 protected ref HAM_HoldBreathCueGate m_TRNServerCueGate;
 protected ref HAM_HoldBreathCueGate m_TRNRemoteCueGate;
 protected ref HAM_HoldBreathRemoteAudio m_TRNRemoteAudio;
 protected void HAM_PublishBreath(HAM_EBreathCue cue)
 {
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  if (!config || !config.EnableBreathSounds || !config.EnableRemoteBreathSounds) return;
  m_TRNBreathSequence++;
  Rpc(HAM_RpcAskBreath, cue, m_TRNBreathSequence);
 }
 [RplRpc(RplChannel.Unreliable, RplRcver.Server)]
 protected void HAM_RpcAskBreath(HAM_EBreathCue cue, int sequence)
 {
  IEntity owner = GetOwner();
  if (!owner || IsDead() || IsUnconscious()) return;
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  if (!config || !config.EnableBreathSounds || !config.EnableRemoteBreathSounds) return;
  // Server RPCs from a non-owner are rejected by native replication.
  // The actor is derived on the server, never supplied by the client.
  int actor = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(owner);
  if (actor <= 0) return;
  if (!m_TRNServerCueGate) m_TRNServerCueGate = new HAM_HoldBreathCueGate();
  if (!m_TRNServerCueGate.Accept(cue, sequence, actor, owner.GetWorld().GetWorldTime())) return;
  HAM_RpcDoBreath(cue, sequence, actor); // Listen server hears remote players too.
  Rpc(HAM_RpcDoBreath, cue, sequence, actor);
 }
 [RplRpc(RplChannel.Unreliable, RplRcver.Broadcast, RplCondition.NoOwner)]
 protected void HAM_RpcDoBreath(HAM_EBreathCue cue, int sequence, int actor)
 {
  if (System.IsConsoleApp() || IsDead() || IsUnconscious()) return;
  IEntity owner = GetOwner();
  PlayerController localPlayer = GetGame().GetPlayerController();
  if (!owner || (localPlayer && localPlayer.GetControlledEntity() == owner)) return;
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  if (!config || !config.EnableBreathSounds || !config.EnableRemoteBreathSounds) return;
  if (!m_TRNRemoteCueGate) m_TRNRemoteCueGate = new HAM_HoldBreathCueGate();
  if (!m_TRNRemoteCueGate.Accept(cue, sequence, actor, owner.GetWorld().GetWorldTime())) return;
  if (AudioSystem.GetDistance(owner.GetOrigin()) > HAM_HoldBreathRemoteAudio.RANGE) return;
  if (!m_TRNRemoteAudio) m_TRNRemoteAudio = new HAM_HoldBreathRemoteAudio();
  m_TRNRemoteAudio.Play(owner, cue);
 }

 override void OnPrepareControls(IEntity owner, ActionManager am, float dt, bool player)
 {
  super.OnPrepareControls(owner, am, dt, player);
  // Dedicated/headless application: preserve vanilla work, skip all CIE work.
  // System console mode is not an Xbox/PlayStation platform check.
  if (System.IsConsoleApp())
   return;
  if (!m_TRNLocalPlayer)
   m_TRNLocalPlayer = GetGame().GetPlayerController();
  if (!m_TRNLocalPlayer || m_TRNLocalPlayer.GetControlledEntity() != owner)
  {
   HAM_ResetHoldBreath();
   return;
  }
  if (!m_bTRNReplicationCached)
  {
   m_TRNReplication = RplComponent.Cast(owner.FindComponent(RplComponent));
   m_bTRNReplicationCached = true;
  }
  if (m_TRNReplication && !m_TRNReplication.IsOwner())
  {
   HAM_ResetHoldBreath();
   return;
  }
  HAM_HoldBreathConfig config = HAM_HoldBreathConfig.Get();
  if (!config || IsDead() || IsUnconscious())
  {
   HAM_ResetHoldBreath();
   return;
  }
  m_bTRNReset = false;
  if (!m_TRNBreathAudio)
   m_TRNBreathAudio = new HAM_HoldBreathAudio();
  if (!m_TRNBreathState)
   m_TRNBreathState = new HAM_HoldBreathStateMachine();
  if (!m_TRNBreathAnimation)
   m_TRNBreathAnimation = new HAM_HoldBreathAnimationBackend();
  if (!m_TRNWeaponManager)
   m_TRNWeaponManager = GetWeaponManagerComponent();
  if (!m_TRNStamina)
   m_TRNStamina = CharacterStaminaComponent.Cast(GetStaminaComponent());

  BaseWeaponComponent weapon;
  BaseSightsComponent sights;
  if (m_TRNWeaponManager)
  {
   weapon = m_TRNWeaponManager.GetCurrentWeapon();
   sights = m_TRNWeaponManager.GetCurrentSights();
  }
  bool weaponChanged = weapon != m_TRNCachedWeapon;
  if (weaponChanged)
  {
   m_TRNCachedWeapon = weapon;
   HAM_HoldBreathWeaponUtils.Log("Weapon changed");
   m_bTRNEligible = HAM_HoldBreathWeaponUtils.IsAllowed(weapon, config);
   HAM_UpdateWeightTarget(weapon, config);
  }
  bool eligible = m_bTRNEligible && weapon && sights && sights.IsSightADSActive();
  eligible = eligible && IsWeaponRaised() && IsWeaponADS();
  eligible = eligible && GetCurrentItemInHands() == weapon.GetOwner();
  eligible = eligible && !GetDisableWeaponControls() && !IsChangingItem() && !IsReloading();
  eligible = eligible && !IsClimbing() && !IsSwimming() && !IsSprinting() && !IsUsingItem();
  bool backendReady = m_TRNBreathAnimation.Bind(GetAnimationComponent());
  eligible = eligible && m_TRNStamina && backendReady;
  float stamina;
  if (m_TRNStamina)
   stamina = m_TRNStamina.GetStamina();
  // A private Overlay context avoids modifying shared ADS ActionRefs.
  if (eligible && am)
   am.ActivateContext("HAM_HoldBreathContext");
  bool pressed = eligible && am && am.GetActionValue("HAM_HoldBreath") > 0.5;
  if (pressed && !m_bTRNLastPressed && weapon)
   HAM_UpdateWeightTarget(weapon, config);
  if (DEBUG && pressed != m_bTRNLastPressed)
  {
   m_bTRNLastPressed = pressed;
   Print(string.Format("[TRN input] pressed=%1 eligible=%2 backend=%3 stamina=%4 weapon=%5", pressed, eligible, backendReady, stamina, weapon));
  }
  m_bTRNLastPressed = pressed;
  HAM_EHoldBreathState previousState = m_TRNBreathState.GetState();
  stamina = HAM_HoldBreathStamina.Update(m_TRNStamina, m_TRNBreathState, stamina, dt, eligible, pressed, config, weaponChanged);
  HAM_EBreathCue breathCue = m_TRNBreathAudio.Update(owner, dt, m_TRNBreathState, config);
  if (breathCue != HAM_EBreathCue.NONE) HAM_PublishBreath(breathCue);
  m_TRNBreathAnimation.Apply(m_TRNBreathState.GetSwayMultiplier(config, m_TRNHoldTarget));
  m_bTRNAvailable = eligible && config.MaxHoldTime > 0 && stamina >= Math.Clamp(config.MinStamina, 0, 1);
  m_bTRNAvailable = m_bTRNAvailable && (m_TRNBreathState.CanStart() || m_TRNBreathState.GetState() == HAM_EHoldBreathState.HOLDING);
  if (DEBUG && previousState != m_TRNBreathState.GetState())
  {
   if (m_TRNBreathState.GetState() == HAM_EHoldBreathState.HOLDING)
    Print("[HAM_HoldBreath] Started");
   else if (previousState == HAM_EHoldBreathState.HOLDING)
   {
    Print("[HAM_HoldBreath] Stopped");
    HAM_EHoldBreathStopReason reason = m_TRNBreathState.GetStopReason();
    if (reason == HAM_EHoldBreathStopReason.MAX_HOLD_TIME)
     Print("[HAM_HoldBreath] Max hold time reached");
    else if (reason == HAM_EHoldBreathStopReason.LOW_STAMINA)
     Print("[HAM_HoldBreath] Low stamina");
    else if (reason == HAM_EHoldBreathStopReason.CONDITIONS_LOST && (!sights || !sights.IsSightADSActive()))
     Print("[HAM_HoldBreath] ADS lost");
   }
  }
 }
}
