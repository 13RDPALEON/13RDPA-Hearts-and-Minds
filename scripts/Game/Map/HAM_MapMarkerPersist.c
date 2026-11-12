//------------------------------------------------------------------------------------------------
// Marqueurs carte persistants, un dossier par joueur.
// $profile:13RDPA_HeartsAndMinds/MapMarkers/Joueur_<identity>/markers.json
// La restauration ne recharge que le dossier du joueur connecté.
//------------------------------------------------------------------------------------------------

void HAM_MapMarkerRestorePlayer(int playerId)
{
	HAM_MapMarkerPersist.RestorePlayer(playerId);
}

class HAM_MapMarkerPersist
{
	protected static const string ROOT = "$profile:13RDPA_HeartsAndMinds/MapMarkers/";
	protected static const int MAX_MARKERS = 400;

	protected static bool s_bStarted;
	protected static bool s_bRestoring;
	protected static int s_iRestoreOwner;
	protected static bool s_bSaveQueued;
	protected static int s_iSavedRemoveOwner;
	protected static ref array<int> s_aDirty;
	protected static ref array<string> s_aFrozen;
	protected static ref array<string> s_aRestored;

	static void Start()
	{
		if (s_bStarted || !Replication.IsServer())
			return;

		s_bStarted = true;
		GetGame().GetCallqueue().CallLater(RestoreOnline, 45000, false);
	}

	static void OnDisconnect(int playerId)
	{
		if (!Replication.IsServer())
			return;

		string identity = IdentityOf(playerId);
		SavePlayer(playerId);
		Freeze(identity);
		if (!s_aRestored)
			s_aRestored = {};

		s_aRestored.RemoveItem(identity);
	}

	static void RestorePlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		string identity = IdentityOf(playerId);
		if (identity.IsEmpty())
			return;

		Unfreeze(identity);
		if (!s_aRestored)
			s_aRestored = {};

		if (s_aRestored.Find(identity) >= 0)
			return;

		s_aRestored.Insert(identity);
		string path = FileOf(identity);
		if (!FileIO.FileExists(path))
			return;

		JsonLoadContext ctx = new JsonLoadContext();
		if (!ctx.LoadFromFile(path))
			return;

		array<int> types = {};
		array<int> posX = {};
		array<int> posY = {};
		array<int> configIds = {};
		array<int> factions = {};
		array<int> colors = {};
		array<int> icons = {};
		array<int> rotations = {};
		array<string> texts = {};
		ctx.ReadValue("types", types);
		ctx.ReadValue("x", posX);
		ctx.ReadValue("y", posY);
		ctx.ReadValue("config", configIds);
		ctx.ReadValue("faction", factions);
		ctx.ReadValue("color", colors);
		ctx.ReadValue("icon", icons);
		ctx.ReadValue("rot", rotations);
		ctx.ReadValue("text", texts);

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
		{
			s_aRestored.RemoveItem(identity);
			return;
		}

		int count = types.Count();
		if (posX.Count() < count)
			count = posX.Count();
		if (posY.Count() < count)
			count = posY.Count();
		if (texts.Count() < count)
			count = texts.Count();

		s_bRestoring = true;
		s_iRestoreOwner = playerId;
		int placed = 0;
		for (int i = 0; i < count; i++)
		{
			if (placed >= MAX_MARKERS)
				break;

			if (IsScriptText(texts[i]))
				continue;

			SCR_MapMarkerBase marker = new SCR_MapMarkerBase();
			marker.SetType(types[i]);
			marker.SetWorldPos(posX[i], posY[i]);
			if (i < configIds.Count())
				marker.SetMarkerConfigID(configIds[i]);
			if (i < factions.Count())
				marker.SetMarkerFactionFlags(factions[i]);
			if (i < colors.Count())
				marker.SetColorEntry(colors[i]);
			if (i < icons.Count())
				marker.SetIconEntry(icons[i]);
			if (i < rotations.Count())
				marker.SetRotation(rotations[i]);
			marker.SetCustomText(texts[i]);
			marker.SetMarkerOwnerID(playerId);
			manager.InsertStaticMarker(marker, false, true);
			placed++;
		}

