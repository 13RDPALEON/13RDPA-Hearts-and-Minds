//------------------------------------------------------------------------------------------------
//! HAM_Para_CharacterCamera1stPerson.c - body-locked 1st person camera during GMS freefall/canopy.
//!
//! The stock pipeline is unusable mid-air on three counts: the camera heading
//! follows the native controller heading (which the falling state steers with
//! mouse X - not our turn model), entity pitch/roll read LEVEL at camera time
//! (the controller re-uprights the entity mid-frame; our post-frame re-assert
//! lands after camera evaluation), and mouse Y is discarded by the falling
//! state before the aiming component ever sees it (no look pitch).
//!
//! So while the flight model owns the body, this camera is fully deterministic:
//! world rotation = flightYaw * flightAttitude * selfIntegratedLook, built from
//! the component's model state and raw aim-action deltas, with the handler's
//! entity composition cancelled out (m_fUseHeading = 0, entity inverse baked
//! into the TM). Look lives INSIDE the body frame, so mouse pans are relative
//! to the rolled view. On the ground the vanilla path runs untouched.

modded class CharacterCamera1stPerson
{
	protected HAM_Para_FreeFallComponent m_GMSFreeFall;
	protected bool m_bGMSFFResolved;
	protected float m_fGMSDbgTimer;
	protected float m_fGMSLookLR; // self-integrated freelook yaw, deg
	protected float m_fGMSLookUD; // self-integrated freelook pitch, deg

	//------------------------------------------------------------------------------------------------
	override void OnUpdate(float pDt, out ScriptedCameraItemResult pOutResult)
	{
		HAM_Para_FreeFallComponent ff = GetGMSFreeFall();
		if (!ff || !ff.HAM_Para_IsBodyCamActive())
		{
			m_fGMSLookLR = 0;
			m_fGMSLookUD = 0;
			super.OnUpdate(pDt, pOutResult);
			return;
		}

		// CAMERA = THE HEAD. Full-transform bone binding - the engine's own
		// CharacterCamera1stPersonBoneTransform mechanism: the camera is glued
		// to the head bone's complete world transform, so it inherits EVERY
		// motion of the body natively - the flight model's root rotation AND
		// the animation pose - with no rebuilt rotation stack at all. The
		// m_CameraTM is an offset in the BONE's frame; the mouse look lives
		// there, so panning is always relative to wherever the head points.
		m_fFOV = GetBaseFOV();

		pOutResult.m_vBaseAngles		= "0 0 0";
		pOutResult.m_fUseHeading		= 0.0;
		pOutResult.m_iDirectBoneMode	= EDirectBoneMode.RelativeTransform;
		pOutResult.m_iDirectBone		= GetCameraBoneIndex();

		vector additiveRotation = "0 0 0";
		m_CharacterHeadAimingComponent.GetLookTransformationLS(pOutResult.m_iDirectBone, pOutResult.m_iDirectBoneMode, m_OffsetLS, additiveRotation, pOutResult.m_CameraTM);

		// self-integrated mouse look as the bone-frame offset rotation;
		// axis speed AND direction are live component attributes
		InputManager im = GetGame().GetInputManager();
		if (im)
		{
			vector sens = ff.HAM_Para_GetAirLookSens();
			m_fGMSLookLR = Math.Clamp(m_fGMSLookLR + (im.GetActionValue("CharacterAimRight") - im.GetActionValue("CharacterAimLeft")) * sens[0], CONST_LR_MIN, CONST_LR_MAX);
			m_fGMSLookUD = Math.Clamp(m_fGMSLookUD + (im.GetActionValue("CharacterAimUp") - im.GetActionValue("CharacterAimDown")) * sens[1], CONST_UD_MIN, CONST_UD_MAX);
		}
		// COMPOSE the look inside the frame GetLookTransformationLS returned -
		// its rotation carries the bone-axis correction that turns the raw
		// skeleton axes into an eyes-forward view (overwriting it aims the
		// camera down the spine); the eye-offset translation rides through
		vector lookM[4];
		Math3D.AnglesToMatrix(Vector(m_fGMSLookLR, m_fGMSLookUD, 0), lookM);
		vector baseTM[4];
		Math3D.MatrixCopy(pOutResult.m_CameraTM, baseTM);
		Math3D.MatrixMultiply4(baseTM, lookM, pOutResult.m_CameraTM);

		pOutResult.m_fFOV					= m_fFOV;
		pOutResult.m_fNearPlane				= 0.0125;
		pOutResult.m_bUpdateWhenBlendOut	= true;
		pOutResult.m_fPositionModelSpace	= 0.0;
		pOutResult.m_bAllowInterpolation	= true;
		pOutResult.m_pOwner					= m_OwnerCharacter;
		pOutResult.m_pWSAttachmentReference	= null;

		// keep our speed shake; skip ground-step head bob (meaningless mid-air)
		if (m_CharacterCameraHandler)
			m_CharacterCameraHandler.AddShakeToToTransform(pOutResult.m_CameraTM, pOutResult.m_fFOV);

		m_fGMSDbgTimer += pDt;
		if (m_fGMSDbgTimer > 1.0)
		{
			m_fGMSDbgTimer = 0;
			Print(string.Format("[HAM_Para_FFCam] headcam(bone): lookLR=%1 lookUD=%2 bone=%3", m_fGMSLookLR, m_fGMSLookUD, pOutResult.m_iDirectBone), LogLevel.NORMAL);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected HAM_Para_FreeFallComponent GetGMSFreeFall()
	{
		if (!m_bGMSFFResolved)
		{
			m_bGMSFFResolved = true;
			if (m_OwnerCharacter)
				m_GMSFreeFall = HAM_Para_FreeFallComponent.Cast(m_OwnerCharacter.FindComponent(HAM_Para_FreeFallComponent));
		}
		return m_GMSFreeFall;
	}
}
