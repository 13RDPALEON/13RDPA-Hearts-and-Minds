//------------------------------------------------------------------------------------------------
// État du front côté serveur : cellules, points prenables, dépôts, lignes de contenu, stocks.
//------------------------------------------------------------------------------------------------

enum HAM_EResource
{
	WATER,
	FOOD,
	AMMO,
	MEN,
	SUPPLIES
}

//------------------------------------------------------------------------------------------------
class HAM_Stock
{
	float m_fWater;
	float m_fFood;
	float m_fAmmo;
	int m_iMen;

	//------------------------------------------------------------------------------------------------
	void Clear()
	{
		m_fWater = 0;
		m_fFood = 0;
		m_fAmmo = 0;
		m_iMen = 0;
	}

	//------------------------------------------------------------------------------------------------
	float Get(HAM_EResource kind)
	{
		switch (kind)
		{
			case HAM_EResource.WATER:
				return m_fWater;
			case HAM_EResource.FOOD:
				return m_fFood;
			case HAM_EResource.AMMO:
				return m_fAmmo;
			case HAM_EResource.MEN:
				return m_iMen;
		}

		return 0;
	}

	//------------------------------------------------------------------------------------------------
	void Add(HAM_EResource kind, float amount)
	{
		if (kind == HAM_EResource.WATER)
		{
			m_fWater = Math.Max(0, m_fWater + amount);
			return;
		}

		if (kind == HAM_EResource.FOOD)
		{
			m_fFood = Math.Max(0, m_fFood + amount);
			return;
		}

		if (kind == HAM_EResource.AMMO)
		{
			m_fAmmo = Math.Max(0, m_fAmmo + amount);
			return;
		}

		if (kind == HAM_EResource.MEN)
		{
			int delta = Math.Round(amount);
			int men = m_iMen + delta;
			if (men < 0)
				men = 0;

			m_iMen = men;
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Retire la quantité entière ou rien.
	bool Take(HAM_EResource kind, float amount)
	{
		if (amount <= 0)
			return true;

		if (Get(kind) + 0.001 < amount)
			return false;

		Add(kind, -amount);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static string Label(HAM_EResource kind)
	{
		switch (kind)
		{
			case HAM_EResource.WATER:
				return "eau";
			case HAM_EResource.FOOD:
				return "nourriture";
			case HAM_EResource.AMMO:
				return "munitions";
			case HAM_EResource.MEN:
				return "hommes";
			case HAM_EResource.SUPPLIES:
				return "fournitures";
		}

		return "?";
	}
}

//------------------------------------------------------------------------------------------------
//! Maître, QG ennemi ou esclave : ce qui se prend.
class HAM_Point
{
	HAM_MapPoint m_Def;
	HAM_Cell m_Cell;
	bool m_bPlayer;
	int m_iEnemyLeft = -1;
	float m_fCaptureProgress;
	float m_fRetakeProgress;
	bool m_bActive;
	IEntity m_Prop;
	ref array<SCR_AIGroup> m_aGroups = {};

	// Patrouilles, véhicules et CQB autour du point, comme autour d'une base du Conflit.
	ref array<SCR_AIGroup> m_aPatrols = {};
	ref array<SCR_AIGroup> m_aVehicleCrews = {};
	ref array<IEntity> m_aVehicles = {};
	ref array<SCR_AIGroup> m_aCqb = {};
	bool m_bCqbPlaced;
	bool m_bGarrisonSpawned;
	bool m_bZoneSpawned;
	bool m_bCaptureBar;
	ref array<int> m_aBarPlayers;
	//! -1 : pas de prise datée. Sinon, temps du monde au moment où CE point est passé aux joueurs.
	float m_fCapturedAt = -1;
	ref array<IEntity> m_aTypeProps = {};
	IEntity m_Cache;
	float m_fNextPatrol;
	float m_fNextVehicle;

	//------------------------------------------------------------------------------------------------
	bool IsMaster()
	{
		return m_Def.IsCellMaster();
	}

	//------------------------------------------------------------------------------------------------
	vector Pos()
	{
		return m_Def.m_vPos;
	}

	//------------------------------------------------------------------------------------------------
	float Radius()
	{
		return m_Def.m_fRadius;
	}

	//------------------------------------------------------------------------------------------------
	int FullStrength()
	{
		return HAM_Settings.Get().UnitsOf(m_Def);
	}

	//------------------------------------------------------------------------------------------------
	int Strength()
	{
		if (m_iEnemyLeft < 0)
			return FullStrength();

		return m_iEnemyLeft;
	}
}

//------------------------------------------------------------------------------------------------
class HAM_Deposit
{
	HAM_MapPoint m_Def;
	HAM_Cell m_Cell;
	ref HAM_Stock m_Stock = new HAM_Stock();
	bool m_bRefillWater;
	bool m_bRefillFood;
	bool m_bRefillAmmo;
	float m_fRefillLeft;
	bool m_bActive;
	bool m_bSpawnedOnce;
	int m_iEnemyLeft = -1;
	IEntity m_Prop;
	ref array<SCR_AIGroup> m_aGroups = {};

	//------------------------------------------------------------------------------------------------
	vector Pos()
	{
		return m_Def.m_vPos;
	}

	//------------------------------------------------------------------------------------------------
	int Strength()
	{
		if (m_iEnemyLeft < 0)
			return HAM_Settings.Get().UnitsOf(m_Def);

		return m_iEnemyLeft;
	}
}

//------------------------------------------------------------------------------------------------
//! Ligne patrouille, CQB, IED ou mines : contenu qui naît à l'approche.
class HAM_Line
{
	HAM_MapPoint m_Def;
	HAM_Cell m_Cell;
	bool m_bActive;
	bool m_bSpawnedOnce;
	float m_fNextSpawn;
	ref array<SCR_AIGroup> m_aGroups = {};
	ref array<IEntity> m_aDevices = {};
	ref array<IEntity> m_aProps = {};
	ref array<IEntity> m_aVehicles = {};

	//------------------------------------------------------------------------------------------------
	vector Pos()
	{
		return m_Def.m_vPos;
	}
}

//------------------------------------------------------------------------------------------------
class HAM_Cell
{
	string m_sName;
	HAM_Point m_Master;
	ref array<HAM_Point> m_aSlaves = {};
	ref array<HAM_Deposit> m_aDeposits = {};
	ref array<HAM_Line> m_aLines = {};
	ref array<string> m_aNeighbors = {};
	bool m_bTouchesHq;
	bool m_bEnemyHq;
	bool m_bRevealed;
	int m_iLocalRep;
	ref HAM_Stock m_Stock = new HAM_Stock();
	float m_fGraceLeft = -1;
	float m_fCycleLeft;
	float m_fReinforceLeft;
	float m_fCapturedAt = -1;
	bool m_bFriendlyActive;
	bool m_bArmorPatrol;
	ref array<IEntity> m_aArmor = {};
	ref array<SCR_AIGroup> m_aFriendly = {};
	ref array<IEntity> m_aHqEntities = {};

	//------------------------------------------------------------------------------------------------
	vector Pos()
	{
		return m_Master.m_Def.m_vPos;
	}

	//------------------------------------------------------------------------------------------------
	bool IsMasterPlayer()
	{
		return m_Master && m_Master.m_bPlayer;
	}

	//------------------------------------------------------------------------------------------------
	//! Maître et tous les esclaves aux joueurs.
	bool IsComplete()
	{
		if (!IsMasterPlayer())
			return false;

		foreach (HAM_Point slave : m_aSlaves)
		{
			if (!slave.m_bPlayer)
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	string TypeLabel()
	{
		return m_Master.m_Def.TypeLabel();
	}
}

//------------------------------------------------------------------------------------------------
class HAM_Convoy
{
	int m_iId;
	string m_sFrom;
	string m_sTo;
	int m_iMen;
	float m_fStarted;
	vector m_vTarget;
	bool m_bLost;
	ref array<Vehicle> m_aVehicles = {};
	ref array<SCR_AIGroup> m_aGroups = {};
}
