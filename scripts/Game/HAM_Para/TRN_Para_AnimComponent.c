class HAM_Para_ParachuteAnimComponentClass : ScriptComponentClass
{
}

//! Plays the parachute deploy animation once, then loops the wind idle.
//! Requires an AnimationPlayerComponent on the same entity.
class HAM_Para_ParachuteAnimComponent : ScriptComponent
{
	[Attribute("{B9F3F48344522718}Assets/Parachutes/GMS_Parachute/gms_parachute_deploy.anm", UIWidgets.ResourceNamePicker, desc: "Deploy (opening) animation", params: "anm")]
	protected ResourceName m_DeployAnim;

	[Attribute("{BAD2BA38EC26EE7A}Assets/Parachutes/GMS_Parachute/gms_parachute_idle.anm", UIWidgets.ResourceNamePicker, desc: "Wind idle loop animation", params: "anm")]
	protected ResourceName m_IdleAnim;

	[Attribute(defvalue: "2583", desc: "Deploy animation length in milliseconds (62 frames @ 24 fps)")]
	protected float m_fDeployLengthMs;

	[Attribute(defvalue: "1", desc: "Play the deploy animation automatically when the entity spawns")]
	protected bool m_bAutoPlay;

	[Attribute(defvalue: "1", desc: "Call SetCustomBindPose before playing (diagnostic toggle)")]
	protected bool m_bSetBindPose;

	[Attribute("{6DAF92DEAEEB97E5}Assets/Parachutes/GMS_Parachute/GMS_Parachute.xob", UIWidgets.ResourceNamePicker, desc: "Mesh used as bind pose for the animation", params: "xob")]
	protected ResourceName m_BindPoseMesh;

	protected AnimationPlayerComponent m_AnimPlayer;
	protected bool m_bHasDeployed;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		SetEventMask(owner, EntityEvent.INIT);
	}

	override void EOnInit(IEntity owner)
	{
		m_AnimPlayer = AnimationPlayerComponent.Cast(owner.FindComponent(AnimationPlayerComponent));
		if (!m_AnimPlayer)
		{
			Print("HAM_Para_ParachuteAnimComponent: owner has no AnimationPlayerComponent", LogLevel.WARNING);
			return;
		}

		if (m_bSetBindPose)
			m_AnimPlayer.SetCustomBindPose(m_BindPoseMesh);

		if (!m_bAutoPlay)
			return;

		// streaming re-inits must not replay the opening - go straight back to the loop
		if (m_bHasDeployed)
			PlayIdle();
		else
			PlayDeploy();
	}

	//! Play the opening sequence, then chain into the idle loop.
	void PlayDeploy()
	{
		if (!m_AnimPlayer)
			return;

		m_bHasDeployed = true;
		m_AnimPlayer.Prepare(m_DeployAnim, 0, 1, false);
		m_AnimPlayer.Play();
		GetGame().GetCallqueue().Remove(PlayIdle);
		GetGame().GetCallqueue().CallLater(PlayIdle, m_fDeployLengthMs, false);
	}

	//! Loop the wind idle.
	void PlayIdle()
	{
		if (!m_AnimPlayer)
			return;

		m_AnimPlayer.Prepare(m_IdleAnim, 0, 1, true);
		m_AnimPlayer.Play();
	}

	void StopAnimation()
	{
		if (m_AnimPlayer)
			m_AnimPlayer.Stop();
	}

	//! The deploy->idle chain is a pending CallLater. A canopy destroyed inside
	//! the ~2.6 s opening window (a hot landing, a mid-air death, the jumper
	//! streaming out) left that call queued against a dead component.
	override void OnDelete(IEntity owner)
	{
		GetGame().GetCallqueue().Remove(PlayIdle);
		m_AnimPlayer = null;
		super.OnDelete(owner);
	}
}
