//------------------------------------------------------------------------------------------------
// RPC staff du tableau véhicules.
//------------------------------------------------------------------------------------------------

modded class SCR_PlayerController
{
	void HAM_VehicleBoardSyncHidden(string pack)
	{
		Rpc(HAM_RpcVehicleBoardHidden, pack);
	}

	void HAM_VehicleBoardSyncShown(string prefab, bool shown)
	{
		Rpc(HAM_RpcVehicleBoardShown, prefab, shown);
	}

	void HAM_VehicleBoardSyncParams(string costs, string ranks, string cargos)
	{
		Rpc(HAM_RpcVehicleBoardParams, costs, ranks, cargos);
	}

	void HAM_VehicleBoardAskSetShown(string prefab, bool shown)
	{
		if (Replication.IsServer())
		{
			HAM_VehicleBoardSetShownServer(prefab, shown);
			return;
		}

		Rpc(HAM_VehicleBoardSetShownServer, prefab, shown);
	}

	void HAM_VehicleBoardAskApplyParams(string prefab, int cost, int rank, float cargo)
	{
		if (Replication.IsServer())
		{
			HAM_VehicleBoardApplyParamsServer(prefab, cost, rank, cargo);
			return;
		}

		Rpc(HAM_VehicleBoardApplyParamsServer, prefab, cost, rank, cargo);
	}

	void HAM_VehicleBoardAskResetParams(string prefab)
	{
		if (Replication.IsServer())
		{
			HAM_VehicleBoardResetParamsServer(prefab);
			return;
		}

		Rpc(HAM_VehicleBoardResetParamsServer, prefab);
	}

	void HAM_VehicleBoardAskSync()
	{
		if (Replication.IsServer())
		{
			HAM_VehicleBoardStore.SyncToPlayer(GetPlayerId());
			return;
		}

		Rpc(HAM_VehicleBoardAskSyncServer);
	}

	void HAM_VehiclePlaceAsk(string prefab, int mode, float x, float y, float z, float yaw)
	{
		if (Replication.IsServer())
		{
			HAM_VehiclePlaceServer(prefab, mode, x, y, z, yaw);
			return;
		}

		Rpc(HAM_RpcVehiclePlace, prefab, mode, x, y, z, yaw);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcVehicleBoardHidden(string pack)
	{
		HAM_VehicleBoardStore.ApplyHiddenPack(pack);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcVehicleBoardShown(string prefab, bool shown)
	{
		HAM_VehicleBoardStore.ApplyShown(prefab, shown);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcVehicleBoardParams(string costs, string ranks, string cargos)
	{
		HAM_VehicleBoardStore.ApplyParamsPack(costs, ranks, cargos);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_VehicleBoardSetShownServer(string prefab, bool shown)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_VehicleBoardStore.SetShown(prefab, shown);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_VehicleBoardApplyParamsServer(string prefab, int cost, int rank, float cargo)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_VehicleBoardStore.ApplyVehicleParams(prefab, cost, rank, cargo);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_VehicleBoardResetParamsServer(string prefab)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_VehicleBoardStore.ClearAllOverrides(prefab);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_VehicleBoardAskSyncServer()
	{
		HAM_VehicleBoardStore.SyncToPlayer(GetPlayerId());
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_RpcVehiclePlace(string prefab, int mode, float x, float y, float z, float yaw)
	{
		HAM_VehiclePlaceServer(prefab, mode, x, y, z, yaw);
	}

	protected void HAM_VehiclePlaceServer(string prefab, int mode, float x, float y, float z, float yaw)
	{
		HAM_VehicleSpawnUI.SpawnConfirmed(prefab, mode, x, y, z, yaw);
	}
}
