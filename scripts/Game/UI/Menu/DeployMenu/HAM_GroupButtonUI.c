modded class SCR_GroupButton
{
	protected static bool s_bTRN_LoggedMissingPlayerId;

	SCR_AIGroup HAM_GetGroup()
	{
		return m_Group;
	}

	override void UpdateGroupFlag()
	{
		if (m_Group && HAM_GroupAccessHelper.Is13RDPALobbyGroup(m_Group))
		{
			HAM_13RDPALobbyGroupDef def = HAM_GroupAccessHelper.GetLobbyGroupDefForGroup(m_Group);
			if (def && !def.m_sFlag.IsEmpty())
			{
				// Forcer texture .edds (jamais LoadImageFromSet / GroupFlagsBlufor)
				if (m_Group.GetFlagIsFromImageSet())
					m_Group.SetFlagIsFromImageSet(false);

				SetImage(def.m_sFlag);
				return;
			}
		}

		super.UpdateGroupFlag();
	}

	override void UpdateGroup(bool canJoin = true)
	{
		if (m_Group && HAM_GroupAccessHelper.Is13RDPALobbyGroup(m_Group))
		{
			int playerId = HAM_GroupAccessHelper.ResolveLocalPlayerId();
			if (playerId <= 0 && !s_bTRN_LoggedMissingPlayerId)
			{
				s_bTRN_LoggedMissingPlayerId = true;
				Print("[13RDPA H&M] boutons de section mis à jour avec playerId=0", LogLevel.NORMAL);
			}

			canJoin = HAM_GroupAccessHelper.CanPlayerJoinLobbyGroup(playerId, m_Group);
			super.UpdateGroup(canJoin);

			bool privacyBlocked = HAM_GroupAccessHelper.BlocksLobbyJoinDueToPrivacy(playerId, m_Group);
			bool enabled = canJoin && !privacyBlocked && !m_Group.IsFull();
			SetEnabled(enabled);
			return;
		}

		super.UpdateGroup(canJoin);
	}

	override void UpdateButtonAvailability(SCR_PlayerControllerGroupComponent groupCtrl)
	{
		if (!m_Group || !groupCtrl)
		{
			super.UpdateButtonAvailability(groupCtrl);
			return;
		}

		int playerId = HAM_GroupAccessHelper.ResolveLocalPlayerId();
		if (HAM_GroupAccessHelper.Is13RDPALobbyGroup(m_Group))
		{
			SetEnabled(HAM_GroupAccessHelper.CanPlayerJoinLobbyGroup(playerId, m_Group) && !HAM_GroupAccessHelper.BlocksLobbyJoinDueToPrivacy(playerId, m_Group) && !m_Group.IsFull());
			return;
		}

		super.UpdateButtonAvailability(groupCtrl);
	}
}

modded class SCR_AIGroup
{
	override string GetCustomName()
	{
		if (m_iNameAuthorID <= 0)
			return m_sCustomName;

		return super.GetCustomName();
	}

	override string GetCustomDescription()
	{
		if (m_iDescriptionAuthorID <= 0)
			return m_sCustomDescription;

		return super.GetCustomDescription();
	}

	override string GetCustomNameWithOriginal()
	{
		string displayName = HAM_GroupAccessHelper.Get13RDPADisplayName(this);
		if (!displayName.IsEmpty())
			return displayName;

		if (m_iNameAuthorID <= 0)
			return HAM_CallsignOrCustomName();

		return super.GetCustomNameWithOriginal();
	}

	protected string HAM_CallsignOrCustomName()
	{
		string company;
		string platoon;
		string squad;
		string character;
		string format;
		GetCallsigns(company, platoon, squad, character, format);
		string originalName = string.Format(format, company, platoon, squad, character);

		if (m_sCustomName.IsEmpty())
			return originalName;

		return m_sCustomName + " ( " + originalName + " )";
	}
}

modded class SCR_GroupFlagImageComponent
{
	override void SetFlagButtonFromImageSet(string name)
	{
		if (!name.IsEmpty() && name.IndexOf(".edds") >= 0)
		{
			ImageWidget image = GetImageWidget();
			if (image)
				image.LoadImageTexture(0, name);

			Resize();
			return;
		}

		super.SetFlagButtonFromImageSet(name);
	}
}