		s_iRestoreOwner = 0;
		s_bRestoring = false;
		if (placed > 0)
			Print(string.Format("[HAM] Marqueurs carte : %1 restaurés pour %2", placed, identity), LogLevel.NORMAL);
	}

	static void AssignRestoreOwner(SCR_MapMarkerBase marker)
	{
		if (!s_bRestoring || !marker || s_iRestoreOwner <= 0)
			return;

		marker.SetMarkerOwnerID(s_iRestoreOwner);
	}

	//! Marqueur joueur partagé : visible par tous, déplaçable et supprimable par tous.
	static bool IsSharedMarker(SCR_MapMarkerBase marker)
	{
		return ShouldPersist(marker);
	}

	static void PrepareShared(SCR_MapMarkerBase marker)
	{
		if (!IsSharedMarker(marker))
			return;

		marker.SetMarkerFactionFlags(0);
	}

	static void RememberRemoveOwner(int ownerId)
	{
		s_iSavedRemoveOwner = ownerId;
	}

	static void OnMarkerAdded(SCR_MapMarkerBase marker)
	{
		if (!Replication.IsServer() || s_bRestoring || !marker)
			return;

		if (!ShouldPersist(marker))
			return;

		NotePlayer(marker.GetMarkerOwnerID());
	}

	static void OnMarkerRemoved(int ownerId)
	{
		if (s_iSavedRemoveOwner > 0)
		{
			ownerId = s_iSavedRemoveOwner;
			s_iSavedRemoveOwner = 0;
		}

		if (!Replication.IsServer() || s_bRestoring)
			return;

		NotePlayer(ownerId);
	}

	static bool AllowServerMarkerDelete(SCR_MapMarkerBase marker)
	{
		return ShouldPersist(marker);
	}

	protected static void RestoreOnline()
	{
		if (!Replication.IsServer())
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);
		foreach (int playerId : ids)
			RestorePlayer(playerId);
	}

	protected static void NotePlayer(int playerId)
	{
		if (playerId <= 0)
			return;

		string identity = IdentityOf(playerId);
		if (identity.IsEmpty() || IsFrozen(identity))
			return;

		if (!s_aDirty)
			s_aDirty = {};

		if (s_aDirty.Find(playerId) < 0)
			s_aDirty.Insert(playerId);

		if (s_bSaveQueued)
			return;

		s_bSaveQueued = true;
		GetGame().GetCallqueue().CallLater(TickSave, 3000, false);
	}

	protected static void TickSave()
	{
		s_bSaveQueued = false;
		if (!s_aDirty)
			return;

		array<int> pending = {};
		foreach (int playerId : s_aDirty)
			pending.Insert(playerId);

		s_aDirty.Clear();
		foreach (int playerId : pending)
			SavePlayer(playerId);
	}

	protected static void SavePlayer(int playerId)
	{
		if (!Replication.IsServer() || playerId <= 0)
			return;

		string identity = IdentityOf(playerId);
		if (identity.IsEmpty() || IsFrozen(identity))
			return;

		SCR_MapMarkerManagerComponent manager = SCR_MapMarkerManagerComponent.GetInstance();
		if (!manager)
			return;

		array<SCR_MapMarkerBase> markers = manager.GetStaticMarkers();
		array<int> types = {};
		array<int> posX = {};
		array<int> posY = {};
		array<int> configIds = {};
		array<int> factions = {};
		array<int> colors = {};
		array<int> icons = {};
		array<int> rotations = {};
		array<string> texts = {};
		if (markers)
		{
			foreach (SCR_MapMarkerBase marker : markers)
			{
				if (!marker || marker.GetMarkerOwnerID() != playerId)
					continue;

				if (!ShouldPersist(marker))
					continue;

				if (types.Count() >= MAX_MARKERS)
					break;

				int xy[2];
				marker.GetWorldPos(xy);
				types.Insert(marker.GetType());
				posX.Insert(xy[0]);
				posY.Insert(xy[1]);
				configIds.Insert(marker.GetMarkerConfigID());
				factions.Insert(marker.GetMarkerFactionFlags());
				colors.Insert(marker.GetColorEntry());
				icons.Insert(marker.GetIconEntry());
				rotations.Insert(marker.GetRotation());
				texts.Insert(marker.GetCustomText());
			}
		}

		string folder = FolderOf(identity);
		HAM_ProfileFiles.EnsureDir(ROOT);
		HAM_ProfileFiles.EnsureDir(folder);
		JsonSaveContext ctx = new JsonSaveContext();
		ctx.WriteValue("identity", identity);
		ctx.WriteValue("types", types);
		ctx.WriteValue("x", posX);
		ctx.WriteValue("y", posY);
		ctx.WriteValue("config", configIds);
		ctx.WriteValue("faction", factions);
		ctx.WriteValue("color", colors);
		ctx.WriteValue("icon", icons);
		ctx.WriteValue("rot", rotations);
		ctx.WriteValue("text", texts);
		ctx.SaveToFile(folder + "markers.json");
	}

	protected static string IdentityOf(int playerId)
	{
		return HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
	}

	protected static string FolderOf(string identity)
	{
		return ROOT + "Joueur_" + HAM_ProfileFiles.SafeId(identity) + "/";
	}

	protected static string FileOf(string identity)
	{
		return FolderOf(identity) + "markers.json";
	}

	protected static void Freeze(string identity)
	{
		if (identity.IsEmpty())
			return;

		if (!s_aFrozen)
			s_aFrozen = {};

		if (s_aFrozen.Find(identity) < 0)
			s_aFrozen.Insert(identity);
	}

	protected static void Unfreeze(string identity)
	{
		if (!s_aFrozen || identity.IsEmpty())
			return;

		s_aFrozen.RemoveItem(identity);
	}

	protected static bool IsFrozen(string identity)
	{
		if (!s_aFrozen || identity.IsEmpty())
			return false;

		return s_aFrozen.Find(identity) >= 0;
	}

	protected static bool ShouldPersist(SCR_MapMarkerBase marker)
	{
		if (!marker)
			return false;

		SCR_EMapMarkerType markerType = marker.GetType();
		if (markerType != SCR_EMapMarkerType.PLACED_CUSTOM)
		{
			if (markerType != SCR_EMapMarkerType.PLACED_MILITARY)
				return false;
		}

		if (IsScriptText(marker.GetCustomText()))
			return false;

		return true;
	}

	protected static bool IsScriptText(string text)
	{
		if (text.IsEmpty())
			return false;

		if (text == "Convoi")
			return true;
		if (text.IndexOf("13RDPA") == 0)
			return true;
		if (text == "Garage" || text == "Arsenal" || text == "Hélicoptères" || text == "Véhicules" || text == "Médical" || text == "Point")
			return true;

		return false;
	}
}

