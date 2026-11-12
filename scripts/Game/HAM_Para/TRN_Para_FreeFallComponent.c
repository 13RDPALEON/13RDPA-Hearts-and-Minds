//------------------------------------------------------------------------------------------------
//! GMS FreeFall - character-side controller component.
//!
//! Attach to the player character prefab (modded SCR_ChimeraCharacter base).
//! Responsibilities:
//!  - detect freefall entry (aircraft exit at altitude / scripted trigger)
//!  - own the flight model on the machine that owns the character's motion
//!  - drive the GMS freefall animation graph on EVERY machine from replicated
//!    state (see the NETWORK ROLES section below)
//!  - deploy the parachute (spawns/attaches the GMS canopy entity from the
//!    parachute mod and switches the character to the para-hang state)
//!  - hand back to normal character movement on landing
//!
//! The character STAYS a character throughout. The native fall command keeps
//! running and keeps playing our arch via the master-instance Falling
//! override; this component glides the entity directly, fixed-wing stepper
//! style. There is no scripted command and no vehicle involved.
//!
//! (Two earlier approaches lived here and are gone: a vanilla-loiter graph
//! attachment via StartLoitering + SCR_LoiterCustomAnimData, and a
//! CharacterCommandScripted subclass. Both were superseded by the Falling
//! override and neither was reachable any more.)
//------------------------------------------------------------------------------------------------
class HAM_Para_FreeFallComponentClass : ScriptComponentClass
{
}

class HAM_Para_FreeFallComponent : ScriptComponent
{
	// Remote characters need FRAME/POSTFRAME presentation updates too.
	// Physics/input remain gated by HAM_Para_IsMotionAuthority().
	bool OnTicksOnRemoteProxy()
	{
		return true;
	}

	[Attribute(defvalue: "150", desc: "Minimum height above ground (m) to allow freefall entry")]
	protected float m_fMinEntryHeight;

	[Attribute(defvalue: "1", desc: "Auto-engage freefall when falling (not in vehicle/cargo, fall speed exceeded)")]
	protected bool m_bAutoEngage;


	[Attribute(defvalue: "10", desc: "Downward speed (m/s) that auto-engages freefall")]
	protected float m_fAutoEngageFallSpeed;

	[Attribute(defvalue: "0", desc: "TEMP: engage freefall pose on the ground 8s after spawn (graph chain test)")]
	protected bool m_bDebugGroundTest;

	[Attribute(defvalue: "120", desc: "Turn spin-up: heading acceleration at full input, deg/s^2")]
	protected float m_fTurnAccelDps2;

	[Attribute(defvalue: "360", desc: "Turn momentum cap: max heading rate, deg/s")]
	protected float m_fTurnMaxRateDps;

	[Attribute(defvalue: "90", desc: "Pitch spin-up: flip acceleration at full input, deg/s^2")]
	protected float m_fPitchAccelDps2;

	[Attribute(defvalue: "270", desc: "Pitch momentum cap: max flip rate, deg/s")]
	protected float m_fPitchMaxRateDps;

	[Attribute(defvalue: "120", desc: "Roll spin-up: barrel-roll acceleration at full Q/E input, deg/s^2")]
	protected float m_fRollAccelDps2;

	[Attribute(defvalue: "360", desc: "Roll momentum cap: max roll rate, deg/s")]
	protected float m_fRollMaxRateDps;

	[Attribute(defvalue: "11.5", desc: "Canopy full-flight forward airspeed, m/s")]
	protected float m_fCanopyFwdMs;

	[Attribute(defvalue: "3.2", desc: "Canopy full-flight sink rate, m/s")]
	protected float m_fCanopySinkMs;

	[Attribute(defvalue: "40", desc: "Canopy max held turn rate, deg/s")]
	protected float m_fCanopyTurnMaxDps;

	[Attribute(defvalue: "0", desc: "DEV: SPACE under canopy toggles slot tuning (A/D W/S Q/E), printing the offset to bake - RE-DISABLE (0) for release")]
	protected bool m_bCanopyTuneMode;

	[Attribute(defvalue: "6.5", desc: "Landing tumble: forward ground speed (m/s) above which touchdown ragdolls")]
	protected float m_fTumbleLandSpeedMs;

	[Attribute(defvalue: "5.5", desc: "Landing tumble: sink rate (m/s) above which touchdown ragdolls")]
	protected float m_fTumbleLandSinkMs;

	[Attribute(defvalue: "0.5", desc: "Air camera: horizontal mouse look speed, deg per count - NEGATIVE inverts")]
	protected float m_fAirLookSensX;

	[Attribute(defvalue: "0.5", desc: "Air camera: vertical mouse look speed, deg per count - NEGATIVE inverts")]
	protected float m_fAirLookSensY;

	[Attribute(defvalue: "6", desc: "Landing damage: effective impact (m/s) where injury starts - below this a PLF walks it off")]
	protected float m_fLandDmgMinMs;

	[Attribute(defvalue: "13", desc: "Landing damage: effective impact (m/s) at or above which the landing is fatal")]
	protected float m_fLandDmgFatalMs;

	[Attribute(defvalue: "120", desc: "Collision damage dealt when reaching the fatal threshold (character pool is ~100)")]
	protected float m_fLandDmgMax;

	[Attribute(defvalue: "0.15", desc: "Ice friction: fraction of rotation momentum shed per second while coasting")]
	protected float m_fRotDampingPerS;

	protected bool m_bTestDone;
	protected bool m_bBypassHeightGate;
	protected float m_fTestTimer;

	[Attribute(defvalue: "0", desc: "Auto-deploy height above ground (m); 0 disables until the parachute milestone")]
	protected float m_fAutoDeployHeight;

	[Attribute(defvalue: "", desc: "Canopy entity prefab (GMS parachute mod)", params: "et")]
	protected ResourceName m_CanopyPrefab;         // legacy V1 fallback, superseded by the V2 set below

	[Attribute("{7079655CF27E538A}Prefabs/Props/GMS_V2/GMS_PLinesL.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: left line bundles", params: "et")]
	protected ResourceName m_CanopyLinesL;

	[Attribute("{51FA8776FC1EA3AA}Prefabs/Props/GMS_V2/GMS_PLinesR.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: right line bundles", params: "et")]
	protected ResourceName m_CanopyLinesR;

	[Attribute("{45A0ED94E56B33AE}Prefabs/Props/GMS_V2/GMS_PRisers.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: risers/links/slider/hooks", params: "et")]
	protected ResourceName m_CanopyRisers;

	[Attribute("{207FE86A2C57A765}Prefabs/Props/GMS_V2/GMS_PCanopy.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: the canopy", params: "et")]
	protected ResourceName m_CanopyCanopy;

	[Attribute("{57CFF652DD71EFBF}Prefabs/Props/GMS_V2/GMS_PDrogue.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: drogue + bridle", params: "et")]
	protected ResourceName m_CanopyDrogue;

	[Attribute("{C585A794DCDE609B}Prefabs/Props/GMS_V2/GMS_PDBag.et", UIWidgets.ResourceNamePicker, desc: "V2 canopy: deployment bag", params: "et")]
	protected ResourceName m_CanopyDBag;

		[Attribute(defvalue: "50", desc: "FAST regime, hands-off: TRUE body pitch off the horizon (deg) to ENTER the fast fwd/back anims (a genuinely steep, committed track). Held pitch input latches the anim regardless of attitude. Raised 35->50 2026-08-25: player consensus was the fast anims kicked in too early.")]
	protected float m_fFastPitchEnterDeg;

	[Attribute(defvalue: "40", desc: "FAST regime, hands-off: true pitch off the horizon (deg) to EXIT back to neutral (the coast has genuinely shallowed). Kept 10 deg under the enter threshold so the boundary does not flicker. Raised 25->40 with the enter threshold.")]
	protected float m_fFastPitchExitDeg;

	[Attribute(defvalue: "0.5", desc: "Wind drift scale under canopy (1 = full weather wind; drift was extreme at 1)")]
	protected float m_fWindDriftScale;

	[Attribute(defvalue: "184.11", desc: "Canopy yaw vs character, deg - SOLVED from the steer-hold fists vs parked toggles (bake the tuner's printed total here)")]
	protected float m_fCanopyYawDeg;

	[Attribute(defvalue: "-0.012 1.424 -0.108", desc: "Canopy origin offset in character space, m - solved from anim data, then hand-tuned in-game 2026-08-22 (bake the tuner's printed offset here)")]
	protected vector m_vCanopyOffset;

	// NOTE: the GMS freefall graph is NOT referenced here any more. It reaches
	// the character as a master-instance override of the vanilla Falling state,
	// not as a runtime attachment, so there is nothing for the component to
	// point at - the .agr/.asi/binding-name attributes only ever fed the
	// removed loiter path.

	protected CharacterAnimationComponent m_AnimComponent;
	protected CharacterControllerComponent m_Controller;
	protected CompartmentAccessComponent m_CompartmentAccess;
	protected ref HAM_Para_ParachuteAnimationBridge m_GMSPresentation;
	protected TAnimGraphVariable m_VarDir;
	protected TAnimGraphVariable m_VarDirX;
	protected TAnimGraphVariable m_VarDirY;
	protected float m_fAnimDirX;
	protected float m_fAnimDirY;
	protected float m_fSentDirX = 999.0;
	protected float m_fSentDirY = 999.0;
	protected TAnimGraphVariable m_VarActive;
	protected TAnimGraphCommand m_CmdFF;
	protected TAnimGraphTag m_TagFF;
	protected TAnimGraphTag m_TagFall;
	protected TAnimGraphTag m_TagFalling;
	protected TAnimGraphTag m_TagLanding;
	protected int m_iDir;
	protected bool m_bFreeFalling;
	// flight-logic state (stepper-style glide on the character entity)
	protected vector m_vFlightVel;
	protected float m_fPitchTrim;    // deg, negative = nose-down intent
	protected float m_fRollTrim;     // deg, visual bank only (follows the turn)
	protected float m_fPitchRate;    // deg/s rotation momentum on the pitch axis
	protected float m_fTurnRate;     // deg/s rotation momentum on the heading axis
	protected float m_fRollRate;     // deg/s rotation momentum on the roll axis (Q/E)
	protected float m_fBankVisDeg;   // visual bank overlay from the live turn rate
	protected float m_fFreeFallTime; // seconds since engage (grounded-exit grace)

	// turn rate that maps to full animation/bank deflection (deg/s)
	protected static const float TURN_ANIM_REF_DPS = 45.0;
	// pitch rates: full anim deflection / regime enter / regime exit (deg/s)
	protected static const float PITCH_ANIM_REF_DPS = 45.0;
	protected static const float PITCH_REGIME_ENTER_DPS = 30.0;
	protected static const float PITCH_REGIME_EXIT_DPS = 12.0;
	protected bool m_bPitchRateRegime; // true while the pitch ANIM follows rotation direction
	protected bool m_bStabilizing;     // X toggled auto-level; any rotation input cancels
	protected bool m_bStabKeyHeld;     // rising-edge detect for the stabilize key
	protected bool m_bJumpKeyHeld;     // rising-edge detect for the deploy key
	protected bool m_bGraphActive;     // this machine has HAM_Para_FF_Active raised in the anim graph
	protected float m_fGroundedTime;   // sustained ground-contact timer (proxy-safe touchdown)

	// --- AUTHORITATIVE REPLICATED STATE -----------------------------------
	// The whole jump used to be re-derived independently on every machine:
	// entry from a local fall-speed heuristic, exit from a broadcast RPC.
	// Broadcasts only reach machines where the proxy is CURRENTLY streamed in
	// (BI: "if the proxy is not streamed on a machine, the broadcast message
	// will not reach this machine"), and a HALO drop streams observers in and
	// out for the whole descent - so any missed DEPLOY/END edge left that
	// observer's jumper stuck in the freefall pose forever, with no re-sync.
	// The mode now lives in authority-owned properties. RplProp state is
	// re-sent on stream-in and JIP automatically, so a machine that missed
	// every edge still converges the moment the proxy appears.
	//
	// Mode and compass are two plain ints rather than one bit-packed one:
	// Enforce Script has no shift operators (<< / >> are a compile error), and
	// packing without them would mean multiply/divide games that read worse
	// than just sending a second int nobody will ever notice on the wire.
	protected static const int HAM_Para_MODE_NONE = 0;
	protected static const int HAM_Para_MODE_FREEFALL = 1;
	protected static const int HAM_Para_MODE_CANOPY = 2;
	protected static const bool HAM_Para_LAND_ANIMS_ENABLED = false;  // parked: see touchdown block
	protected static const int HAM_Para_DIR_MAX = 16;  // 0-8 compass, 9 hold, 10 pull, 11 open, 12-16 landings
	protected static const float HAM_Para_PULL_ANIM_S = 2.9;    // pull_parachute_02 length (Pull holds its arch after)
	// the Blender demo is ONE synchronized clip that starts at activation
	// (pack 0.1 s, drogue out 0.5 s, bag ride, stretch f88 = 2.933 s): all
	// six parts share a single clock that starts at the HAND-PULL moment.
	// Now fires on the ripcord itself (was 1500, then 500): 1.5 s ahead of
	// the animated hand yank, traded deliberately for a snappier opening.
	// Every constant below is this + its own offset, so they all move
	// together and the internal spacing is unchanged.
	protected static const int HAM_Para_DEPLOY_DELAY_MS = 0;     // pack starts the instant the ripcord is pulled
	protected static const float HAM_Para_OPEN_START_S = 2.93;  // line stretch: 0 + 2.933 (the snatch)
	protected static const float HAM_Para_OPEN_END_S = 5.01;    // stretch + open clip (2.233 s) - 0.15 guard
	protected static const float HAM_Para_UPRIGHT_END_S = 4.9;  // world-up hold ends as the hang settles
	protected static const float HAM_Para_UPRIGHT_BLEND_S = 0.8; // ease from world-up into the spine lock
	// the tuned canopy attach expressed in SPINE5's bone frame, solved from
	// the hold pose (gms_ff_canopy_torso.txa recompose, 0.000000 m error):
	// boneWS x this = the exact flight attach - and because it rides the
	// BONE, the harness follows the pull/snatch animation at the same
	// relative position it has under canopy
	// ENGINE-truth value, read back from the live settle cache (the engine
	// bone frame is yawed ~180 deg vs a raw txa recompose - do NOT derive
	// this offline, print it from m_mSpineRel and bake what the engine says)
	protected static const vector HAM_Para_SPINE_ANCHOR = "-0.00498 0.17081 -0.07888";

	[RplProp(onRplName: "OnGMSRplState")]
	protected int m_iRplMode;

	[RplProp()]
	protected vector m_vRplFlightAngles;
	[RplProp()]
	protected vector m_vRplFlightVelocity;
	[RplProp()]
	protected bool m_bRplFlightSample;
	[RplProp()]
	protected float m_fRplCanopyAge;
	[RplProp()]
	protected int m_iFlightEpoch;
	protected float m_fFlightSendAge;
	protected float m_fFlightReceiveAge;
	protected float m_fChuteSendAge;
	protected vector m_vImpactAnchor;
	protected vector m_vImpactVelocity;
	protected float m_fImpactWindow;
	protected float m_fImpactAge;
	protected bool m_bImpactVelocityValid;
	protected bool m_bProxyAttitudeInit;
	protected vector m_vProxyAttitude;
	protected float m_fLandingDamageGrace;

	// Sampling actual displacement is shared by both parachutes. In particular,
	// the non-owner server must not grade its own unrelated physics fall speed.
	protected void HAM_Para_MeasureImpactVelocity(IEntity owner, float dt)
	{
		m_fFlightReceiveAge += dt;
		m_fLandingDamageGrace = Math.Max(0, m_fLandingDamageGrace - dt);
		m_fChuteSendAge += dt;
		if (!HAM_Para_IsStateAuthority() || !m_bFreeFalling || m_iLandDir != 0) return;
		m_fImpactWindow += dt;
		m_fImpactAge += dt;
		if (m_fImpactWindow < 0.5) return;
		vector velocity = (owner.GetOrigin() - m_vImpactAnchor) / m_fImpactWindow;
		m_vImpactAnchor = owner.GetOrigin();
		m_fImpactWindow = 0;
		if (velocity[1] < -0.5 && velocity.Length() < HAM_Para_TELEPORT_SPEED_MS)
		{
			m_vImpactVelocity = velocity;
			m_fImpactAge = 0;
			m_bImpactVelocityValid = true;
		}
	}

	protected bool HAM_Para_GetLandingVelocity(out vector velocity)
	{
		if (m_bImpactVelocityValid && m_fImpactAge < 1.5)
		{
			velocity = m_vImpactVelocity;
			return true;
		}
		if (HAM_Para_IsMotionAuthority())
		{
			velocity = m_vFlightVel;
			return true;
		}
		if (m_bRplFlightSample && m_fFlightReceiveAge < 0.75)
		{
			velocity = m_vRplFlightVelocity;
			return true;
		}
		return false;
	}

