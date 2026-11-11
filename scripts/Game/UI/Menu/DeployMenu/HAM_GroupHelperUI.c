modded class SCR_GroupHelperUI
{
	//------------------------------------------------------------------------------------------------
	static override string GetTranslatedGroupNameAndRoleName(notnull SCR_AIGroup group)
	{
		string displayName = HAM_GroupAccessHelper.Get13RDPADisplayName(group);
		if (!displayName.IsEmpty())
			return displayName;

		return super.GetTranslatedGroupNameAndRoleName(group);
	}

	//------------------------------------------------------------------------------------------------
	static override string GetTranslatedGroupName(notnull SCR_AIGroup group)
	{
		string displayName = HAM_GroupAccessHelper.Get13RDPADisplayName(group);
		if (!displayName.IsEmpty())
			return displayName;

		return super.GetTranslatedGroupName(group);
	}

	//------------------------------------------------------------------------------------------------
	static override string GetRoleName(notnull SCR_AIGroup group)
	{
		if (HAM_GroupAccessHelper.Is13RDPALobbyGroup(group))
			return string.Empty;

		return super.GetRoleName(group);
	}
}
