//------------------------------------------------------------------------------------------------
// Paradrop : carte, avion décor, chute, restitution du sac.
//------------------------------------------------------------------------------------------------

enum HAM_EParadropState
{
	NONE,
	PLANE,
	WAIT_JUMP,
	FALLING,
	ON_GROUND,
	RESTORING
}

class HAM_ParadropSession
{
	int m_iPlayerId;
	IEntity m_Chute;
	int m_iState;
	int m_iEpoch;
	float m_fX;
	float m_fZ;
	float m_fGroundAt;

	void HAM_ParadropSession(int playerId)
	{
		m_iPlayerId = playerId;
		m_iState = HAM_EParadropState.NONE;
	}
}

void HAM_ParadropRelease(int playerId, int epoch)
{
	HAM_ParadropComponent.Release(playerId, epoch);
}

void HAM_ParadropMoveToPlane(int playerId, int epoch)
{
	HAM_ParadropComponent.MoveToPlane(playerId, epoch);
}

void HAM_ParadropCompleteFinish(int playerId, int epoch)
{
	HAM_ParadropComponent.CompleteFinish(playerId, epoch);
}

class HAM_ParadropComponentClass : ScriptComponentClass
{
}

class HAM_ParadropComponent : ScriptComponent
{
	protected static HAM_ParadropComponent s_Instance;
	protected static IEntity s_Plane;
	protected static ref array<ref HAM_ParadropSession> s_aSessions;
	protected static const ResourceName PLANE = "{95C187D0523295EE}Prefabs/Props/PropsHeine/C-130FAB.et";
	protected static const ResourceName CHUTE = "{C47F5D977AC94514}Prefabs/Items/Equipment/Backpacks/6TP_Parachute.et";
	protected static const float PLANE_Y = 1000;
	protected static const float DROP_HEIGHT = 500;
	protected static const float JUMP_DELAY_MS = 4000;
	protected static const float GROUND_DELAY_MS = 20000;

	static HAM_ParadropComponent Get()
	{
		return s_Instance;
	}

	static IEntity Plane()
	{
		return s_Plane;
	}

	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		s_Instance = this;
		if (!s_aSessions)
			s_aSessions = {};

		if (!Replication.IsServer())
		{
			GetGame().GetCallqueue().CallLater(HAM_ParadropAudioTick, 200, true);
			return;
		}

