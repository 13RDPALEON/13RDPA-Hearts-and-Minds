modded class SCR_VONController
{
	override protected void SetActiveTransmit(notnull SCR_VONEntry entry)
	{
		if (!m_VONComp)
			AssignVONComponent();

		if (!m_VONComp)
			return;

		super.SetActiveTransmit(entry);
	}
}
