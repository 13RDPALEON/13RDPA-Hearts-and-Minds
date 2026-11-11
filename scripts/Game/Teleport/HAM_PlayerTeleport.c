//------------------------------------------------------------------------------------------------
// Téléport joueur → joueur / base capturée.
// Sur pied le mouvement est autorité owner : téléporter sur le client propriétaire.
// Chaque drapeau E_PlayerTeleport posé dans le monde est une destination.
//------------------------------------------------------------------------------------------------

class HAM_TeleportPointComponentClass : ScriptComponentClass
{
}

class HAM_TeleportPointComponent : ScriptComponent
{
	protected static ref array<HAM_TeleportPointComponent> s_aPoints;

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);

		if (!s_aPoints)
			s_aPoints = {};

		if (s_aPoints.Find(this) < 0)
			s_aPoints.Insert(this);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_aPoints)
			s_aPoints.RemoveItem(this);

		super.OnDelete(owner);
	}

	static int GetPoints(out array<HAM_TeleportPointComponent> points)
	{
		points.Clear();
		if (!s_aPoints)
			return 0;

		foreach (HAM_TeleportPointComponent point : s_aPoints)
		{
			if (point && point.GetOwner())
				points.Insert(point);
		}

		return points.Count();
	}

	string GetPointName()
	{
		IEntity owner = GetOwner();
		if (!owner)
			return "Drapeau";

		string name = owner.GetName();
		if (name.IsEmpty())
			return "Drapeau";

		return name;
	}
}