		HAM_ParadropBagStore.WipeOnBoot();
		GetGame().GetCallqueue().CallLater(SpawnPlane, 2000, false);
		GetGame().GetCallqueue().CallLater(Tick, 500, true);
	}

	override void OnDelete(IEntity owner)
	{
		if (s_Instance == this)
			s_Instance = null;

		if (GetGame() && GetGame().GetCallqueue())
			GetGame().GetCallqueue().Remove(Tick);

		super.OnDelete(owner);
	}

	protected void SpawnPlane()
	{
		if (!Replication.IsServer() || s_Plane)
			return;

		Resource resource = Resource.Load(PLANE);
		if (!resource || !resource.IsValid())
			return;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = Vector(0, PLANE_Y, 0);
		s_Plane = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
	}

	static vector PlaneCenter()
	{
		return Vector(0.115, 1002.192, 1.776);
	}

	static HAM_ParadropSession FindSession(int playerId)
	{
		if (!s_aSessions)
			return null;

		foreach (HAM_ParadropSession session : s_aSessions)
		{
			if (session && session.m_iPlayerId == playerId)
				return session;
		}

		return null;
	}

	static bool IsInPlane(IEntity user)
	{
		if (!user)
			return false;

		// La session n'existe que sur le serveur. Le client reconnaît l'avion à l'altitude.
		if (user.GetOrigin()[1] > 800)
			return true;

		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return false;

		int playerId = manager.GetPlayerIdFromControlledEntity(user);
		HAM_ParadropSession session = FindSession(playerId);
		if (!session)
			return false;

		return session.m_iState == HAM_EParadropState.PLANE;
	}

	//! Freefall / TP sortie : bloque les dégâts de chute natifs (coma).
	static bool IsDropProtected(IEntity user)
	{
		if (!user)
			return false;

		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return false;

		int playerId = manager.GetPlayerIdFromControlledEntity(user);
		HAM_ParadropSession session = FindSession(playerId);
		if (!session)
			return false;

		return session.m_iState == HAM_EParadropState.WAIT_JUMP
			|| session.m_iState == HAM_EParadropState.FALLING
			|| session.m_iState == HAM_EParadropState.ON_GROUND
			|| session.m_iState == HAM_EParadropState.RESTORING;
	}

	//! victim : corps du kill (GetVictimEntity) si GetPlayerControlledEntity est déjà null.
	static void Abandon(int playerId, IEntity victim = null)
	{
		if (!Replication.IsServer())
			return;

		HAM_ParadropSession session = FindSession(playerId);
		if (!session)
			return;

		// Invalide Release/MoveToPlane/CompleteFinish encore en queue (epoch).
		CancelPending(playerId);
		Engine(playerId, false);

		// Corps vivant OU cadavre : EndFreeFall coupe FX (écran / vent) + graph
		// même si m_bFreeFalling est déjà false (ACT_END partiel).
		IEntity body = BodyOf(playerId);
		if (!body)
			body = victim;

		if (body)
		{
			HAM_Para_FreeFallComponent freefall = HAM_Para_FreeFallComponent.Cast(body.FindComponent(HAM_Para_FreeFallComponent));
			if (freefall)
				freefall.EndFreeFall();
		}

		IEntity living = LivingBody(playerId);
		if (living)
			HAM_ParadropBagStore.RestoreToCharacter(playerId, living);
		else
			HAM_ParadropBagStore.Delete(playerId);

		if (session.m_Chute)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(session.m_Chute);
			session.m_Chute = null;
		}

		session.m_iState = HAM_EParadropState.NONE;
		s_aSessions.RemoveItem(session);

		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (player)
			player.HAM_ParadropClearPick();
	}

	protected static int Stamp(HAM_ParadropSession session)
	{
		session.m_iEpoch = session.m_iEpoch + 1;
		return session.m_iEpoch;
	}

	protected static void CancelPending(int playerId)
	{
		HAM_ParadropSession session = FindSession(playerId);
		if (session)
			session.m_iEpoch = session.m_iEpoch + 1;
	}

	static void Request(int playerId, float x, float z)
	{
		if (!Replication.IsServer() || !s_Instance)
			return;

		float ground = GroundAt(x, z);
		if (!SCR_Global.IsPositionWithinTerrainBounds(Vector(x, ground + 0.2, z)))
			return;

		// Nouvelle demande : toujours repartir propre (mort, saut coincé, 2e tentative).
		if (FindSession(playerId))
			Abandon(playerId);

		if (!s_Plane)
			s_Instance.SpawnPlane();

		if (!s_Plane)
			return;

		IEntity body = LivingBody(playerId);
		if (!body)
			return;

		if (!body.FindComponent(HAM_Para_FreeFallComponent))
			return;

		HAM_ParadropSession session = new HAM_ParadropSession(playerId);
		s_aSessions.Insert(session);
		session.m_fX = x;
		session.m_fZ = z;
		if (!ParkBackpack(body, playerId))
		{
			s_aSessions.RemoveItem(session);
			return;
		}

		session.m_Chute = EquipChute(body);
		if (!session.m_Chute)
		{
			HAM_ParadropBagStore.RestoreToCharacter(playerId, body);
			s_aSessions.RemoveItem(session);
			return;
		}

		session.m_iState = HAM_EParadropState.PLANE;
		GetGame().GetCallqueue().CallLater(HAM_ParadropMoveToPlane, 400, false, playerId, Stamp(session));
	}

	static void Jump(int playerId)
	{
		if (!Replication.IsServer())
			return;

		HAM_ParadropSession session = FindSession(playerId);
		if (!session || session.m_iState != HAM_EParadropState.PLANE)
			return;

		session.m_iState = HAM_EParadropState.WAIT_JUMP;
		Engine(playerId, false);
		GetGame().GetCallqueue().CallLater(HAM_ParadropRelease, JUMP_DELAY_MS, false, playerId, Stamp(session));
	}

	static void Release(int playerId, int epoch)
	{
		if (!Replication.IsServer())
			return;

		HAM_ParadropSession session = FindSession(playerId);
		if (!session || session.m_iEpoch != epoch || session.m_iState != HAM_EParadropState.WAIT_JUMP)
			return;

		float ground = GroundAt(session.m_fX, session.m_fZ);
		session.m_iState = HAM_EParadropState.FALLING;
		IEntity body = BodyOf(playerId);
		if (!body)
		{
			Abandon(playerId);
			return;
		}

		// Engager le freefall AVANT le TP : sinon le moteur inflige le coma à l'arrivée.
		HAM_Para_FreeFallComponent chute = HAM_Para_FreeFallComponent.Cast(body.FindComponent(HAM_Para_FreeFallComponent));
		if (chute)
			chute.HAM_Para_BeginDrop();

		MovePlayer(playerId, Vector(session.m_fX, ground + DROP_HEIGHT, session.m_fZ));
		Physics phys = body.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(Vector(0, -2, 0));
			phys.SetAngularVelocity(vector.Zero);
		}
	}

	protected static void Tick()
	{
		if (!Replication.IsServer() || !s_aSessions)
			return;

		float now = GetGame().GetWorld().GetWorldTime();
		for (int i = s_aSessions.Count() - 1; i >= 0; i--)
		{
			HAM_ParadropSession session = s_aSessions[i];
			if (!session)
				continue;

			if (session.m_iState == HAM_EParadropState.NONE)
			{
				s_aSessions.Remove(i);
				continue;
			}

			IEntity body = LivingBody(session.m_iPlayerId);
			if (!body)
			{
				Abandon(session.m_iPlayerId);
				continue;
			}

			if (session.m_iState == HAM_EParadropState.FALLING && HasLanded(body))
			{
				session.m_iState = HAM_EParadropState.ON_GROUND;
				session.m_fGroundAt = now;
			}

			if (session.m_iState == HAM_EParadropState.ON_GROUND && now >= session.m_fGroundAt + GROUND_DELAY_MS)
				BeginFinish(session);
		}
	}

	protected static IEntity LivingBody(int playerId)
	{
		IEntity body = BodyOf(playerId);
		if (!body)
			return null;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (controller && controller.IsDead())
			return null;

		return body;
	}

	protected static void BeginFinish(HAM_ParadropSession session)
	{
		IEntity body = LivingBody(session.m_iPlayerId);
		if (!body)
		{
			Abandon(session.m_iPlayerId);
			return;
		}

		session.m_iState = HAM_EParadropState.RESTORING;
		if (session.m_Chute)
		{
			SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
			if (manager)
				manager.TryRemoveItemFromInventory(session.m_Chute);

			SCR_EntityHelper.DeleteEntityAndChildren(session.m_Chute);
			session.m_Chute = null;
		}

		GetGame().GetCallqueue().CallLater(HAM_ParadropCompleteFinish, 500, false, session.m_iPlayerId, Stamp(session));
	}

	static void CompleteFinish(int playerId, int epoch)
	{
		if (!Replication.IsServer())
			return;

		HAM_ParadropSession session = FindSession(playerId);
		if (!session || session.m_iEpoch != epoch || session.m_iState != HAM_EParadropState.RESTORING)
			return;

		CancelPending(playerId);
		IEntity body = LivingBody(playerId);
		if (body)
			HAM_ParadropBagStore.RestoreToCharacter(playerId, body);
		else
			HAM_ParadropBagStore.Delete(playerId);

		session.m_iState = HAM_EParadropState.NONE;
		s_aSessions.RemoveItem(session);
		Engine(playerId, false);
	}

	protected static bool ParkBackpack(IEntity body, int playerId)
	{
		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(body.FindComponent(EquipedLoadoutStorageComponent));
		if (!loadout)
			return true;

		IEntity backpack = loadout.GetClothFromArea(LoadoutBackpackArea);
		if (!backpack)
		{
			HAM_ParadropBagStore.Delete(playerId);
			return true;
		}

		if (!HAM_ParadropBagStore.SaveFromCharacter(playerId, body, backpack))
			return false;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		if (!manager || !manager.TryRemoveItemFromInventory(backpack))
		{
			HAM_ParadropBagStore.Delete(playerId);
			return false;
		}

		SCR_EntityHelper.DeleteEntityAndChildren(backpack);
		return true;
	}

	protected static IEntity EquipChute(IEntity body)
	{
		Resource resource = Resource.Load(CHUTE);
		if (!resource || !resource.IsValid())
			return null;

		EntitySpawnParams params = new EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		params.Transform[3] = body.GetOrigin();
		IEntity chute = GetGame().SpawnEntityPrefab(resource, body.GetWorld(), params);
		if (!chute)
			return null;

		if (!Wear(body, chute))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(chute);
			return null;
		}

		return chute;
	}

	protected static bool Wear(IEntity body, IEntity item)
	{
		if (!body || !item)
			return false;

		SCR_InventoryStorageManagerComponent manager = SCR_InventoryStorageManagerComponent.Cast(body.FindComponent(SCR_InventoryStorageManagerComponent));
		EquipedLoadoutStorageComponent loadout = EquipedLoadoutStorageComponent.Cast(body.FindComponent(EquipedLoadoutStorageComponent));
		if (!manager || !loadout)
			return false;

		return manager.EquipAny(loadout, item);
	}

	protected static float GroundAt(float x, float z)
	{
		BaseWorld world = GetGame().GetWorld();
		if (!world)
			return 0;

		return world.GetSurfaceY(x, z);
	}

	protected static bool HasLanded(IEntity body)
	{
		CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (!controller)
			return false;

		if (controller.IsFalling())
			return false;

		vector origin = body.GetOrigin();
		float ground = GroundAt(origin[0], origin[2]);
		return origin[1] - ground < 2;
	}

	protected static IEntity BodyOf(int playerId)
	{
		PlayerManager manager = GetGame().GetPlayerManager();
		if (!manager)
			return null;

		return manager.GetPlayerControlledEntity(playerId);
	}

	static void MoveToPlane(int playerId, int epoch)
	{
		if (!s_Plane)
			return;

		HAM_ParadropSession session = FindSession(playerId);
		if (!session || session.m_iEpoch != epoch || session.m_iState != HAM_EParadropState.PLANE)
			return;

		MovePlayer(playerId, PlaneCenter());
		Engine(playerId, true);
	}

	protected static void MovePlayer(int playerId, vector position)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (!player)
			return;

		player.HAM_ParadropMove(position[0], position[1], position[2]);
	}

	protected static void Engine(int playerId, bool on)
	{
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerManager().GetPlayerController(playerId));
		if (player)
			player.HAM_ParadropEngine(on);
	}
}

