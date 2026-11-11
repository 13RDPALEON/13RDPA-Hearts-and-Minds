[BaseContainerProps(configRoot: true)]
class HAM_13RDPA_GroupAccessConfig
{
	[Attribute(desc: "IDs autorisés pour Game Master et Etat Major (Steam64 ou UID Bohemia)")]
	ref array<string> m_aWhitelistIds;

	[Attribute(desc: "Noms de joueurs autorisés pour Game Master et Etat Major (insensible à la casse)")]
	ref array<string> m_aWhitelistNames;

	[Attribute(desc: "IDs recevant automatiquement le grade Major à la connexion (Steam64 ou UID Bohemia)")]
	ref array<string> m_aMajorRankIds;

	[Attribute(desc: "Noms de joueurs recevant automatiquement le grade Major (insensible à la casse)")]
	ref array<string> m_aMajorRankNames;
}

class HAM_13RDPA_GroupAccessJsonSection
{
	ref array<string> whitelistIds;
	ref array<string> whitelistNames;
	ref array<string> majorRankIds;
	ref array<string> majorRankNames;
}

class HAM_13RDPALobbyGroupDef
{
	SCR_EGroupRole m_eRole;
	string m_sName;
	int m_iSize;
	ResourceName m_sFlag;
	int m_iRadioFrequency;
	bool m_bWhitelistOnly;

	void HAM_13RDPALobbyGroupDef(SCR_EGroupRole role, string name, int size, ResourceName flag, int radioFrequency, bool whitelistOnly = false)
	{
		m_eRole = role;
		m_sName = name;
		m_iSize = size;
		m_sFlag = flag;
		m_iRadioFrequency = radioFrequency;
		m_bWhitelistOnly = whitelistOnly;
	}
}

class HAM_GroupAccessHelper
{
	protected static const ResourceName CONFIG_PATH = "{858F503D2D193912}Configs/Groups/HAM_GroupAccess.conf";
	protected static const string GROUP_GAME_MASTER = "Game Master";
	protected static const string GROUP_ETAT_MAJOR = "Etat Major";
	protected static const string GROUP_SANS_AFFECTATION = "Sans Affectation";

	protected static ref HAM_13RDPA_GroupAccessConfig s_Config;
	protected static ref array<string> s_JsonWhitelistIds;
	protected static ref array<string> s_JsonWhitelistNames;
	protected static bool s_bJsonWhitelistActive;
	protected static ref array<string> s_JsonMajorRankIds;
	protected static ref array<string> s_JsonMajorRankNames;
	protected static ref array<ref HAM_13RDPALobbyGroupDef> s_aLobbyGroupDefs;
	protected static ref map<int, ref array<string>> s_mCachedPlayerIdentityCandidates;

	protected static const string INVALID_IDENTITY = "00000000-0000-0000-0000-000000000000";

