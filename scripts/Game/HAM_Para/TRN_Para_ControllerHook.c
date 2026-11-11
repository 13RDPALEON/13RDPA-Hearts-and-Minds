//------------------------------------------------------------------------------------------------
//! Command-timing hook: graph commands must be called INSIDE the character
//! update (input phase), before the anim system drains its command queue.
//! EOnFrame lands after evaluation - commands sent there are never seen.
//! OnApplyControls is the sanctioned injection point (vanilla loiter uses it).
//------------------------------------------------------------------------------------------------
modded class SCR_CharacterControllerComponent
{
	protected HAM_Para_FreeFallComponent m_GMSFreeFall;

	override protected void OnApplyControls(IEntity owner, float timeSlice)
	{
		super.OnApplyControls(owner, timeSlice);

		// Keep searching until it resolves. The old one-shot latch cached the
		// result of the FIRST OnApplyControls - if that ever ran before the
		// freefall component's OnPostInit, the null was cached for the life of
		// the character and the graph command was never asserted on that
		// machine, which reads exactly like "the animation didn't replicate".
		if (!m_GMSFreeFall)
			m_GMSFreeFall = HAM_Para_FreeFallComponent.Cast(owner.FindComponent(HAM_Para_FreeFallComponent));

		if (m_GMSFreeFall)
			m_GMSFreeFall.OnApplyControlsHook(timeSlice);
	}
}