class HAM_ParadropPick
{
	protected static bool s_bPicking;
	protected static bool s_bHasPoint;
	protected static bool s_bConfirmOpen;
	protected static float s_fBeganAt;
	protected static float s_fX;
	protected static float s_fZ;

	static void Begin()
	{
		Cancel();
		s_bPicking = true;
		s_fBeganAt = GetGame().GetWorld().GetWorldTime();
		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	static bool IsPicking()
	{
		return s_bPicking;
	}

	static void Cancel()
	{
		s_bPicking = false;
		s_bHasPoint = false;
		s_bConfirmOpen = false;
	}

	//! Ignore un ClearPick serveur retardé après un nouveau Begin (mort puis re-saut).
	static void CancelStale()
	{
		if (s_bPicking && GetGame().GetWorld().GetWorldTime() - s_fBeganAt < 2500)
			return;

		Cancel();
	}

	static bool IsConfirmOpen()
	{
		return s_bConfirmOpen;
	}

	static void OpenConfirm()
	{
		if (!s_bPicking || !s_bHasPoint || s_bConfirmOpen)
			return;

		s_bConfirmOpen = true;
		new HAM_ParadropConfirmDialog();
	}

	static void CloseConfirm()
	{
		s_bConfirmOpen = false;
	}

	static void ReopenMap()
	{
		if (!s_bPicking)
			return;

		MenuManager menus = GetGame().GetMenuManager();
		if (!menus || menus.FindMenuByPreset(ChimeraMenuPreset.MapMenu))
			return;

		menus.OpenMenu(ChimeraMenuPreset.MapMenu);
	}

	static void Take(float x, float z)
	{
		s_fX = x;
		s_fZ = z;
		s_bHasPoint = true;
	}

	static bool HasPoint()
	{
		return s_bHasPoint;
	}

	static void Confirm()
	{
		if (!s_bPicking || !s_bHasPoint)
			return;

		float x = s_fX;
		float z = s_fZ;
		s_bPicking = false;
		s_bConfirmOpen = false;
		s_bHasPoint = false;
		SCR_PlayerController player = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		if (player)
			player.HAM_ParadropAsk(x, z);

		MenuManager menus = GetGame().GetMenuManager();
		if (menus)
			menus.CloseMenuByPreset(ChimeraMenuPreset.MapMenu);
	}

	static string PointText()
	{
		if (!s_bHasPoint)
			return "Clique sur la carte.";

		return "Point " + Math.Round(s_fX).ToString() + " / " + Math.Round(s_fZ).ToString();
	}
}

modded class SCR_PlayerController
{
	void HAM_ParadropAsk(float x, float z)
	{
		if (Replication.IsServer())
			HAM_ParadropComponent.Request(GetPlayerId(), x, z);
		else
			Rpc(HAM_RpcParadropAsk, x, z);
	}

	void HAM_ParadropJump()
	{
		if (Replication.IsServer())
			HAM_ParadropComponent.Jump(GetPlayerId());
		else
			Rpc(HAM_RpcParadropJump);
	}

	void HAM_ParadropMove(float x, float y, float z)
	{
		IEntity body = GetControlledEntity();
		RplComponent replication = null;
		if (body)
			replication = RplComponent.Cast(body.FindComponent(RplComponent));

		bool owner = !replication || replication.IsOwner();
		if (owner)
			HAM_DoParadropMove(x, y, z);
		else
			Rpc(HAM_RpcParadropMove, x, y, z);
	}

	void HAM_ParadropEngine(bool on)
	{
		Rpc(HAM_RpcParadropEngine, on);
		if (SCR_PlayerController.GetLocalPlayerId() == GetPlayerId())
			HAM_ParadropAudio.Set(on);
	}

	void HAM_ParadropClearPick()
	{
		Rpc(HAM_RpcParadropClearPick);
		if (SCR_PlayerController.GetLocalPlayerId() == GetPlayerId())
			HAM_ParadropPick.CancelStale();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcParadropAsk(float x, float z)
	{
		HAM_ParadropComponent.Request(GetPlayerId(), x, z);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcParadropClearPick()
	{
		HAM_ParadropPick.CancelStale();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcParadropJump()
	{
		HAM_ParadropComponent.Jump(GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcParadropMove(float x, float y, float z)
	{
		HAM_DoParadropMove(x, y, z);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcParadropEngine(bool on)
	{
		HAM_ParadropAudio.Set(on);
	}

	protected void HAM_DoParadropMove(float x, float y, float z)
	{
		IEntity body = GetControlledEntity();
		if (!body)
			return;

		vector transform[4];
		body.GetWorldTransform(transform);
		transform[3] = Vector(x, y, z);

		BaseGameEntity character = BaseGameEntity.Cast(body);
		if (character)
			character.Teleport(transform);
		else
			body.SetWorldTransform(transform);

		Physics phys = body.GetPhysics();
		if (phys)
		{
			phys.SetVelocity(vector.Zero);
			phys.SetAngularVelocity(vector.Zero);
		}
	}
}
