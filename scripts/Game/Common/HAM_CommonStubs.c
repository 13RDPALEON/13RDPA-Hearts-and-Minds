//------------------------------------------------------------------------------------------------
// Utilitaires partagés (identifiants réseau et rayon du dépôt).
//------------------------------------------------------------------------------------------------

class HAM_RplUtil
{
	static RplId IdOf(IEntity ent)
	{
		RplId none;
		if (!ent)
			return none;

		RplComponent rpl = RplComponent.Cast(ent.FindComponent(RplComponent));
		if (!rpl)
			return none;

		return rpl.Id();
	}
}

class HAM_SpawnConfig
{
	protected static ref HAM_SpawnConfig s_Instance;
	protected float m_fDepotRadius;

	static HAM_SpawnConfig Get()
	{
		if (!s_Instance)
		{
			s_Instance = new HAM_SpawnConfig();
			s_Instance.m_fDepotRadius = 80.0;
		}

		return s_Instance;
	}

	float DepotRadius()
	{
		return m_fDepotRadius;
	}

	void SetDepotRadius(float value)
	{
		m_fDepotRadius = Math.Clamp(value, 5.0, 200.0);
	}

	bool ApplyVehicleDepotJson(JsonLoadContext ctx)
	{
		if (!ctx.StartObject("vehicleDepot"))
			return false;

		float f;
		if (ctx.ReadValue("radiusMeters", f))
			SetDepotRadius(f);

		ctx.EndObject();
		return true;
	}
}

modded class SCR_PlayerController
{
	void HAM_ShowAdminNote(string text)
	{
		Rpc(HAM_RpcAdminNote, text);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void HAM_RpcAdminNote(string text)
	{
		SCR_HintManagerComponent.ShowCustomHint(text, "13RDPA H&M", 8);
	}
}