modded class SCR_PlayerController
{
	protected static const float HAM_TELEPORT_COOLDOWN_MS = 3000;
	protected float m_fTRN_NextTeleport;

	void HAM_SyncTeleportSettings(bool playerToPlayer, bool betweenBases)
	{
		Rpc(HAM_RpcTeleportSettings, playerToPlayer, betweenBases);
		if (Replication.IsServer() && SCR_PlayerController.GetLocalPlayerId() == GetPlayerId())
			HAM_TeleportSettings.ApplyClient(playerToPlayer, betweenBases);
	}

	void HAM_RequestSetTeleportSettings(bool playerToPlayer, bool betweenBases)
	{
		if (Replication.IsServer())
			HAM_TeleportSettings.Apply(playerToPlayer, betweenBases);
		else
			Rpc(HAM_RpcSetTeleportSettings, playerToPlayer, betweenBases);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcSetTeleportSettings(bool playerToPlayer, bool betweenBases)
	{
		HAM_TeleportSettings.Apply(playerToPlayer, betweenBases);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcTeleportSettings(bool playerToPlayer, bool betweenBases)
	{
		HAM_TeleportSettings.ApplyClient(playerToPlayer, betweenBases);
	}

	void HAM_RequestTeleportToPlayer(int targetPlayerId)
	{
		if (Replication.IsServer())
			HAM_TeleportToPlayer(GetPlayerId(), targetPlayerId);
		else
			Rpc(HAM_RpcTeleportToPlayer, targetPlayerId);
	}

	void HAM_RequestTeleportToBase(int baseIndex)
	{
		if (Replication.IsServer())
			HAM_TeleportToBase(GetPlayerId(), baseIndex);
		else
			Rpc(HAM_RpcTeleportToBase, baseIndex);
	}

	// Liste construite serveur : le client ne stream pas les personnages trop loin.
	void HAM_RequestTeleportList()
	{
		if (Replication.IsServer())
			HAM_SendTeleportList(GetPlayerId());
		else
			Rpc(HAM_RpcAskTeleportList);
	}

	void HAM_RequestBaseTeleportList()
	{
		if (Replication.IsServer())
			HAM_SendBaseTeleportList(GetPlayerId());
		else
			Rpc(HAM_RpcAskBaseTeleportList);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcTeleportToPlayer(int targetPlayerId)
	{
		HAM_TeleportToPlayer(GetPlayerId(), targetPlayerId);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcTeleportToBase(int baseIndex)
	{
		HAM_TeleportToBase(GetPlayerId(), baseIndex);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcAskTeleportList()
	{
		HAM_SendTeleportList(GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcAskBaseTeleportList()
	{
		HAM_SendBaseTeleportList(GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcDoTeleportOwner(vector position)
	{
		SCR_Global.TeleportLocalPlayer(position, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcTeleportList(string idsPacked, string namesPacked)
	{
		HAM_PlayerTeleportBoard.ApplyServerList(idsPacked, namesPacked);
	}

	protected void HAM_SendTeleportList(int askerId)
	{
		if (!Replication.IsServer())
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		array<int> ids = {};
		players.GetPlayers(ids);

		string idsPacked;
		string namesPacked;
		bool first = true;

		foreach (int playerId : ids)
		{
			if (playerId < 1 || playerId == askerId)
				continue;

			IEntity controlled = players.GetPlayerControlledEntity(playerId);
			if (!controlled)
				continue;

			DamageManagerComponent damage = DamageManagerComponent.Cast(controlled.FindComponent(DamageManagerComponent));
			if (damage && damage.GetState() == EDamageState.DESTROYED)
				continue;

			string name = players.GetPlayerName(playerId);
			if (name.IsEmpty())
				name = "Joueur " + playerId.ToString();

			name.Replace("|", " ");
			name.Replace(";", " ");

			if (!first)
			{
				idsPacked = idsPacked + ";";
				namesPacked = namesPacked + "|";
			}

			first = false;
			idsPacked = idsPacked + playerId.ToString();
			namesPacked = namesPacked + name;
		}

		Rpc(HAM_RpcTeleportList, idsPacked, namesPacked);
		if (SCR_PlayerController.GetLocalPlayerId() == askerId)
			HAM_PlayerTeleportBoard.ApplyServerList(idsPacked, namesPacked);
	}

	protected void HAM_SendBaseTeleportList(int askerId)
	{
		if (!Replication.IsServer())
			return;

		if (!HAM_TeleportSettings.BetweenBasesEnabled())
		{
			Rpc(HAM_RpcTeleportList, "", "");
			if (SCR_PlayerController.GetLocalPlayerId() == askerId)
				HAM_PlayerTeleportBoard.ApplyServerList("", "");
			return;
		}

		SCR_Faction playerFaction = HAM_GetPlayerFaction(askerId);
		if (!playerFaction || !playerFaction.IsPlayable())
		{
			Rpc(HAM_RpcTeleportList, "", "");
			if (SCR_PlayerController.GetLocalPlayerId() == askerId)
				HAM_PlayerTeleportBoard.ApplyServerList("", "");
			return;
		}

		string playerKey = playerFaction.GetFactionKey();

		if (HAM_Mission.IsRunning())
		{
			HAM_SendAlliedBaseList(askerId);
			return;
		}

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign || !campaign.GetBaseManager())
		{
			HAM_SendFlagTeleportList(askerId);
			return;
		}

		array<SCR_CampaignMilitaryBaseComponent> bases = {};
		campaign.GetBaseManager().GetBases(bases);

		IEntity asker = GetGame().GetPlayerManager().GetPlayerControlledEntity(askerId);
		vector askerPos = vector.Zero;
		if (asker)
			askerPos = asker.GetOrigin();

		// Exclure seulement la base la plus proche (celle où on se trouve), pas un rayon fixe.
		int nearestIndex = -1;
		float nearestDist = -1;
		int count = bases.Count();
		if (asker)
		{
			for (int n = 0; n < count; n++)
			{
				SCR_CampaignMilitaryBaseComponent nearBase = bases[n];
				if (!nearBase || !nearBase.GetOwner())
					continue;

				if (!HAM_BaseFactionMatches(nearBase, playerKey))
					continue;

				vector delta = nearBase.GetOwner().GetOrigin() - askerPos;
				delta[1] = 0;
				float dist = delta.LengthSq();
				if (nearestDist < 0 || dist < nearestDist)
				{
					nearestDist = dist;
					nearestIndex = n;
				}
			}

			// Seulement si vraiment sur place (< 150 m).
			if (nearestDist < 0 || nearestDist > 22500)
				nearestIndex = -1;
		}

		string idsPacked;
		string namesPacked;
		bool first = true;

		for (int i = 0; i < count; i++)
		{
			SCR_CampaignMilitaryBaseComponent base = bases[i];
			if (!base || !base.GetOwner())
				continue;

			if (!HAM_BaseFactionMatches(base, playerKey))
				continue;

			if (i == nearestIndex)
				continue;

			string name = base.GetBaseName();
			if (name.IsEmpty())
				name = "Base " + i.ToString();
			if (base.IsHQ())
				name = name + " (QG)";

			name.Replace("|", " ");
			name.Replace(";", " ");

			if (!first)
			{
				idsPacked = idsPacked + ";";
				namesPacked = namesPacked + "|";
			}

			first = false;
			idsPacked = idsPacked + i.ToString();
			namesPacked = namesPacked + name;
		}

		Rpc(HAM_RpcTeleportList, idsPacked, namesPacked);
		if (SCR_PlayerController.GetLocalPlayerId() == askerId)
			HAM_PlayerTeleportBoard.ApplyServerList(idsPacked, namesPacked);
	}

	protected void HAM_SendFlagTeleportList(int askerId)
	{
		array<HAM_TeleportPointComponent> points = {};
		int count = HAM_TeleportPointComponent.GetPoints(points);

		IEntity asker = GetGame().GetPlayerManager().GetPlayerControlledEntity(askerId);
		vector askerPos = vector.Zero;
		if (asker)
			askerPos = asker.GetOrigin();

		int nearestIndex = -1;
		float nearestDist = -1;
		for (int n = 0; n < count; n++)
		{
			HAM_TeleportPointComponent nearPoint = points[n];
			if (!nearPoint || !nearPoint.GetOwner())
				continue;

			vector delta = nearPoint.GetOwner().GetOrigin() - askerPos;
			delta[1] = 0;
			float dist = delta.LengthSq();
			if (nearestDist < 0 || dist < nearestDist)
			{
				nearestDist = dist;
				nearestIndex = n;
			}
		}

		if (nearestDist < 0 || nearestDist > 22500)
			nearestIndex = -1;

		string idsPacked;
		string namesPacked;
		bool first = true;

		for (int i = 0; i < count; i++)
		{
			if (i == nearestIndex)
				continue;

			HAM_TeleportPointComponent point = points[i];
			if (!point || !point.GetOwner())
				continue;

			string name = point.GetPointName();
			name.Replace("|", " ");
			name.Replace(";", " ");

			if (!first)
			{
				idsPacked = idsPacked + ";";
				namesPacked = namesPacked + "|";
			}

			first = false;
			idsPacked = idsPacked + i.ToString();
			namesPacked = namesPacked + name;
		}

		Rpc(HAM_RpcTeleportList, idsPacked, namesPacked);
		if (SCR_PlayerController.GetLocalPlayerId() == askerId)
			HAM_PlayerTeleportBoard.ApplyServerList(idsPacked, namesPacked);
	}

	protected void HAM_TeleportToFlag(int requesterId, int pointIndex)
	{
		array<HAM_TeleportPointComponent> points = {};
		int count = HAM_TeleportPointComponent.GetPoints(points);
		if (pointIndex < 0 || pointIndex >= count)
			return;

		HAM_TeleportPointComponent point = points[pointIndex];
		if (!point || !point.GetOwner())
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		IEntity requester = players.GetPlayerControlledEntity(requesterId);
		if (!requester)
			return;

		vector spot = HAM_Util.SafeArrival(point.GetOwner().GetOrigin());

		if (!HAM_DoTeleport(requesterId, requester, spot))
			return;

		Print(string.Format("[13RDPA H&M Teleport] %1 → %2", players.GetPlayerName(requesterId), point.GetPointName()), LogLevel.NORMAL);
	}

	protected void HAM_TeleportToPlayer(int requesterId, int targetPlayerId)
	{
		if (!Replication.IsServer())
			return;

		if (!HAM_TeleportSettings.PlayerToPlayerEnabled())
			return;

		if (requesterId < 1 || targetPlayerId < 1 || requesterId == targetPlayerId)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (!players)
			return;

		IEntity requester = players.GetPlayerControlledEntity(requesterId);
		IEntity target = players.GetPlayerControlledEntity(targetPlayerId);
		if (!requester || !target)
			return;

		DamageManagerComponent targetDamage = DamageManagerComponent.Cast(target.FindComponent(DamageManagerComponent));
		if (targetDamage && targetDamage.GetState() == EDamageState.DESTROYED)
			return;

		vector origin = target.GetOrigin();
		vector mat[4];
		target.GetTransform(mat);
		vector forward = mat[2];
		forward[1] = 0;
		if (forward.LengthSq() < 0.01)
			forward = Vector(0, 0, 1);
		else
			forward.Normalize();

		vector right = Vector(forward[2], 0, -forward[0]);
		vector spot = origin - forward * 2.2 + right * 0.8;
		spot[1] = GetGame().GetWorld().GetSurfaceY(spot[0], spot[2]) + 0.15;

		if (!HAM_DoTeleport(requesterId, requester, spot))
			return;

		Print(string.Format("[13RDPA H&M Teleport] %1 → près de %2", players.GetPlayerName(requesterId), players.GetPlayerName(targetPlayerId)), LogLevel.NORMAL);
	}

	protected void HAM_TeleportToBase(int requesterId, int baseIndex)
	{
		if (!Replication.IsServer())
			return;

		if (!HAM_TeleportSettings.BetweenBasesEnabled())
			return;

		if (requesterId < 1 || baseIndex < 0)
			return;

		SCR_Faction playerFaction = HAM_GetPlayerFaction(requesterId);
		if (!playerFaction || !playerFaction.IsPlayable())
			return;

		if (HAM_Mission.IsRunning())
		{
			HAM_TeleportToAlliedBase(requesterId, baseIndex);
			return;
		}

		SCR_GameModeCampaign campaign = SCR_GameModeCampaign.GetInstance();
		if (!campaign || !campaign.GetBaseManager())
		{
			HAM_TeleportToFlag(requesterId, baseIndex);
			return;
		}

		array<SCR_CampaignMilitaryBaseComponent> bases = {};
		campaign.GetBaseManager().GetBases(bases);
		if (baseIndex >= bases.Count())
			return;

		SCR_CampaignMilitaryBaseComponent base = bases[baseIndex];
		if (!base || !base.GetOwner())
			return;

		if (!HAM_BaseFactionMatches(base, playerFaction.GetFactionKey()))
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		IEntity requester = players.GetPlayerControlledEntity(requesterId);
		if (!requester)
			return;

		vector spot = HAM_Util.SafeArrival(base.GetOwner().GetOrigin());

		if (!HAM_DoTeleport(requesterId, requester, spot))
			return;

		Print(string.Format("[13RDPA H&M Teleport] %1 → base %2", players.GetPlayerName(requesterId), base.GetBaseName()), LogLevel.NORMAL);
	}

	//! H&M : les bases sont le QG et les maîtres tenus. On exclut celle où l'on se trouve.
	protected void HAM_SendAlliedBaseList(int askerId)
	{
		array<string> names = {};
		HAM_Mission.AlliedBases(names);
		IEntity asker = GetGame().GetPlayerManager().GetPlayerControlledEntity(askerId);
		string idsPacked;
		string namesPacked;
		bool first = true;
		for (int i = 0; i < names.Count(); i++)
		{
			vector pos = HAM_Mission.BasePos(names[i]);
			if (asker && vector.DistanceSqXZ(asker.GetOrigin(), pos) < 22500)
				continue;

			string label = names[i];
			if (label == HAM_Mission.s_sDesignatedHq)
				label = label + " (QG actif)";

			label.Replace("|", " ");
			label.Replace(";", " ");
			if (!first)
			{
				idsPacked = idsPacked + ";";
				namesPacked = namesPacked + "|";
			}

			first = false;
			idsPacked = idsPacked + i.ToString();
			namesPacked = namesPacked + label;
		}

		Rpc(HAM_RpcTeleportList, idsPacked, namesPacked);
		if (SCR_PlayerController.GetLocalPlayerId() == askerId)
			HAM_PlayerTeleportBoard.ApplyServerList(idsPacked, namesPacked);
	}

	protected void HAM_TeleportToAlliedBase(int requesterId, int baseIndex)
	{
		array<string> names = {};
		HAM_Mission.AlliedBases(names);
		if (baseIndex < 0 || baseIndex >= names.Count())
			return;

		IEntity requester = GetGame().GetPlayerManager().GetPlayerControlledEntity(requesterId);
		if (!requester)
			return;

		vector spot = HAM_Util.SafeArrival(HAM_Mission.BasePos(names[baseIndex]));
		if (HAM_DoTeleport(requesterId, requester, spot))
			Print(string.Format("[13RDPA H&M Teleport] joueur %1 → %2", requesterId, names[baseIndex]), LogLevel.NORMAL);
	}

	protected bool HAM_DoTeleport(int requesterId, IEntity requester, vector spot)
	{
		if (!requester)
			return false;

		float now = GetGame().GetWorld().GetWorldTime();
		if (now < m_fTRN_NextTeleport)
			return false;

		DamageManagerComponent requesterDamage = DamageManagerComponent.Cast(requester.FindComponent(DamageManagerComponent));
		if (requesterDamage && requesterDamage.GetState() == EDamageState.DESTROYED)
			return false;

		CompartmentAccessComponent compartment = CompartmentAccessComponent.Cast(requester.FindComponent(CompartmentAccessComponent));
		if (compartment && compartment.GetCompartment())
		{
			Print(string.Format("[13RDPA H&M Teleport] joueur %1 refusé : déjà en véhicule", requesterId), LogLevel.WARNING);
			return false;
		}

		if (!SCR_Global.IsPositionWithinTerrainBounds(spot))
		{
			Print("[13RDPA H&M Teleport] position hors carte", LogLevel.WARNING);
			return false;
		}

		m_fTRN_NextTeleport = now + HAM_TELEPORT_COOLDOWN_MS;

		IEntity root = requester.GetRootParent();
		if (root == requester)
		{
			Rpc(HAM_RpcDoTeleportOwner, spot);
			if (SCR_PlayerController.GetLocalPlayerId() == requesterId)
				SCR_Global.TeleportLocalPlayer(spot, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
		}
		else
		{
			SCR_Global.TeleportPlayer(requesterId, spot, SCR_EPlayerTeleportedReason.FAST_TRAVEL);
		}

		return true;
	}

	protected SCR_Faction HAM_GetPlayerFaction(int playerId)
	{
		IEntity controlled = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		if (!controlled)
			return null;

		FactionAffiliationComponent affiliation = FactionAffiliationComponent.Cast(controlled.FindComponent(FactionAffiliationComponent));
		if (!affiliation)
			return null;

		return SCR_Faction.Cast(affiliation.GetAffiliatedFaction());
	}

	protected bool HAM_BaseFactionMatches(SCR_CampaignMilitaryBaseComponent base, string playerKey)
	{
		if (!base || playerKey.IsEmpty())
			return false;

		SCR_CampaignFaction campaignFaction = base.GetCampaignFaction();
		if (campaignFaction)
		{
			string key = campaignFaction.GetFactionKey();
			if (key == playerKey)
				return true;
		}

		Faction faction = base.GetFaction();
		if (faction)
		{
			string key = faction.GetFactionKey();
			if (key == playerKey)
				return true;
		}

		return false;
	}
}
