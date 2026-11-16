modded class SCR_EditorManagerEntity
{
	protected SCR_VoNComponent m_SNK_CharacterVONBeforeOpen;

	override void Open(bool showErrorNotification = true)
	{
		if (IsAuthority() && !m_bIsOpened && !FindComponent(BaseRadioComponent))
			SNK_SkipVanillaEditorRadioBlock();

		if (!m_bIsOpened && IsOwner())
			m_SNK_CharacterVONBeforeOpen = SNK_FindCharacterVONComponent();

		super.Open(showErrorNotification);

		if (!IsOwner())
			return;

		GetGame().GetCallqueue().Remove(SNK_BindCharacterVON);
		GetGame().GetCallqueue().CallLater(SNK_BindCharacterVON, 300, false);
	}

	protected void SNK_SkipVanillaEditorRadioBlock()
	{
		PlayerController playerController = GetGame().GetPlayerManager().GetPlayerController(GetPlayerID());
		if (!playerController)
			return;

		SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
		if (!vonController)
			return;

		vonController.m_bIsEditorRadioAdded = true;
	}

	protected SCR_VoNComponent SNK_FindCharacterVONComponent()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return null;

		SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
		if (vonController)
		{
			SCR_VoNComponent activeVON = vonController.GetVONComponent();
			if (activeVON && activeVON != FindComponent(SCR_VoNComponent))
				return activeVON;
		}

		IEntity character = playerController.GetControlledEntity();
		if (!character)
			return null;

		return SCR_VoNComponent.Cast(character.FindComponent(SCR_VoNComponent));
	}

	protected void SNK_BindCharacterVON()
	{
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return;

		SCR_VONController vonController = SCR_VONController.Cast(playerController.FindComponent(SCR_VONController));
		if (!vonController)
			return;

		SCR_VoNComponent characterVON = m_SNK_CharacterVONBeforeOpen;
		m_SNK_CharacterVONBeforeOpen = null;

		if (!characterVON)
			characterVON = SNK_FindCharacterVONComponent();

		if (!characterVON)
			return;

		vonController.SetVONComponent(characterVON);
	}

	void SNK_PushCameraPosition(vector position)
	{
		Rpc(SNK_RpcSetCameraPosition_Server, position);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void SNK_RpcSetCameraPosition_Server(vector position)
	{
		SNK_ApplyCameraPosition(position);
		Rpc(SNK_RpcSetCameraPosition_Broadcast, position);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Broadcast)]
	protected void SNK_RpcSetCameraPosition_Broadcast(vector position)
	{
		SNK_ApplyCameraPosition(position);
	}

	protected void SNK_ApplyCameraPosition(vector position)
	{
		vector transform[4];
		GetWorldTransform(transform);
		transform[3] = position;
		SetWorldTransform(transform);
	}
}
