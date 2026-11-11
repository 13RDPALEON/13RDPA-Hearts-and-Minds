//------------------------------------------------------------------------------------------------
// 13RDPA — clic droit GM : dump de la tenue pour l'édition.
//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class HAM_DumpMannequinContextAction : SCR_SelectedEntitiesContextAction
{
	//------------------------------------------------------------------------------------------------
	void HAM_InitInfo(SCR_UIInfo info)
	{
		m_Info = info;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetCharacter(selectedEntity) != null;
	}

	//------------------------------------------------------------------------------------------------
	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return GetCharacter(selectedEntity) != null;
	}

	//------------------------------------------------------------------------------------------------
	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		IEntity owner = GetCharacter(selectedEntity);
		if (!owner)
			return;

		HAM_MannequinDumper.Dump(owner);
	}

	//------------------------------------------------------------------------------------------------
	protected IEntity GetCharacter(SCR_EditableEntityComponent selectedEntity)
	{
		if (!selectedEntity)
			return null;

		IEntity owner = selectedEntity.GetOwner();
		if (!owner)
			return null;

		if (SCR_ChimeraCharacter.Cast(owner))
			return owner;

		return null;
	}
}
