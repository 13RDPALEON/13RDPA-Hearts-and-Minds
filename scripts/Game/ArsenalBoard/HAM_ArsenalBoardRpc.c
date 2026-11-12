//------------------------------------------------------------------------------------------------
// RPC staff Flipchart arsenal.
//------------------------------------------------------------------------------------------------

modded class SCR_PlayerController
{
	void HAM_ArsenalBoardSyncHidden(string pack)
	{
		Rpc(HAM_RpcArsenalBoardHidden, pack);
	}

	void HAM_ArsenalBoardSyncShown(string prefab, bool shown)
	{
		Rpc(HAM_RpcArsenalBoardShown, prefab, shown);
	}

	void HAM_ArsenalBoardAskFlush()
	{
		if (Replication.IsServer())
		{
			HAM_ArsenalBoardStore.FlushPending();
			return;
		}

		Rpc(HAM_ArsenalBoardFlushServer);
	}

	void HAM_ArsenalBoardSyncParams(string costs, string ranks, string bags)
	{
		Rpc(HAM_RpcArsenalBoardParams, costs, ranks, bags);
	}

	void HAM_ArsenalBoardAskSetShown(string prefab, bool shown)
	{
		if (Replication.IsServer())
		{
			HAM_ArsenalBoardSetShownServer(prefab, shown);
			return;
		}

		Rpc(HAM_ArsenalBoardSetShownServer, prefab, shown);
	}

	void HAM_ArsenalBoardAskApplyParams(string prefab, int cost, int rank, float bag, bool setBag)
	{
		if (Replication.IsServer())
		{
			HAM_ArsenalBoardApplyParamsServer(prefab, cost, rank, bag, setBag);
			return;
		}

		Rpc(HAM_ArsenalBoardApplyParamsServer, prefab, cost, rank, bag, setBag);
	}

	void HAM_ArsenalBoardAskResetParams(string prefab)
	{
		if (Replication.IsServer())
		{
			HAM_ArsenalBoardResetParamsServer(prefab);
			return;
		}

		Rpc(HAM_ArsenalBoardResetParamsServer, prefab);
	}

	void HAM_ArsenalBoardAskSync()
	{
		if (Replication.IsServer())
		{
			HAM_ArsenalBoardStore.SyncToPlayer(GetPlayerId());
			return;
		}

		Rpc(HAM_ArsenalBoardAskSyncServer);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcArsenalBoardHidden(string pack)
	{
		HAM_ArsenalBoardStore.ApplyHiddenPack(pack);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcArsenalBoardShown(string prefab, bool shown)
	{
		HAM_ArsenalBoardStore.ApplyShown(prefab, shown);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArsenalBoardFlushServer()
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_ArsenalBoardStore.FlushPending();
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcArsenalBoardParams(string costs, string ranks, string bags)
	{
		HAM_ArsenalBoardStore.ApplyParamsPack(costs, ranks, bags);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArsenalBoardSetShownServer(string prefab, bool shown)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_ArsenalBoardStore.SetShown(prefab, shown);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArsenalBoardApplyParamsServer(string prefab, int cost, int rank, float bag, bool setBag)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_ArsenalBoardStore.ApplyItemParams(prefab, cost, rank, bag, setBag);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArsenalBoardResetParamsServer(string prefab)
	{
		if (!HAM_ArsenalBoardStore.IsStaff(GetPlayerId()))
			return;

		HAM_ArsenalBoardStore.ClearAllOverrides(prefab);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void HAM_ArsenalBoardAskSyncServer()
	{
		HAM_ArsenalBoardStore.SyncToPlayer(GetPlayerId());
	}
}
