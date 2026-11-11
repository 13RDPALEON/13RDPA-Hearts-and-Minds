class HAM_Para_ParachuteV2AnimComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Animation driver for ONE model of the multi-model parachute (V2).
//!
//! The V2 parachute is FOUR entities - lines, canopy, drogue, d-bag - each
//! with its own skeleton (the engine skeleton ceiling is per-skeleton:
//! measured, 369 bones imported and 429 crashed, and the assembly needs
//! ~550). All four ship frame-locked clips on a shared timeline, so sync is
//! nothing more than starting every entity's clip at the same moment:
//! attach all four at the SAME slot with the SAME offset (they were baked
//! into one shared asset space) and call PlayDeploy() on the same frame.
//!
//! Timeline events (30 fps authoring, deploy_demo = 155 f = 5167 ms):
//!   2933 ms  line stretch - the bag strips: d-bag entity HIDES
//!            (m_iHideAtMs=2933) and the canopy entity SHOWS
//!            (m_iShowAtMs=2933, spawns hidden)
//!   5167 ms  deploy done - chains into the neutral steering hold
//!
//! Steering is ANALOG: the steer clips are 31 frames == pull 0..1, sampled
//! by PARAMETER, not by time (SetSteering parks the clip at pull * length).
//! Frame 1 of any steer clip is the neutral canopy-flight pose with both
//! toggles held - that frame doubles as the idle hold, so V2 needs no
//! separate idle clip.
class HAM_Para_ParachuteV2AnimComponent : ScriptComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Deploy animation for THIS model", params: "anm")]
	protected ResourceName m_DeployAnim;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Steer RIGHT clip (31 f == pull 0..1)", params: "anm")]
	protected ResourceName m_SteerRAnim;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Steer LEFT clip", params: "anm")]
	protected ResourceName m_SteerLAnim;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Flare clip (both toggles)", params: "anm")]
	protected ResourceName m_SteerFAnim;

	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Bind pose mesh - MUST be this model's own .xob", params: "xob")]
	protected ResourceName m_BindPoseMesh;

	[Attribute(defvalue: "5167", desc: "Deploy clip length in ms (155 f @ 30 fps)")]
	protected int m_fDeployLengthMs;

	[Attribute(defvalue: "1033", desc: "Steer clip length in ms (31 f @ 30 fps)")]
	protected int m_fSteerLengthMs;

	[Attribute(defvalue: "1", desc: "Play the deploy automatically on spawn")]
	protected bool m_bAutoPlay;

	[Attribute(defvalue: "0", desc: "LOOP the deploy for inspection: replays the whole sequence including the bag/canopy swap, instead of settling into the neutral hold")]
	protected bool m_bLoopDeploy;

	[Attribute(defvalue: "1", desc: "Call SetCustomBindPose before playing")]
	protected bool m_bSetBindPose;

	[Attribute(defvalue: "-1", desc: "Hide this entity N ms into the deploy (-1 = never). D-bag: 2933 = line stretch")]
	protected int m_iHideAtMs;

	[Attribute(defvalue: "-1", desc: "Spawn hidden, show N ms into the deploy (-1 = never). Canopy: 2933")]
	protected int m_iShowAtMs;

	protected AnimationPlayerComponent m_AnimPlayer;
	protected bool m_bHasDeployed;
	protected bool m_bDeployFinished;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		m_AnimPlayer = AnimationPlayerComponent.Cast(owner.FindComponent(AnimationPlayerComponent));
		if (!m_AnimPlayer)
		{
			Print("HAM_Para_ParachuteV2AnimComponent: owner has no AnimationPlayerComponent", LogLevel.WARNING);
			return;
		}

		if (m_bSetBindPose && m_BindPoseMesh != ResourceName.Empty)
			m_AnimPlayer.SetCustomBindPose(m_BindPoseMesh);

		// A network-aware spawner may already have sought this assembly to its
		// current phase. A deferred INIT must not restart or hide that playback.
		if (m_bHasDeployed) return;

		if (m_iShowAtMs >= 0)
			owner.ClearFlags(EntityFlags.VISIBLE, false);

		if (!m_bAutoPlay)
			return;

		PlayDeploy();
	}

	//! Seek all parts to the same deployment age on stream-in / late join.
	//! Visibility thresholds and remaining callbacks use that same age.
	void StartAt(float elapsed)
	{
		if (!m_AnimPlayer)
		{
			m_AnimPlayer = AnimationPlayerComponent.Cast(GetOwner().FindComponent(AnimationPlayerComponent));
			if (!m_AnimPlayer) return;
			if (m_bSetBindPose && m_BindPoseMesh != ResourceName.Empty)
				m_AnimPlayer.SetCustomBindPose(m_BindPoseMesh);
		}
		ScriptCallQueue q = GetGame().GetCallqueue();
		q.Remove(PlayDeploy);
		q.Remove(OnDeployEnd);
		q.Remove(HideOwner);
		q.Remove(ShowOwner);
		m_bHasDeployed = true;
		elapsed = Math.Max(elapsed, 0);
		float elapsedMs = elapsed * 1000;
		bool visible = true;
		if (m_iShowAtMs >= 0 && elapsedMs < m_iShowAtMs) visible = false;
		if (m_iHideAtMs >= 0 && elapsedMs >= m_iHideAtMs) visible = false;
		if (visible) GetOwner().SetFlags(EntityFlags.VISIBLE, false);
		else GetOwner().ClearFlags(EntityFlags.VISIBLE, false);
		if (m_iShowAtMs >= 0 && elapsedMs < m_iShowAtMs)
			q.CallLater(ShowOwner, m_iShowAtMs - elapsedMs, false);
		if (m_iHideAtMs >= 0 && elapsedMs < m_iHideAtMs)
			q.CallLater(HideOwner, m_iHideAtMs - elapsedMs, false);
		m_AnimPlayer.Stop();
		if (elapsedMs >= m_fDeployLengthMs)
		{
			HoldNeutral();
			return;
		}
		m_bDeployFinished = false;
		m_AnimPlayer.Prepare(m_DeployAnim, elapsed, 1, false);
		m_AnimPlayer.Play();
		q.CallLater(OnDeployEnd, m_fDeployLengthMs - elapsedMs, false);
	}

	//------------------------------------------------------------------------------------------------
	//! Start the opening. All four V2 entities call this on the SAME frame
	//! for a synchronized deployment - the clips are frame-locked by build.
	void PlayDeploy()
	{
		if (!m_AnimPlayer)
			return;

		if (m_DeployAnim == ResourceName.Empty)
		{
			Print("HAM_Para_ParachuteV2Anim: entity has NO deploy anim bound (bind pose: " + m_BindPoseMesh + ")", LogLevel.WARNING);
			return;
		}

		if (!m_bHasDeployed)
			Print("HAM_Para_ParachuteV2Anim: playing " + m_DeployAnim, LogLevel.NORMAL);

		m_bHasDeployed = true;
		m_bDeployFinished = false;
		m_AnimPlayer.Stop();
		m_AnimPlayer.Prepare(m_DeployAnim, 0, 1, false);
		m_AnimPlayer.Play();

		ScriptCallQueue q = GetGame().GetCallqueue();
		q.Remove(OnDeployEnd);
		q.CallLater(OnDeployEnd, m_fDeployLengthMs, false);
		if (m_iHideAtMs >= 0)
		{
			q.Remove(HideOwner);
			q.CallLater(HideOwner, m_iHideAtMs, false);
		}
		if (m_iShowAtMs >= 0)
		{
			q.Remove(ShowOwner);
			q.CallLater(ShowOwner, m_iShowAtMs, false);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Deploy finished: settle into flight, or restart for inspection.
	protected void OnDeployEnd()
	{
		if (m_bLoopDeploy)
		{
			// restore the pre-deploy visibility state so the swap replays
			IEntity owner = GetOwner();
			if (owner)
			{
				if (m_iShowAtMs >= 0)
					owner.ClearFlags(EntityFlags.VISIBLE, false);
				if (m_iHideAtMs >= 0)
					owner.SetFlags(EntityFlags.VISIBLE, false);
			}
			PlayDeploy();
			return;
		}
		HoldNeutral();
	}

	//------------------------------------------------------------------------------------------------
	//! Neutral canopy flight = frame 1 of the steer clip, parked (speed 0).
	void HoldNeutral()
	{
		m_bDeployFinished = true;
		if (!m_AnimPlayer)
			return;

		m_AnimPlayer.Prepare(m_SteerRAnim, 0, 0, false);
		m_AnimPlayer.Play();
	}

	//------------------------------------------------------------------------------------------------
	//! Analog steering: axis -1..1 (left..right), flare 0..1. Flare wins
	//! when dominant. Samples the clip BY PARAMETER - parked at
	//! pull * clip length - preserving the authored curved toggle path.
	void SetSteering(float axis, float flare)
	{
		// never interrupt the opening: a steering park would replace the
		// deploy clip mid-bloom (SetSteering re-prepares the player)
		if (!m_AnimPlayer || !m_bDeployFinished)
			return;

		ResourceName clip = m_SteerRAnim;
		float pull = Math.AbsFloat(axis);
		if (flare > pull)
		{
			clip = m_SteerFAnim;
			pull = flare;
		}
		else if (axis < 0)
		{
			clip = m_SteerLAnim;
		}

		pull = Math.Clamp(pull, 0, 1);
		// Prepare's startTime is FLOAT SECONDS, not ms; park just shy of the
		// clip end so a full pull cannot wrap or clamp out
		float timeS = pull * (m_fSteerLengthMs * 0.001) * 0.99;
		m_AnimPlayer.Prepare(clip, timeS, 0, false);
		m_AnimPlayer.Play();
	}

	//------------------------------------------------------------------------------------------------
	protected void HideOwner()
	{
		IEntity owner = GetOwner();
		if (owner)
			owner.ClearFlags(EntityFlags.VISIBLE, false);
		Print("HAM_Para_V2Anim HIDE " + m_BindPoseMesh, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	protected void ShowOwner()
	{
		IEntity owner = GetOwner();
		if (owner)
			owner.SetFlags(EntityFlags.VISIBLE, false);
		Print("HAM_Para_V2Anim SHOW " + m_BindPoseMesh, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	//! Runtime override of the inspection loop (used by the test rig spawner).
	void SetLoop(bool loop)
	{
		m_bLoopDeploy = loop;
	}

	//------------------------------------------------------------------------------------------------
	//! Restage for the real jump: cancel whatever spawn-time autoplay
	//! started, hide the model, and begin the deploy after delayMs. The
	//! staggered starts put each part's authored motion on the right beat
	//! (drogue launch mid-pull, bag exit at the open beat).
	void ScheduleDeploy(int delayMs)
	{
		ScriptCallQueue q = GetGame().GetCallqueue();
		q.Remove(OnDeployEnd);
		q.Remove(HideOwner);
		q.Remove(ShowOwner);
		q.Remove(PlayDeploy);
		if (m_AnimPlayer)
			m_AnimPlayer.Stop();
		m_bHasDeployed = false;
		m_bDeployFinished = false;
		IEntity owner = GetOwner();
		if (owner)
			owner.ClearFlags(EntityFlags.VISIBLE, false);
		Print(string.Format("HAM_Para_V2Anim SCHEDULE +%1ms showAt=%2 %3", delayMs, m_iShowAtMs, m_BindPoseMesh), LogLevel.NORMAL);
		q.CallLater(PlayDeploy, delayMs, false);
	}

	//------------------------------------------------------------------------------------------------
	void StopAnimation()
	{
		if (m_AnimPlayer)
			m_AnimPlayer.Stop();
	}

	//------------------------------------------------------------------------------------------------
	//! Pending CallLaters must not outlive the component (hot landing /
	//! mid-air death inside the opening window).
	override void OnDelete(IEntity owner)
	{
		ScriptCallQueue q = GetGame().GetCallqueue();
		q.Remove(OnDeployEnd);
		q.Remove(HoldNeutral);
		q.Remove(HideOwner);
		q.Remove(ShowOwner);
		q.Remove(PlayDeploy);
		m_AnimPlayer = null;
		super.OnDelete(owner);
	}
}