	//------------------------------------------------------------------------------------------------
	protected static void EnsureLobbyGroupDefs()
	{
		if (s_aLobbyGroupDefs)
			return;

		// Ordre = ordre d'affichage lobby. Chaque groupe DOIT avoir un SCR_EGroupRole unique
		// (HAM_PurgeNon13RDPAGroups ne garde qu'un groupe par role).
		s_aLobbyGroupDefs = {
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.HELI_TRANSPORT, GROUP_GAME_MASTER, 2, "{73713FC1E99C2A7F}UI/HAM/Groups/13RDPA_Group_GameMaster.edds", 85000, true),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.MECHANIZED, GROUP_ETAT_MAJOR, 5, "{5CCDCD64A485FF2E}UI/HAM/Groups/13RDPA_Group_EtatMajor.edds", 50000, true),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.COMMANDER, "PM", 6, "{B0D6DB537A234A53}UI/HAM/Groups/13RDPA_Group_PM.edds", 52000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.ASSAULT, "India", 25, "{3F4F623DAF72BA54}UI/HAM/Groups/13RDPA_Group_India.edds", 31000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.RECON, "Bravo", 8, "{67B18C209309EE76}UI/HAM/Groups/13RDPA_Group_Bravo.edds", 41000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.ANTITANK, "Lima", 12, "{39120A142EFED4A1}UI/HAM/Groups/13RDPA_Group_Lima.edds", 46000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.TRANSPORT, "Sierra", 5, "{9D230D277A04DC44}UI/HAM/Groups/13RDPA_Group_Sierra.edds", 56000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.ENGINEER, "Hotel", 6, "{2CA72ABCF1E4C9EF}UI/HAM/Groups/13RDPA_Group_Hotel.edds", 61000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.MACHINEGUN, "Alpha", 4, "{C720BBBB04F2A31F}UI/HAM/Groups/13RDPA_Group_Alpha.edds", 71000),
			new HAM_13RDPALobbyGroupDef(SCR_EGroupRole.RESERVES, GROUP_SANS_AFFECTATION, 24, "{7375D267DA89ECEB}UI/HAM/Groups/13RDPA_Group_SansAffectation.edds", 51000)
		};
	}

	//------------------------------------------------------------------------------------------------
	static array<ref HAM_13RDPALobbyGroupDef> GetLobbyGroupDefs()
	{
		EnsureLobbyGroupDefs();
		return s_aLobbyGroupDefs;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPA_GroupAccessConfig GetConfig()
	{
		if (s_Config)
			return s_Config;

		Resource container = BaseContainerTools.LoadContainer(CONFIG_PATH);
		if (!container)
		{
			Print("[13RDPA H&M] Impossible de charger 13RDPA_GroupAccess.conf", LogLevel.ERROR);
			return null;
		}

		s_Config = HAM_13RDPA_GroupAccessConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(container.GetResource().ToBaseContainer()));
		if (!s_Config)
			Print("[13RDPA H&M] 13RDPA_GroupAccess.conf illisible", LogLevel.ERROR);

		return s_Config;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPALobbyGroupDef GetLobbyGroupDef(SCR_EGroupRole role)
	{
		foreach (HAM_13RDPALobbyGroupDef def : GetLobbyGroupDefs())
		{
			if (def && def.m_eRole == role)
				return def;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static int GetLobbyGroupSortIndex(SCR_EGroupRole role)
	{
		int index = 0;
		foreach (HAM_13RDPALobbyGroupDef def : GetLobbyGroupDefs())
		{
			if (def && def.m_eRole == role)
				return index;

			index++;
		}

		return 999;
	}

	//------------------------------------------------------------------------------------------------
	static int GetLobbyGroupSortIndex(notnull SCR_AIGroup group)
	{
		return GetLobbyGroupSortIndex(group.GetGroupRole());
	}

	//------------------------------------------------------------------------------------------------
	static void SortLobbyGroups(array<SCR_AIGroup> groups)
	{
		if (!groups || groups.IsEmpty())
			return;

		for (int i = 1; i < groups.Count(); i++)
		{
			SCR_AIGroup keyGroup = groups[i];
			int keyIndex = GetLobbyGroupSortIndex(keyGroup);
			int j = i - 1;

			while (j >= 0 && GetLobbyGroupSortIndex(groups[j]) > keyIndex)
			{
				groups[j + 1] = groups[j];
				j--;
			}

			groups[j + 1] = keyGroup;
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool Is13RDPALobbyRole(SCR_EGroupRole role)
	{
		return GetLobbyGroupDef(role) != null;
	}

	//------------------------------------------------------------------------------------------------
	static bool Is13RDPALobbyGroup(notnull SCR_AIGroup group)
	{
		SCR_Faction faction = SCR_Faction.Cast(group.GetFaction());
		if (!faction || faction.GetFactionKey() != HAM_Units.PLAYER_FACTION)
			return false;

		return Is13RDPALobbyRole(group.GetGroupRole());
	}

	//------------------------------------------------------------------------------------------------
	static string Get13RDPADisplayName(notnull SCR_AIGroup group)
	{
		if (!Is13RDPALobbyGroup(group))
			return string.Empty;

		HAM_13RDPALobbyGroupDef def = GetLobbyGroupDef(group.GetGroupRole());
		if (!def)
			return string.Empty;

		return def.m_sName;
	}

	//------------------------------------------------------------------------------------------------
	static string ResolveGroupDisplayName(notnull SCR_AIGroup group)
	{
		string displayName = Get13RDPADisplayName(group);
		if (!displayName.IsEmpty())
			return displayName;

		string groupName = group.GetCustomName();
		if (!groupName.IsEmpty())
			return groupName;

		return group.GetGroupRoleName();
	}

	//------------------------------------------------------------------------------------------------
	static string GetCanonicalLobbyGroupName(string groupName)
	{
		if (groupName == GROUP_GAME_MASTER || groupName.Contains(GROUP_GAME_MASTER))
			return GROUP_GAME_MASTER;

		if (groupName == GROUP_ETAT_MAJOR || groupName.Contains(GROUP_ETAT_MAJOR))
			return GROUP_ETAT_MAJOR;

		if (groupName == "PM" || groupName.Contains("POLICE MILITAIRE"))
			return "PM";

		if (groupName == "India" || groupName.Contains("India"))
			return "India";

		if (groupName == "Bravo" || groupName.Contains("Bravo"))
			return "Bravo";

		if (groupName == "Lima" || groupName.Contains("Lima"))
			return "Lima";

		if (groupName == "Sierra" || groupName.Contains("Sierra"))
			return "Sierra";

		if (groupName == "Hotel" || groupName.Contains("Hotel"))
			return "Hotel";

		if (groupName == "Alpha" || groupName.Contains("Alpha"))
			return "Alpha";

		if (groupName == GROUP_SANS_AFFECTATION || groupName.Contains(GROUP_SANS_AFFECTATION))
			return GROUP_SANS_AFFECTATION;

		return groupName;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPALobbyGroupDef GetLobbyGroupDefByName(string groupName)
	{
		string canonicalName = GetCanonicalLobbyGroupName(groupName);
		foreach (HAM_13RDPALobbyGroupDef def : GetLobbyGroupDefs())
		{
			if (def && def.m_sName == canonicalName)
				return def;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPALobbyGroupDef GetLobbyGroupDefByFlag(ResourceName flag)
	{
		if (flag.IsEmpty())
			return null;

		foreach (HAM_13RDPALobbyGroupDef def : GetLobbyGroupDefs())
		{
			if (def && def.m_sFlag == flag)
				return def;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPALobbyGroupDef GetLobbyGroupDefForGroup(notnull SCR_AIGroup group)
	{
		HAM_13RDPALobbyGroupDef def = GetLobbyGroupDef(group.GetGroupRole());
		if (def)
			return def;

		def = GetLobbyGroupDefByFlag(group.GetGroupFlag());
		if (def)
			return def;

		def = GetLobbyGroupDefByName(group.GetCustomName());
		if (def)
			return def;

		return GetLobbyGroupDefByName(ResolveGroupDisplayName(group));
	}

	//------------------------------------------------------------------------------------------------
	static HAM_13RDPALobbyGroupDef GetWhitelistLobbyGroupDef(notnull SCR_AIGroup group)
	{
		HAM_13RDPALobbyGroupDef def = GetLobbyGroupDef(group.GetGroupRole());
		if (def && def.m_bWhitelistOnly)
			return def;

		def = GetLobbyGroupDefByFlag(group.GetGroupFlag());
		if (def && def.m_bWhitelistOnly)
			return def;

		def = GetLobbyGroupDefByName(group.GetCustomName());
		if (def && def.m_bWhitelistOnly)
			return def;

		def = GetLobbyGroupDefByName(ResolveGroupDisplayName(group));
		if (def && def.m_bWhitelistOnly)
			return def;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsWhitelistLobbyGroup(notnull SCR_AIGroup group)
	{
		return GetWhitelistLobbyGroupDef(group) != null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsWhitelistLobbyGroup(SCR_GroupRolePresetConfig preset)
	{
		if (!preset)
			return false;

		HAM_13RDPALobbyGroupDef def = GetLobbyGroupDef(preset.GetGroupRole());
		if (def && def.m_bWhitelistOnly)
			return true;

		def = GetLobbyGroupDefByName(preset.GetGroupName());
		return def && def.m_bWhitelistOnly;
	}

	//------------------------------------------------------------------------------------------------
	static string GetPlayerNameString(int playerId)
	{
		PlayerManager playerManager = GetGame().GetPlayerManager();
		if (!playerManager)
			return string.Empty;

		return playerManager.GetPlayerName(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static string NormalizePlayerName(string playerName)
	{
		if (playerName.IsEmpty())
			return string.Empty;

		string normalized = playerName;
		normalized.Trim();
		normalized.ToLower();

		// [13RDPA]Flo et "Flo" doivent matcher la whitelist par nom
		if (normalized.IndexOf("[") == 0)
		{
			int close = normalized.IndexOf("]");
			if (close > 0 && close < normalized.Length() - 1)
			{
				normalized = normalized.Substring(close + 1, normalized.Length() - close - 1);
				normalized.Trim();
			}
		}

		return normalized;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsNameAllowed(string playerName, array<string> allowedNames)
	{
		if (!allowedNames || allowedNames.IsEmpty() || playerName.IsEmpty())
			return false;

		string normalizedPlayerName = NormalizePlayerName(playerName);
		if (normalizedPlayerName.IsEmpty())
			return false;

		foreach (string allowedName : allowedNames)
		{
			if (allowedName.IsEmpty())
				continue;

			if (NormalizePlayerName(allowedName) == normalizedPlayerName)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static string GetPlayerIdentityString(int playerId)
	{
		UUID identityUuid = SCR_PlayerIdentityUtils.GetPlayerIdentityId(playerId);
		string identity = string.Format("%1", identityUuid);
		if (!IsInvalidIdentityString(identity))
			return identity;

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static int ResolveLocalPlayerId()
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (pc)
		{
			int controllerPlayerId = pc.GetPlayerId();
			if (controllerPlayerId > 0)
				return controllerPlayerId;
		}

		int localPlayerId = SCR_PlayerController.GetLocalPlayerId();
		if (localPlayerId > 0)
			return localPlayerId;

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsInvalidIdentityString(string identity)
	{
		if (identity.IsEmpty())
			return true;

		return NormalizeIdentityId(identity) == INVALID_IDENTITY;
	}

	//------------------------------------------------------------------------------------------------
	static bool CachePlayerIdentityCandidates(int playerId)
	{
		array<string> candidates = BuildPlayerIdentityCandidates(playerId);
		if (candidates.IsEmpty())
			return false;

		if (!s_mCachedPlayerIdentityCandidates)
			s_mCachedPlayerIdentityCandidates = new map<int, ref array<string>>();

		s_mCachedPlayerIdentityCandidates.Set(playerId, candidates);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> BuildPlayerIdentityCandidates(int playerId)
	{
		array<string> candidates = {};
		string identity = GetPlayerIdentityString(playerId);
		if (!identity.IsEmpty())
			candidates.Insert(identity);

		string playerIdStr = playerId.ToString();
		if (!playerIdStr.IsEmpty())
			candidates.Insert(playerIdStr);

		return candidates;
	}

	//------------------------------------------------------------------------------------------------
	static array<string> GetPlayerIdentityCandidates(int playerId)
	{
		if (Replication.IsServer() && s_mCachedPlayerIdentityCandidates && s_mCachedPlayerIdentityCandidates.Contains(playerId))
			return s_mCachedPlayerIdentityCandidates.Get(playerId);

		return BuildPlayerIdentityCandidates(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static void UpdatePlayerWhitelistStatus(int playerId)
	{
		if (!Replication.IsServer())
			return;

		CachePlayerIdentityCandidates(playerId);

		bool whitelisted = IsPlayerWhitelisted(playerId);
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!pc)
			return;

		pc.HAM_SetGroupWhitelistState(whitelisted);
	}

	//------------------------------------------------------------------------------------------------
	static void UpdatePlayerWhitelistStatusWithRetry(int playerId)
	{
		if (!Replication.IsServer())
			return;

		UpdatePlayerWhitelistStatus(playerId);

		array<string> candidates = GetPlayerIdentityCandidates(playerId);
		if (!candidates.IsEmpty())
			return;

		GetGame().GetCallqueue().CallLater(HAM_RetryUpdatePlayerWhitelistStatus, 500, false, playerId, 1);
	}

	//------------------------------------------------------------------------------------------------
	protected static void HAM_RetryUpdatePlayerWhitelistStatus(int playerId, int attempt)
	{
		UpdatePlayerWhitelistStatus(playerId);

		array<string> candidates = GetPlayerIdentityCandidates(playerId);
		if (!candidates.IsEmpty())
			return;

		if (attempt >= 12)
		{
			Print(string.Format("[13RDPA H&M] Whitelist non résolue pour joueur %1 après %2 tentatives", playerId, attempt), LogLevel.WARNING);
			return;
		}

		GetGame().GetCallqueue().CallLater(HAM_RetryUpdatePlayerWhitelistStatus, 1000, false, playerId, attempt + 1);
	}

	//------------------------------------------------------------------------------------------------
	static void RefreshAllConnectedPlayersWhitelist()
	{
		if (!Replication.IsServer())
			return;

		PlayerManager playerManager = GetGame().GetPlayerManager();
		array<int> playerIds = {};
		playerManager.GetPlayers(playerIds);

		foreach (int playerId : playerIds)
			UpdatePlayerWhitelistStatusWithRetry(playerId);
	}

	//------------------------------------------------------------------------------------------------
	static bool ApplyJsonGroupAccess(JsonLoadContext ctx)
	{
		HAM_13RDPA_GroupAccessJsonSection section = new HAM_13RDPA_GroupAccessJsonSection();
		if (!ctx.ReadValue("groupAccess", section))
			return false;

		array<string> whitelistIds = {};
		array<string> whitelistNames = {};
		array<string> majorRankIds = {};
		array<string> majorRankNames = {};
		bool loadedWhitelistIds = ExtractIdList(section.whitelistIds, whitelistIds);
		bool loadedWhitelistNames = ExtractIdList(section.whitelistNames, whitelistNames);
		bool loadedMajorIds = ExtractIdList(section.majorRankIds, majorRankIds);
		bool loadedMajorNames = ExtractIdList(section.majorRankNames, majorRankNames);

		if (loadedWhitelistIds)
		{
			s_JsonWhitelistIds = whitelistIds;
			s_bJsonWhitelistActive = true;
			if (!loadedWhitelistNames)
				s_JsonWhitelistNames = {};
		}

		if (loadedWhitelistNames)
		{
			s_JsonWhitelistNames = whitelistNames;
			s_bJsonWhitelistActive = true;
		}

		if (loadedMajorIds)
			s_JsonMajorRankIds = majorRankIds;

		if (loadedMajorNames)
			s_JsonMajorRankNames = majorRankNames;

		int whitelistIdCount = 0;
		if (s_JsonWhitelistIds)
			whitelistIdCount = s_JsonWhitelistIds.Count();

		int whitelistNameCount = 0;
		if (s_JsonWhitelistNames)
			whitelistNameCount = s_JsonWhitelistNames.Count();

		int majorIdCount = 0;
		if (s_JsonMajorRankIds)
			majorIdCount = s_JsonMajorRankIds.Count();

		int majorNameCount = 0;
		if (s_JsonMajorRankNames)
			majorNameCount = s_JsonMajorRankNames.Count();

		if (whitelistIdCount > 0 || whitelistNameCount > 0 || majorIdCount > 0 || majorNameCount > 0)
		{
			Print(string.Format("[13RDPA H&M] Accès JSON chargé — whitelist ids: %1, noms: %2, major ids: %3, noms: %4", whitelistIdCount, whitelistNameCount, majorIdCount, majorNameCount), LogLevel.NORMAL);
			RefreshAllConnectedPlayersWhitelist();
			return true;
		}

		Print("[13RDPA H&M] groupAccess présent mais aucune liste d'IDs lue (format JSON invalide ?)", LogLevel.WARNING);
		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool ExtractIdList(array<string> source, out array<string> outIds)
	{
		outIds = {};
		if (!source)
			return false;

		foreach (string entry : source)
		{
			string trimmed = entry;
			trimmed.Trim();
			if (!trimmed.IsEmpty())
				outIds.Insert(trimmed);
		}

		return !outIds.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> GetWhitelistIds()
	{
		if (s_JsonWhitelistIds && !s_JsonWhitelistIds.IsEmpty())
			return s_JsonWhitelistIds;

		HAM_13RDPA_GroupAccessConfig config = GetConfig();
		if (config && config.m_aWhitelistIds)
			return config.m_aWhitelistIds;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> GetWhitelistNames()
	{
		if (s_bJsonWhitelistActive)
			return s_JsonWhitelistNames;

		if (s_JsonWhitelistNames && !s_JsonWhitelistNames.IsEmpty())
			return s_JsonWhitelistNames;

		HAM_13RDPA_GroupAccessConfig config = GetConfig();
		if (config && config.m_aWhitelistNames)
			return config.m_aWhitelistNames;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> GetMajorRankIds()
	{
		if (s_JsonMajorRankIds && !s_JsonMajorRankIds.IsEmpty())
			return s_JsonMajorRankIds;

		HAM_13RDPA_GroupAccessConfig config = GetConfig();
		if (config && config.m_aMajorRankIds)
			return config.m_aMajorRankIds;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static array<string> GetMajorRankNames()
	{
		if (s_JsonMajorRankNames && !s_JsonMajorRankNames.IsEmpty())
			return s_JsonMajorRankNames;

		HAM_13RDPA_GroupAccessConfig config = GetConfig();
		if (config && config.m_aMajorRankNames)
			return config.m_aMajorRankNames;

		return null;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlayerInConfigIdList(int playerId, array<string> allowedIds)
	{
		if (!allowedIds || allowedIds.IsEmpty())
			return false;

		array<string> candidates = GetPlayerIdentityCandidates(playerId);
		foreach (string candidate : candidates)
		{
			if (IsIdAllowed(candidate, allowedIds))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlayerInConfigNameList(int playerId, array<string> allowedNames)
	{
		if (!allowedNames || allowedNames.IsEmpty())
			return false;

		return IsNameAllowed(GetPlayerNameString(playerId), allowedNames);
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlayerWhitelisted(int playerId)
	{
		int resolvedPlayerId = playerId;
		if (resolvedPlayerId <= 0)
			resolvedPlayerId = ResolveLocalPlayerId();

		// .conf / JSON : source de vérité commune client + serveur
		if (IsPlayerInConfigIdList(resolvedPlayerId, GetWhitelistIds()))
			return true;

		if (IsPlayerInConfigNameList(resolvedPlayerId, GetWhitelistNames()))
			return true;

		if (!Replication.IsServer())
		{
			SCR_PlayerController localPc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
			if (localPc)
			{
				int localPlayerId = localPc.GetPlayerId();
				if (localPlayerId <= 0)
					localPlayerId = ResolveLocalPlayerId();

				if (localPlayerId > 0 && resolvedPlayerId == localPlayerId && localPc.HAM_IsGroupWhitelistKnown())
					return localPc.HAM_IsGroupWhitelisted();
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsPlayerMajorRankListed(int playerId)
	{
		if (IsPlayerInConfigIdList(playerId, GetMajorRankIds()))
			return true;

		return IsPlayerInConfigNameList(playerId, GetMajorRankNames());
	}

	//------------------------------------------------------------------------------------------------
	static string NormalizeIdentityId(string identityId)
	{
		if (identityId.IsEmpty())
			return string.Empty;

		string normalized = identityId;
		normalized.Replace("{", "");
		normalized.Replace("}", "");
		normalized.Trim();
		normalized.ToLower();
		return normalized;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsIdAllowed(string playerId, array<string> allowedIds)
	{
		if (!allowedIds || allowedIds.IsEmpty() || playerId.IsEmpty())
			return false;

		string normalizedPlayerId = NormalizeIdentityId(playerId);
		if (normalizedPlayerId.IsEmpty())
			return false;

		foreach (string allowedId : allowedIds)
		{
			if (allowedId.IsEmpty())
				continue;

			if (NormalizeIdentityId(allowedId) == normalizedPlayerId)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	static bool CanPlayerJoinLobbyGroup(int playerId, notnull SCR_AIGroup group)
	{
		if (!Is13RDPALobbyGroup(group))
			return false;

		HAM_13RDPALobbyGroupDef whitelistDef = GetWhitelistLobbyGroupDef(group);
		if (whitelistDef)
		{
			bool whitelisted = IsPlayerWhitelisted(playerId);
			if (!whitelisted)
				return false;

			return PassesLobbyCapacityRequirements(playerId, group);
		}

		return PassesBasicJoinRequirements(playerId, group);
	}

	//------------------------------------------------------------------------------------------------
	static string GetLobbyJoinDenialReason(int playerId, notnull SCR_AIGroup group)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return string.Empty;

		if (group.IsFull())
			return string.Format("complet (%1/%2)", group.GetPlayerCount(), group.GetMaxMembers());

		if (IsWhitelistLobbyGroup(group) && !IsPlayerWhitelisted(playerId))
			return "pas dans la whitelist";

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	static bool PassesLobbyCapacityRequirements(int playerId, notnull SCR_AIGroup group)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		if (!groupsManager)
			return true;

		// Déjà dans ce groupe : autoriser (sélection lobby, pas un nouveau join)
		if (groupsManager.GetPlayerGroup(playerId) == group)
			return true;

		if (group.IsFull())
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static bool PassesBasicJoinRequirements(int playerId, notnull SCR_AIGroup group)
	{
		Faction groupFaction = group.GetFaction();
		if (!groupFaction)
			return false;

		Faction playerFaction = SCR_FactionManager.SGetPlayerFaction(playerId);
		if (playerFaction && playerFaction != groupFaction)
			return false;

		return PassesLobbyCapacityRequirements(playerId, group);
	}

	//------------------------------------------------------------------------------------------------
	static int GetLobbyGroupMatchScore(notnull SCR_AIGroup group, notnull HAM_13RDPALobbyGroupDef def)
	{
		int score = 0;
		string customName = group.GetCustomName();

		if (customName == def.m_sName)
			score += 100;
		else if (customName.Contains(def.m_sName))
			score += 75;

		if (group.IsMaxMembersLimited() && group.GetMaxMembers() == def.m_iSize)
			score += 50;

		ResourceName flag = group.GetGroupFlag();
		if (flag == def.m_sFlag)
			score += 25;

		string roleName = group.GetGroupRoleName();
		if (roleName == def.m_sName)
			score += 10;

		return score;
	}

	//------------------------------------------------------------------------------------------------
	static bool BlocksLobbyJoinDueToPrivacy(int playerId, notnull SCR_AIGroup group)
	{
		if (!group.IsPrivate())
			return false;

		// GM/EM : accès contrôlé par whitelist, pas par le flag private vanilla (HELI_TRANSPORT = private)
		if (IsWhitelistLobbyGroup(group) && IsPlayerWhitelisted(playerId))
			return false;

		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void ApplyLobbyGroupSetup(notnull SCR_AIGroup group, notnull HAM_13RDPALobbyGroupDef def)
	{
		if (group.GetGroupRole() != def.m_eRole)
			group.SetGroupRole(def.m_eRole);

		group.SetCustomName(def.m_sName, 0);

		if (!def.m_sFlag.IsEmpty())
		{
			// Les .edds custom ne sont PAS dans GroupFlagsBlufor.imageset
			group.SetFlagIsFromImageSet(false);
			group.SetCustomGroupFlag(def.m_sFlag);
		}

		if (def.m_iSize > 0 && group.GetMaxMembers() != def.m_iSize)
			group.SetMaxGroupMembers(def.m_iSize);

		if (def.m_iRadioFrequency > 0 && group.GetRadioFrequency() != def.m_iRadioFrequency)
			group.SetRadioFrequency(def.m_iRadioFrequency);

		if (!Replication.IsServer())
			return;

		// Un groupe prédéfini (India, etc.) est détruit quand le dernier joueur part, et n'est pas recréé.
		group.SetCanDeleteIfNoPlayer(false);

		if (!group.IsPrivacyChangeable())
			group.SetPrivacyChangeable(true);

		if (group.IsPrivate())
			group.SetPrivate(false);
	}

}

modded class SCR_GroupsManagerComponent
{
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		if (!Replication.IsServer())
			return;

		if (m_sDefaultGroupPrefab.IsEmpty())
			m_sDefaultGroupPrefab = "{04D3B38E23F51754}Prefabs/AI/Groups/Slave_Group.et";

		m_bAllowGroupMenu = true;
		HAM_LockPlayableFaction();
		GetGame().GetCallqueue().CallLater(HAM_Finalize13RDPALobbyGroups, 3000, false);
	}

	protected void HAM_LockPlayableFaction()
	{
		FactionManager manager = GetGame().GetFactionManager();
		if (!manager)
			return;

		array<Faction> factions = {};
		manager.GetFactionsList(factions);
		foreach (Faction faction : factions)
		{
			SCR_Faction scrFaction = SCR_Faction.Cast(faction);
			if (!scrFaction)
				continue;

			bool playable = scrFaction.GetFactionKey() == HAM_Units.PLAYER_FACTION;
			scrFaction.SetIsPlayable(playable, false);
		}
	}

	override void CreatePredefinedGroups()
	{
		super.CreatePredefinedGroups();

		if (!Replication.IsServer())
			return;

		HAM_Finalize13RDPALobbyGroups();
		GetGame().GetCallqueue().CallLater(HAM_Finalize13RDPALobbyGroups, 3000, false);
	}

	override bool HasPlayerRequiredRank(SCR_GroupRolePresetConfig preset, int playerId, bool ignoreGroupRequiredRank)
	{
		if (HAM_MilsimSettingsComponent.HAM_NoMinimumRank())
			return true;

		if (preset && HAM_GroupAccessHelper.IsPlayerWhitelisted(playerId))
		{
			if (HAM_GroupAccessHelper.IsWhitelistLobbyGroup(preset))
				return true;

			HAM_13RDPALobbyGroupDef def = HAM_GroupAccessHelper.GetLobbyGroupDefByFlag(preset.GetGroupFlag());
			if (def && def.m_bWhitelistOnly)
				return true;
		}

		bool allowed = super.HasPlayerRequiredRank(preset, playerId, ignoreGroupRequiredRank);
		if (!allowed)
			Print(string.Format("[13RDPA H&M] Rang insuffisant pour le joueur %1", playerId), LogLevel.WARNING);

		return allowed;
	}

	//------------------------------------------------------------------------------------------------
	override array<SCR_AIGroup> GetSortedPlayableGroupsByFaction(Faction faction)
	{
		array<SCR_AIGroup> groups = super.GetSortedPlayableGroupsByFaction(faction);
		if (!groups || groups.IsEmpty())
			return groups;

		SCR_Faction scrFaction = SCR_Faction.Cast(faction);
		if (!scrFaction || scrFaction.GetFactionKey() != HAM_Units.PLAYER_FACTION)
			return groups;

		HAM_GroupAccessHelper.SortLobbyGroups(groups);
		return groups;
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_Finalize13RDPALobbyGroups()
	{
		if (!Replication.IsServer())
			return;

		SCR_GroupsManagerComponent instance = GetInstance();
		if (instance && instance != this)
			return;

		HAM_LockPlayableFaction();
		HAM_PurgeNon13RDPAGroups();
		HAM_SpawnMissingLobbyGroups();
		HAM_Apply13RDPALobbyGroupSetup();
		SetNewGroupsAllowed(false);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_PurgeNon13RDPAGroups()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		Faction usFaction = factionManager.GetFactionByKey(HAM_Units.PLAYER_FACTION);
		if (!usFaction)
			return;

		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(usFaction);
		if (!groups)
			return;

		ref map<SCR_EGroupRole, SCR_AIGroup> bestGroups = new map<SCR_EGroupRole, SCR_AIGroup>();
		ref map<SCR_EGroupRole, int> bestScores = new map<SCR_EGroupRole, int>();

		foreach (SCR_AIGroup group : groups)
		{
			if (!group)
				continue;

			SCR_EGroupRole role = group.GetGroupRole();
			HAM_13RDPALobbyGroupDef def = HAM_GroupAccessHelper.GetLobbyGroupDef(role);
			if (!def)
			{
				DeleteGroupDelayed(group);
				continue;
			}

			int score = HAM_GroupAccessHelper.GetLobbyGroupMatchScore(group, def);
			if (!bestGroups.Contains(role))
			{
				bestGroups.Set(role, group);
				bestScores.Set(role, score);
				continue;
			}

			if (score > bestScores.Get(role))
			{
				DeleteGroupDelayed(bestGroups.Get(role));
				bestGroups.Set(role, group);
				bestScores.Set(role, score);
			}
			else
			{
				DeleteGroupDelayed(group);
			}
		}

		foreach (HAM_13RDPALobbyGroupDef expectedDef : HAM_GroupAccessHelper.GetLobbyGroupDefs())
		{
			if (!expectedDef || bestGroups.Contains(expectedDef.m_eRole))
				continue;

			Print(string.Format("[13RDPA H&M] Groupe manquant après purge: %1", expectedDef.m_sName), LogLevel.WARNING);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected SCR_AIGroup HAM_FindLobbyGroup(SCR_EGroupRole role, Faction faction)
	{
		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(faction);
		if (!groups)
			return null;

		foreach (SCR_AIGroup group : groups)
		{
			if (!group)
				continue;

			if (group.GetGroupRole() == role)
				return group;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_SpawnMissingLobbyGroups()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		Faction usFaction = factionManager.GetFactionByKey(HAM_Units.PLAYER_FACTION);
		SCR_Faction scrFaction = SCR_Faction.Cast(usFaction);
		if (!scrFaction)
			return;

		foreach (HAM_13RDPALobbyGroupDef def : HAM_GroupAccessHelper.GetLobbyGroupDefs())
		{
			if (!def)
				continue;

			if (HAM_FindLobbyGroup(def.m_eRole, usFaction))
				continue;

			Print(string.Format("[13RDPA H&M] Création du groupe manquant: %1", def.m_sName), LogLevel.NORMAL);

			SetNewGroupsAllowed(true);
			SCR_AIGroup newGroup = CreateNewPlayableGroup(usFaction, def.m_eRole);
			SetNewGroupsAllowed(false);
			if (!newGroup)
			{
				Print(string.Format("[13RDPA H&M] Échec création du groupe: %1", def.m_sName), LogLevel.ERROR);
				continue;
			}

			newGroup.SetCanDeleteIfNoPlayer(false);

			SCR_GroupRolePresetConfig config = FindGroupRolePresetConfig(scrFaction, def.m_eRole);
			if (config)
				config.SetupGroup(newGroup);

			HAM_GroupAccessHelper.ApplyLobbyGroupSetup(newGroup, def);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_Apply13RDPALobbyGroupSetup()
	{
		FactionManager factionManager = GetGame().GetFactionManager();
		if (!factionManager)
			return;

		Faction usFaction = factionManager.GetFactionByKey(HAM_Units.PLAYER_FACTION);
		if (!usFaction)
			return;

		array<SCR_AIGroup> groups = GetPlayableGroupsByFaction(usFaction);
		if (!groups)
			return;

		foreach (SCR_AIGroup group : groups)
		{
			if (!group)
				continue;

			HAM_13RDPALobbyGroupDef def = HAM_GroupAccessHelper.GetLobbyGroupDef(group.GetGroupRole());
			if (!def)
				continue;

			HAM_GroupAccessHelper.ApplyLobbyGroupSetup(group, def);
		}
	}
}

modded class SCR_PlayerControllerGroupComponent
{
	override void RPC_AskJoinGroup(int groupID)
	{
		SCR_GroupsManagerComponent groupsManager = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup group = null;
		if (groupsManager)
			group = groupsManager.FindGroup(groupID);

		int playerId = GetPlayerID();
		bool allowed = group && HAM_GroupAccessHelper.CanPlayerJoinLobbyGroup(playerId, group);

		if (group && HAM_GroupAccessHelper.Is13RDPALobbyGroup(group) && !allowed)
			return;

		super.RPC_AskJoinGroup(groupID);
	}

	override bool CanPlayerJoinGroup(int playerID, notnull SCR_AIGroup group)
	{
		if (HAM_GroupAccessHelper.Is13RDPALobbyGroup(group))
		{
			if (HAM_GroupAccessHelper.CanPlayerJoinLobbyGroup(playerID, group))
				return true;

			if (HAM_GroupAccessHelper.IsWhitelistLobbyGroup(group))
			{
				bool whitelisted = HAM_GroupAccessHelper.IsPlayerWhitelisted(playerID);
				string reason = HAM_GroupAccessHelper.GetLobbyJoinDenialReason(playerID, group);
				if (reason.IsEmpty())
					reason = "capacité/inconnu";

				Print(string.Format("[13RDPA H&M] Accès refusé %1 (nom: %2, identity: %3, groupe: %4, whitelist: %5, raison: %6)", playerID, HAM_GroupAccessHelper.GetPlayerNameString(playerID), HAM_GroupAccessHelper.GetPlayerIdentityString(playerID), HAM_GroupAccessHelper.ResolveGroupDisplayName(group), whitelisted, reason), LogLevel.WARNING);
			}

			return false;
		}

		return super.CanPlayerJoinGroup(playerID, group);
	}
}