modded class SCR_MapMarkerManagerComponent
{
	override void OnAddSynchedMarker(SCR_MapMarkerBase marker)
	{
		HAM_MapMarkerPersist.AssignRestoreOwner(marker);
		HAM_MapMarkerPersist.PrepareShared(marker);
		super.OnAddSynchedMarker(marker);
		HAM_MapMarkerPersist.OnMarkerAdded(marker);
	}

	override void OnRemoveSynchedMarker(int markerID)
	{
		int ownerId = -1;
		SCR_MapMarkerBase marker = GetStaticMarkerByID(markerID);
		if (!marker)
			marker = GetDisabledMarkerByID(markerID);
		if (marker)
			ownerId = marker.GetMarkerOwnerID();

		super.OnRemoveSynchedMarker(markerID);
		HAM_MapMarkerPersist.OnMarkerRemoved(ownerId);
	}

	override void RemoveStaticMarker(SCR_MapMarkerBase marker)
	{
		if (marker && marker.GetMarkerID() != -1 && marker.GetMarkerOwnerID() == -1)
		{
			if (!Replication.IsServer() && HAM_MapMarkerPersist.AllowServerMarkerDelete(marker))
			{
				if (!m_MarkerSyncComp)
				{
					if (!FindMarkerSyncComponent())
						return;
				}

				m_MarkerSyncComp.AskRemoveStaticMarker(marker.GetMarkerID());
				return;
			}
		}

		super.RemoveStaticMarker(marker);
	}
}

modded class SCR_MapMarkerSyncComponent
{
	override protected void RPC_AskRemoveStaticMarker(int markerID)
	{
		SCR_MapMarkerManagerComponent markerMgr = SCR_MapMarkerManagerComponent.GetInstance();
		if (markerMgr)
		{
			SCR_MapMarkerBase marker = markerMgr.GetStaticMarkerByID(markerID);
			if (!marker)
				marker = markerMgr.GetDisabledMarkerByID(markerID);

			if (marker && HAM_MapMarkerPersist.IsSharedMarker(marker))
			{
				int originalOwner = marker.GetMarkerOwnerID();
				HAM_MapMarkerPersist.RememberRemoveOwner(originalOwner);
				SCR_PlayerController caller = SCR_PlayerController.Cast(GetOwner());
				if (caller && originalOwner != caller.GetPlayerId())
				{
					if (originalOwner > 0)
					{
						PlayerController ownerController = GetGame().GetPlayerManager().GetPlayerController(originalOwner);
						if (ownerController)
						{
							SCR_MapMarkerSyncComponent ownerSync = SCR_MapMarkerSyncComponent.Cast(ownerController.FindComponent(SCR_MapMarkerSyncComponent));
							if (ownerSync)
								ownerSync.RemovePlayerMarker(markerID);
						}
					}

					marker.SetMarkerOwnerID(caller.GetPlayerId());
				}
			}
		}

		super.RPC_AskRemoveStaticMarker(markerID);
	}
}

modded class SCR_MapMarkersUI
{
	override static bool IsOwnedMarker(notnull SCR_MapMarkerBase marker)
	{
		if (HAM_MapMarkerPersist.IsSharedMarker(marker))
			return true;

		return super.IsOwnedMarker(marker);
	}
}
