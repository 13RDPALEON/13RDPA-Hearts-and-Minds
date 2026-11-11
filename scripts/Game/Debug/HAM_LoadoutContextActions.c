//------------------------------------------------------------------------------------------------
// 13RDPA — clic droit GM : copier / coller la tenue d'un personnage vers un autre.
//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class HAM_CopyLoadoutContextAction : SCR_SelectedEntitiesContextAction
{
	void HAM_InitInfo(SCR_UIInfo info)
	{
		m_Info = info;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return HAM_LoadoutActionUtil.GetCharacter(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return HAM_LoadoutActionUtil.GetCharacter(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		HAM_LoadoutActionUtil.Send(selectedEntity, false);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleUIInfo("m_Info")]
class HAM_PasteLoadoutContextAction : SCR_SelectedEntitiesContextAction
{
	void HAM_InitInfo(SCR_UIInfo info)
	{
		m_Info = info;
	}

	override bool CanBeShown(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return HAM_LoadoutActionUtil.GetCharacter(selectedEntity) != null;
	}

	override bool CanBePerformed(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition, int flags)
	{
		return HAM_LoadoutActionUtil.GetCharacter(selectedEntity) != null;
	}

	override void Perform(SCR_EditableEntityComponent selectedEntity, vector cursorWorldPosition)
	{
		HAM_LoadoutActionUtil.Send(selectedEntity, true);
	}
}

class HAM_LoadoutActionUtil
{
	static IEntity GetCharacter(SCR_EditableEntityComponent selectedEntity)
	{
		if (!selectedEntity)
			return null;

		IEntity owner = selectedEntity.GetOwner();
		if (!SCR_ChimeraCharacter.Cast(owner))
			return null;

		return owner;
	}

	static void Send(SCR_EditableEntityComponent selectedEntity, bool paste)
	{
		IEntity character = GetCharacter(selectedEntity);
		if (!character)
			return;

		RplComponent rpl = RplComponent.Cast(character.FindComponent(RplComponent));
		if (!rpl)
			return;

		RplId id = rpl.Id();
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (!player)
		{
			if (!Replication.IsServer())
				return;

			if (paste)
				HAM_LoadoutClipboard.Paste(character);
			else
				HAM_LoadoutClipboard.Copy(character);

			return;
		}

		if (paste)
			player.HAM_PasteLoadout(id);
		else
			player.HAM_CopyLoadout(id);
	}
}
