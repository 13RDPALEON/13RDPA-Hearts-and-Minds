//------------------------------------------------------------------------------------------------
// 13RDPA — statut whitelist GM/EM répliqué serveur → client (lobby deploy).
//------------------------------------------------------------------------------------------------

modded class SCR_PlayerController
{
	[RplProp(onRplName: "HAM_OnGroupWhitelistUpdated")]
	protected bool m_bTRN_GroupWhitelisted;

	[RplProp(onRplName: "HAM_OnGroupWhitelistUpdated")]
	protected bool m_bTRN_GroupWhitelistKnown;

	bool HAM_IsGroupWhitelisted()
	{
		return m_bTRN_GroupWhitelisted;
	}

	bool HAM_IsGroupWhitelistKnown()
	{
		return m_bTRN_GroupWhitelistKnown;
	}

	void HAM_SetGroupWhitelistState(bool whitelisted)
	{
		if (!Replication.IsServer())
			return;

		m_bTRN_GroupWhitelisted = whitelisted;
		m_bTRN_GroupWhitelistKnown = true;
		Replication.BumpMe();
		HAM_OnGroupWhitelistUpdated();
	}

	protected void HAM_OnGroupWhitelistUpdated()
	{
		GetGame().GetCallqueue().CallLater(HAM_RefreshLobbyGroupButtonsDelayed, 100, false);
	}

	protected static void HAM_RefreshLobbyGroupButtonsDelayed()
	{
		SCR_GroupRequestUIComponent groupUi = SCR_GroupRequestUIComponent.HAM_GetActiveInstance();
		if (!groupUi)
			return;

		groupUi.HAM_RefreshAllGroupButtons();
	}
}

modded class SCR_GroupRequestUIComponent
{
	protected static SCR_GroupRequestUIComponent s_HAM_ActiveInstance;

	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		s_HAM_ActiveInstance = this;
	}

	override void HandlerDeattached(Widget w)
	{
		if (s_HAM_ActiveInstance == this)
			s_HAM_ActiveInstance = null;

		super.HandlerDeattached(w);
	}

	static SCR_GroupRequestUIComponent HAM_GetActiveInstance()
	{
		return s_HAM_ActiveInstance;
	}

	void HAM_RefreshAllGroupButtons()
	{
		string summary = string.Format("sections playerId=%1", HAM_GroupAccessHelper.ResolveLocalPlayerId());

		foreach (SCR_DeployButtonBase btn : m_aButtons)
		{
			SCR_GroupButton groupBtn = SCR_GroupButton.Cast(btn);
			if (!groupBtn)
				continue;

			groupBtn.UpdateGroup();
			groupBtn.UpdateButtonAvailability(m_PlyGroupComp);

			SCR_AIGroup group = groupBtn.HAM_GetGroup();
			string groupName = "sans-groupe";
			if (group)
				groupName = HAM_GroupAccessHelper.ResolveGroupDisplayName(group);

			string state = "gris";
			Widget root = groupBtn.GetRootWidget();
			if (root && root.IsEnabled())
				state = "actif";

			summary = summary + string.Format(" | %1=%2", groupName, state);
		}

		Print("[13RDPA H&M] " + summary, LogLevel.NORMAL);
	}

	//------------------------------------------------------------------------------------------------
	// Vanilla n'appelle UpdateButtonAvailability qu'après join — on force les deux.
	override void UpdateGroupButton(SCR_GroupButton groupButton, bool canJoinGroup)
	{
		groupButton.UpdateGroup(canJoinGroup);
		if (m_PlyGroupComp)
			groupButton.UpdateButtonAvailability(m_PlyGroupComp);
	}

	//------------------------------------------------------------------------------------------------
	protected override void RequestJoinGroup(notnull SCR_GroupButton groupBtn)
	{
		if (SCR_FactionCommanderPlayerComponent.IsLocalPlayerCommander())
			return;

		int groupId = groupBtn.GetGroupId();
		SCR_AIGroup group = m_GroupManager.FindGroup(groupId);
		bool canJoin = false;
		if (group)
			canJoin = HAM_GroupAccessHelper.CanPlayerJoinLobbyGroup(HAM_GroupAccessHelper.ResolveLocalPlayerId(), group);

		if (!canJoin)
			return;

		m_PlyGroupComp.SetSelectedGroupID(groupId);
		super.RequestJoinGroup(groupBtn);
	}
}
