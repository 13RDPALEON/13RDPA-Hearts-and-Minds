class HAM_Para_ParachuteBagComponentClass : ScriptComponentClass
{
}

//------------------------------------------------------------------------------------------------
//! Marks a wearable/carriable item as a parachute rig.
//!  - gates deployment: the freefall component only opens a chute when the
//!    character has an item carrying this component
//!  - owns WHICH canopy deploys and WHERE it sits: the Canopy Slot is a
//!    PointInfo, so selecting it in the World Editor property grid gives the
//!    native translation/rotation gizmo to drag the slot around
//!  - Workbench: enable Debug Preview on a placed entity to see the canopy
//!    model and a test body riding the slot live while you drag the gizmo
//! The slot is authored relative to THIS RIG ENTITY's origin. At deploy the
//! canopy parents to the worn rig itself, so the frame you tune in the
//! preview is exactly the frame used in the air, and the canopy inherits
//! the bag's own attachment bone through every animation. Leave the
//! PointInfo PivotID empty - bone pivots would resolve against this item's
//! own skeleton.
//------------------------------------------------------------------------------------------------
class HAM_Para_ParachuteBagComponent : ScriptComponent
{
	[Attribute(defvalue: "{9A9DF3317DA0DC5F}Prefabs/Props/GMS_ParachuteCanopy.et", desc: "Canopy prefab this rig deploys", params: "et")]
	protected ResourceName m_CanopyPrefab;

	[Attribute(desc: "Canopy slot in the character frame - select to move/rotate with the viewport gizmo")]
	protected ref PointInfo m_CanopySlot;

	[Attribute(defvalue: "0", desc: "WORKBENCH: preview the canopy + a test body at the slot in the World Editor")]
	protected bool m_bDebugPreview;

	//------------------------------------------------------------------------------------------------
	ResourceName GetCanopyPrefab()
	{
		return m_CanopyPrefab;
	}

	//------------------------------------------------------------------------------------------------
	//! Local matrix of the canopy slot (entity/character frame) - exactly
	//! what the gizmo says, nothing added behind the scenes
	void GetCanopySlotTransform(out vector mat[4])
	{
		if (m_CanopySlot)
		{
			m_CanopySlot.Init(GetOwner());
			m_CanopySlot.GetModelTransform(mat);
			return;
		}
		vector rot[3];
		Math3D.AnglesToMatrix(vector.Zero, rot);
		mat[0] = rot[0];
		mat[1] = rot[1];
		mat[2] = rot[2];
		mat[3] = vector.Zero;
	}

#ifdef WORKBENCH
	// animated body stand-in playing the PARA-HANG loop: the hang animation
	// suspends the body well below the entity origin, so a bind-pose body
	// would make the preview lie about where the canopy sits in the air
	protected static const ResourceName PREVIEW_BODY_PREFAB = "{69EE3A01C4F2AA02}Prefabs/GMS_AirOps/GMS_BodyPreviewMesh.et";
	protected static const ResourceName PREVIEW_BODY_BINDPOSE = "{9F3AF591CD6DFE1A}Assets/Characters/Basebody/Basebody_Male_01.xob";
	protected static const ResourceName PREVIEW_BODY_HANG_ANM = "{9431E44A3C4E1B4B}Assets/Characters/GMS_FreeFall/Anims/gms_ff_para_hang.anm";
	// stand-in with mesh + NATIVE AnimationPlayerComponent: the real canopy
	// prefab's Game-module script driver cannot come along into the edit
	// world, so the preview drives the animation itself (wind idle = the
	// inflated in-flight shape, which is the pose slot tuning must match)
	protected static const ResourceName PREVIEW_CANOPY_PREFAB = "{69EE3A01C4F2AA01}Prefabs/GMS_AirOps/GMS_CanopyPreviewMesh.et";
	protected static const ResourceName PREVIEW_CANOPY_BINDPOSE = "{6DAF92DEAEEB97E5}Assets/Parachutes/GMS_Parachute/GMS_Parachute.xob";
	protected static const ResourceName PREVIEW_CANOPY_IDLE_ANM = "{BAD2BA38EC26EE7A}Assets/Parachutes/GMS_Parachute/gms_parachute_idle.anm";
	protected IEntity m_PreviewCanopy;
	protected IEntity m_PreviewBody;

	//------------------------------------------------------------------------------------------------
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		if (!m_bDebugPreview)
		{
			DestroyPreview();
			return;
		}

		if (!m_PreviewCanopy)
		{
			m_PreviewCanopy = SpawnPreview(owner, PREVIEW_CANOPY_PREFAB);
			if (m_PreviewCanopy)
			{
				// play the wind idle so the preview matches the ANIMATED
				// in-flight pose, not the bind pose (worst case a stalled
				// editor clock still samples the inflated first frame)
				AnimationPlayerComponent animPlayer = AnimationPlayerComponent.Cast(m_PreviewCanopy.FindComponent(AnimationPlayerComponent));
				if (animPlayer)
				{
					animPlayer.SetCustomBindPose(PREVIEW_CANOPY_BINDPOSE);
					animPlayer.Prepare(PREVIEW_CANOPY_IDLE_ANM, 0, 1, true);
					animPlayer.Play();
				}
			}
		}
		if (!m_PreviewBody)
		{
			m_PreviewBody = SpawnPreview(owner, PREVIEW_BODY_PREFAB);
			if (m_PreviewBody)
			{
				AnimationPlayerComponent bodyAnim = AnimationPlayerComponent.Cast(m_PreviewBody.FindComponent(AnimationPlayerComponent));
				if (bodyAnim)
				{
					bodyAnim.SetCustomBindPose(PREVIEW_BODY_BINDPOSE);
					bodyAnim.Prepare(PREVIEW_BODY_HANG_ANM, 0, 1, true);
					bodyAnim.Play();
				}
			}
		}

		vector slotLocal[4];
		GetCanopySlotTransform(slotLocal);
		vector ownerTM[4];
		owner.GetWorldTransform(ownerTM);
		vector slotWorld[4];
		Math3D.MatrixMultiply4(ownerTM, slotLocal, slotWorld);

		// canopy rides the slot gizmo; the test body stands in for the
		// character, which is always at the entity origin
		if (m_PreviewCanopy)
			m_PreviewCanopy.SetWorldTransform(slotWorld);
		if (m_PreviewBody)
			m_PreviewBody.SetWorldTransform(ownerTM);

		// slot marker: origin sphere + forward arrow
		Shape.CreateSphere(COLOR_YELLOW, ShapeFlags.ONCE | ShapeFlags.NOOUTLINE | ShapeFlags.TRANSP, slotWorld[3], 0.06);
		Shape.CreateArrow(slotWorld[3], slotWorld[3] + slotWorld[2] * 0.5, 0.03, COLOR_GREEN, ShapeFlags.ONCE);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity SpawnPreview(IEntity owner, ResourceName prefab)
	{
		if (!prefab)
			return null;
		Resource res = Resource.Load(prefab);
		if (!res)
			return null;
		EntitySpawnParams sp = new EntitySpawnParams();
		sp.TransformMode = ETransformMode.WORLD;
		owner.GetWorldTransform(sp.Transform);
		return GetGame().SpawnEntityPrefab(res, owner.GetWorld(), sp);
	}

	//------------------------------------------------------------------------------------------------
	protected void DestroyPreview()
	{
		if (m_PreviewCanopy)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewCanopy);
			m_PreviewCanopy = null;
		}
		if (m_PreviewBody)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(m_PreviewBody);
			m_PreviewBody = null;
		}
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		DestroyPreview();
		super.OnDelete(owner);
	}
#endif
}
