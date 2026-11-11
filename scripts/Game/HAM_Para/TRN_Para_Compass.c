//------------------------------------------------------------------------------------------------
//! GMS FreeFall - animation compass quantizer.
//!
//! Turns the continuous flight attitude into the discrete state the GMS
//! freefall animation graph consumes.
//!
//! Graph contract (must match the HAM_Para_FreeFall .agr built in the Animation Editor):
//!   command  CMD_HAM_Para_FreeFall   (int param: 1 = enter, 0 = exit)
//!   variable HAM_Para_FF_Active      (bool: gates the GMS branch of the master graph)
//!   variable HAM_Para_FF_Dir         (int: 0 neutral, 1-8 compass CW from Forward
//!                                (F,RF,R,RB,B,LB,L,LF), 9 para-hang)
//!
//! Only the motion authority computes this - it is derived from input nobody
//! else has - and the result reaches other machines through the freefall
//! component's replicated state, never by being re-derived locally.
//!
//! This file was HAM_Para_FreeFallCommand.c, a CharacterCommandScripted subclass
//! carrying a second full flight model (gravity integration, glide, entity
//! rotation) for a scripted-command approach that was superseded by the
//! master-instance Falling override. Nothing ever instantiated it: the
//! component's m_Command member was declared, null-checked and cleared, but
//! never assigned, so every override in it was unreachable. Only the compass
//! quantizer was ever live, and that is all that is left.
//------------------------------------------------------------------------------------------------
class HAM_Para_FreeFallCompass
{
	// pitch/turn compass model: soft deadzones with Schmitt hysteresis
	protected static const float PITCH_ENTER_AMT = 0.30;  // sin scale (~17 deg nose attitude)
	protected static const float PITCH_EXIT_AMT  = 0.17;  // ~10 deg, release back to neutral band
	protected static const float TURN_ENTER_AMT  = 0.35;  // smoothed turn-input fraction
	protected static const float TURN_EXIT_AMT   = 0.20;

	//------------------------------------------------------------------------------------------------
	//! Pitch/turn control-model compass (0 neutral, 1-8 CW from F).
	//! fwd:  sin-mapped nose attitude, + = nose down / tracking forward -
	//!       the sin keeps the F/B pick sensible through flips (360 wrap).
	//! turn: smoothed turn INPUT [-1..1] - deliberately the axis MOVEMENT,
	//!       never the wrapped heading angle, so a held spin can circle
	//!       forever without the animation state ever flipping.
	//! Per-axis Schmitt hysteresis: soft deadzones without edge chatter.
	//! pitchEnter/pitchExit: optional sin-scale overrides for the F/B
	//! regime thresholds (pass sin(angle) - the demo gates at 45/38 deg
	//! instead of the classic 17.5/10)
	static int QuantizeDirPitchTurn(float fwd, float turn, int currentDir,
		float pitchEnter = 0.30, float pitchExit = 0.17)
	{
		// recover the previous per-axis states from the compass dir
		int pitchState = 0;   // -1 back, 0 neutral, 1 forward
		int turnState = 0;    // -1 left, 0 none, 1 right
		if (currentDir == 1)      { pitchState = 1; }
		else if (currentDir == 2) { pitchState = 1;  turnState = 1; }
		else if (currentDir == 3) { turnState = 1; }
		else if (currentDir == 4) { pitchState = -1; turnState = 1; }
		else if (currentDir == 5) { pitchState = -1; }
		else if (currentDir == 6) { pitchState = -1; turnState = -1; }
		else if (currentDir == 7) { turnState = -1; }
		else if (currentDir == 8) { pitchState = 1;  turnState = -1; }

		// pitch axis: exit to the neutral band first, then re-enter
		if (pitchState == 1 && fwd < pitchExit)
			pitchState = 0;
		else if (pitchState == -1 && fwd > -pitchExit)
			pitchState = 0;
		if (pitchState == 0)
		{
			if (fwd > pitchEnter)
				pitchState = 1;
			else if (fwd < -pitchEnter)
				pitchState = -1;
		}

		// turn axis: same Schmitt shape on the input fraction
		if (turnState == 1 && turn < TURN_EXIT_AMT)
			turnState = 0;
		else if (turnState == -1 && turn > -TURN_EXIT_AMT)
			turnState = 0;
		if (turnState == 0)
		{
			if (turn > TURN_ENTER_AMT)
				turnState = 1;
			else if (turn < -TURN_ENTER_AMT)
				turnState = -1;
		}

		// compass lookup
		if (pitchState == 1)
		{
			if (turnState == 1)
				return 2;
			if (turnState == -1)
				return 8;
			return 1;
		}
		if (pitchState == -1)
		{
			if (turnState == 1)
				return 4;
			if (turnState == -1)
				return 6;
			return 5;
		}
		if (turnState == 1)
			return 3;
		if (turnState == -1)
			return 7;
		return 0;
	}
}
