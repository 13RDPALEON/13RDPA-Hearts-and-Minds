//------------------------------------------------------------------------------------------------
// Posé sur les prefabs de zone (maître, esclave, dépôt, logistique du QG).
// Le serveur y recopie le nom, le camp et les stocks ; les clients les lisent pour les actions.
//------------------------------------------------------------------------------------------------

class HAM_ZoneComponentClass : ScriptComponentClass
{
}

class HAM_ZoneComponent : ScriptComponent
{
	[RplProp()]
	protected string m_sZoneName;

	[RplProp()]
	protected int m_iRole = -1;

	[RplProp()]
	protected bool m_bPlayer;

	[RplProp()]
	protected int m_iWater;

	[RplProp()]
	protected int m_iFood;

	[RplProp()]
	protected int m_iAmmo;

	[RplProp()]
	protected int m_iMen;

	protected static ref array<HAM_ZoneComponent> s_aAll;

	//------------------------------------------------------------------------------------------------
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!s_aAll)
			s_aAll = {};

		if (s_aAll.Find(this) < 0)
			s_aAll.Insert(this);
	}

	//------------------------------------------------------------------------------------------------
	override void OnDelete(IEntity owner)
	{
		if (s_aAll)
			s_aAll.RemoveItem(this);

		super.OnDelete(owner);
	}

	//------------------------------------------------------------------------------------------------
	//! Retire les caisses de rations (maître, logistique du QG). Leurs actions sont sur le tableau.
	static void DeleteRole(int role)
	{
		if (!s_aAll)
			return;

		array<IEntity> owners = {};
		foreach (HAM_ZoneComponent zone : s_aAll)
		{
			if (zone && zone.m_iRole == role && zone.GetOwner())
				owners.Insert(zone.GetOwner());
		}

		foreach (IEntity owner : owners)
		{
			HAM_Util.Delete(owner);
		}
	}

	//------------------------------------------------------------------------------------------------
	static HAM_ZoneComponent Find(string zoneName, int role)
	{
		if (!s_aAll)
			return null;

		foreach (HAM_ZoneComponent zone : s_aAll)
		{
			if (zone && zone.m_iRole == role && zone.m_sZoneName == zoneName)
				return zone;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Zone du rôle demandé la plus proche, dans le rayon.
	static HAM_ZoneComponent FindNear(vector pos, float radius, int role)
	{
		if (!s_aAll)
			return null;

		HAM_ZoneComponent best;
		float bestSq = radius * radius;
		foreach (HAM_ZoneComponent zone : s_aAll)
		{
			if (!zone || zone.m_iRole != role || !zone.GetOwner())
				continue;

			float distSq = vector.DistanceSq(zone.GetOwner().GetOrigin(), pos);
			if (distSq > bestSq)
				continue;

			bestSq = distSq;
			best = zone;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	string GetZoneName()
	{
		return m_sZoneName;
	}

	int GetRole()
	{
		return m_iRole;
	}

	bool IsPlayer()
	{
		return m_bPlayer;
	}

	int GetWater()
	{
		return m_iWater;
	}

	int GetFood()
	{
		return m_iFood;
	}

	int GetAmmo()
	{
		return m_iAmmo;
	}

	int GetMen()
	{
		return m_iMen;
	}

	//------------------------------------------------------------------------------------------------
	void SetZone(string zoneName, int role)
	{
		if (m_sZoneName == zoneName && m_iRole == role)
			return;

		m_sZoneName = zoneName;
		m_iRole = role;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void SetPlayer(bool player)
	{
		if (m_bPlayer == player)
			return;

		m_bPlayer = player;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	void SetStocks(float water, float food, float ammo, int men)
	{
		int water2 = Math.Round(water);
		int food2 = Math.Round(food);
		int ammo2 = Math.Round(ammo);
		if (water2 == m_iWater && food2 == m_iFood && ammo2 == m_iAmmo && men == m_iMen)
			return;

		m_iWater = water2;
		m_iFood = food2;
		m_iAmmo = ammo2;
		m_iMen = men;
		Replication.BumpMe();
	}

	//------------------------------------------------------------------------------------------------
	//! Réplique même si les chiffres sont déjà ceux du serveur : le premier envoi peut s'être perdu.
	void PushStocks(float water, float food, float ammo, int men)
	{
		m_iWater = Math.Round(water);
		m_iFood = Math.Round(food);
		m_iAmmo = Math.Round(ammo);
		m_iMen = men;
		Replication.BumpMe();
	}
}