	protected void HAM_Para_AcceptFlightSample(vector angles, vector velocity, float canopyAge)
	{
		m_vRplFlightAngles = angles;
		m_vRplFlightVelocity = velocity;
		m_fRplCanopyAge = canopyAge;
		m_bRplFlightSample = true;
		m_fFlightReceiveAge = 0;
		Replication.BumpMe();
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RpcAsk_GMSFlightSample(int epoch, vector angles, vector velocity, float canopyAge)
	{
		if (!HAM_Para_IsStateAuthority() || !m_bFreeFalling || m_iLandDir != 0 || epoch != m_iFlightEpoch) return;
		// Reject NaN, infinities and values outside the supported flight envelope.
		for (int axis = 0; axis < 3; axis++)
			if (!(angles[axis] >= -180 && angles[axis] <= 180)) return;
		float speed = velocity.Length();
		if (!(speed >= 0 && speed < HAM_Para_TELEPORT_SPEED_MS)) return;
		if (!(canopyAge >= 0 && canopyAge < 86400)) return;
		if (!m_bCanopy) canopyAge = 0;
		HAM_Para_AcceptFlightSample(angles, velocity, canopyAge);
	}

	protected void HAM_Para_SendFlightSample(vector angles, float dt)
	{
		m_fFlightSendAge += dt;
		if (m_fFlightSendAge < 0.1) return;
		m_fFlightSendAge = 0;
		for (int axis = 0; axis < 3; axis++)
			angles[axis] = Math.Repeat(angles[axis] + 180, 360) - 180;
		if (HAM_Para_IsStateAuthority()) HAM_Para_AcceptFlightSample(angles, m_vFlightVel, m_fCanopyTime);
		else Rpc(RpcAsk_GMSFlightSample, m_iFlightEpoch, angles, m_vFlightVel, m_fCanopyTime);
	}

	[RplProp(onRplName: "OnGMSRplDir")]
	protected int m_iRplDir;

	// landing variant (0 = none, 12-16 = the queue-overlay land clips).
	// Chosen by the STATE AUTHORITY from the measured arrival, honored by
	// every machine's canopy dir window (the owner ignores m_iRplDir by
	// design, so the landing rides its own channel).
	[RplProp(onRplName: "OnGMSRplLand")]
	protected int m_iRplLand;

	// the OWNER's computed chute placement, relative to the character root.
	// Observers apply this verbatim instead of re-deriving it from their
	// local bone state - per-machine pose/timing differences put the chute
	// off the shoulders on every viewer but the jumper. Root-relative, so
	// it composes with the replicated character transform on any machine.
	[RplProp()]
	protected vector m_vRplChutePos;
	[RplProp()]
	protected vector m_vRplChuteAng;
	protected vector m_vSentChutePos;
	protected vector m_vSentChuteAng = "999 0 0";
	protected int m_iLandDir;
	protected bool m_bLandVelCleared;
	protected bool m_bLandShiftPending;    // root-motion handback shift armed
	protected int m_iLandShiftVariant;
	protected vector m_vLastAirVelVec;

	// P1 analog steering: turn (-1..1) and flare/forward (0..1 / -1..1)
	// for the parametric graph slots HAM_Para_FF_DirX/DirY. Owner computes,
	// server rebroadcasts - same authority split as m_iRplDir.
	[RplProp(onRplName: "OnGMSRplDirX")]
	protected float m_fRplDirX;

	[RplProp(onRplName: "OnGMSRplDirY")]
	protected float m_fRplDirY;

	protected RplComponent m_RplComp;
	// arrival speeds sampled from the REPLICATED physics velocity on every
	// machine - the landing model used to read the local flight model, which
	// on the (non-owning) server is not the motion that actually happened
	protected float m_fLastAirHorizVel;
	protected float m_fStillTime;      // authority: sustained near-zero vertical motion
	protected float m_fRplReconcileTime; // non-authority: drift check throttle
	protected int m_iRplDriftStreak;     // consecutive disagreeing drift samples
	// post-process effects are cleared against the camera they were SET on;
	// re-reading GetCurrentCameraId() at cleanup time leaked them onto the
	// screen whenever the camera changed in between (death cam, spectator)
	protected int m_iChromAberCamId = -1;
	protected int m_iSpeedDoFCamId = -1;

	// --- canopy flight (same momentum philosophy, parachute-weighted) ------
	protected static const float CANOPY_PITCH_RATE_DPS = 40.0;   // W/S attitude slider rate, deg/s
	protected static const float CANOPY_ATT_MIN_DEG = -88.0;     // full nose-down (near-vertical dive)
	protected static const float CANOPY_ATT_MAX_DEG = 32.0;      // full nose-up (deep flare)
	// How far the pitch-attitude slider may shift the toggle ANIMATION, in
	// either direction. Attitude and brakes are the same physical control -
	// the only way to raise the nose is toggle pressure, and the only way to
	// dive it is letting them up - so the slider moves the arms too. Half
	// depth, and signed: S adds up to +0.5, W subtracts up to -0.5. The full
	// sweep stays reserved for the brakes, so a genuine flare still reads as
	// the bigger motion and W can visibly bleed one off.
	protected static const float CANOPY_ATT_TOGGLE_ANIM_MAX = 0.5;
	protected static const float CANOPY_TURN_ACCEL_DPS2 = 28.0;  // A/D turn-rate slider wind-up
	protected static const float CANOPY_FLARE_DUR_S = 1.8;       // legacy (retired)
	protected static const float HAM_Para_BRAKE_STROKE_RATE = 0.85;    // full toggle stroke in ~1.2 s (X / Z)
	protected static const float HAM_Para_BRAKE_DUMP_RATE = 3.3;       // Q: all slack out in ~0.3 s
	protected static const float HAM_Para_GLIDE_BRAKE = 0.20;          // E trim: light brakes flatten the polar
	protected static const float HAM_Para_GLIDE_ATT_DEG = -4.0;        // E trim: a touch nose-DOWN - keeps the speed on and the miles coming
	protected static const float HAM_Para_STALL_BREAK_S = 2.8;         // deep-brake time to a full stall
	protected static const float CANOPY_MOMENTUM_T = 1.9;        // horizontal SmoothCD time - floaty but not slidey
	protected static const float CANOPY_VERT_T = 1.4;            // vertical response time
	protected static const float CANOPY_OPEN_SHOCK_T = 1.1;      // opening deceleration time constant
	protected static const float CANOPY_BANK_MAX_DEG = 32.0;     // pendulum bank at full turn rate
	// NO-CANOPY GROUND IMPACT. A jumper who reaches the deck still in
	// freefall never got a chute out, and nothing about that is survivable.
	// Checked directly against height and sink rate instead of waiting on the
	// grounded/wedged timers below, which needed 0.25 s of confirmed ground
	// contact - often never satisfied at terminal velocity - or a 2 s
	// no-motion fallback, which is why bodies lay in the freefall pose, alive,
	// before anything resolved.
	//! Returned when height above ground cannot be determined. Deliberately huge:
	//! every ground test compares with <=, so unknown height can never read as
	//! contact.
	protected static const float HAM_Para_AGL_UNKNOWN = 100000.0;
	protected static const float HAM_Para_FF_IMPACT_AGL_M = 2.0;    // ground contact window
	protected static const float HAM_Para_FF_IMPACT_SINK_MS = 10.0; // sink that makes it an impact, not a step off a rock
	protected static const float CANOPY_LOOP_START_S = 5.3;      // cloth loop starts once the wing is flying (bloom ends 5.17 s)
	// Steering unlocks here: the wing is open, the bloom is done and the
	// jumper has settled into the canopy-ride pose. Before this the opening
	// choreography owns the character, so toggle input is ignored rather
	// than steering a canopy that is still inflating.
	protected static const float CANOPY_STEER_START_S = 5.4;
	protected bool m_bCanopy;          // under canopy (m_bFreeFalling stays true = airborne under GMS control)
	protected float m_fAirspeed;       // wing energy state, m/s along the flight path
	protected float m_fVerticalVel;    // MODEL-owned vertical velocity - never re-read from physics
	protected vector m_vWind;          // sampled weather-system wind, world frame
	protected float m_fWindResample;   // wind resample countdown
	protected float m_fStallTime;      // sustained slow-and-nose-high timer (stall onset)
	protected float m_fLastAirVertVel; // last airborne physics vertical (m/s, negative = falling) - raw-freefall impact readout
	protected CharacterHeadAimingComponent m_HeadAiming; // head-look limit override while airborne
	protected float m_fLineTwistTime;  // post-deploy line-twist countdown
	protected float m_fYawVisOffset;   // visual yaw offset (twist wobble + turn trail)
	protected bool m_bCanopyLoud;      // which cloth layer is playing
	protected IEntity m_CanopyEntity;              // legacy single-prop handle (unused by V2)
	protected ref array<IEntity> m_aCanopyParts = {};
	protected float m_fSteerSentX = 999.0;
	protected float m_fSteerSentY = 999.0;
	protected vector m_vSlotBasePos;   // rig-local slot position the canopy spawned with
	protected vector m_vTuneOffset;    // live-tuning delta accumulated this deploy
	protected float m_fTunePrintTimer;
	protected bool m_bTuneActive;      // X-toggled canopy tuning mode
	protected float m_fTuneYaw;        // Z/C fine yaw during tuning, deg
	protected bool m_bTuneKeyHeld;     // rising-edge detect (the X action is click-filtered)
	protected float m_fCanopyTime;
	protected int m_iDbgLastDir = -99;   // debug: last dir pushed to the graph
	protected bool m_bCanopyPartsSettled;   // world-up hold done, rigid attach active
	protected int m_iSpine5Bone = -2;       // lazy bone lookup (-2 = not tried, -1 = missing)
	protected vector m_mSpineRel[4];        // solved attach expressed in Spine5 space
	protected bool m_bSpineRelCached;
	protected bool m_bSpineDbgPrinted;
	protected float m_fDeployStartPitch;   // attitude flown at the ripcord
	protected float m_fDeployStartRoll;    // bank at the ripcord, decays across the pull
	protected float m_fCanopyAttDeg;   // HELD pitch-attitude slider, deg (+ nose-up .. - nose-down dive)
	protected float m_fFlareTime;      // legacy X-flare envelope (retired; kept for state resets)
	protected float m_fBrakeSlider;    // toggle stroke 0 (full flight) .. 1 (full flare)
	protected float m_fFlareSurge;     // rate-based flare lift transient (decays ~1.4 s)
	protected bool m_bGlideTrim;       // E: auto-trim for maximum glide range
	protected bool m_bGlideKeyHeld;
	protected bool m_bStalled;         // full stall: the wing has quit until the brakes come up
	protected vector m_vProxyVisPos;   // observer-side smoothed display position
	protected vector m_vProxyRawPos;   // last position REPLICATION put the entity at
	protected float m_fProxyRawAge;    // seconds since that snapshot (extrapolation clock)
	protected bool m_bProxySmoothInit;

	// FRAME-EVENT WATCHDOG. The flight model lives in EOnFrame, but the
	// entity event mask is a whole-mask WRITE, not an OR: any component that
	// calls SetEventMask on this character after our OnPostInit silently
	// clears our FRAME bit. The graph command is asserted from the modded
	// controller (OnApplyControls) which keeps firing regardless, so the
	// symptom is exactly "freefall starts and animates but I cannot turn":
	// animation alive, flight model dead. These track it and re-arm.
	protected float m_fLastFrameEventTime;   // seconds since the last EOnFrame
	protected bool m_bFrameWatchdogWarned;   // one-shot log per character
	protected float m_fPendPitch;      // pendulum pitch, deg
	protected float m_fPendPitchVel;
	protected float m_fPrevFwdSpeed;
	protected float m_fVelYV;          // vertical SmoothCD filter state
	protected bool m_bCanopyLoopOn;
	protected bool m_bOpenSndPlayed;

	// speed screen FX (local player only)
	protected static const ResourceName SPEEDFX_LAYOUT = "{69EE3A01C4F1BB07}UI/layouts/GMS_AirOps/GMS_FreeFallVignette.layout";
	protected static const ResourceName FLIGHTHUD_LAYOUT = "{69EE3A01C4F3AA00}UI/layouts/GMS_AirOps/GMS_FlightHUD.layout";
	// vanilla chromatic aberration post-process (same one regeneration uses)
	// Own fixed-param post-process materials. The vanilla PP emats bind their
	// parameters to the vanilla screen-effect scripts' statics (ChromAberrPP is
	// driven by SCR_RegenerationScreenEffect at priority 19), so reusing them
	// renders NOTHING unless those effects happen to run. Plain paths resolve
	// the same way our sound assets do.
	protected static const string CHROMABER_EMAT = "UI/Materials/GMS_AirOps/GMS_ChromAberrStrongPP.emat";
	protected static const int CHROMABER_PRIORITY = 17;
	protected static const string SPEEDDOF_EMAT = "UI/Materials/GMS_AirOps/GMS_SpeedDoFPP.emat";
	protected static const int SPEEDDOF_PRIORITY = 18;
	protected Widget m_wSpeedFX;
	protected Widget m_wFlightHUD;
	protected ImageWidget m_wHorizonLine;
	protected TextWidget m_wPitchBankText;
	protected float m_fShakeAccum;
	protected bool m_bChromAber;
	protected bool m_bSpeedDoF;

	// 2D wind loop: a baked intro that fades itself in, then a seam-crossfaded
	// full loop (AudioSystem has no per-handle volume, so the rise lives in the
	// file). The soft/mid band layers this once switched between are gone with
	// the band selector that chose them - the intro/full pair replaced it.
	protected static const string WIND_WAV_FULL = "Sounds/GMS_AirOps/Freefall Wind.wav";
	protected static const string WIND_WAV_INTRO = "Sounds/GMS_AirOps/Freefall Wind.wav";
	protected static const float WIND_INTRO_LEN_S = 52.78; // single 52.84 s seamless loop, retrigger just before its end
	protected static const string DEPLOY_WAV = "Sounds/GMS_AirOps/gms_para_deploy.wav";
	// canopy flight loops (cloth flapping): calm layer + roaring layer
	// switched by airspeed, same fade-masked swap as the freefall bands
	protected static const string CANOPY_WAV = "Sounds/GMS_AirOps/gms_para_canopy_loop.wav";
	protected static const string CANOPY_WAV_SOFT = "Sounds/GMS_AirOps/gms_para_canopy_soft.wav";

	// EARPLUG BEDS. Earplug mods duck the SFX MASTER BUS
	// (AudioSystem.SetMasterVolume(AudioSystem.SFX, quieter)), but our audio
	// goes out through AudioSystem.PlaySound, which takes neither a bus
	// category nor a gain and exposes no per-handle volume - so we cannot
	// ride the bus down. Same answer the loud/soft canopy beds already use:
	// the volume lives in the FILE. These are the same recordings at ~-10 dB,
	// selected while the bus is ducked, so plugs muffle the jump instead of
	// silencing it.
	protected static const string WIND_WAV_FULL_EP = "Sounds/GMS_AirOps/Freefall Wind_ep.wav";
	protected static const string WIND_WAV_INTRO_EP = "Sounds/GMS_AirOps/Freefall Wind_ep.wav";
	protected static const string DEPLOY_WAV_EP = "Sounds/GMS_AirOps/gms_para_deploy_ep.wav";
	protected static const string CANOPY_WAV_EP = "Sounds/GMS_AirOps/gms_para_canopy_loop_ep.wav";
	protected static const string CANOPY_WAV_SOFT_EP = "Sounds/GMS_AirOps/gms_para_canopy_soft_ep.wav";
	protected AudioHandle m_WindHandle;
	protected int m_iWindLayer;
	protected float m_fFXIntensity;       // smoothed 0..1 speed intensity (FX + audio bands)
	protected float m_fWindLoopTime;      // seconds into the current wind loop pass
	protected float m_fFXCreateRetryTime; // widget creation retry cooldown
	protected int m_iFXCreateTries;

	// exact length of the baked wind loops; retrigger runs on this CLOCK
	// because IsSoundPlayed cannot query raw PlaySound handles (always
	// false), which made state-based retriggering restart audible audio
	protected static const float WIND_LOOP_LEN_S = 52.78; // loop file length minus seam margin
	protected float m_fVelXV;
	protected float m_fVelZV;
	protected float m_fFlightYaw;
	protected float m_fDebugTimer;   // TEMP: heartbeat log throttle
	protected vector m_vLastPos;     // TEMP: position-derived velocity
	protected float m_fMeasuredDown; // TEMP: measured downward speed (m/s)

	// earplug watch: cached user setting + the live ducked verdict
	protected float m_fSfxConfiguredVol = -1.0;  // player's own SFX setting, 0..1 (-1 = unread)
	protected float m_fSfxPollTime;              // countdown to the next settings read
	protected bool m_bAudioDucked;               // plugs currently in

	// FOREIGN-CHUTE HANDOVER TIMERS. Ending our jump for another parachute
	// mod is not enough on its own: the jumper is still falling fast, so
	// auto-engage (which runs every frame on the authority) re-entered
	// freefall within a frame or two, and the owner kept flying for a full
	// round trip waiting for the replicated END. Both put our transform
	// writes back into the exact window where the other mod's
	// GetInVehicle(forceTeleport) seat entry begins - if that call is
	// stomped before it starts, IsGettingIn() never goes true and the
	// boarding yield never fires. Whether a static-line jumper got seated
	// was literally a per-frame race, which is why it worked sometimes.
	protected float m_fAutoEngageCooldown;  // authority: no auto re-engage after a handover
	protected float m_fYieldLatch;          // owner: ignore replicated re-engage during the handover

	// FALSE-ENGAGE GUARDS. The position-derived fall speed turns any TELEPORT
	// into one colossal single-tick "fall": Air Ops repositioning players for
	// the C-130 red-light phase and GM sky-teleports both read as hundreds of
	// m/s for one sample, and the engage check used to accept a single
	// sample. Displacements implying more than HAM_Para_TELEPORT_SPEED_MS are
	// discarded as teleports (nothing a body does reaches it; head-down
	// terminal is ~90), and engage now requires the fall condition to HOLD
	// for HAM_Para_ENGAGE_SUSTAIN_S rather than trip once.
	protected static const float HAM_Para_TELEPORT_SPEED_MS = 120.0;
	protected static const float HAM_Para_ENGAGE_SUSTAIN_S = 0.4;
	protected float m_fFallCondTime;   // seconds the engage condition has held
	//! False when the last position sample was rejected as a teleport. UNKNOWN
	//! SPEED IS NOT ZERO SPEED: reporting 0 made the stillness test believe a
	//! jumper at terminal velocity was hanging motionless, which fired the
	//! "wedged" release in mid-air and graded a stale sample as an impact.
	protected bool m_bMeasuredValid = true;
	protected vector m_vMeasuredVel; // full position-derived velocity (m/s), see HAM_Para_GetFallVelocity

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.FRAME | EntityEvent.POSTFRAME);
		owner.SetFlags(EntityFlags.ACTIVE, true);
		// seed the position sampler, or the first measurement is the whole
		// distance from the world origin over one tick - a spawn-frame
		// "velocity" of several hundred m/s
		m_vLastPos = owner.GetOrigin();
		m_bCanopyTuneMode = false;
		// backup tick independent of entity frame events (EOnFrame on
		// character-attached script components proved unreliable)
		GetGame().GetCallqueue().CallLater(BackupTick, 200, true);
		m_AnimComponent = CharacterAnimationComponent.Cast(owner.FindComponent(CharacterAnimationComponent));
		m_Controller = CharacterControllerComponent.Cast(owner.FindComponent(CharacterControllerComponent));
		m_CompartmentAccess = CompartmentAccessComponent.Cast(owner.FindComponent(CompartmentAccessComponent));
		if (m_AnimComponent)
		{
			m_VarDir = -1;
			m_VarDirX = -1;
			m_VarDirY = -1;
			m_VarActive = -1;
			m_CmdFF = -1;
			m_TagFF = m_AnimComponent.BindTag("TagGMSFF");
			m_TagFall = m_AnimComponent.BindTag("TagFall");
			m_TagFalling = m_AnimComponent.BindTag("TagFalling");
			m_TagLanding = m_AnimComponent.BindTag("TagLanding");
		}
		else
		{
			m_VarDir = -1;
			m_VarDirX = -1;
			m_VarDirY = -1;
			m_VarActive = -1;
			m_CmdFF = -1;
			m_TagFF = -1;
		}
		Print(string.Format("[HAM_Para_FreeFall] component init on %1 (anim=%2 ctrl=%3)", owner, m_AnimComponent != null, m_Controller != null), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Nothing here may outlive the character. Without this every jumper left
	//! behind a repeating 5 Hz callqueue entry, an orphaned canopy parented to
	//! a dead entity, screen widgets, stuck post-process and a live audio
	//! handle - on disconnect, respawn, or a proxy streaming out.
	override void OnDelete(IEntity owner)
	{
		if (m_GMSPresentation) m_GMSPresentation.Release();
		// OnDelete also runs during shutdown, where the game object may already
		// be gone - never assume GetGame()/GetCallqueue() are still there
		if (GetGame() && GetGame().GetCallqueue())
		{
			GetGame().GetCallqueue().Remove(BackupTick);
			GetGame().GetCallqueue().Remove(RecoverFromTumble);
		}

		ReleaseLocalFX();
		DestroyCanopy();

		if (m_HeadAiming)
			m_HeadAiming.ResetLimitAnglesOverride();

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	bool IsFreeFalling()
	{
		return m_bFreeFalling;
	}

	//------------------------------------------------------------------------------------------------
	// --- NETWORK ROLES ----------------------------------------------------
	// Two distinct authorities, and conflating them is what desynced the jump:
	//  - STATE authority is the SERVER: it alone decides when the jump starts,
	//    when the canopy is out, and when it ends, and publishes that in
	//    m_iRplMode. Every other machine obeys; nobody re-derives it.
	//  - MOTION authority is the OWNER: character movement in Reforger is
	//    owner-authoritative, so only the owning machine may write the
	//    transform. Proxies used to run the whole flight model and fight
	//    network interpolation for the proxy's position every frame.
	//------------------------------------------------------------------------------------------------
	protected RplComponent HAM_Para_GetRpl()
	{
		// latch only on SUCCESS. Caching a null - which is what a one-shot
		// "searched" flag does if this is first called before RplComponent has
		// initialised - would make every proxy report itself as the motion
		// authority for the rest of the character's life, reinstating the exact
		// bug this rework removes.
		if (!m_RplComp)
		{
			IEntity owner = GetOwner();
			if (owner)
				m_RplComp = RplComponent.Cast(owner.FindComponent(RplComponent));
		}
		return m_RplComp;
	}

	//! True while this character is entering or occupying ANY vehicle
	//! compartment. Other parachute mods (Pronto's static line, the OG
	//! framework, MFF) fly their canopy as a VEHICLE and seat the jumper in
	//! it with GetInVehicle(forceTeleport) from the owner, retrying every
	//! 50 ms. Our flight model writes the character transform every frame,
	//! which stomps that seat entry - on dedicated the retry loop lost every
	//! round until touchdown, so the jumper plummeted in our freefall pose
	//! and was only seated (or killed) on the ground.
	//!
	//! CALLERS GATE THIS ON THE RIG: yielding to a boarding is only correct
	//! when the jumper has NO compatible rig (FindParachuteBag() null) - the
	//! Pronto static-line case. With our rig worn, a parachute-vehicle seat
	//! entry is the OG framework's hijack, and yielding would hand our jump
	//! to the exact takeover the Chute Framework Compat blocks. Never true
	//! for our own canopy - V2 parts are parented children, not compartments.
	protected bool HAM_Para_IsBoardingVehicle()
	{
		if (!m_CompartmentAccess)
		{
			IEntity caOwner = GetOwner();
			if (caOwner)
				m_CompartmentAccess = CompartmentAccessComponent.Cast(caOwner.FindComponent(CompartmentAccessComponent));
			if (!m_CompartmentAccess)
				return false;
		}
		return m_CompartmentAccess.IsGettingIn() || m_CompartmentAccess.IsInCompartment();
	}

	//! True where the transform may be written. A character with no
	//! RplComponent at all is unreplicated (editor preview / offline test), so
	//! we own it - but only once we have actually looked and found nothing on
	//! a fully initialised entity, never because the lookup ran too early.
	protected bool HAM_Para_IsMotionAuthority()
	{
		RplComponent rpl = HAM_Para_GetRpl();
		if (rpl)
			return rpl.IsOwner();

		// no RplComponent found: unreplicated only if replication is not
		// running at all, otherwise assume proxy and stay hands-off
		return !Replication.IsRunning();
	}

	//! True where the jump state is decided. Server in MP, the local machine
	//! in singleplayer (which is a player-hosted server).
	protected bool HAM_Para_IsStateAuthority()
	{
		return Replication.IsServer();
	}

	//------------------------------------------------------------------------------------------------
	//! Authority-only: fold the live state into the replicated property.
	//! Cheap to call every frame - it only bumps replication on a real change.
	protected void HAM_Para_PublishState()
	{
		if (!HAM_Para_IsStateAuthority())
			return;

		int mode = HAM_Para_MODE_NONE;
		if (m_bCanopy)
			mode = HAM_Para_MODE_CANOPY;
		else if (m_bFreeFalling)
			mode = HAM_Para_MODE_FREEFALL;

		// integer clamp by comparison - Math.Clamp is the float overload and
		// this value is an anim-graph index, not a measurement
		int dir = m_iDir;
		if (dir < 0)
			dir = 0;
		else if (dir > HAM_Para_DIR_MAX)
			dir = HAM_Para_DIR_MAX;

		if (mode == m_iRplMode && dir == m_iRplDir)
			return;

		m_iRplMode = mode;
		m_iRplDir = dir;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Compass-only update. Split from the mode callback so a dir change during
	//! steady flight does not re-run the mode transition logic.
	protected void OnGMSRplLand()
	{
		m_iLandDir = m_iRplLand;
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGMSRplDir()
	{
		// the motion authority owns its own compass and recomputes it from
		// local input next frame; everyone else plays what the authority sends
		if (!HAM_Para_IsMotionAuthority())
			m_iDir = m_iRplDir;
	}

	//------------------------------------------------------------------------------------------------
	//! Runs on every proxy (owner included) when the authority's state lands,
	//! and on stream-in / JIP with the current value - which is precisely the
	//! recovery path the old broadcast-RPC-only design had no answer for.
	protected void OnGMSRplState()
	{
		bool wantFF = m_iRplMode != HAM_Para_MODE_NONE;
		bool wantCanopy = m_iRplMode == HAM_Para_MODE_CANOPY;

		OnGMSRplDir();

		if (wantFF && !m_bFreeFalling && m_fYieldLatch <= 0)
			BeginRemoteFreeFall();
		else if (!wantFF && m_bFreeFalling)
			EndFreeFall();

		if (!m_bFreeFalling)
		{
			// authority says the jump is over: the canopy goes with it,
			// whatever this machine believed a moment ago
			DestroyCanopy();
			m_bCanopy = false;
			return;
		}

		if (wantCanopy && !m_bCanopy)
		{
			m_bCanopy = true;
			InitCanopyFlight();
			SpawnCanopy(FindParachuteBag());
		}
		else if (!wantCanopy && m_bCanopy)
		{
			m_bCanopy = false;
			DestroyCanopy();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Enter the freefall state on a machine that was TOLD to, rather than one
	//! that guessed. No height gate (the authority already applied it) and no
	//! flight-model seeding on non-owners - they only play the pose.
	protected void BeginRemoteFreeFall()
	{
		// Remote state entry needs the same event re-arm as authority entry.
		IEntity remoteOwner = GetOwner();
		if (remoteOwner)
		{
			SetEventMask(remoteOwner, EntityEvent.FRAME | EntityEvent.POSTFRAME);
			Activate(remoteOwner);
			remoteOwner.SetFlags(EntityFlags.ACTIVE, true);
		}
		if (m_bFreeFalling)
			return;

		ResetFlightState(vector.Zero);
		m_bFreeFalling = true;
		ApplyAirLookLimits();
	}

	//------------------------------------------------------------------------------------------------
	//! Camera access: true once the first glide frame captured the heading -
	//! the attitude getters below are only meaningful when this returns true
	bool HasFlightAttitude()
	{
		return m_bFreeFalling && m_fFlightYaw != -999.0;
	}

	float GetFlightYawDeg()
	{
		return m_fFlightYaw;
	}

	float GetPitchDeg()
	{
		return m_fPitchTrim;
	}

	float GetRollDeg()
	{
		return m_fRollTrim + m_fBankVisDeg;
	}

	//------------------------------------------------------------------------------------------------
	//! Zero the flight model. Shared by the authority's engage and by the
	//! told-to-engage path on every other machine, so no stale attitude,
	//! momentum or FX state survives from a previous jump.
	protected void ResetFlightState(vector vInitialVelocity)
	{
		m_vFlightVel = vInitialVelocity;
		m_vImpactAnchor = GetOwner().GetOrigin();
		m_fImpactWindow = 0;
		m_fImpactAge = 0;
		m_bImpactVelocityValid = false;
		m_fLandingDamageGrace = 0;
		m_bProxyAttitudeInit = false;
		m_fFlightSendAge = 0.1;
		m_fFlightReceiveAge = 0;
		m_fChuteSendAge = 0.1;
		if (HAM_Para_IsStateAuthority())
		{
			m_iFlightEpoch++;
			m_bRplFlightSample = false;
			m_fRplCanopyAge = 0;
		}
		m_fPitchTrim = 0;
		m_fRollTrim = 0;
		m_fPitchRate = 0;
		m_fTurnRate = 0;
		m_fRollRate = 0;
		m_fBankVisDeg = 0;
		m_fFreeFallTime = 0;
		m_bPitchRateRegime = false;
		m_fFXIntensity = 0;
		m_fWindLoopTime = 0;
		m_fFXCreateRetryTime = 0;
		m_fGroundedTime = 0;
		m_fStillTime = 0;
		m_fLastAirVertVel = 0;
		m_fLastAirHorizVel = 0;
		m_bStabilizing = false;
		m_bStabKeyHeld = true;
		m_bJumpKeyHeld = true;
		m_bCanopy = false;
		m_fCanopyTime = 0;
		m_bCanopyLoopOn = false;
		m_fYawVisOffset = 0;
		m_fVelXV = 0;
		m_fVelZV = 0;
		m_fFlightYaw = -999.0;   // sentinel: capture on first glide frame
		m_bProxySmoothInit = false;

		// LANDING STATE MUST NOT SURVIVE INTO THE NEXT JUMP. m_iLandDir was only
		// ever cleared by InitCanopyFlight and HAM_Para_FinishLanding, so any jump that
		// ended without both of those running left it set - and a non-zero
		// m_iLandDir makes UpdateAuthorityTouchdown return early ("landing already
		// playing"). The NEXT jump then never resolves its touchdown, never ends,
		// and leaves the jumper standing in the freefall branch: arms out, weapon
		// gone, inventory unusable. Reported as "second jump, no access to my kit".
		// A fresh jump owns a fresh landing state, unconditionally.
		m_iLandDir = 0;
		m_iRplLand = 0;
		m_iLandShiftVariant = 0;
		m_bLandVelCleared = false;
		m_bLandShiftPending = false;
	}

	//------------------------------------------------------------------------------------------------
	//! PROXY MOTION SMOOTHING (observers only). The flight model moves the
	//! jumper by direct transform writes, which replicate as raw position
	//! snapshots - at network tick rate a 50 m/s faller visibly STEPS on
	//! other machines. Fix: dead-reckon on the proxy - predict forward along
	//! the replicated physics velocity (the authority publishes it every
	//! frame for exactly this) and exponentially smooth the visual position
	//! toward that prediction. Steps become ramps; the prediction horizon
	//! cancels the smoothing lag. Never runs on the motion authority.
	protected void HAM_Para_SmoothProxyMotion(IEntity owner, float timeSlice)
	{
		// Native character movement replication owns proxy positions. Never
		// integrate replicated velocity into that position a second time.
	}

	//------------------------------------------------------------------------------------------------
	//! Entry point - call on aircraft exit at altitude (or bind to an action).
	//! AUTHORITY ONLY: the state authority engages and publishes; every other
	//! machine enters through OnGMSRplState/BeginRemoteFreeFall. This used to
	//! be run independently on all machines from a local fall-speed guess,
	//! which is how proxies engaged jumps nobody else was in.
	void StartFreeFall(vector vInitialVelocity)
	{
		// Re-arm the frame events. Set once at OnPostInit they can be cleared
		// out from under us by any later SetEventMask on this entity, and
		// without EOnFrame there is no flight model - the jumper animates but
		// cannot steer. Cheap, and idempotent when nothing clobbered them.
		IEntity ffOwner = GetOwner();
		if (ffOwner)
		{
			SetEventMask(ffOwner, EntityEvent.FRAME | EntityEvent.POSTFRAME);
			ffOwner.SetFlags(EntityFlags.ACTIVE, true);
		}
		if (m_bFreeFalling || !m_AnimComponent || !m_Controller)
			return;

		if (!HAM_Para_IsStateAuthority())
			return;

		IEntity owner = GetOwner();
		float heightAGL = GetHeightAGL(owner);
		if (!m_bBypassHeightGate && heightAGL < m_fMinEntryHeight)
		{
			Print(string.Format("[HAM_Para_FreeFall] entry denied: height %1 m AGL < %2", heightAGL, m_fMinEntryHeight), LogLevel.NORMAL);
			return;
		}
		Print(string.Format("[HAM_Para_FreeFall] engaging freefall at %1 m AGL", heightAGL), LogLevel.NORMAL);
		m_fAutoEngageCooldown = 0;
		m_fYieldLatch = 0;

		ResetFlightState(vInitialVelocity);
		m_bFreeFalling = true;
		ApplyAirLookLimits();
		HAM_Para_PublishState();
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! The parachute rig gates deployment and owns the canopy config. The
	//! component can sit on ANY item the modder deems a parachute: worn
	//! back/vest/jacket slots only. A stored spare is not an equipped rig.
	HAM_Para_ParachuteBagComponent FindParachuteBag()
	{
		IEntity owner = GetOwner();

		SCR_CharacterInventoryStorageComponent inv = SCR_CharacterInventoryStorageComponent.Cast(owner.FindComponent(SCR_CharacterInventoryStorageComponent));
		if (inv)
		{
			IEntity worn = inv.GetClothFromArea(LoadoutBackpackArea);
			if (worn)
			{
				HAM_Para_ParachuteBagComponent bag = HAM_Para_ParachuteBagComponent.Cast(worn.FindComponent(HAM_Para_ParachuteBagComponent));
				if (bag)
					return bag;
			}
			worn = inv.GetClothFromArea(LoadoutVestArea);
			if (worn)
			{
				HAM_Para_ParachuteBagComponent bag = HAM_Para_ParachuteBagComponent.Cast(worn.FindComponent(HAM_Para_ParachuteBagComponent));
				if (bag)
					return bag;
			}
			worn = inv.GetClothFromArea(LoadoutJacketArea);
			if (worn)
			{
				HAM_Para_ParachuteBagComponent bag = HAM_Para_ParachuteBagComponent.Cast(worn.FindComponent(HAM_Para_ParachuteBagComponent));
				if (bag)
					return bag;
			}
		}

		return null;
	}


	//------------------------------------------------------------------------------------------------
	void DeployParachute()
	{
		if (!m_bFreeFalling || m_bCanopy)
			return;

		// NO COMPATIBLE RIG = NOT OUR JUMP, so hand the whole thing over.
		//
		// Refusing the deploy alone was not enough. HAM_Para_LocalAction runs this
		// LOCALLY before it RPCs the server, so an authority-only check let the
		// jumper play our entire pull/open sequence on a rig that can never
		// deploy. But simply returning is worse: we stay engaged, and the
		// freefall model keeps writing the character transform and physics
		// velocity every frame - which fights the other parachute mod when it
		// seats the jumper in its own canopy, and stopped Pronto chutes
		// deploying at all on dedicated.
		//
		// So: the AUTHORITY ends the jump outright and publishes, which releases
		// the character to whichever mod owns that rig, and every client
		// follows through the replicated state. The jumper's own machine plays
		// nothing in the meantime. Third-party OBSERVERS still never enforce the
		// rig rule - inventory streams in at its own pace and a viewer without
		// the backpack yet would otherwise refuse a deploy everyone else saw.
		HAM_Para_ParachuteBagComponent bag = FindParachuteBag();
		if (!bag)
		{
			if (HAM_Para_IsStateAuthority())
			{
				Print("[HAM_Para_FreeFall] no GMS rig worn - releasing the jump to the other parachute mod", LogLevel.NORMAL);
				m_fAutoEngageCooldown = 8.0;   // give the other mod's deploy + seat entry the sky to itself
				HAM_Para_DoAction(HAM_Para_ACT_END);
				HAM_Para_PublishState();
				return;
			}
			if (HAM_Para_IsLocalPlayer())
			{
				// surrender NOW, not an RTT from now: the owner is the only machine
				// that writes the transform, so a local end makes the other mod's
				// very first seat attempt stick. The latch stops a replicated
				// freefall state (published before the server processed its own
				// handover) from re-engaging us mid-transition.
				m_fYieldLatch = 3.0;
				HAM_Para_DoAction(HAM_Para_ACT_END);
				return;
			}
		}

		Print("[HAM_Para_FreeFall] ripcord pulled - deploying canopy", LogLevel.NORMAL);

		// audio hand-off happens at the OPEN beat, not the ripcord: the
		// freefall wind keeps roaring through the pull (still at terminal
		// under the drogue) and the opening boom replaces it at 2.9 s -
		// both handled in UpdateCanopy's local-player audio block.

		m_bCanopy = true;
		InitCanopyFlight();
		SpawnCanopy(bag);
		HAM_Para_PublishState();
	}

	//------------------------------------------------------------------------------------------------
	//! Seed the canopy flight model from the current freefall state. Split out
	//! of DeployParachute so a machine that learns about the canopy from the
	//! replicated state (stream-in, or a DEPLOY broadcast it never received)
	//! enters the hang with the same initial conditions instead of a blank one.
	protected void InitCanopyFlight()
	{
		// hand the attitude to the pendulum: opening shock rocks the body
		// back, the SmoothCD deceleration produces the snatch organically
		m_fCanopyTime = 0;
		if (!HAM_Para_IsMotionAuthority() && m_bRplFlightSample) m_fCanopyTime = m_fRplCanopyAge;
		m_vTuneOffset = vector.Zero;
		m_fTunePrintTimer = 0;
		m_bTuneActive = false;
		m_bTuneKeyHeld = true;
		m_fCanopyAttDeg = 0;
		m_fFlareTime = 0;
		m_fBrakeSlider = 0;
		m_fFlareSurge = 0;
		m_bGlideTrim = false;
		m_bStalled = false;
		// SEED the pendulum with the attitude actually flown at the ripcord
		// (wrapped, clamped so an inverted deploy cannot demand a loop) -
		// the body then KEEPS its freefall attitude under the drogue and the
		// SNATCH at line stretch is what swings it upright, via the spring.
		float ripPitch = Math.Repeat(m_fPitchTrim + 180.0, 360.0) - 180.0;
		m_fDeployStartPitch = Math.Clamp(ripPitch, -120.0, 45.0);
		m_fDeployStartRoll = m_fRollTrim + m_fBankVisDeg;
		m_fPendPitch = m_fDeployStartPitch;
		m_fPendPitchVel = 0;
		m_fPrevFwdSpeed = Vector(m_vFlightVel[0], 0, m_vFlightVel[2]).Length();
		// the wing inherits your energy: deploy out of a fast track and the
		// opening surge carries it; floor keeps a straight-down opening flying
		m_fAirspeed = Math.Clamp(m_fPrevFwdSpeed, 8.0, 46.0);
		m_fVelYV = 0;
		// inherit the freefall descent once; the opening shock decelerates
		// from here and the model owns the vertical from now on
		m_fVerticalVel = 0;
		IEntity dpOwner = GetOwner();
		if (dpOwner)
		{
			Physics dpPhys = dpOwner.GetPhysics();
			if (dpPhys)
				m_fVerticalVel = dpPhys.GetVelocity()[1];
		}
		m_bCanopyLoopOn = false;
		m_bCanopyLoud = false;
		m_bOpenSndPlayed = false;
		m_fWindResample = 0;
		m_fStallTime = 0;
		m_fYawVisOffset = 0;
		// deploying mid-spin twists the lines: weak controls and a yaw
		// wobble until they unwind
		m_fLineTwistTime = 0;
		if (Math.AbsFloat(m_fTurnRate) > 35.0)
			m_fLineTwistTime = 2.2;
		m_bStabilizing = false;
		m_fPitchRate = 0;
		m_fRollRate = 0;
		// a spinning deploy hands over HALF the spin (the opening kills most
		// of it) - under hold-slider turns a full inherited spiral would
		// otherwise keep carving until manually countered
		m_fTurnRate = Math.Clamp(m_fTurnRate * 0.5, -m_fCanopyTurnMaxDps, m_fCanopyTurnMaxDps);
		m_fRollTrim = 0;
		m_bCanopyPartsSettled = false;
		m_bSpineRelCached = false;
		m_vSentChuteAng = "999 0 0";
		m_iLandDir = 0;
		m_bLandVelCleared = false;
		m_bLandShiftPending = false;
		m_iDir = 10;   // pull beat first
	}

	//------------------------------------------------------------------------------------------------
	//------------------------------------------------------------------------------------------------
	//! Blender's always-up constraint, at runtime. The assembly's POSITION
	//! always rides the same body point (the pack between the shoulders -
	//! rootTM x m_vCanopyOffset - so the straps stay strapped to the
	//! character in every pose), while its ORIENTATION is held WORLD-UP
	//! through the deploy so the canopy extends skyward whatever the body
	//! attitude (belly-down pull included). As the hang settles the pitch
	//! and roll ease from level into the root's, and at the end of the
	//! window the parts snap to the exact rigid root attach - the solved
	//! transform that puts the parked toggles in the fists.
	protected void HAM_Para_ApplyChuteTM(vector rootTM[4], vector tm[4])
	{
		foreach (IEntity aPart : m_aCanopyParts)
		{
			if (aPart)
				aPart.SetWorldTransform(tm);
		}
		vector rel[4];
		Math3D.MatrixInvMultiply4(rootTM, tm, rel);
		HAM_Para_PublishChuteRel(rel[3], Math3D.MatrixToAngles(rel));
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_Para_PlaceCanopyParts(IEntity owner)
	{
		if (m_bCanopyPartsSettled || m_aCanopyParts.IsEmpty())
			return;

		vector rootTM[4];
		owner.GetWorldTransform(rootTM);

		// OBSERVERS: apply the owner's replicated placement verbatim - the
		// jumper's machine is the single source of truth for where the
		// chute sits on the body. No local bone math, no divergence.
		if (!HAM_Para_IsMotionAuthority())
		{
			if (m_vRplChutePos != vector.Zero)
			{
				vector relM[4];
				Math3D.AnglesToMatrix(m_vRplChuteAng, relM);
				relM[3] = m_vRplChutePos;
				vector obsTM[4];
				Math3D.MatrixMultiply4(rootTM, relM, obsTM);
				foreach (IEntity oPart : m_aCanopyParts)
				{
					if (oPart)
						oPart.SetWorldTransform(obsTM);
				}
				return;
			}
			// no replicated placement yet: fall through to local estimate
		}

		// the exact rigid attach - the end state, and the blend target
		vector slotLocal[4];
		Math3D.AnglesToMatrix(Vector(m_fCanopyYawDeg, 0, 0), slotLocal);
		slotLocal[3] = m_vCanopyOffset;
		vector rigidTM[4];
		Math3D.MatrixMultiply4(rootTM, slotLocal, rigidTM);

		// SPINE5 is the anchor bone for the whole ride - both shoulders hang
		// off it, i.e. it is where the harness is actually sewn
		Animation charAnim = owner.GetAnimation();
		// RETRY until it resolves. The old code cached whatever the FIRST
		// lookup returned - including -1 when the skeleton was not ready yet
		// (routine on a proxy that has just streamed in). A cached -1 still
		// passed the "!= -2" test below, so GetBoneMatrix(-1) failed forever
		// and the harness stayed rigid-attached for the whole jump.
		if (charAnim && m_iSpine5Bone < 0)
		{
			int spineIdx = charAnim.GetBoneIndex("Spine5");
			if (spineIdx >= 0)
				m_iSpine5Bone = spineIdx;
		}
		vector boneWS[4];
		bool haveBone = false;
		vector boneLS[4];
		if (charAnim && m_iSpine5Bone >= 0 && charAnim.GetBoneMatrix(m_iSpine5Bone, boneLS))
		{
			Math3D.MatrixMultiply4(rootTM, boneLS, boneWS);
			haveBone = true;
		}
		if (!m_bSpineDbgPrinted)
		{
			m_bSpineDbgPrinted = true;
			Print(string.Format("[HAM_Para_FreeFall] Spine5 lookup: anim=%1 boneId=%2 haveBone=%3 boneLS=(%4 %5 %6)",
				charAnim != null, m_iSpine5Bone, haveBone,
				boneLS[3][0].ToString(-1, 3), boneLS[3][1].ToString(-1, 3), boneLS[3][2].ToString(-1, 3)), LogLevel.NORMAL);
		}

		float blend = Math.Clamp((m_fCanopyTime - HAM_Para_UPRIGHT_END_S) / HAM_Para_UPRIGHT_BLEND_S, 0.0, 1.0);
		if (blend >= 1.0)
		{
			// SETTLED: the chute stays sewn to SPINE5 for the whole canopy
			// flight. The spine-relative transform is cached ONCE from the
			// exact solved attach (torso is static in the hang, so this is
			// bit-identical to the old rigid attach while the hold plays) -
			// and if the spine ever moves (look-arounds, landing, cutaway)
			// the harness rides with it instead of detaching.
			if (!haveBone)
			{
				// No bone THIS frame: rigid root attach as a stopgap, then try again
				// next frame. This used to latch m_bCanopyPartsSettled, which made a
				// single early frame permanent: the guard at the top of this function
				// then returned immediately forever, so the harness stayed at the
				// rigid default and - on an observer - the replicated placement that
				// arrived a moment later was never applied. That is the "chute does
				// not follow other players, sits in the default position" report.
				HAM_Para_ApplyChuteTM(rootTM, rigidTM);
				return;
			}
			if (!m_bSpineRelCached)
			{
				// ENGINE-TRUTH constant (measured from the live settle cache
				// in the hold pose). Baked, NOT cached at runtime: an
				// observer's bone can be mid-blend at ITS settle moment
				// (late dir replication), and caching that pose put the
				// chute in the wrong place on every other machine.
				Math3D.AnglesToMatrix(Vector(-179.481, 1.888, 0.334), m_mSpineRel);
				m_mSpineRel[3] = "-0.00498 0.17081 -0.07888";
				m_bSpineRelCached = true;
			}
			vector lockTM[4];
			Math3D.MatrixMultiply4(boneWS, m_mSpineRel, lockTM);
			HAM_Para_ApplyChuteTM(rootTM, lockTM);
			return;
		}

		// DEPLOY HOLD: the POSITION is the flight attach expressed in
		// SPINE5's frame, applied to the LIVE bone - the harness follows
		// the pull/snatch animation at exactly the relationship it has
		// under canopy (in the hold pose this equals rigidTM to 0.0 mm).
		// Only the ORIENTATION differs: WORLD-UP (Blender's always-up
		// constraint) easing into the root attitude as the hang settles.
		vector anchor;
		if (haveBone)
		{
			anchor = boneWS[3]
				+ boneWS[0] * HAM_Para_SPINE_ANCHOR[0]
				+ boneWS[1] * HAM_Para_SPINE_ANCHOR[1]
				+ boneWS[2] * HAM_Para_SPINE_ANCHOR[2];
		}
		else
		{
			anchor = rigidTM[3];
		}
		vector rootAng = Math3D.MatrixToAngles(rootTM);   // yaw pitch roll
		vector ang = Vector(rootAng[0] + m_fCanopyYawDeg,
			rootAng[1] * blend, rootAng[2] * blend);
		vector tm[4];
		Math3D.AnglesToMatrix(ang, tm);
		tm[3] = vector.Lerp(anchor, rigidTM[3], blend);
		HAM_Para_ApplyChuteTM(rootTM, tm);
	}

	//------------------------------------------------------------------------------------------------
	protected void SpawnCanopy(HAM_Para_ParachuteBagComponent bag)
	{
		if (!m_aCanopyParts.IsEmpty())
			return;

		// EXACT placement, solved from the animation data itself: the
		// character-space transform that puts the parked toggles into the
		// steer-hold fists (yaw 184.11, offset -0.02/1.26/-0.18 - the pack
		// on the back). Residual ~2 cm/hand = the clips' own asymmetry.
		// The bag no longer positions anything; it remains the rig gate.
		vector slotLocal[4];
		Math3D.AnglesToMatrix(Vector(m_fCanopyYawDeg, 0, 0), slotLocal);
		slotLocal[3] = m_vCanopyOffset;

		IEntity owner = GetOwner();
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		owner.GetTransform(sp.Transform);

		ref array<ResourceName> parts = {m_CanopyLinesL, m_CanopyLinesR,
			m_CanopyRisers, m_CanopyCanopy, m_CanopyDrogue, m_CanopyDBag};
		foreach (ResourceName prefab : parts)
		{
			if (prefab == ResourceName.Empty)
				continue;
			Resource res = Resource.Load(prefab);
			if (!res.IsValid())
			{
				Print("[HAM_Para_FreeFall] canopy part failed to load: " + prefab, LogLevel.WARNING);
				continue;
			}
			IEntity part = GetGame().SpawnEntityPrefab(res, owner.GetWorld(), sp);
			if (part)
				m_aCanopyParts.Insert(part);
		}
		if (!m_aCanopyParts.IsEmpty())
		{
			// parent the canopy to the RIG ENTITY when it is worn (attached
			// somewhere under the character): the canopy then inherits the
			// bag's own slot/bone and follows the animated back, and the
			// slot PointInfo applies in the exact frame the editor preview
			// tunes it in. Character-origin fallback for unattached rigs.
			IEntity attachTo = owner;
			if (bag)
			{
				IEntity rigEnt = bag.GetOwner();
				IEntity walk = null;
				if (rigEnt)
					walk = rigEnt.GetParent();
				while (walk)
				{
					if (walk == owner)
					{
						attachTo = rigEnt;
						break;
					}
					walk = walk.GetParent();
				}
			}

			vector baseTM[4];
			owner.GetWorldTransform(baseTM);
			vector canopyTM[4];
			Math3D.MatrixMultiply4(baseTM, slotLocal, canopyTM);
			m_vSlotBasePos = m_vCanopyOffset;
			m_fSteerSentX = 999.0;
			m_fSteerSentY = 999.0;

			// RIGID attach to the character root: the whole hold pose
			// (torso+arms) is root-static now, so root-parenting preserves
			// the solved fist/toggle geometry every frame by construction
			m_bCanopyPartsSettled = false;
			foreach (IEntity part : m_aCanopyParts)
			{
				owner.AddChild(part, -1);

				// STAGGERED deploy start, keyed to the character's beats:
				// the drogue's authored launch plays mid-pull (the toss),
				// everything else starts at the OPEN beat so the bag exits
				// the pack on screen. Line stretch then lands at
				// 2.9 + 2.933 = 5.83 s, bloom completes at 8.07 s - the
				// script's dir windows, physics gates and audio all key off
				// HAM_Para_OPEN_START_S to match. The five main parts stay
				// frame-locked with each other (same delay = same clock).
				HAM_Para_ParachuteV2AnimComponent v2 = HAM_Para_ParachuteV2AnimComponent.Cast(part.FindComponent(HAM_Para_ParachuteV2AnimComponent));
				if (v2)
					v2.StartAt(m_fCanopyTime);
			}
			HAM_Para_PlaceCanopyParts(owner);
			Print(string.Format("[HAM_Para_FreeFall] V2 canopy: %1/6 parts placed (world-up deploy hold)", m_aCanopyParts.Count()), LogLevel.NORMAL);

			Print("[HAM_Para_FreeFall] canopy spawned, attached and blooming", LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Touchdown over the speed limits knocks the jumper down: a brief
	//! unconscious ragdoll and the get-up, tuned by the two Tumble attributes
	//! grounded - true only when the authority actually confirmed ground contact.
	//! False means we are releasing the state for another reason and must not kill.
	protected void CheckLandingTumble(bool grounded)
	{
		// NOT AN ARRIVAL, NOT AN IMPACT. The wedged branch calls this when the
		// system has lost track of the jumper - there is no landing to grade, and
		// grading one killed people in mid-air off a stale sample. Release the
		// state and touch nothing else.
		if (!grounded)
		{
			Print("[HAM_Para_FreeFall] state released without ground contact - no impact graded", LogLevel.NORMAL);
			return;
		}

		// Measured from the REPLICATED physics velocity of the last airborne
		// frame, not from the local flight model. This runs on the authority,
		// which does not own character motion - m_vFlightVel / m_fVerticalVel
		// there belong to a simulation the player never flew, so the damage
		// and tumble verdict was being computed from the wrong numbers.
		vector actualVelocity;
		// Native contact callbacks may follow the script's landing handback.
		// Cover only that confirmed canopy arrival, never an arbitrary END.
		if (m_bCanopy) m_fLandingDamageGrace = 0.5;
		if (HAM_Para_GetLandingVelocity(actualVelocity))
		{
			m_fLastAirVertVel = actualVelocity[1];
			m_fLastAirHorizVel = Vector(actualVelocity[0], 0, actualVelocity[2]).Length();
			m_vLastAirVelVec = actualVelocity;
		}
		else if (m_bCanopy)
		{
			Print("[HAM_Para_FreeFall] canopy landing has no fresh motion sample; stale physics damage rejected", LogLevel.WARNING);
			return;
		}
		float horiz = m_fLastAirHorizVel;
		float sink = -m_fLastAirVertVel;
		float impact = Math.Sqrt(horiz * horiz + sink * sink);

		// effective impact: vertical hits with full force, horizontal only
		// partially - it can be PLF'd / run out
		float eff = Math.Sqrt(sink * sink + 0.35 * horiz * horiz);
		Print(string.Format("[HAM_Para_FreeFall] touchdown: fwd=%1 sink=%2 eff=%3 (injury>%4 fatal>%5, tumble fwd>%6 sink>%7)",
			horiz, sink, eff, m_fLandDmgMinMs, m_fLandDmgFatalMs, m_fTumbleLandSpeedMs, m_fTumbleLandSinkMs), LogLevel.NORMAL);

		// --- impact damage first: hard landings injure, very hard ones kill ---
		// Arriving with no canopy is fatal on its own terms, whatever the
		// measured numbers say: the last airborne sample can be stale or
		// truncated by the impact frame, and a jumper with no chute out has no
		// survivable arrival to grade.
		// No canopy on a CONFIRMED arrival is fatal on its own terms (the last
		// airborne sample can be truncated by the impact frame). Without that
		// confirmation, fall back to the measured-impact threshold, which is the
		// behaviour that shipped for months.
		if ((!m_bCanopy && grounded) || eff >= m_fLandDmgFatalMs)
		{
			Print(string.Format("[HAM_Para_FreeFall] fatal impact (%1 m/s effective)", eff), LogLevel.NORMAL);
			if (m_Controller)
				m_Controller.ForceDeath();
			return; // death brings its own ragdoll; skip tumble timers
		}

		if (eff > m_fLandDmgMinMs && m_fLandDmgFatalMs > m_fLandDmgMinMs)
		{
			float frac = (eff - m_fLandDmgMinMs) / (m_fLandDmgFatalMs - m_fLandDmgMinMs);
			float dmg = Math.Pow(frac, 1.5) * m_fLandDmgMax;
			ChimeraCharacter chr = ChimeraCharacter.Cast(GetOwner());
			if (chr && dmg > 0)
			{
				SCR_CharacterDamageManagerComponent dmgMan = SCR_CharacterDamageManagerComponent.Cast(chr.GetDamageManager());
				if (dmgMan)
				{
					HitZone hz = dmgMan.GetDefaultHitZone();
					if (hz)
					{
						hz.HandleDamage(dmg, EDamageType.COLLISION, null);
						Print(string.Format("[HAM_Para_FreeFall] hard landing: %1 collision damage (%2 m/s effective)", Math.Round(dmg), eff), LogLevel.NORMAL);
					}
				}
			}
		}

		// --- tumble: anything past the speed gates knocks the jumper down ---
		if (horiz < m_fTumbleLandSpeedMs && sink < m_fTumbleLandSinkMs && impact < 8.5)
			return;

		// ragdoll tumble removed (2026-08-22): hot arrivals keep their feet;
		// the impact damage above still tells the truth about the landing
		Print(string.Format("[HAM_Para_FreeFall] hot landing (%1 m/s fwd, %2 m/s down)", Math.Round(horiz), Math.Round(sink)), LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void RecoverFromTumble()
	{
		if (m_Controller && !m_Controller.IsDead())
			m_Controller.SetUnconscious(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyCanopy()
	{
		foreach (IEntity part : m_aCanopyParts)
		{
			if (part)
				SCR_EntityHelper.DeleteEntityAndChildren(part);
		}
		m_aCanopyParts.Clear();
		if (m_CanopyEntity)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_CanopyEntity);
			m_CanopyEntity = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Canopy flight: the same momentum philosophy, parachute-weighted.
	//! Brakes are a slider, front risers a held dive, turns carry momentum
	//! into a pendulum bank with turning sink; everything eases through
	//! SmoothCD so the wing feels heavy, floaty and organic.
	protected void UpdateCanopy(IEntity owner, float timeSlice, float inX, float inY)
	{
		m_fCanopyTime += timeSlice;

		// LANDING: the variant clip is playing - the flight model must stop
		// flying the character into the ground. Keep the graph fed and the
		// chute placed; the engine owns the body on the ground.
		if (m_iLandDir != 0 && HAM_Para_IsMotionAuthority())
		{
			// first frame of the hold: the flight model's last write left a
			// sink velocity on the physics - zero it or the body burrows
			if (!m_bLandVelCleared)
			{
				m_bLandVelCleared = true;
				m_bLandShiftPending = true;
				m_iLandShiftVariant = m_iLandDir;
				Physics landPhys = owner.GetPhysics();
				if (landPhys)
					landPhys.SetVelocity(vector.Zero);
			}
			m_iDir = m_iLandDir;
			ApplyGraphState();
			HAM_Para_SmoothProxyMotion(owner, timeSlice);
			HAM_Para_PlaceCanopyParts(owner);
			return;
		}

		// OBSERVER PLAYBACK: same rule as the freefall path - the whole model
		// below reads input an observer does not have and ends in a transform
		// write. An observer holds the hang pose; the canopy entity is already
		// parented to the rig, so it follows the replicated body for free.
		if (!HAM_Para_IsMotionAuthority())
		{
			if (m_iLandDir != 0) m_iDir = m_iLandDir;
			else m_iDir = m_iRplDir;
			ApplyGraphState();
			HAM_Para_ForwardCanopySteering();
			HAM_Para_PlaceCanopyParts(owner);
			return;
		}

		// --- LIVE SLOT TUNING (X toggles): the editor can never replicate
		// the character system's root handling of the hang anim, so the game
		// itself is the only honest preview. In tune mode the flight keys
		// steer the CANOPY in the rig frame - A/D lateral, W/S up/down,
		// Q/E fore/aft - and the total slot offset to bake is printed.
		// (The X action is click-filtered - a single-frame pulse - so this
		// is a TOGGLE with edge detection, never a hold check.)
		if (m_bCanopyTuneMode && !m_aCanopyParts.IsEmpty() && HAM_Para_IsLocalPlayer())
		{
			InputManager tuneIM = GetGame().GetInputManager();
			if (tuneIM)
			{
				// SPACE toggles the tuner now - X belongs to the flare
				bool tuneKey = tuneIM.GetActionValue("CharacterJump") > 0.5;
				if (tuneKey && !m_bTuneKeyHeld)
				{
					m_bTuneActive = !m_bTuneActive;
					if (m_bTuneActive)
					{
						Print("[HAM_Para_FreeFall] TUNE MODE ON - A/D lateral, W/S up/down, Q/E fore/aft; SPACE again to finish", LogLevel.NORMAL);
					}
					else
					{
						vector fin = m_vSlotBasePos + m_vTuneOffset;
						float finYaw = m_fCanopyYawDeg + m_fTuneYaw;
						Print("[HAM_Para_FreeFall] TUNE MODE OFF - final slot offset: "
							+ fin[0].ToString(-1, 3) + " " + fin[1].ToString(-1, 3) + " " + fin[2].ToString(-1, 3)
							+ "  <- rig Canopy Slot Offset | yaw: " + finYaw.ToString(-1, 3)
							+ " deg  <- FreeFall component Canopy Yaw", LogLevel.NORMAL);
					}
				}
				m_bTuneKeyHeld = tuneKey;

				if (m_bTuneActive)
				{
					float dLat = inX;
					float dUp = inY;
					float dFwd = tuneIM.GetActionValue("CharacterLeanRight") - tuneIM.GetActionValue("CharacterLeanLeft");

					// Z / C: VERY subtle yaw about the slot (4 deg/s)
					float dYawDir = 0;
					if (tuneIM.GetActionValue("HAM_Para_TuneYawL") > 0.5)
						dYawDir = dYawDir - 1.0;
					if (tuneIM.GetActionValue("HAM_Para_TuneYawR") > 0.5)
						dYawDir = dYawDir + 1.0;
					if (dYawDir != 0)
					{
						float dA = dYawDir * 4.0 * timeSlice;
						m_fTuneYaw += dA;
						vector rigTMr[4];
						IEntity rigPar = m_aCanopyParts[0].GetParent();
						if (rigPar)
							rigPar.GetWorldTransform(rigTMr);
						else
							owner.GetWorldTransform(rigTMr);
						vector rigR[3];
						rigR[0] = rigTMr[0];
						rigR[1] = rigTMr[1];
						rigR[2] = rigTMr[2];
						vector yawM[3];
						Math3D.AnglesToMatrix(Vector(dA, 0, 0), yawM);
						vector t1[3];
						Math3D.MatrixMultiply3(rigR, yawM, t1);
						foreach (IEntity partY : m_aCanopyParts)
						{
							vector pTM[4];
							partY.GetWorldTransform(pTM);
							vector pR[3];
							pR[0] = pTM[0];
							pR[1] = pTM[1];
							pR[2] = pTM[2];
							vector t2[3];
							Math3D.MatrixInvMultiply3(rigR, pR, t2);
							vector nR[3];
							Math3D.MatrixMultiply3(t1, t2, nR);
							pTM[0] = nR[0];
							pTM[1] = nR[1];
							pTM[2] = nR[2];
							partY.SetWorldTransform(pTM);
						}
						float yawTotal = m_fCanopyYawDeg + m_fTuneYaw;
						Print("[HAM_Para_FreeFall] TUNE yaw total: " + yawTotal.ToString(-1, 3) + " deg  <- set as Canopy Yaw", LogLevel.NORMAL);
					}

					if (dLat != 0 || dUp != 0 || dFwd != 0)
					{
						vector dLocal = Vector(dLat, dUp, dFwd) * (0.4 * timeSlice);
						m_vTuneOffset = m_vTuneOffset + dLocal;

						IEntity firstPart = m_aCanopyParts[0];
						IEntity rig = firstPart.GetParent();
						vector rigTM[4];
						if (rig)
							rig.GetWorldTransform(rigTM);
						else
							owner.GetWorldTransform(rigTM);
						vector wTM[4];
						firstPart.GetWorldTransform(wTM);
						wTM[3] = wTM[3] + rigTM[0] * dLocal[0] + rigTM[1] * dLocal[1] + rigTM[2] * dLocal[2];
						foreach (IEntity part : m_aCanopyParts)
							part.SetWorldTransform(wTM);

						m_fTunePrintTimer += timeSlice;
						if (m_fTunePrintTimer > 0.4)
						{
							m_fTunePrintTimer = 0;
							vector total = m_vSlotBasePos + m_vTuneOffset;
							Print("[HAM_Para_FreeFall] TUNE slot offset: "
								+ total[0].ToString(-1, 3) + " " + total[1].ToString(-1, 3) + " "
								+ total[2].ToString(-1, 3) + " | yaw " + (m_fCanopyYawDeg + m_fTuneYaw).ToString(-1, 3), LogLevel.NORMAL);
						}
					}
					// the wing holds trim while tuning - eat the flight inputs
					inX = 0;
					inY = 0;
				}
			}
		}

		// --- PITCH ATTITUDE SLIDER (held, like freefall): W noses the wing
		// down toward a near-vertical dive, S raises it; release the keys and
		// the attitude HOLDS where you left it. Nose-up BLEEDS speed (gravity
		// plus induced drag) and slows the wing - it is NOT a flare; the
		// flare maneuver lives on X below.
		if (Math.AbsFloat(inY) > 0.05)
			m_fCanopyAttDeg = Math.Clamp(m_fCanopyAttDeg - inY * CANOPY_PITCH_RATE_DPS * timeSlice, CANOPY_ATT_MIN_DEG, CANOPY_ATT_MAX_DEG);

		// hanging slow with the nose held high edges toward the stall...
		if (m_fBrakeSlider > 0.8)
		{
			// DEEP BRAKES: ~80%% is the minimum-sink hold - USABLE, but on a
			// fuse: hold it long enough and the wing lets go. Burying the
			// toggles shortens the fuse sharply. (~8 s at exactly 80%%,
			// ~2.4 s pinned at 100%%.) The buffet rock is the warning.
			m_fStallTime += timeSlice * (0.35 + (m_fBrakeSlider - 0.8) * 4.0);
		}
		else if (m_fCanopyAttDeg > 22.0 && m_fAirspeed < m_fCanopyFwdMs * 0.72)
			m_fStallTime += timeSlice;
		else
			m_fStallTime = Math.Max(m_fStallTime - timeSlice * 2.0, 0.0);

		// FULL STALL: the wing QUITS flying - it rocks back, the bottom
		// drops out, and the toggles are mush until you give the slack back
		if (!m_bStalled && m_fStallTime > HAM_Para_STALL_BREAK_S)
		{
			m_bStalled = true;
			Print("[HAM_Para_FreeFall] STALL - the wing has quit", LogLevel.NORMAL);
		}
		if (m_bStalled && m_fBrakeSlider < 0.55)
		{
			// RECOVERY: hands up - the canopy surges forward into a dive to
			// find its airspeed again; ride it out, then fly away
			m_bStalled = false;
			m_fStallTime = 0;
			m_fCanopyAttDeg = -16.0;
			m_fLineTwistTime = 1.2;
			Print("[HAM_Para_FreeFall] stall recovery - surge and dive", LogLevel.NORMAL);
		}
		// ...and a stalled wing QUITS: it noses itself down out of the deep
		// nose-high mush until it flies again - S cannot pin it in the sky
		if (m_fStallTime > 1.2)
			m_fCanopyAttDeg = Math.Max(m_fCanopyAttDeg - 6.0 * timeSlice, 14.0);

		// --- ANALOG BRAKES (the toggle stroke IS a slider, like a real
		// canopy): X strokes the toggles DOWN, Z eases them back UP, Q dumps
		// all slack for full flight, E trims for maximum glide range. The
		// FLARE is not a separate move any more - it is a FAST, DEEP stroke
		// with speed in hand: the application RATE feeds a decaying lift
		// surge (snatch the toggles down at speed and the wing planes out;
		// stroke them slowly, or flare with the speed already spent, and it
		// just mushes).
		float prevBrake = m_fBrakeSlider;
		InputManager fim = GetGame().GetInputManager();
		if (fim && m_Controller && HAM_Para_IsLocalPlayer() && !m_bTuneActive)
		{
			bool bUp = fim.GetActionValue("HAM_Para_BrakeUp") > 0.5;
			bool bDown = fim.GetActionValue("HAM_Para_BrakeDown") > 0.5;
			bool bDump = fim.GetActionValue("HAM_Para_BrakeDump") > 0.5;
			bool bGlide = fim.GetActionValue("HAM_Para_GlideTrim") > 0.5;

			if (bUp)
				m_fBrakeSlider = Math.Min(m_fBrakeSlider + HAM_Para_BRAKE_STROKE_RATE * timeSlice, 1.0);
			if (bDown)
				m_fBrakeSlider = Math.Max(m_fBrakeSlider - HAM_Para_BRAKE_STROKE_RATE * timeSlice, 0.0);
			if (bDump)
				m_fBrakeSlider = Math.Max(m_fBrakeSlider - HAM_Para_BRAKE_DUMP_RATE * timeSlice, 0.0);
			if (bGlide && !m_bGlideKeyHeld)
			{
				m_bGlideTrim = !m_bGlideTrim;
				Print(string.Format("[HAM_Para_FreeFall] glide trim %1", m_bGlideTrim), LogLevel.NORMAL);
			}
			m_bGlideKeyHeld = bGlide;
			// any hands-on-brakes input takes the wing back from the trimmer
			if (bUp || bDown || bDump)
				m_bGlideTrim = false;
		}

		// E GLIDE TRIM: ease brakes and attitude toward the max-range point
		// (light brakes flatten the polar without spending the airspeed the
		// way a flare does) - slow, deliberate, like trimming for distance
		if (m_bGlideTrim)
		{
			m_fBrakeSlider = Math.Lerp(m_fBrakeSlider, HAM_Para_GLIDE_BRAKE, Math.Clamp(timeSlice * 0.8, 0.0, 1.0));
			m_fCanopyAttDeg = Math.Lerp(m_fCanopyAttDeg, HAM_Para_GLIDE_ATT_DEG, Math.Clamp(timeSlice * 0.6, 0.0, 1.0));
		}

		float brake = m_fBrakeSlider;
		// rate-based flare surge: only NEW stroke, only with speed in hand
		float dBrake = Math.Max(brake - prevBrake, 0.0);
		float speedInHand = Math.Max(m_fAirspeed / m_fCanopyFwdMs - 0.75, 0.0);
		m_fFlareSurge = Math.Min(m_fFlareSurge + dBrake * speedInHand * 2.6, 1.4);
		m_fFlareSurge = m_fFlareSurge * Math.Clamp(1.0 - timeSlice / 1.7, 0.0, 1.0);

		// deep brakes level the wing and kill turn authority (both toggles
		// buried leaves nothing differential to steer with)
		if (brake > 0.6)
		{
			float deep = (brake - 0.6) / 0.4;
			m_fCanopyAttDeg = Math.Lerp(m_fCanopyAttDeg, 8.0, Math.Clamp(2.0 * deep * timeSlice, 0.0, 1.0));
			m_fTurnRate = m_fTurnRate * Math.Clamp(1.0 - 2.0 * deep * timeSlice, 0.0, 1.0);
		}

		float attRad = m_fCanopyAttDeg * Math.DEG2RAD;
		float noseUp = Math.Max(m_fCanopyAttDeg, 0.0);      // deg, nose-high side
		float noseUpFrac = noseUp / CANOPY_ATT_MAX_DEG;     // 0..1
		float diveRad = Math.Max(-attRad, 0.0);             // rad, dive side

		// --- turn-rate SLIDER (held, same semantics as pitch): A/D wind the
		// turn rate up or down and it HOLDS where you leave it - a set carve
		// keeps carving until you counter it back out. Authority still scales
		// gently with airspeed and line twist still cuts control; a small
		// centering band snaps near-zero rates to straight flight so a
		// counter-tap cleanly ends the turn.
		float turnAuthority = Math.Clamp(m_fAirspeed / m_fCanopyFwdMs, 0.5, 1.25);
		turnAuthority = turnAuthority * (1.0 - 0.45 * brake);
		if (m_bStalled)
			turnAuthority = turnAuthority * 0.12;
		if (m_fLineTwistTime > 0)
			turnAuthority = turnAuthority * 0.25;
		// No steering until the canopy is fully open and settled. Only the
		// INPUT is gated - any rate already carried in from a spinning exit
		// keeps running and bleeds off as before, because that residual
		// rotation IS the line-twist penalty and killing it here would hand
		// back a free recovery from a bad deploy.
		bool steerUnlocked = m_fCanopyTime >= CANOPY_STEER_START_S;
		if (steerUnlocked && Math.AbsFloat(inX) > 0.05)
			m_fTurnRate = Math.Clamp(m_fTurnRate + inX * CANOPY_TURN_ACCEL_DPS2 * turnAuthority * timeSlice, -m_fCanopyTurnMaxDps, m_fCanopyTurnMaxDps);
		else if (Math.AbsFloat(m_fTurnRate) < 1.5)
			m_fTurnRate = 0;
		m_fFlightYaw += m_fTurnRate * timeSlice;
		if (m_fFlightYaw > 180.0)
			m_fFlightYaw -= 360.0;
		else if (m_fFlightYaw < -180.0)
			m_fFlightYaw += 360.0;
		float turnFrac = Math.Clamp(m_fTurnRate / m_fCanopyTurnMaxDps, -1.0, 1.0);

		// coordinated bank straight from physics: atan(v*omega/g), the real
		// formula - a hard fast spiral banks STEEP; smoothed for pendulum feel
		float bankTargetDeg = Math.Atan2(m_fAirspeed * Math.AbsFloat(m_fTurnRate) * Math.DEG2RAD, 9.81) * Math.RAD2DEG;
		bankTargetDeg = Math.Min(bankTargetDeg, 50.0);
		if (turnFrac < 0)
			bankTargetDeg = -bankTargetDeg;
		float kBank = Math.Clamp(timeSlice * 1.8, 0.0, 1.0);
		m_fBankVisDeg = Math.Lerp(m_fBankVisDeg, bankTargetDeg, kBank);

		// steep sustained spirals FEED the energy model - diving turns
		// build real speed
		m_fAirspeed += 2.0 * turnFrac * turnFrac * Math.Clamp(Math.AbsFloat(bankTargetDeg) / 50.0, 0.0, 1.3) * timeSlice;

		// --- ENERGY MODEL, attitude-driven: the wing is an airframe. Gravity
		// works along the flight path - nose-down feeds airspeed, nose-up
		// bleeds it (the same g*sin(att), just signed) - plus parasitic drag
		// toward the attitude's trim speed and induced drag off the raised
		// nose. The flare bleed is gravity-honest, NOT an airbrake dump: it
		// decelerates firmly but naturally and eases as the speed comes off,
		// while a dive's surge still carries through the level-out.
		float vTrimEff = Math.Lerp(m_fCanopyFwdMs, 5.0, noseUpFrac);
		// brakes raise the whole wing's angle of attack: trim speed drops
		// toward a walk at full stroke, and the toggles drag continuously
		vTrimEff = vTrimEff * (1.0 - 0.62 * brake);
		float dragRatio = Math.Max(m_fAirspeed / m_fCanopyFwdMs, 1.0);
		float inducedDrag = Math.Sin(Math.Max(attRad, 0.0)) * (1.6 + 0.16 * m_fAirspeed);
		// forward bleed is a CURVE, not a wall: the flare's drag scales with
		// how fast the wing still flies, so speed falls away progressively
		// and the bleed tapers off as it dies - while the sink arrest above
		// stays the strong, immediate half of the maneuver
		float speedFrac = Math.Clamp(m_fAirspeed / m_fCanopyFwdMs, 0.0, 2.0);
		float brakeDrag = brake * (1.8 + 0.26 * m_fAirspeed) + m_fFlareSurge * speedFrac * 2.0;
		m_fAirspeed += (-9.81 * Math.Sin(attRad) - 0.115 * (m_fAirspeed - vTrimEff) * dragRatio - inducedDrag - brakeDrag) * timeSlice;
		m_fAirspeed = Math.Clamp(m_fAirspeed, 1.5, 46.0);

		// lift balance: lift grows with speed squared, so excess speed
		// FLATTENS the glide (level out of a dive and the descent nearly
		// stops while the surge lasts); below trim the wing mushes
		float liftFactor = Math.Clamp((m_fAirspeed / m_fCanopyFwdMs) * (m_fAirspeed / m_fCanopyFwdMs), 0.7, 9.0);
		float sinkT = m_fCanopySinkMs / liftFactor;
		sinkT = sinkT - 0.8 * Math.Sin(Math.Clamp(noseUp / 24.0, 0.0, 1.0) * Math.PI); // min-sink around half nose-up
		// the brake polar: sink falls steadily deeper into the brakes and
		// bottoms out around 80%% stroke - THE slowest descent the wing has
		// - then mushes past ~85%% as the deep stall approaches
		sinkT = sinkT - 1.1 * Math.Sin(Math.Clamp(brake / 1.6, 0.0, 1.0) * Math.PI);
		if (brake > 0.85)
			sinkT = sinkT + 2.5 * (brake - 0.85) / 0.15;
		sinkT = sinkT + Math.AbsFloat(turnFrac) * 2.5;            // turning sink
		if (m_fStallTime > 1.2)
			sinkT = sinkT + 2.2;                                  // pre-stall: the wing is letting go
		if (m_bStalled)
		{
			// full stall: no lift worth the name - a rocking, mushing drop
			sinkT = sinkT + 6.5;
			m_fAirspeed = Math.Lerp(m_fAirspeed, 2.5, Math.Clamp(timeSlice * 1.8, 0.0, 1.0));
		}
		// PITCH-UP LIFT, strong: raising the nose converts speed above ~0.78
		// trim into a genuine zoom-climb - pull up out of a dive surge and
		// the wing ROTATES the energy skyward; the climb dies exactly as the
		// bleed spends the speed. This is the dynamic maneuver...
		float excess = Math.Max(m_fAirspeed / m_fCanopyFwdMs - 0.78, 0.0);
		float pitchLift = Math.Sin(Math.Max(attRad, 0.0)) * excess * m_fCanopyFwdMs * 2.8;
		// ...while the FLARE SURGE is the landing tool: the rate-fed
		// transient ARRESTS the descent on whatever speed the stroke caught
		// - hard - but can only take the sink to zero, never balloon the
		// jumper upward (the zoom-climb stays the attitude maneuver's job)
		sinkT = Math.Max(sinkT - pitchLift, -7.0);
		float flareLift = m_fFlareSurge * m_fCanopyFwdMs * 2.4;
		sinkT = sinkT - Math.Min(flareLift, Math.Max(sinkT, 0.0));

		// path split: the dive rotates the airspeed vector into the vertical;
		// the lift-balance sink rides on the level component
		float fwdT = m_fAirspeed * Math.Cos(attRad);
		sinkT = m_fAirspeed * Math.Sin(diveRad) + sinkT * Math.Cos(diveRad);

		// --- momentum: heavier carve than freefall; the opening snatch
		// emerges from SmoothCD pulling terminal velocity down to sink rate
		float yawRad = m_fFlightYaw * Math.DEG2RAD;
		vector cfwd = Vector(Math.Sin(yawRad), 0, Math.Cos(yawRad));
		vector vTarget = cfwd * fwdT;
		// the deeper the dive, the faster prior glide converts into the
		// plunge - momentum drift stays visible for a moment, then it's
		// all downhill. Brakes ALSO tighten the momentum: a flare must be
		// FELT in the world speed, not just in the energy state.
		float horizT = Math.Lerp(CANOPY_MOMENTUM_T, 0.9, Math.Clamp(diveRad / (Math.PI * 0.5), 0.0, 1.0));
		horizT = Math.Lerp(horizT, 0.7, noseUpFrac);
		// PULL beat: the drogue barely touches horizontal momentum - the
		// track carries until the wing actually opens
		if (m_fCanopyTime < HAM_Para_OPEN_START_S)
			horizT = 6.0;
		float vx = Math.SmoothCD(m_vFlightVel[0], vTarget[0], m_fVelXV, horizT, 40.0, timeSlice);
		float vz = Math.SmoothCD(m_vFlightVel[2], vTarget[2], m_fVelZV, horizT, 40.0, timeSlice);
		m_vFlightVel = Vector(vx, 0, vz);

		// --- WIND: the canopy rides the airmass. Weather-system wind is
		// sampled every few seconds and becomes drift, with a light gust
		// texture so the air feels alive. Land upwind like you mean it.
		m_fWindResample -= timeSlice;
		if (m_fWindResample <= 0)
		{
			m_fWindResample = 5.0;
			m_vWind = vector.Zero;
			TimeAndWeatherManagerEntity weather = null;
			ChimeraWorld chWorld = ChimeraWorld.CastFrom(owner.GetWorld());
			if (chWorld)
				weather = chWorld.GetTimeAndWeatherManager();
			if (weather)
			{
				float wSpd = weather.GetWindSpeed();
				float wDir = weather.GetWindDirection() * Math.DEG2RAD;
				// direction is meteorological (blowing FROM) - drift goes the
				// opposite way; flip the sign here if it reads reversed in-game
				m_vWind = Vector(-Math.Sin(wDir), 0, -Math.Cos(wDir)) * wSpd;
			}
		}
		float gust = 1.0 + 0.15 * Math.Sin(m_fCanopyTime * 0.9) * Math.Sin(m_fCanopyTime * 0.53);
		vector windNow = m_vWind * gust * m_fWindDriftScale;

		// vertical: MODEL-OWNED state. Re-reading the physics here made the
		// smoother fight the engine's gravity every frame - a feedback war
		// that settled ~13 m/s below the intended sink. Our value is the
		// authority; the physics gets overwritten with it below.
		Physics phys = owner.GetPhysics();
		float vertT = CANOPY_VERT_T;
		float sinkTarget = -sinkT;
		if (m_fCanopyTime < HAM_Para_OPEN_START_S)
		{
			// pull + payout: only the drogue is out - hold a drogue-braked
			// fall, the real deceleration starts at line stretch
			sinkTarget = -42.0;
			vertT = 1.2;
		}
		else if (m_fCanopyTime < HAM_Para_OPEN_START_S + 3.0)
			vertT = CANOPY_OPEN_SHOCK_T;
		else if (m_fFlareSurge > 0.25 || (noseUp > 12.0 && m_fAirspeed > m_fCanopyFwdMs * 1.1))
			vertT = 0.6; // a flare pop or a fast pull-up answers immediately
		else if (m_fVerticalVel < -sinkT)
			vertT = 2.6; // pulling OUT of a dive: descent bleeds off in a slow arc
		float vy = Math.SmoothCD(m_fVerticalVel, sinkTarget, m_fVelYV, vertT, 60.0, timeSlice);
		m_fVerticalVel = vy;

		// --- pendulum: bank into the turn; pitch swings with forward accel
		// and brake strokes (spring-damper keeps it organic)
		float fwdSpeed = Vector(vx, 0, vz).Length();
		float fwdAccel = 0;
		if (timeSlice > 0)
			fwdAccel = (fwdSpeed - m_fPrevFwdSpeed) / timeSlice;
		m_fPrevFwdSpeed = fwdSpeed;
		// the body pendulum FOLLOWS the commanded attitude (that IS the
		// slider): full nose-down shows the vertical dive, full flare raises
		// the nose a real +32 (negative pitch = nose down in this rig);
		// accel swing and turbulence ride on top
		float pendTarget = m_fCanopyAttDeg + Math.Clamp(fwdAccel * -2.2, -22.0, 22.0);
		// organic air: slow multi-period turbulence sway, louder when fast
		float swayAmp = 0.5 + 0.5 * Math.Clamp(m_fAirspeed / m_fCanopyFwdMs, 0.0, 2.0);
		pendTarget = pendTarget + (Math.Sin(m_fCanopyTime * 1.7) * 1.2 + Math.Sin(m_fCanopyTime * 0.61) * 1.8) * swayAmp;
		// stall rocks the wing hard - the buffet is the warning, the full
		// stall is a violent nose-high wallow
		if (m_bStalled)
			pendTarget = pendTarget + 14.0 + Math.Sin(m_fCanopyTime * 7.0) * 11.0;
		else if (m_fStallTime > 1.2)
			pendTarget = pendTarget + Math.Sin(m_fCanopyTime * 9.0) * 6.0;
		if (m_fCanopyTime < HAM_Para_OPEN_START_S)
		{
			// drogue fall: the body keeps flying its ripcord attitude with a
			// slow relax toward a gentle belly arch - no uprighting yet
			float relax = Math.Clamp(m_fCanopyTime / HAM_Para_OPEN_START_S, 0.0, 1.0) * 0.35;
			pendTarget = Math.Lerp(m_fDeployStartPitch, -25.0, relax);
		}
		m_fPendPitch = Math.SmoothCD(m_fPendPitch, pendTarget, m_fPendPitchVel, 0.9, 200.0, timeSlice);

		// line twist wobble decays as the lines unwind; the body also
		// slightly TRAILS a hard turn (visual only)
		if (m_fLineTwistTime > 0)
			m_fLineTwistTime = Math.Max(m_fLineTwistTime - timeSlice, 0.0);
		m_fYawVisOffset = -m_fTurnRate * 0.10 + Math.Sin(m_fCanopyTime * 8.0) * 8.0 * (m_fLineTwistTime / 2.2);

		// publish attitude for the shared rotation writers (post-frame incl.)
		// bank was set by the coordinated-turn block above
		m_fPitchTrim = m_fPendPitch;
		m_fRollTrim = 0;
		// ripcord bank bleeds off across the pull instead of snapping flat
		if (m_fDeployStartRoll != 0)
		{
			m_fDeployStartRoll -= m_fDeployStartRoll * Math.Clamp(timeSlice * 1.6, 0.0, 1.0);
			if (Math.AbsFloat(m_fDeployStartRoll) < 0.5)
				m_fDeployStartRoll = 0;
		}

		// --- motion authority (same pattern as freefall, vertical owned too).
		// OWNER ONLY: on an observer this was writing the proxy's transform
		// and physics velocity every frame from a locally guessed canopy
		// model, fighting the replicated motion for control of the character.
		if (!m_bDebugNoMotion && HAM_Para_IsMotionAuthority())
		{
			vector rotM[3];
			float visPitch = 0;
			float visRoll = 0;
			if (m_bApplyRotation)
			{
				visPitch = m_fPitchTrim;
				visRoll = m_fBankVisDeg + m_fDeployStartRoll;
			}
			Math3D.AnglesToMatrix(Vector(m_fFlightYaw + m_fYawVisOffset, visPitch, visRoll), rotM);
			vector tm2[4];
			tm2[0] = rotM[0];
			tm2[1] = rotM[1];
			tm2[2] = rotM[2];
			vector vWorld = Vector(vx + windNow[0], vy, vz + windNow[2]);
			tm2[3] = owner.GetOrigin() + vWorld * timeSlice;
			owner.SetWorldTransform(tm2);
			if (phys)
			{
				phys.SetVelocity(vWorld);
				phys.SetAngularVelocity(vector.Zero);
			}
		}

		// analog steering for the graph: DirX = signed toggle turn,
		// DirY = flare envelope. Both arms park by these parameters.
		//
		// DirY also carries the pitch-attitude slider, SIGNED: S rides the
		// toggles down (nose up), W lets them up (nose down). Holding either
		// with the arms parked read wrong - the wing changed attitude but the
		// jumper never moved. Summed with the brake stroke rather than maxed,
		// so W can visibly BLEED OFF a held flare instead of being ignored
		// whenever the brakes happen to be deeper; clamped so the arms cannot
		// overpull past full flare or lift past full flight.
		float attToggleShift = Math.Clamp(m_fCanopyAttDeg / CANOPY_ATT_MAX_DEG, -1.0, 1.0)
			* CANOPY_ATT_TOGGLE_ANIM_MAX;
		m_fAnimDirX = turnFrac;
		m_fAnimDirY = Math.Clamp(m_fBrakeSlider + attToggleShift, 0.0, 1.0);
		HAM_Para_PublishAnalog();
		HAM_Para_ForwardCanopySteering();

		// The motion owner selects and publishes the pull/open/hang phase.
		int previousCanopyDir = m_iDir;
		if (m_fCanopyTime < HAM_Para_OPEN_START_S)
			m_iDir = 10;   // pull, then held arch until line stretch
		else if (m_fCanopyTime < HAM_Para_OPEN_END_S)
			m_iDir = 11;   // the snatch rides the stretch->bloom window
		else
			m_iDir = 9;
		if (m_iDir != previousCanopyDir) HAM_Para_PublishDir();
		ApplyGraphState();
		HAM_Para_PlaceCanopyParts(owner);

		// FX fade themselves out at canopy speeds; a riser dive tickles them
		UpdateVignetteShake(Vector(vx, vy, vz).Length(), timeSlice);

		// cloth loops after the opening transient: calm layer at trim, the
		// roaring layer when the wing is fast; clock-based retrigger, layer
		// swaps hidden under fades (airspeed hysteresis stops flapping)
		if (m_Controller && HAM_Para_IsLocalPlayer())
		{
			// opening boom lands on the OPEN beat: line stretch + the
			// character's G-force snatch (the visual parts appear here too).
			// The freefall wind dies INTO the boom, not before it.
			if (!m_bOpenSndPlayed && m_fCanopyTime >= HAM_Para_OPEN_START_S)
			{
				m_bOpenSndPlayed = true;
				AudioSystem.PlaySound(HAM_Para_DeployWav());
				if (m_iWindLayer > 0)
				{
					AudioSystem.TerminateSoundFadeOut(m_WindHandle, true, 1.2);
					m_iWindLayer = 0;
				}
			}
			else if (m_iWindLayer > 0 && m_fCanopyTime < HAM_Para_OPEN_START_S)
			{
				// keep the freefall wind machine ticking through the pull so
				// an intro end or a loop seam inside the window can't drop it
				m_fWindLoopTime += timeSlice;
				if (m_iWindLayer == 1 && m_fWindLoopTime >= WIND_INTRO_LEN_S)
				{
					m_WindHandle = AudioSystem.PlaySound(HAM_Para_WindWav(false));
					m_fWindLoopTime = 0;
					m_iWindLayer = 2;
				}
				else if (m_iWindLayer == 2 && m_fWindLoopTime >= WIND_LOOP_LEN_S)
				{
					AudioSystem.TerminateSound(m_WindHandle);
					m_WindHandle = AudioSystem.PlaySound(HAM_Para_WindWav(false));
					m_fWindLoopTime = 0;
				}
			}

			bool wantLoud = m_bCanopyLoud;
			if (m_fAirspeed > m_fCanopyFwdMs * 1.35)
				wantLoud = true;
			else if (m_fAirspeed < m_fCanopyFwdMs * 1.15)
				wantLoud = false;

			if (!m_bCanopyLoopOn && m_fCanopyTime >= CANOPY_LOOP_START_S)
			{
				m_bCanopyLoopOn = true;
				m_bCanopyLoud = wantLoud;
				m_WindHandle = AudioSystem.PlaySound(GetCanopyWav(wantLoud));
				m_fWindLoopTime = 0;
			}
			else if (m_bCanopyLoopOn)
			{
				m_fWindLoopTime += timeSlice;
				if (wantLoud != m_bCanopyLoud)
				{
					m_bCanopyLoud = wantLoud;
					AudioSystem.TerminateSoundFadeOut(m_WindHandle, true, 0.6);
					m_WindHandle = AudioSystem.PlaySound(GetCanopyWav(wantLoud));
					m_fWindLoopTime = 0;
				}
				else if (m_fWindLoopTime >= WIND_LOOP_LEN_S)
				{
					AudioSystem.TerminateSound(m_WindHandle);
					m_WindHandle = AudioSystem.PlaySound(GetCanopyWav(m_bCanopyLoud));
					m_fWindLoopTime = 0;
				}
			}
		}

		m_fDebugTimer += timeSlice;
		if (m_fDebugTimer > 2.0)
		{
			m_fDebugTimer = 0;
			Print(string.Format("[HAM_Para_FreeFall] canopy: air=%1 att=%2 turn=%3 fwd=%4 sink=%5 agl=%6",
				Math.Round(m_fAirspeed), Math.Round(m_fCanopyAttDeg), Math.Round(m_fTurnRate), Math.Round(fwdSpeed), Math.Round(-vy), Math.Round(GetHeightAGL(owner))), LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Speed screen FX (local player only): vignette that closes in and wind
	//! buffeting shake, both ramping with total airspeed
	protected void UpdateSpeedFX(float timeSlice)
	{
		if (!m_Controller || !HAM_Para_IsLocalPlayer())
			return;

		// speed from the AUTHORITATIVE physics velocity, then smoothed:
		// position-delta measurement jitters frame-to-frame while our motion
		// authority and the physics both touch the transform, and a flapping
		// intensity strobes the vignette and machine-guns the audio bands
		float vertical = m_fMeasuredDown;
		IEntity ffOwner = GetOwner();
		if (ffOwner)
		{
			Physics phys = ffOwner.GetPhysics();
			if (phys)
				vertical = phys.GetVelocity()[1];
		}
		float speed = Vector(m_vFlightVel[0], vertical, m_vFlightVel[2]).Length();
		UpdateVignetteShake(speed, timeSlice);
		float intensity = m_fFXIntensity;

		// EARPLUGS: pick the quiet beds while the SFX bus is ducked. On a
		// CHANGE, cut the bed that is playing so the new level is heard now -
		// the wind loop runs ~53 s, so waiting for its natural restart would
		// leave plugs feeling broken for most of a jump. Clearing the handle
		// is enough; the state machine below re-arms the layer by itself.
		bool ducked = HAM_Para_IsAudioDucked(timeSlice);
		if (ducked != m_bAudioDucked)
		{
			m_bAudioDucked = ducked;
			if (m_WindHandle != AudioHandle.Invalid)
			{
				AudioSystem.TerminateSoundFadeOut(m_WindHandle, true, 0.3);
				m_WindHandle = AudioHandle.Invalid;
				m_iWindLayer = 0;
				m_fWindLoopTime = 0;
			}
		}

		// wind: starts ONCE per fall as a 10 s intro with a BAKED 6 s fade-in
		// (the audio API has no volume control, so the rise lives in the
		// file), then hands off to the 25 s seam-crossfaded loop just before
		// the intro's natural end. The only restarts after that are the clock
		// retriggers at the seam, where the content matches itself.
		// m_iWindLayer: 0 = silent, 1 = intro, 2 = loop.
		m_fWindLoopTime += timeSlice;
		if (m_iWindLayer == 0)
		{
			if (intensity >= 0.12)
			{
				m_WindHandle = AudioSystem.PlaySound(HAM_Para_WindWav(true));
				m_fWindLoopTime = 0;
				m_iWindLayer = 1;
			}
		}
		else if (intensity < 0.04)
		{
			// all but stopped mid-air (deploy hand-off crossfades separately)
			AudioSystem.TerminateSoundFadeOut(m_WindHandle, true, 2.5);
			m_iWindLayer = 0;
		}
		else if (m_iWindLayer == 1 && m_fWindLoopTime >= WIND_INTRO_LEN_S)
		{
			// the intro ends by itself - just start the loop over its last
			// breath, no terminate needed
			m_WindHandle = AudioSystem.PlaySound(HAM_Para_WindWav(false));
			m_fWindLoopTime = 0;
			m_iWindLayer = 2;
		}
		else if (m_iWindLayer == 2 && m_fWindLoopTime >= WIND_LOOP_LEN_S)
		{
			// clock-based seam retrigger: restart just before the file ends
			// naturally, at the crossfaded seam where content matches
			AudioSystem.TerminateSound(m_WindHandle);
			m_WindHandle = AudioSystem.PlaySound(HAM_Para_WindWav(false));
			m_fWindLoopTime = 0;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Shared vignette + buffeting shake, smoothed against a jittery speed
	//! source; used by both the freefall and canopy paths (local player only)
	protected void UpdateVignetteShake(float speed, float timeSlice)
	{
		// Vignette, horizon artificiel, aberration et flou : retirés.
		return;

		// earlier onset and a fuller ramp: canopy dives and surges show
		// meaningfully, freefall saturates hard
		float targetIntensity = Math.Clamp((speed - 14.0) / 50.0, 0.0, 1.0);
		float kFX = Math.Clamp(timeSlice * 3.0, 0.0, 1.0);
		m_fFXIntensity = Math.Lerp(m_fFXIntensity, targetIntensity, kFX);
		float intensity = m_fFXIntensity;

		// chromatic aberration fringes in once things get properly fast
		IEntity fxOwner = GetOwner();
		if (fxOwner)
		{
			BaseWorld fxWorld = fxOwner.GetWorld();
			if (fxWorld)
			{
				// REMEMBER the camera each effect was set on. Clearing against
				// whatever GetCurrentCameraId() happens to return later cleared
				// the wrong camera whenever the view changed in between (death
				// cam, spectator, entering a vehicle) and left the effect
				// burned onto the screen with nothing able to remove it.
				if (!m_bChromAber && intensity > 0.35)
				{
					m_bChromAber = true;
					m_iChromAberCamId = fxWorld.GetCurrentCameraId();
					fxWorld.SetCameraPostProcessEffect(m_iChromAberCamId, CHROMABER_PRIORITY, PostProcessEffectType.ChromAber, CHROMABER_EMAT);
				}
				else if (m_bChromAber && intensity < 0.25)
				{
					m_bChromAber = false;
					fxWorld.SetCameraPostProcessEffect(m_iChromAberCamId, CHROMABER_PRIORITY, PostProcessEffectType.ChromAber, "");
					m_iChromAberCamId = -1;
				}

				// lens depth of field layers on top once properly fast
				if (!m_bSpeedDoF && intensity > 0.55)
				{
					m_bSpeedDoF = true;
					m_iSpeedDoFCamId = fxWorld.GetCurrentCameraId();
					fxWorld.SetCameraPostProcessEffect(m_iSpeedDoFCamId, SPEEDDOF_PRIORITY, PostProcessEffectType.DepthOfField, SPEEDDOF_EMAT);
				}
				else if (m_bSpeedDoF && intensity < 0.45)
				{
					m_bSpeedDoF = false;
					fxWorld.SetCameraPostProcessEffect(m_iSpeedDoFCamId, SPEEDDOF_PRIORITY, PostProcessEffectType.DepthOfField, "");
					m_iSpeedDoFCamId = -1;
				}
			}
		}

		if (m_iFXCreateTries < 2 && (!m_wSpeedFX || !m_wFlightHUD))
		{
			// retry at most once a second - never per-frame
			m_fFXCreateRetryTime -= timeSlice;
			if (m_fFXCreateRetryTime <= 0)
			{
				m_fFXCreateRetryTime = 1.0;
				m_iFXCreateTries++;
				if (!m_wSpeedFX)
				{
					m_wSpeedFX = GetGame().GetWorkspace().CreateWidgets(SPEEDFX_LAYOUT);
					if (m_wSpeedFX)
						m_wSpeedFX.SetOpacity(0);
				}
				if (!m_wFlightHUD)
				{
					m_wFlightHUD = GetGame().GetWorkspace().CreateWidgets(FLIGHTHUD_LAYOUT);
					if (m_wFlightHUD)
					{
						m_wHorizonLine = ImageWidget.Cast(m_wFlightHUD.FindAnyWidget("GMS_HorizonLine"));
						m_wPitchBankText = TextWidget.Cast(m_wFlightHUD.FindAnyWidget("GMS_PitchBankText"));
					}
				}
			}
		}
		if (m_wSpeedFX)
			m_wSpeedFX.SetOpacity(intensity);

		// flight HUD: a tiny artificial horizon - the line counter-rotates the
		// bank and slides with pitch, the text reads the raw numbers
		if (m_wFlightHUD)
		{
			float dispPitch = Math.Repeat(m_fPitchTrim + 180.0, 360.0) - 180.0;
			float dispRoll = Math.Repeat(m_fRollTrim + m_fBankVisDeg + 180.0, 360.0) - 180.0;
			if (m_wHorizonLine)
			{
				m_wHorizonLine.SetRotation(-dispRoll);
				FrameSlot.SetPosY(m_wHorizonLine, -1.0 + Math.Clamp(dispPitch, -45.0, 45.0) * 0.7);
			}
			if (m_wPitchBankText)
			{
				string bankSide = "";
				if (dispRoll > 1.0)
					bankSide = "R";
				else if (dispRoll < -1.0)
					bankSide = "L";
				m_wPitchBankText.SetText(string.Format("%1°  %2%3°", Math.Round(dispPitch), bankSide, Math.Round(Math.AbsFloat(dispRoll))));
			}
		}

		m_fShakeAccum += timeSlice;
		if (intensity > 0.1 && m_fShakeAccum > 0.18)
		{
			m_fShakeAccum = 0;
			SCR_CameraShakeManagerComponent.AddCameraShake(0.35 * intensity, 0.5 * intensity, 0.05, 0.08, 0.18);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! True while the SFX bus sits below what the player actually configured -
	//! the signature of an earplug mod. Compared against the CONFIGURED level
	//! rather than a fixed number, so someone who simply runs quiet SFX is not
	//! mistaken for wearing plugs. Reading the settings module every frame is
	//! wasteful, so the setting is cached and refreshed once a second while the
	//! bus itself is sampled live.
	protected bool HAM_Para_IsAudioDucked(float timeSlice)
	{
		m_fSfxPollTime -= timeSlice;
		if (m_fSfxConfiguredVol < 0.0 || m_fSfxPollTime <= 0.0)
		{
			m_fSfxPollTime = 1.0;
			UserSettings engineSettings = GetGame().GetEngineUserSettings();
			if (engineSettings)
			{
				BaseContainer audioSettings = engineSettings.GetModule("AudioSettings");
				if (audioSettings)
				{
					float configuredPct;
					if (audioSettings.Get("VolumeSfx", configuredPct))
						m_fSfxConfiguredVol = configuredPct * 0.01;
				}
			}
		}
		if (m_fSfxConfiguredVol < 0.0)
			return false;   // setting unreadable - never muffle on a guess
		// 0.02 of headroom: bus and setting are floats rounded through a
		// percentage, so they never compare exactly equal
		return AudioSystem.GetMasterVolume(AudioSystem.SFX) < m_fSfxConfiguredVol - 0.02;
	}

	//------------------------------------------------------------------------------------------------
	protected string GetCanopyWav(bool loud)
	{
		if (loud)
			{
				if (m_bAudioDucked)
					return CANOPY_WAV_EP;
				return CANOPY_WAV;
			}
		if (m_bAudioDucked)
			return CANOPY_WAV_SOFT_EP;
		return CANOPY_WAV_SOFT;
	}

	//------------------------------------------------------------------------------------------------
	protected string HAM_Para_DeployWav()
	{
		if (m_bAudioDucked)
			return DEPLOY_WAV_EP;
		return DEPLOY_WAV;
	}

	//------------------------------------------------------------------------------------------------
	protected string HAM_Para_WindWav(bool intro)
	{
		if (intro)
			{
				if (m_bAudioDucked)
					return WIND_WAV_INTRO_EP;
				return WIND_WAV_INTRO;
			}
		if (m_bAudioDucked)
			return WIND_WAV_FULL_EP;
		return WIND_WAV_FULL;
	}

	//------------------------------------------------------------------------------------------------
	//! Tear down every LOCAL-ONLY side effect of a jump: screen widgets, camera
	//! post-process, and the wind/cloth audio handle. Split out of EndFreeFall
	//! so OnDelete can run it too - a character deleted mid-air (disconnect,
	//! respawn, Game Master) used to leave the vignette and flight HUD on
	//! screen and the wind loop playing with no owner left to stop it.
	protected void ReleaseLocalFX()
	{
		if (m_wSpeedFX)
		{
			m_wSpeedFX.RemoveFromHierarchy();
			m_wSpeedFX = null;
		}

		if (m_wFlightHUD)
		{
			m_wFlightHUD.RemoveFromHierarchy();
			m_wFlightHUD = null;
			m_wHorizonLine = null;
			m_wPitchBankText = null;
		}

		// clear against the camera the effect was SET on, never the current
		// one: a camera change in between (death cam, spectator, vehicle)
		// meant the clear landed on the wrong id and the effect stuck
		if (m_bChromAber || m_bSpeedDoF)
		{
			IEntity efOwner = GetOwner();
			BaseWorld efWorld = null;
			if (efOwner)
				efWorld = efOwner.GetWorld();
			if (efWorld)
			{
				if (m_bChromAber && m_iChromAberCamId != -1)
					efWorld.SetCameraPostProcessEffect(m_iChromAberCamId, CHROMABER_PRIORITY, PostProcessEffectType.ChromAber, "");
				if (m_bSpeedDoF && m_iSpeedDoFCamId != -1)
					efWorld.SetCameraPostProcessEffect(m_iSpeedDoFCamId, SPEEDDOF_PRIORITY, PostProcessEffectType.DepthOfField, "");
			}
			m_bChromAber = false;
			m_bSpeedDoF = false;
			m_iChromAberCamId = -1;
			m_iSpeedDoFCamId = -1;
		}

		// UNCONDITIONAL: the old flag test missed the window between deploy
		// (which zeroes m_iWindLayer) and CANOPY_LOOP_START_S (which raises
		// m_bCanopyLoopOn) - land inside those first 3 s and the handle was
		// never terminated. Terminating an already-dead handle is harmless.
		AudioSystem.TerminateSoundFadeOut(m_WindHandle, true, 1.8);
		m_iWindLayer = 0;
		m_bCanopyLoopOn = false;
		m_fWindLoopTime = 0;
	}

	//------------------------------------------------------------------------------------------------
	void EndFreeFall()
	{
		if (m_GMSPresentation) m_GMSPresentation.Release();
		ReleaseLocalFX();

		DestroyCanopy();
		m_bCanopy = false;

		if (m_VarActive != -1 && m_AnimComponent)
			m_AnimComponent.SetVariableBool(m_VarActive, false);
		m_bFreeFalling = false;
		m_bGraphActive = false;
		m_iDir = 0;
		m_fAnimDirX = 0;
		m_fAnimDirY = 0;
		m_fSentDirX = 999.0;
		m_fSentDirY = 999.0;
		m_fGroundedTime = 0;

		if (m_HeadAiming)
			m_HeadAiming.ResetLimitAnglesOverride();

		HAM_Para_PublishState();
	}

	//------------------------------------------------------------------------------------------------
	//! Push the current mode into the anim graph. Runs on EVERY machine - this
	//! is the one piece of the jump that is not authority- or owner-gated,
	//! because anim graph variables are per-machine and do not replicate; the
	//! replicated state exists precisely so every machine can write them from
	//! the same truth. m_iDir is locally computed on the motion authority and
	//! taken from m_iRplDir everywhere else.
	protected void ApplyGraphState()
	{
		HAM_Para_SetFlightPresentation(1, 0, m_fAnimDirX, m_fAnimDirY);
	}

	protected void HAM_Para_SetFlightPresentation(int mode, int phase, float x, float y)
	{
		if (!m_AnimComponent) return;
		if (!m_GMSPresentation) m_GMSPresentation = new HAM_Para_ParachuteAnimationBridge(m_AnimComponent);
		m_GMSPresentation.SetState(mode, phase, m_iDir, x, y);
		m_bGraphActive = mode != 0;
	}

	// The aircraft visual and flight controller share one attachment owner.
	void HAM_Para_SetStaticLineHold(float left, float right)
	{
		if (m_bFreeFalling || !m_AnimComponent) return;
		if (!m_GMSPresentation) m_GMSPresentation = new HAM_Para_ParachuteAnimationBridge(m_AnimComponent);
		int mode = 0;
		if (left > 0.001 || right > 0.001) mode = 3;
		m_GMSPresentation.SetState(mode, 0, 0, left, right);
	}

	//------------------------------------------------------------------------------------------------
	//! Get the locally computed compass to the state authority. On the
	//! authority itself this is a direct publish; on an owning client it is an
	//! ask. Nobody else ever calls it - proxies have nothing to contribute.
	protected void HAM_Para_PublishDir()
	{
		if (HAM_Para_IsStateAuthority())
		{
			HAM_Para_PublishState();
			return;
		}
		if (HAM_Para_IsMotionAuthority())
			Rpc(RpcAsk_GMSDir, m_iDir);
	}

	//------------------------------------------------------------------------------------------------
	//! STATE AUTHORITY ONLY. Decide whether the jump has ended, apply the
	//! landing consequences, and publish.
	//!
	//! Deliberately geometry-free. The old fallback probe compared the origin
	//! against GetSurfaceY, which reports TERRAIN height only - land on a roof,
	//! a ship deck, a bridge or a vehicle and it read hundreds of metres AGL,
	//! so the only escape hatch a stuck machine had could never fire. Both
	//! signals below work anywhere on the map:
	//!   1. the engine's own fall state clearing, held briefly so the command
	//!      handover cannot flicker it;
	//!   2. vertical motion stopping for a good while - which also unsticks a
	//!      jumper wedged somewhere the fall command never resolves.
	protected void UpdateAuthorityTouchdown(float timeSlice)
	{
		if (m_bDebugGroundTest || m_fFreeFallTime <= 1.0)
			return;

		// CRATER CHECK: no canopy + ground + real sink = dead now, not in two
		// seconds. Runs ahead of the timers so the impact resolves on the frame
		// it happens; the timer paths below still handle canopy arrivals.
		if (!m_bCanopy && m_iLandDir == 0 && !HAM_Para_IsBoardingVehicle())
		{
			// CORROBORATE BEFORE KILLING. Two independent readings must agree that
			// this is the ground: a real descent rate AND a trustworthy low height.
			// A single bad sample must never be lethal - this check runs every frame
			// of every jump, so a one-frame glitch is a certainty over a session.
			IEntity impactOwner = GetOwner();
			float impactAgl = HAM_Para_AGL_UNKNOWN;
			if (impactOwner)
				impactAgl = GetHeightAGL(impactOwner);
			if (impactOwner && m_fMeasuredDown > HAM_Para_FF_IMPACT_SINK_MS
				&& impactAgl <= HAM_Para_FF_IMPACT_AGL_M
				&& impactAgl > -2.0
				&& m_Controller && !m_Controller.IsFalling())
			{
				Print(string.Format("[HAM_Para_FreeFall] no-canopy ground impact at %1 m/s - fatal", Math.Round(m_fMeasuredDown)), LogLevel.NORMAL);
				if (m_Controller)
					m_Controller.ForceDeath();
				HAM_Para_DoAction(HAM_Para_ACT_END);
				HAM_Para_PublishState();
				return;
			}
		}

		if (m_Controller && !m_Controller.IsFalling())
			m_fGroundedTime += timeSlice;
		else
			m_fGroundedTime = 0;

		// Only a TRUSTED reading may count toward stillness. A rejected sample
		// says nothing about the jumper's speed, and treating it as stillness is
		// what fired the wedged release at altitude.
		if (m_bMeasuredValid && Math.AbsFloat(m_fMeasuredDown) < 0.5)
			m_fStillTime += timeSlice;
		else
			m_fStillTime = 0;

		bool landed = m_fGroundedTime > 0.25;
		bool wedged = m_fStillTime > 2.0 && m_fFreeFallTime > 4.0;
		if (!landed && !wedged)
			return;

		// One landing selection per arrival - but ONLY while the landing clips are
		// actually enabled. With them parked, a stray m_iLandDir (replicated in
		// from another machine, or left over) would silently block every touchdown
		// forever, which is a stuck jumper rather than a missed animation.
		if (m_iLandDir != 0 && HAM_Para_LAND_ANIMS_ENABLED)
			return;   // landing already playing - one selection per arrival

		if (wedged && !landed)
			Print("[HAM_Para_FreeFall] no vertical motion for 2 s - releasing freefall state", LogLevel.WARNING);

		// arriving hot under canopy is a TUMBLE - honesty in the landing is
		// what makes the flare skill matter. And arriving with NO canopy at
		// all is a terminal-velocity impact: same damage model, which at
		// ~50 m/s down is simply fatal. Damage, death and the unconscious
		// ragdoll are authority business by definition.
		// Only a CONFIRMED ground arrival may be graded as an impact. The wedged
		// branch is a lost-the-plot safety release, not a landing - grading it as
		// one turned a harmless state recovery into an unexplained death.
		CheckLandingTumble(landed);

		// LANDING ANIMATIONS PARKED (2026-08-22): the in-place clips + root
		// motion handback did not hold up for the forward variants - ragdoll
		// at speed is back. All wiring below stays dormant behind this gate;
		// flip HAM_Para_LAND_ANIMS_ENABLED to revisit.
		if (HAM_Para_LAND_ANIMS_ENABLED && m_Controller && !m_Controller.IsDead() && m_bCanopy)
		{
			float fwdDot = 0;
			vector horizV = Vector(m_vLastAirVelVec[0], 0, m_vLastAirVelVec[2]);
			if (horizV.Length() > 0.01)
			{
				float yawRad = m_fFlightYaw * Math.DEG2RAD;
				vector facing = Vector(Math.Sin(yawRad), 0, Math.Cos(yawRad));
				fwdDot = vector.Dot(horizV, facing);
			}

			int landDir = 12;         // Land_03: the flat, slow arrival
			int landMs = 1400;
			if (Math.AbsFloat(m_fTurnRate) > 22.0)
			{
				// arriving out of a sharp turn: knees together, mirrored
				// to match the turn direction
				if (m_fTurnRate > 0)
					landDir = 15;
				else
					landDir = 14;
				landMs = 4500;
			}
			else if (fwdDot < -1.2)
			{
				landDir = 16;         // drifting backward -> fall backward
				landMs = 4500;
			}
			else if (fwdDot > 2.0)
			{
				landDir = 13;         // running it out forward -> Land_02
				landMs = 2400;
			}

			Print(string.Format("[HAM_Para_FreeFall] landing variant %1 (fwdDot=%2 turn=%3)",
				landDir, fwdDot.ToString(-1, 1), m_fTurnRate.ToString(-1, 0)), LogLevel.NORMAL);
			m_iLandDir = landDir;
			m_iRplLand = landDir;
			Replication.BumpMe();
			GetGame().GetCallqueue().Remove(HAM_Para_FinishLanding);
			GetGame().GetCallqueue().CallLater(HAM_Para_FinishLanding, landMs, false);
			return;
		}

		HAM_Para_LocalAction(HAM_Para_ACT_END);
	}

	//------------------------------------------------------------------------------------------------
	//! The landing clip has played out - hand the character back to normal
	//! locomotion everywhere.
	protected void HAM_Para_FinishLanding()
	{
		m_iRplLand = 0;
		Replication.BumpMe();
		m_iLandDir = 0;
		HAM_Para_LocalAction(HAM_Para_ACT_END);
	}

	//------------------------------------------------------------------------------------------------
	protected bool m_bGMSAirStaminaTracked;
	protected float m_fGMSAirStamina;

	// Flight displacement must not charge the character for running.
	// Keep pre-jump fatigue and native recovery; do not force full stamina.
	protected void HAM_Para_UpdateAirborneStamina()
	{
		if (!HAM_Para_IsMotionAuthority() || !m_bFreeFalling || m_iLandDir != 0 || !m_Controller || m_Controller.IsDead() || m_Controller.IsSwimming() || (m_AnimComponent && m_AnimComponent.IsRagdollActive()))
		{
			m_bGMSAirStaminaTracked = false;
			return;
		}
		CharacterStaminaComponent stamina = m_Controller.GetStaminaComponent();
		if (!stamina) return;
		float current = stamina.GetStamina();
		if (m_bGMSAirStaminaTracked && current < m_fGMSAirStamina)
			stamina.AddStamina(m_fGMSAirStamina - current);
		m_fGMSAirStamina = stamina.GetStamina();
		m_bGMSAirStaminaTracked = true;
	}
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		HAM_Para_UpdateAirborneStamina();
		HAM_Para_MeasureImpactVelocity(owner, timeSlice);
		m_fLastFrameEventTime = 0;   // watchdog: the frame event is alive
		if (m_fYieldLatch > 0)
			m_fYieldLatch -= timeSlice;
		// position-derived fall speed (independent of controller velocity).
		// On the server this measures the OWNER's replicated motion, which is
		// what the authority must judge landings by.
		vector pos = owner.GetOrigin();
		if (timeSlice > 0)
		{
			vector measured = (pos - m_vLastPos) * (1.0 / timeSlice);
			if (measured.Length() > HAM_Para_TELEPORT_SPEED_MS)
			{
				// teleport, not motion: the speed is UNKNOWN this frame. Do not claim
				// zero - every consumer treats 0 as "standing still".
				m_bMeasuredValid = false;
				m_fFallCondTime = 0;
			}
			else
			{
				m_vMeasuredVel = measured;
				m_fMeasuredDown = -m_vMeasuredVel[1];
				m_bMeasuredValid = true;
			}
		}
		m_vLastPos = pos;

		// landing handback: the land clips carry real root motion (measured
		// from the exported data) but play IN PLACE - when the variant ends,
		// move the entity to where the body visually finished so normal
		// locomotion resumes in the right spot, facing the right way
		if (m_bLandShiftPending && m_iLandDir == 0)
		{
			m_bLandShiftPending = false;
			if (HAM_Para_IsMotionAuthority())
			{
				vector disp = "0.037 0 0.057";   // Land03 / FallBwd: in place
				float dyaw = 14.7;
				if (m_iLandShiftVariant == 13)
					disp = "0.037 0 -2.643";      // Land02: the run-out
				else if (m_iLandShiftVariant == 14)
					disp = "0.357 0 -1.543";      // knees together, left
				else if (m_iLandShiftVariant == 15)
				{
					disp = "-0.357 0 -1.543";     // mirrored, right
					dyaw = -14.7;
				}
				vector ltm[4];
				owner.GetWorldTransform(ltm);
				vector endPos = ltm[3] + ltm[0] * disp[0] + ltm[2] * disp[2];
				vector ang = Math3D.MatrixToAngles(ltm);
				vector newTM[4];
				Math3D.AnglesToMatrix(Vector(ang[0] + dyaw, ang[1], ang[2]), newTM);
				newTM[3] = endPos;
				// a plain SetWorldTransform on a live character is stomped by
				// the character controller - Teleport is the API that sticks
				ChimeraCharacter landChar = ChimeraCharacter.Cast(owner);
				if (landChar)
					landChar.Teleport(newTM);
				else
					owner.SetWorldTransform(newTM);
			}
		}

		// arrival speeds sampled from the physics velocity on EVERY machine.
		// The landing model used to read m_vFlightVel / m_fVerticalVel, which
		// are local flight-model state - on the server (which does not own
		// character motion) those are not the descent that actually happened,
		// so impact damage was computed from numbers nobody experienced.
		// Since 1.8 a dead or ragdolled character can hand back a non-null but
		// DANGLING Physics pointer, and this sample runs BEFORE the safety
		// disengage below - skip it for any body that disengage is about to
		// reject anyway (same crash class as HAM_Para_GetFallVelocity).
		if (m_bFreeFalling
			&& !(m_Controller && m_Controller.IsDead())
			&& !(m_AnimComponent && m_AnimComponent.IsRagdollActive()))
		{
			Physics trackPhys = owner.GetPhysics();
			if (trackPhys)
			{
				vector trackVel = trackPhys.GetVelocity();
				// SANITY-CAP THE IMPACT SAMPLE. This is the number the landing model
				// grades a jumper on, and it was stored raw: a physics spike wrote
				// -142 m/s (terminal is ~90 head-down) and sat there until a touchdown
				// read it as a lethal arrival. Beyond the teleport threshold it is not
				// a descent, so keep the last believable sample instead.
				if (trackVel[1] < -0.5 && trackVel.Length() < HAM_Para_TELEPORT_SPEED_MS)
				{
					m_fLastAirVertVel = trackVel[1];
					m_fLastAirHorizVel = Vector(trackVel[0], 0, trackVel[2]).Length();
					m_vLastAirVelVec = trackVel;
				}
			}
		}

		// --- DRIFT RECONCILE (non-authority machines) -----------------------
		// The RplProp callback only fires on a CHANGE. If a machine ever ends
		// up disagreeing with the published state - a local safety exit the
		// authority did not follow, an ordering oddity around stream-in - a
		// state that never changes again would leave it wrong forever, which
		// is exactly the failure class this whole rework exists to kill. One
		// comparison a second closes it permanently.
		if (!HAM_Para_IsStateAuthority())
		{
			m_fRplReconcileTime += timeSlice;
			if (m_fRplReconcileTime > 1.0)
			{
				m_fRplReconcileTime = 0;
				bool rplFF = m_iRplMode != HAM_Para_MODE_NONE;
				bool rplCanopy = m_iRplMode == HAM_Para_MODE_CANOPY;
				if (rplFF != m_bFreeFalling || (rplFF && rplCanopy != m_bCanopy))
					m_iRplDriftStreak++;
				else
					m_iRplDriftStreak = 0;

				// TWO consecutive disagreeing samples, not one. The owning
				// client legitimately runs ahead of the authority for a round
				// trip after a predicted deploy; correcting on the first
				// sample would yank the canopy away and re-spawn it a moment
				// later. Real drift never resolves itself.
				if (m_iRplDriftStreak >= 2)
				{
					m_iRplDriftStreak = 0;
					Print(string.Format("[HAM_Para_FreeFall] state drift (local ff=%1 canopy=%2, authority ff=%3 canopy=%4) - resyncing",
						m_bFreeFalling, m_bCanopy, rplFF, rplCanopy), LogLevel.WARNING);
					OnGMSRplState();
				}
			}
		}

		if (!m_bFreeFalling)
		{
			// SAFETY NET (mostly for proxies): a canopy must never outlive the
			// freefall state on any machine, whatever path ended it
			if (m_CanopyEntity || !m_aCanopyParts.IsEmpty())
				DestroyCanopy();
			// ...and neither may the POSE: if this machine ever raised the
			// graph flag, force it down. Anim graph variables are per-machine
			// (they do not replicate), so a proxy that missed the end event
			// would otherwise leave the jumper stuck in the freefall pose.
			if (m_bGraphActive)
			{
				if (m_GMSPresentation) m_GMSPresentation.Release();
				m_bGraphActive = false;
				if (m_VarActive != -1 && m_AnimComponent)
					m_AnimComponent.SetVariableBool(m_VarActive, false);
			}
			m_fGroundedTime = 0;
			TryAutoEngage(owner, timeSlice);
			return;
		}

		m_fFreeFallTime += timeSlice;

		// safety disengage: HAM_Para_FF_Active gates the vanilla fall wildcards in
		// the master graph, so it MUST drop the moment death/vehicle/swim
		// machinery needs the character back. This one stays unanimous rather
		// than authority-only: death and seat occupancy are themselves
		// replicated states, so every machine reaches the same verdict on its
		// own, and the graph has to yield locally the instant it does.
		// A live ragdoll ends it too. Partly because a ragdolling jumper is no
		// longer flying anything, and partly for safety: everything below this
		// point questions the character's movement state, and on a ragdoll
		// those engine reads are exactly what crashed the server in 1.8 (see
		// HAM_Para_GetFallVelocity). Ragdoll can lead death here - unconscious
		// bodies ragdoll while IsDead() still reads false.
		ChimeraCharacter chimChar = ChimeraCharacter.Cast(owner);
		// GetParent() ends it too: being attached mid-flight (grabbing a rope,
		// boarding a shooting seat) means something else is carrying the
		// character now, and two systems writing one transform is the fight
		// that reads as rubber-banding.
		if ((m_Controller && (m_Controller.IsDead() || m_Controller.IsSwimming()))
			|| (m_AnimComponent && m_AnimComponent.IsRagdollActive())
			|| (chimChar && chimChar.IsInVehicle())
			|| (HAM_Para_IsBoardingVehicle() && !FindParachuteBag())
			|| owner.GetParent())
		{
			HAM_Para_LocalAction(HAM_Para_ACT_END);
			return;
		}

		// --- TOUCHDOWN: STATE AUTHORITY ONLY --------------------------------
		// Every machine used to guess at this from its own local simulation.
		// A proxy that guessed "still falling" while the jumper had already
		// landed stayed latched in the pose; a proxy that guessed "landed"
		// early dropped out of a jump still in progress. The authority decides
		// once and publishes; everyone else follows m_iRplMode.
		if (HAM_Para_IsStateAuthority())
		{
			UpdateAuthorityTouchdown(timeSlice);
			if (!m_bFreeFalling)
				return;
		}

		// --- INPUT: MOTION AUTHORITY ONLY -----------------------------------
		// The input context on a third-party observer is not the jumper's
		// input, so re-deriving the flight model there produced a different
		// attitude - and therefore a different animation - from the one the
		// jumper was actually flying.
		bool motionAuth = HAM_Para_IsMotionAuthority();
		float inX = 0.0;
		float inY = 0.0;
		if (motionAuth)
		{
			CharacterInputContext inputCtx = null;
			if (m_Controller)
				inputCtx = m_Controller.GetInputContext();
			if (inputCtx)
			{
				float moveSpeed;
				vector moveDir;
				inputCtx.GetMovement(moveSpeed, moveDir);
				float scale = Math.Clamp(moveSpeed, 0.0, 1.0);
				inX = moveDir[0] * scale;
				inY = moveDir[2] * scale;
			}
		}

		if (m_bCanopy)
		{
			UpdateCanopy(owner, timeSlice, inX, inY);
			return;
		}

		if (HAM_Para_UpdateSpecializedFreeFall(owner, timeSlice, inX, inY))
			return;

		// --- OBSERVER PLAYBACK ------------------------------------------
		// Everything below this point is the flight model: it reads input
		// nobody but the owner has and it writes the transform. Running it
		// on a proxy made every observer simulate its own private jump and
		// fight network interpolation for the character's position. An
		// observer's whole job is to hold the pose the authority published.
		if (!motionAuth)
		{
			ApplyGraphState();
			HAM_Para_SmoothProxyMotion(owner, timeSlice);
			return;
		}

		// --- flight-logic glide, stepper style on the character --------
		// local-player extras: roll input, manual deploy, stabilize toggle
		InputManager inputMgr = null;
		float inRoll = 0.0;
		if (HAM_Para_IsLocalPlayer())
			inputMgr = GetGame().GetInputManager();
		if (inputMgr)
		{
			inRoll = inputMgr.GetActionValue("CharacterLeanRight") - inputMgr.GetActionValue("CharacterLeanLeft");

			// Space pulls the ripcord (rising edge, past the engage grace;
			// keys already held at engage must release first)
			bool jumpNow = inputMgr.GetActionValue("CharacterJump") > 0.5;
			if (jumpNow && !m_bJumpKeyHeld && m_fFreeFallTime > 1.0)
			{
				m_bJumpKeyHeld = jumpNow;
				HAM_Para_LocalAction(HAM_Para_ACT_DEPLOY);
				return;
			}
			m_bJumpKeyHeld = jumpNow;

			// X (stance key, meaningless airborne) toggles auto-level
			bool stabNow = inputMgr.GetActionValue("CharacterStandCrouchToggle") > 0.5;
			if (stabNow && !m_bStabKeyHeld)
				HAM_Para_LocalAction(HAM_Para_ACT_STABILIZE);
			m_bStabKeyHeld = stabNow;
		}
		// any manual rotation input hands control straight back
		if (Math.AbsFloat(inX) > 0.05 || Math.AbsFloat(inY) > 0.05 || Math.AbsFloat(inRoll) > 0.05)
			m_bStabilizing = false;

		// both rotation axes are MOMENTUM sliders: input ACCELERATES the
		// rotation rate, release lets it coast on light ice friction,
		// counter-input brakes then reverses. Wind up slowly into fast
		// flips (pitch) and spins (turn); attitude wraps 360 on both.
		float rotDamp = Math.Clamp(1.0 - m_fRotDampingPerS * timeSlice, 0.0, 1.0);
		if (Math.AbsFloat(inY) > 0.05)
			m_fPitchRate = Math.Clamp(m_fPitchRate - inY * m_fPitchAccelDps2 * timeSlice, -m_fPitchMaxRateDps, m_fPitchMaxRateDps);
		else
			m_fPitchRate = m_fPitchRate * rotDamp;
		m_fPitchTrim += m_fPitchRate * timeSlice;
		if (m_fPitchTrim > 180.0)
			m_fPitchTrim -= 360.0;
		else if (m_fPitchTrim < -180.0)
			m_fPitchTrim += 360.0;

		// capture yaw ONCE at engage (per-frame re-extraction from a
		// pitched/rolled matrix is what caused the death spin)
		if (m_fFlightYaw == -999.0)
		{
			vector cm[4];
			owner.GetTransform(cm);
			vector f0 = cm[2];
			f0[1] = 0;
			if (f0.Length() > 0.01)
			{
				f0.Normalize();
				m_fFlightYaw = Math.Atan2(f0[0], f0[2]) * Math.RAD2DEG;
			}
			else
			{
				m_fFlightYaw = 0;
			}
		}

		// turn momentum: same curve on the heading axis. The live RATE
		// (never the input, never the wrapped angle) drives the L/R
		// animations, so a coasting spin RETAINS its animation state
		// until the momentum bleeds off or is countered.
		if (Math.AbsFloat(inX) > 0.05)
			m_fTurnRate = Math.Clamp(m_fTurnRate + inX * m_fTurnAccelDps2 * timeSlice, -m_fTurnMaxRateDps, m_fTurnMaxRateDps);
		else
			m_fTurnRate = m_fTurnRate * rotDamp;
		m_fFlightYaw += m_fTurnRate * timeSlice;
		if (m_fFlightYaw > 180.0)
			m_fFlightYaw -= 360.0;
		else if (m_fFlightYaw < -180.0)
			m_fFlightYaw += 360.0;
		float turnFrac = Math.Clamp(m_fTurnRate / TURN_ANIM_REF_DPS, -1.0, 1.0);
		// bank overlay leans into the live turn rate (visual only)
		m_fBankVisDeg = turnFrac * 25.0;

		// roll momentum on Q/E (the lean actions, repurposed while
		// airborne): same slider - wind up, coast on the ice, counter
		// to stop; barrel rolls wrap 360. Local player input only.
		if (Math.AbsFloat(inRoll) > 0.05)
			m_fRollRate = Math.Clamp(m_fRollRate + inRoll * m_fRollAccelDps2 * timeSlice, -m_fRollMaxRateDps, m_fRollMaxRateDps);
		else
			m_fRollRate = m_fRollRate * rotDamp;
		m_fRollTrim += m_fRollRate * timeSlice;
		if (m_fRollTrim > 180.0)
			m_fRollTrim -= 360.0;
		else if (m_fRollTrim < -180.0)
			m_fRollTrim += 360.0;

		// auto-stabilize: corrective rates chase level flight with the
		// same icy momentum - existing spin bleeds off first, then the
		// attitude swings home; completes (or is overridden) silently
		if (m_bStabilizing)
		{
			float kStab = Math.Clamp(timeSlice * 2.5, 0.0, 1.0);
			m_fPitchRate = Math.Lerp(m_fPitchRate, Math.Clamp(-m_fPitchTrim * 1.6, -100.0, 100.0), kStab);
			m_fRollRate = Math.Lerp(m_fRollRate, Math.Clamp(-m_fRollTrim * 1.6, -100.0, 100.0), kStab);
			m_fTurnRate = Math.Lerp(m_fTurnRate, 0.0, kStab);
			if (Math.AbsFloat(m_fPitchTrim) < 3.0 && Math.AbsFloat(m_fRollTrim) < 3.0
				&& Math.AbsFloat(m_fPitchRate) < 5.0 && Math.AbsFloat(m_fRollRate) < 5.0)
				m_bStabilizing = false;
		}

		// glide target from attitude in the CURRENT heading frame:
		// turning rotates the heading while held pitch keeps tracking,
		// so the pitch + turn sliders carve the drift direction together
		float yawRad = m_fFlightYaw * Math.DEG2RAD;
		vector cfwd = Vector(Math.Sin(yawRad), 0, Math.Cos(yawRad));
		float fwdT = 28.0 * Math.Sin(-m_fPitchTrim * Math.DEG2RAD);
		vector vTarget = cfwd * fwdT;

		// ice-skating momentum
		float vx = Math.SmoothCD(m_vFlightVel[0], vTarget[0], m_fVelXV, 1.6, 60.0, timeSlice);
		float vz = Math.SmoothCD(m_vFlightVel[2], vTarget[2], m_fVelZV, 1.6, 60.0, timeSlice);
		m_vFlightVel = Vector(vx, 0, vz);

		// fixed-wing motion authority on the character: full transform
		// (rotation = our attitude) + physics velocity so the engine
		// interpolates smoothly instead of ghost-stepping SetOrigin
		Physics phys = owner.GetPhysics();
		float vertical = 0;
		if (phys)
			vertical = phys.GetVelocity()[1];
		// (the last-airborne descent rate used by the landing model is
		// sampled at the top of EOnFrame on every machine now - writing it
		// here as well could stamp a POSITIVE vertical from a zoom climb
		// over the fall speed the impact is supposed to be judged on)

		vector rotM[3];
		float visPitch = 0;
		float visRoll = 0;
		if (m_bApplyRotation)
		{
			visPitch = m_fPitchTrim;
			visRoll = m_fRollTrim + m_fBankVisDeg;
		}
		if (!m_bDebugNoMotion)
		{
			// motion authority (skipped entirely in the isolation test;
			// compass/command/heartbeat below always run)
			Math3D.AnglesToMatrix(Vector(m_fFlightYaw, visPitch, visRoll), rotM);
			vector tm2[4];
			tm2[0] = rotM[0];
			tm2[1] = rotM[1];
			tm2[2] = rotM[2];
			tm2[3] = owner.GetOrigin() + m_vFlightVel * timeSlice;
			owner.SetWorldTransform(tm2);
			if (phys)
			{
				phys.SetVelocity(Vector(vx, vertical, vz));
				phys.SetAngularVelocity(vector.Zero);
			}
		}

		// animation compass: F/B from the sin-mapped pitch (wrapped
		// angles still pick the sensible loop inverted), L/R from the
		// smoothed turn INPUT - axis movement, never the heading angle.
		// pitch anim regime: while the pitch axis is ROTATING, the F/B
		// animation follows the ROTATION DIRECTION and stays latched
		// through the whole flip (a backflip keeps the Back anim past
		// inverted) - it only changes when the flip reverses. Attitude
		// mapping applies once the axis is at rest (held tracking).
		float absPitchRate = Math.AbsFloat(m_fPitchRate);
		if (m_bPitchRateRegime)
		{
			if (absPitchRate < PITCH_REGIME_EXIT_DPS)
				m_bPitchRateRegime = false;
		}
		else
		{
			if (absPitchRate > PITCH_REGIME_ENTER_DPS)
				m_bPitchRateRegime = true;
		}
		float fwdAmt;
		if (Math.AbsFloat(inY) > 0.25)
		{
			// INPUT LATCH: while a pitch button is HELD, the F/B animation
			// for that direction stays committed - attitude and rotation
			// mapping only take over once the player lets go. Forward
			// stick (inY > 0) pitches nose-down = the Forward/fast family;
			// the physics applies -inY with the same convention.
			if (inY > 0)
				fwdAmt = 1.0;
			else
				fwdAmt = -1.0;
		}
		else if (m_bPitchRateRegime)
			fwdAmt = Math.Clamp(-m_fPitchRate / PITCH_ANIM_REF_DPS, -1.0, 1.0);
		else
		{
			// HANDS OFF: the entity transform carries the flight pitch
			// (the writer applies m_fPitchTrim to the root), so the
			// FORWARD axis' vertical dip IS sin(pitch-off-horizon) -
			// feedback-free (the clips pose bones, never the entity) and
			// wrap-honest. Level with the horizon (either face) reads 0 =
			// slow; vertical either way reads +/-1 = the fast families
			// (head-down = F "fast down", head-up = B "fast up").
			vector bodyTM[4];
			owner.GetWorldTransform(bodyTM);
			fwdAmt = -bodyTM[2][1];
		}
		// analog slots (P2 parametric belly-fly reads these; harmless now)
		m_fAnimDirX = turnFrac;
		m_fAnimDirY = fwdAmt;
		HAM_Para_PublishAnalog();

		int newDir = HAM_Para_FreeFallCompass.QuantizeDirPitchTurn(fwdAmt, turnFrac, m_iDir,
			Math.Sin(m_fFastPitchEnterDeg * Math.DEG2RAD), Math.Sin(m_fFastPitchExitDeg * Math.DEG2RAD));
		if (newDir != m_iDir)
		{
			m_iDir = newDir;
			// the compass is derived from input only the motion authority
			// has, so it cannot be re-derived on observers - push it to
			// the authority, which folds it into the replicated state.
			// Change-driven and already hysteresis-damped, so this is a
			// handful of ints per jump, not a per-frame stream.
			HAM_Para_PublishDir();
		}
		// per-frame command assertion while freefalling: FallMaster is
		// evaluated on BOTH stance paths (low/raised) - holding the
		// command keeps every instance of it in our state; safe here
		// because these are state-to-state transitions (no any-state
		// re-entry edge exists in FallMaster). The command itself is sent
		// from OnApplyControlsHook (input phase) - see
		// HAM_Para_FreeFallControllerHook.c for the timing rationale; the lazy
		// rebind of its handle now lives in ApplyGraphState.
		ApplyGraphState();

		UpdateSpeedFX(timeSlice);

		m_fDebugTimer += timeSlice;
		if (m_fDebugTimer > 2.0)
		{
			m_fDebugTimer = 0;
			bool inBranch = false;
			bool tFall = false;
			bool tFalling = false;
			bool tLanding = false;
			bool ctrlFalling = false;
			if (m_AnimComponent)
			{
				if (m_TagFF != -1)
					inBranch = m_AnimComponent.IsPrimaryTag(m_TagFF);
				tFall = m_AnimComponent.IsPrimaryTag(m_TagFall);
				tFalling = m_AnimComponent.IsPrimaryTag(m_TagFalling);
				tLanding = m_AnimComponent.IsPrimaryTag(m_TagLanding);
			}
			if (m_Controller)
				ctrlFalling = m_Controller.IsFalling();
			Print(string.Format("[HAM_Para_FreeFall] glide: dir=%1 pitch=%2 pitchRate=%3 turnRate=%4 rollRate=%5 ctrlFalling=%6 IN-BRANCH=%7",
				m_iDir, Math.Round(m_fPitchTrim), Math.Round(m_fPitchRate), Math.Round(m_fTurnRate), Math.Round(m_fRollRate), ctrlFalling, inBranch), LogLevel.NORMAL);
		}

		// AAD is a safety device, and a safety device with two opinions is a
		// double deployment: only the authority may fire it, and it publishes
		// the canopy like any other state change.
		if (m_fAutoDeployHeight > 0 && HAM_Para_IsStateAuthority() && !m_bCanopy)
		{
			if (GetHeightAGL(owner) <= m_fAutoDeployHeight)
				DeployParachute();
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Re-assert the flight attitude AFTER the character controller has run
	//! (it re-uprights the entity during the frame; post-frame wins visually)
	[Attribute(defvalue: "1", desc: "Apply entity attitude rotation during freefall (disable to isolate anim-only)")]
	protected bool m_bApplyRotation;

	[Attribute(defvalue: "0", desc: "Suppress ALL our motion authority - pure native fall, graph/input still live (isolation test)")]
	protected bool m_bDebugNoMotion;

	//------------------------------------------------------------------------------------------------
	//! ALWAYS-UP: the canopy assembly rides Spine5 for POSITION only - its
	//! orientation is re-asserted world-upright every frame (canopy top to
	//! the sky), yawed to the character's heading plus the tuned yaw. Same
	//! constraint the Blender demo used. Runs on every machine - the parts
	//! are local spawns.
	protected void HAM_Para_UprightCanopyParts(IEntity owner)
	{
		if (m_aCanopyParts.IsEmpty())
			return;

		// follow the CHARACTER ROOT attitude (flight-model yaw/pendulum/
		// bank), NOT the spine bone: the steer arm poses were authored
		// against a fixed body<->canopy relationship, so the assembly must
		// stay rigid with the root while the spine's ANIMATION (lean, sway)
		// plays beneath it - that keeps the hands bound to the toggles.
		vector charTM[4];
		owner.GetWorldTransform(charTM);
		vector yawFlip[4];
		Math3D.AnglesToMatrix(Vector(m_fCanopyYawDeg + m_fTuneYaw, 0, 0), yawFlip);
		vector rot[4];
		Math3D.MatrixMultiply4(charTM, yawFlip, rot);
		foreach (IEntity part : m_aCanopyParts)
		{
			if (!part)
				continue;
			vector tm[4];
			part.GetWorldTransform(tm);
			rot[3] = tm[3];
			part.SetWorldTransform(rot);
		}
	}

	//! Optional equipment-specific flight model, after shared safety/landing checks.
	protected bool HAM_Para_UpdateSpecializedFreeFall(IEntity owner, float dt, float inX, float inY)
	{
		return false;
	}

	override void EOnPostFrame(IEntity owner, float timeSlice)
	{
		if (!m_bApplyRotation || !m_bFreeFalling || m_bDebugNoMotion || owner.GetParent()) return;
		vector angles;
		if (HAM_Para_IsMotionAuthority())
		{
			if (m_fFlightYaw == -999.0) return;
			angles = Vector(m_fFlightYaw + m_fYawVisOffset, m_fPitchTrim, m_fRollTrim + m_fBankVisDeg);
			HAM_Para_SendFlightSample(angles, timeSlice);
		}
		else
		{
			if (!m_bRplFlightSample) return;
			if (!m_bProxyAttitudeInit)
			{
				m_vProxyAttitude = m_vRplFlightAngles;
				m_bProxyAttitudeInit = true;
			}
			float blend = Math.Clamp(timeSlice * 15, 0, 1);
			for (int axis = 0; axis < 3; axis++)
			{
				float delta = Math.Repeat(m_vRplFlightAngles[axis] - m_vProxyAttitude[axis] + 180, 360) - 180;
				m_vProxyAttitude[axis] = m_vProxyAttitude[axis] + delta * blend;
			}
			angles = m_vProxyAttitude;
			m_fFlightYaw = angles[0];
		}
		// Correct attitude without explicitly resubmitting interpolated position.
		owner.SetYawPitchRoll(angles);
		// Child placement follows the final root on owners and observers alike.
		if (m_bCanopy) HAM_Para_PlaceCanopyParts(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! TRUE only on the machine whose player actually controls THIS character.
	//! CharacterControllerComponent.IsPlayerControlled() is true for EVERY
	//! player's character on EVERY machine - using it to gate FX, audio and
	//! input meant another player's jump roared in your headset, vignetted
	//! your screen, and your Space/X presses were read into their proxy
	//! simulation. On a dedicated server this is false for everyone, which is
	//! correct: the server simulates, it does not see or hear.
	protected bool HAM_Para_IsLocalPlayer()
	{
		IEntity owner = GetOwner();
		return owner && owner == SCR_PlayerController.GetLocalControlledEntity();
	}

	//------------------------------------------------------------------------------------------------
	// --- MP ACTION MIRRORING: Space/X are LOCAL InputManager reads, invisible
	// to the server's parallel simulation of this character. Movement sliders
	// arrive via the replicated input context (so steering matches), but these
	// EDGES must be RPC'd to the server - otherwise the server-side model
	// never deploys/flares, keeps freefalling at terminal, and its
	// authoritative state stomps the client's canopy (hang animation reverts,
	// turns lose the fight).
	protected static const int HAM_Para_ACT_DEPLOY = 1;
	protected static const int HAM_Para_ACT_STABILIZE = 2;
	protected static const int HAM_Para_ACT_FLARE = 3;
	protected static const int HAM_Para_ACT_END = 4;

	//! Authority-side sanity test for a remote END request: the jump is over
	//! if the character is out of the fight (dead / swimming / boarded) or is
	//! no longer airborne by the engine's own reckoning. Anything else is a
	//! client claiming something the server cannot corroborate.
	protected bool HAM_Para_AuthorityAgreesJumpIsOver()
	{
		if (!m_bFreeFalling)
			return true;

		ChimeraCharacter chr = ChimeraCharacter.Cast(GetOwner());
		if (chr && chr.IsInVehicle())
			return true;
		// boarding counts as over ONLY when no compatible rig is worn - that is
		// the Pronto static-line handover. With OUR rig on, a parachute-vehicle
		// seat entry is the OG framework trying to hijack the jump (the exact
		// takeover the Chute Framework Compat exists to block), and agreeing to
		// end here would open the door the compat closed. A compatible-rig
		// jumper who genuinely ends up seated is still covered by the
		// IsInVehicle() check above once the engine actually seats them.
		if (HAM_Para_IsBoardingVehicle() && !FindParachuteBag())
			return true;
		if (m_Controller && (m_Controller.IsDead() || m_Controller.IsSwimming()))
			return true;
		// ragdoll before the IsFalling() read below, same reason as the
		// EOnFrame safety disengage
		if (m_AnimComponent && m_AnimComponent.IsRagdollActive())
			return true;
		if (m_Controller && !m_Controller.IsFalling() && m_fFreeFallTime > 1.0)
			return true;

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_Para_DoAction(int action)
	{
		switch (action)
		{
			case HAM_Para_ACT_DEPLOY:
				if (m_bFreeFalling && !m_bCanopy)
					DeployParachute();
				break;
			case HAM_Para_ACT_STABILIZE:
				m_bStabilizing = true;
				break;
			case HAM_Para_ACT_FLARE:
				if (m_bCanopy)
					m_fFlareTime = CANOPY_FLARE_DUR_S;
				break;
			case HAM_Para_ACT_END:
				// landing/disengage: EVERY machine must drop the canopy, or
				// the wing hangs off the jumper forever on other screens
				if (m_bFreeFalling)
					EndFreeFall();
				DestroyCanopy();
				break;
		}
	}

	//! Run the action locally (instant response) and mirror it: the authority
	//! pushes straight to every proxy, an owning client asks the server (which
	//! re-broadcasts). Proxy machines never originate - they only receive.
	//!
	//! The broadcast is the LOW-LATENCY path, not the source of truth. It only
	//! reaches machines that currently have this proxy streamed in, so every
	//! action it carries is also folded into m_iRplMode by the authority -
	//! that is what catches the observer who was out of scope when it went out.
	protected void HAM_Para_LocalAction(int action)
	{
		HAM_Para_DoAction(action);
		if (HAM_Para_IsStateAuthority())
		{
			HAM_Para_PublishState();
			Rpc(RpcDo_GMSAction, action);
		}
		else if (HAM_Para_IsMotionAuthority())
		{
			Rpc(RpcAsk_GMSAction, action);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Authority-side entry point for client requests. EVERYTHING arriving here
	//! is untrusted: an RPC on this component can be sent by any client that
	//! has this character streamed in, not just the player flying it, and the
	//! handler is not given the sender's identity - so the defence has to be
	//! semantic rather than an ownership test. Out-of-range actions are
	//! dropped, and the one destructive action (END) is only honoured when the
	//! authority independently agrees the jump is over. A spoofed mid-air END
	//! now does nothing instead of dumping the victim out of their canopy.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GMSAction(int action)
	{
		if (!HAM_Para_IsStateAuthority())
			return;
		if (action < HAM_Para_ACT_DEPLOY || action > HAM_Para_ACT_END)
			return;
		if (action == HAM_Para_ACT_END && !HAM_Para_AuthorityAgreesJumpIsOver())
		{
			Print("[HAM_Para_FreeFall] rejected a remote END request the authority does not agree with", LogLevel.WARNING);
			return;
		}

		HAM_Para_DoAction(action);
		HAM_Para_PublishState();
		// mirror to every proxy so OBSERVERS also see the mode switch
		// (deploy is guarded by m_bCanopy, so the sender re-receiving is a no-op)
		Rpc(RpcDo_GMSAction, action);
	}

	//------------------------------------------------------------------------------------------------
	//! Authority-side compass update from the motion authority. Bounds-checked
	//! for the same reason: this feeds an anim graph variable on every machine.
	//------------------------------------------------------------------------------------------------
	//! One parameter drives the character's arms AND the canopy morphs: park
	//! every V2 part's steer clip at the same pull. On-change only - each
	//! SetSteering re-prepares the anim player, so per-frame calls would thrash.
	protected void HAM_Para_ForwardCanopySteering()
	{
		if (m_aCanopyParts.IsEmpty())
			return;
		// the opening must finish untouched; leave the sent-trackers
		// stale so the first post-deploy value always forwards
		if (m_fCanopyTime < CANOPY_STEER_START_S)
			return;
		if (Math.AbsFloat(m_fAnimDirX - m_fSteerSentX) < 0.02
			&& Math.AbsFloat(m_fAnimDirY - m_fSteerSentY) < 0.02)
			return;

		m_fSteerSentX = m_fAnimDirX;
		m_fSteerSentY = m_fAnimDirY;
		foreach (IEntity part : m_aCanopyParts)
		{
			HAM_Para_ParachuteV2AnimComponent v2 = HAM_Para_ParachuteV2AnimComponent.Cast(part.FindComponent(HAM_Para_ParachuteV2AnimComponent));
			if (v2)
				v2.SetSteering(m_fAnimDirX, m_fAnimDirY);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Owner-side analog steering publish: quantized on-change only, so the
	//! wire cost is a couple of floats per steering adjustment, not per frame.
	protected void HAM_Para_PublishAnalog()
	{
		if (!HAM_Para_IsMotionAuthority())
			return;
		if (Math.AbsFloat(m_fAnimDirX - m_fSentDirX) < 0.03
			&& Math.AbsFloat(m_fAnimDirY - m_fSentDirY) < 0.03)
			return;

		m_fSentDirX = m_fAnimDirX;
		m_fSentDirY = m_fAnimDirY;
		if (HAM_Para_IsStateAuthority())
		{
			m_fRplDirX = m_fAnimDirX;
			m_fRplDirY = m_fAnimDirY;
			Replication.BumpMe();
		}
		else
		{
			Rpc(RpcAsk_GMSAnalog, m_fAnimDirX, m_fAnimDirY);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Owner -> everyone: the chute assembly's root-relative transform.
	//! Change-driven (2 cm / 1 deg quanta), so steady flight is silent.
	protected void HAM_Para_PublishChuteRel(vector relPos, vector relAng)
	{
		if (!HAM_Para_IsMotionAuthority())
			return;
		if (m_fChuteSendAge < 0.1) return;
		if (vector.Distance(relPos, m_vSentChutePos) < 0.02
			&& vector.Distance(relAng, m_vSentChuteAng) < 1.0)
			return;

		m_fChuteSendAge = 0;
		m_vSentChutePos = relPos;
		m_vSentChuteAng = relAng;
		if (HAM_Para_IsStateAuthority())
		{
			m_vRplChutePos = relPos;
			m_vRplChuteAng = relAng;
			Replication.BumpMe();
		}
		else
		{
			Rpc(RpcAsk_GMSChuteRel, relPos, relAng);
		}
	}

	//------------------------------------------------------------------------------------------------
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GMSChuteRel(vector relPos, vector relAng)
	{
		if (!HAM_Para_IsStateAuthority() || !m_bCanopy)
			return;

		m_vRplChutePos = relPos;
		m_vRplChuteAng = relAng;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Server-side analog update from the motion authority. Bounds-checked:
	//! feeds an anim graph variable on every machine.
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GMSAnalog(float x, float y)
	{
		if (!HAM_Para_IsStateAuthority() || !m_bFreeFalling)
			return;

		m_fRplDirX = Math.Clamp(x, -1.0, 1.0);
		m_fRplDirY = Math.Clamp(y, -1.0, 1.0);
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGMSRplDirX()
	{
		if (!HAM_Para_IsMotionAuthority())
		{
			m_fAnimDirX = m_fRplDirX;
			HAM_Para_ForwardCanopySteering();
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void OnGMSRplDirY()
	{
		if (!HAM_Para_IsMotionAuthority())
			m_fAnimDirY = m_fRplDirY;
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcAsk_GMSDir(int dir)
	{
		if (!HAM_Para_IsStateAuthority() || !m_bFreeFalling)
			return;

		if (dir < 0 || dir > HAM_Para_DIR_MAX)
			return;

		m_iDir = dir;
		HAM_Para_PublishState();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_GMSAction(int action)
	{
		HAM_Para_DoAction(action);
	}

	//------------------------------------------------------------------------------------------------
	//! True while the flight model owns full 3-axis entity rotation (same gate as
	//! EOnPostFrame) - the modded 1st person camera follows body pitch AND roll then.
	bool HAM_Para_IsBodyCamActive()
	{
		return m_bApplyRotation && m_bFreeFalling && m_fFlightYaw != -999.0;
	}

	bool HAM_Para_HandlesParachuteFallDamage()
	{
		// Toute la chute GMS (freefall + voile) : le moteur ne doit pas mettre K.O. au TP.
		return m_bFreeFalling;
	}

	void HAM_Para_BeginDrop()
	{
		m_bBypassHeightGate = true;
		vector down = "0 -2 0";
		StartFreeFall(down);
		m_bBypassHeightGate = false;
	}

	//------------------------------------------------------------------------------------------------
	//! The attitude the model is asserting THIS frame (deg): <0, pitch, roll+bank>.
	//! The camera must read this instead of the entity: the character controller
	//! re-uprights the entity mid-frame and our EOnPostFrame re-assert lands only
	//! AFTER camera evaluation, so the entity transform reads level from there.
	vector HAM_Para_GetBodyCamAttitude()
	{
		return Vector(0, m_fPitchTrim, m_fRollTrim + m_fBankVisDeg);
	}

	//------------------------------------------------------------------------------------------------
	//! The flight model's yaw (deg) - the camera locks to THIS, never to the
	//! native controller heading (mid-air the falling state steers that with
	//! the mouse, which is not our turn model).
	float HAM_Para_GetBodyCamYaw()
	{
		return m_fFlightYaw + m_fYawVisOffset;
	}

	//------------------------------------------------------------------------------------------------
	//! Air-camera look sensitivity <x, y, 0>, deg per filtered mouse count;
	//! negative components invert - LIVE-tunable attributes, no code changes.
	vector HAM_Para_GetAirLookSens()
	{
		return Vector(m_fAirLookSensX, m_fAirLookSensY, 0);
	}

	//------------------------------------------------------------------------------------------------
	//! The GMS graph states carry no aiming-limit setup, which leaves head-look
	//! PITCH clamped to zero mid-air (yaw stays free) - so the camera could not
	//! look up or down in freefall. Override generous limits while airborne;
	//! EndFreeFall restores the defaults.
	protected void ApplyAirLookLimits()
	{
		if (!m_HeadAiming)
		{
			IEntity hlOwner = GetOwner();
			if (hlOwner)
				m_HeadAiming = CharacterHeadAimingComponent.Cast(hlOwner.FindComponent(CharacterHeadAimingComponent));
		}
		if (m_HeadAiming)
		{
			m_HeadAiming.SetPitchLimitReductionMultiplier(1.0);
			m_HeadAiming.SetLimitAnglesOverride(-89.0, 89.0, -160.0, 160.0);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Backup 5 Hz tick via CallQueue - engages freefall even if EOnFrame
	//! never fires on this component. Position-derived fall speed only.
	protected void BackupTick()
	{
		IEntity owner = GetOwner();
		if (!owner)
		{
			GetGame().GetCallqueue().Remove(BackupTick);
			return;
		}

		// Engage is an authority decision now, so on a client this timer has
		// nothing left to do - stand it down once replication has settled
		// rather than run a 5 Hz callback per character for the whole session.
		// (OnDelete removes it too; this is the live-character case.)
		if (Replication.IsRunning() && !HAM_Para_IsStateAuthority())
		{
			GetGame().GetCallqueue().Remove(BackupTick);
			return;
		}

		if (m_bFreeFalling)
			return;

		vector pos = owner.GetOrigin();
		vector tickMeasured = (pos - m_vLastPos) * 5.0; // 1 / 0.2 s tick
		m_vLastPos = pos;
		if (tickMeasured.Length() > HAM_Para_TELEPORT_SPEED_MS)
		{
			m_bMeasuredValid = false;
			m_fFallCondTime = 0;
		}
		else
		{
			m_vMeasuredVel = tickMeasured;
			m_fMeasuredDown = -m_vMeasuredVel[1];
			m_bMeasuredValid = true;
		}

		// TEMP ground test: engage the whole loiter/graph chain while
		// standing still, isolating it from the mid-air command problem
		if (m_bDebugGroundTest && !m_bTestDone && m_Controller && HAM_Para_IsLocalPlayer())
		{
			m_fTestTimer += 0.2;
			if (m_fTestTimer > 8.0)
			{
				m_bTestDone = true;
				Print("[HAM_Para_FreeFall] GROUND TEST: engaging on the spot", LogLevel.NORMAL);
				m_bBypassHeightGate = true;
				StartFreeFall(vector.Zero);
				m_bBypassHeightGate = false;
			}
		}

		TryAutoEngage(owner, 0.2);
	}

	//------------------------------------------------------------------------------------------------
	//! Called from the modded controller's OnApplyControls (input phase) so
	//! graph commands land BEFORE this frame's animation evaluation.
	void OnApplyControlsHook(float timeSlice)
	{
		// This hook is driven by the character controller, so it survives an
		// event-mask clobber that kills EOnFrame. Use it to notice that and
		// re-arm, otherwise the jumper is stuck with no flight model.
		if (m_bFreeFalling)
		{
			m_fLastFrameEventTime += timeSlice;
			if (m_fLastFrameEventTime > 0.5)
			{
				IEntity wdOwner = GetOwner();
				if (wdOwner)
				{
					SetEventMask(wdOwner, EntityEvent.FRAME | EntityEvent.POSTFRAME);
					wdOwner.SetFlags(EntityFlags.ACTIVE, true);
				}
				if (!m_bFrameWatchdogWarned)
				{
					m_bFrameWatchdogWarned = true;
					Print("[HAM_Para_FreeFall] FRAME WATCHDOG: EOnFrame stalled >0.5s while freefalling - flight model was dead (no steering). Event mask re-armed.", LogLevel.WARNING);
				}
				m_fLastFrameEventTime = 0;
			}
		}
		else
		{
			m_fLastFrameEventTime = 0;
		}

		if (m_bFreeFalling) ApplyGraphState();
		if (m_GMSPresentation) m_GMSPresentation.Update();
	}

	//------------------------------------------------------------------------------------------------
	//! Auto-engage: on foot (not in any vehicle seat or cargo compartment),
	//! alive, and falling toward the ground faster than the threshold.
	//! StartFreeFall itself still gates on m_fMinEntryHeight AGL.
	//!
	//! STATE AUTHORITY ONLY. This used to run on every machine, each deciding
	//! independently whether a jump had begun - so an observer could start a
	//! jump for a proxy nobody else believed was jumping, and (worse) could
	//! RE-start one a frame after being told the jump had ended, because a
	//! proxy's interpolated motion still looked like a fall. That is the loop
	//! that latched the freefall pose on for good.
	protected bool HAM_Para_DeployOnDetectedFall() { return false; }

	protected void TryAutoEngage(IEntity owner, float timeSlice)
	{
		// no animation component means StartFreeFall would refuse anyway, so
		// bail before touching any character system at all - this runs at 5 Hz
		// on EVERY character on the server, AI included
		if (!m_bAutoEngage || !m_Controller || !m_AnimComponent)
			return;

		// a handover just released this jumper to another parachute mod - stay
		// out of the sky until its deploy has had time to seat them
		if (m_fAutoEngageCooldown > 0)
		{
			m_fAutoEngageCooldown -= timeSlice;
			return;
		}

		if (!HAM_Para_IsStateAuthority())
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(owner);
		if (!character)
			return;

		// Heartbeat only while airborne under GMS (avoids log spam on ground).
		// Reads only the position-derived velocity - never the physics body.
		if (m_bFreeFalling && HAM_Para_IsLocalPlayer())
		{
			m_fDebugTimer += timeSlice;
			if (m_fDebugTimer > 3.0)
			{
				m_fDebugTimer = 0;
				Print(string.Format("[HAM_Para_FreeFall] hb: measuredVel=%1 measuredDown=%2 inVeh=%3 dead=%4 agl=%5",
					m_vMeasuredVel, m_fMeasuredDown, character.IsInVehicle(), m_Controller.IsDead(), GetHeightAGL(owner)), LogLevel.NORMAL);
			}
		}
		else
		{
			m_fDebugTimer = 0;
		}

		// --- DISQUALIFIERS ---------------------------------------------------
		// A corpse is not a jumper, and asking one about its motion is what
		// crashed the server (see HAM_Para_GetFallVelocity). Death is now tested
		// three ways - the damage manager, the controller, and an active
		// ragdoll - because a character can be in any one of those states
		// while the other two still read "alive".
		//
		// EVERY disqualifier runs BEFORE the first physics read: since 1.8 a
		// corpse's Physics is a raw pointer that can dangle while non-null, so
		// HAM_Para_GetFallVelocity on a body that fails any check below is the
		// access-violation crash (illegal read at 0x70), not just wasted work.
		if (character.IsInVehicle())
			return;

		// mid seat-entry (another mod pulling the jumper into its canopy
		// vehicle) is not a fall, and re-engaging here restarts the fight
		if (HAM_Para_IsBoardingVehicle())
			return;

		SCR_DamageManagerComponent dmg = character.GetDamageManager();
		if (dmg && dmg.IsDestroyed())
			return;

		if (m_Controller.IsDead())
			return;

		if (m_AnimComponent.IsRagdollActive())
			return;

		// ATTACHED IS NOT FALLING. A character parented to something is being
		// carried by it, however airborne the physics look. SHS Drive-By is the
		// case that found this: a shooting-seat rider is released from the
		// vehicle COMPARTMENT (so IsInVehicle() reads false) and parented to the
		// seat instead, hanging with no ground under the capsule - SHS suppress
		// the engine fall command for exactly that reason. Pressed U in a flying
		// helicopter and every gate of ours passed: not in a vehicle, well above
		// the entry height, physics falling. Straight into the skydive.
		// Deliberately generic rather than an SHS-specific check - rappel, FRIES
		// and rope mods all park characters the same way, and we own no
		// dependency on any of them. Our canopy parts are children OF the
		// character, never the reverse, so this never sees our own rig.
		if (owner.GetParent())
			return;

		// LADDERS ARE NOT FREEFALL: sliding down one is fast enough to trip
		// the fall-speed gate, and the auto-engage was yanking climbers into
		// the skydive pose mid-slide. The ladder command is non-null only
		// while it is the ACTIVE humanoid command; IsClimbing covers the
		// vault/climb transitions either side of it.
		CharacterCommandHandlerComponent cmdHandler = m_AnimComponent.GetCommandHandler();
		if (cmdHandler && cmdHandler.GetCommandLadder())
			return;
		if (m_Controller.IsClimbing())
			return;

		// STANDING IS NOT FALLING. Feet on anything - including the floor of a
		// frozen C-130 at 4000 m during the red-light phase - means no jump,
		// whatever the altitude and whatever a teleport just did to the
		// position-derived numbers. A real exit reads IsFalling within a frame
		// of leaving the ramp.
		if (!m_Controller.IsFalling())
		{
			m_fFallCondTime = 0;
			return;
		}

		// Static-line begins its authored exit as soon as the guarded fall state is detected.
		// StartFreeFall retains the normal altitude and state-authority gates.
		if (HAM_Para_DeployOnDetectedFall())
		{
			StartFreeFall(HAM_Para_GetFallVelocity(owner));
			return;
		}

		vector vel = HAM_Para_GetFallVelocity(owner);

		float fallSpeed = Math.Max(-vel[1], m_fMeasuredDown);
		if (fallSpeed < m_fAutoEngageFallSpeed)
		{
			m_fFallCondTime = 0;
			return;
		}

		// the condition must HOLD, not trip once: a teleport or physics blip
		// gives one hot sample and then quiet, and the quiet sample resets
		// this. A real jumper only accelerates, so the wait costs a fraction
		// of a second at the very top of the fall.
		m_fFallCondTime += timeSlice;
		if (m_fFallCondTime < HAM_Para_ENGAGE_SUSTAIN_S)
			return;

		StartFreeFall(vel);
	}

	//------------------------------------------------------------------------------------------------
	//! Fall velocity WITHOUT CharacterControllerComponent.GetVelocity().
	//!
	//! THIS IS THE 1.8 SERVER CRASH FIX. That call reaches through the
	//! controller into the character's anim-phys movement agent, and since 1.8
	//! the agent can be gone while the controller itself is still a perfectly
	//! valid object - a corpse/ragdoll being the everyday case on a server full
	//! of AI. The engine then dereferences null (access violation reading 0x70)
	//! and takes the process down, inside TryAutoEngage, five times a second
	//! per body. Script cannot null-check an engine internal, so the call is
	//! removed rather than guarded.
	//!
	//! Both replacements cannot fault: the physics body is the same velocity
	//! EOnFrame already judges landing impacts from, and m_vMeasuredVel is a
	//! plain GetOrigin difference.
	//!
	//! The measured vector is not just a null-guard, it is the answer whenever
	//! the body reports nothing - character motion is driven by the animation
	//! system rather than the solver, so a jumper's physics velocity can sit at
	//! zero while they are plainly falling. Losing it would cost the exit
	//! momentum StartFreeFall seeds the glide with, not just the trigger.
	protected vector HAM_Para_GetFallVelocity(IEntity owner)
	{
		vector vel = vector.Zero;
		Physics phys = owner.GetPhysics();
		if (phys)
			vel = phys.GetVelocity();

		if (vel.Length() < 1.0)
			return m_vMeasuredVel;

		// otherwise keep the two in agreement on the axis that decides entry:
		// whichever descent is faster is the one to believe
		if (-vel[1] < m_fMeasuredDown)
			vel = Vector(vel[0], -m_fMeasuredDown, vel[2]);

		return vel;
	}

	//------------------------------------------------------------------------------------------------
	//! Height above the TERRAIN, not above whatever is underneath. GetSurfaceY
	//! ignores buildings, ships, bridges and vehicles, so this over-reports on
	//! top of any of them.
	//!
	//! That is acceptable for the two remaining callers - the entry height gate
	//! and the auto-deploy altitude are both meant to be terrain-relative - but
	//! it is NOT a ground-contact test, and it used to be used as one. The
	//! touchdown check now runs on the authority off the character's own fall
	//! state instead (see UpdateAuthorityTouchdown), because a jumper who
	//! landed on a roof read hundreds of metres AGL here and could never
	//! satisfy a ground probe built on this.
	protected float GetHeightAGL(IEntity owner)
	{
		// FAIL SAFE, NOT FAIL LETHAL. This used to return 0 when the world
		// lookup failed - i.e. "on the ground" - and the no-canopy impact test
		// reads anything under 2 m as a crater. One failed lookup at altitude
		// therefore force-killed a jumper in mid-air. Unknown height must mean
		// HIGH, so every ground test fails closed instead of firing.
		vector pos = owner.GetOrigin();
		BaseWorld world = owner.GetWorld();
		if (!world)
			return HAM_Para_AGL_UNKNOWN;
		float surfaceY = world.GetSurfaceY(pos[0], pos[2]);
		float agl = pos[1] - surfaceY;
		// a negative AGL means the sample is nonsense (bad surface read), not
		// that the jumper is underground - treat it as unknown
		if (agl < -2.0)
			return HAM_Para_AGL_UNKNOWN;
		return agl;
	}
}
