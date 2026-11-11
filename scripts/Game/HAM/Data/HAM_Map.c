//------------------------------------------------------------------------------------------------
// Lignes d'un script de carte, comme define_mod.sqf : un nom, une position, un rôle.
// Un maître nomme les cellules collées (séparées par ";"). Un esclave, un dépôt, une patrouille,
// un CQB, une ligne IED ou mines nomme son maître.
//------------------------------------------------------------------------------------------------

enum HAM_ERole
{
	HQ,
	ENEMY_HQ,
	MASTER,
	SLAVE,
	DEPOSIT,
	PATROL,
	CQB,
	IED,
	MINES,
	SUPPLY,
	CIVILIANS
}

enum HAM_EMasterType
{
	FUEL,
	RADIO,
	CAMP,
	HIDEOUT,
	CITY,
	BASE,
	SUPPLY_VANILLA,
	SUPPLY_PERSONAL
}

class HAM_MapPoint
{
	string m_sName;
	HAM_ERole m_eRole;
	HAM_EMasterType m_eType;
	vector m_vPos;
	float m_fRadius;
	string m_sCell;
	ref array<string> m_aNeighbors = {};

	//------------------------------------------------------------------------------------------------
	bool IsCellMaster()
	{
		return m_eRole == HAM_ERole.MASTER || m_eRole == HAM_ERole.ENEMY_HQ;
	}

	//------------------------------------------------------------------------------------------------
	bool IsCapturable()
	{
		return m_eRole == HAM_ERole.MASTER || m_eRole == HAM_ERole.ENEMY_HQ || m_eRole == HAM_ERole.SLAVE;
	}

	//------------------------------------------------------------------------------------------------
	string TypeLabel()
	{
		if (m_eRole == HAM_ERole.ENEMY_HQ)
			return "QG ennemi";

		if (m_eRole == HAM_ERole.HQ)
			return "QG";

		if (m_eRole == HAM_ERole.SLAVE && m_eType != HAM_EMasterType.SUPPLY_VANILLA && m_eType != HAM_EMasterType.SUPPLY_PERSONAL)
			return "Esclave";

		if (m_eRole == HAM_ERole.DEPOSIT)
			return "Dépôt";

		if (m_eRole == HAM_ERole.SUPPLY)
			return "Ressources";

		switch (m_eType)
		{
			case HAM_EMasterType.FUEL:
				return "Carburant";
			case HAM_EMasterType.RADIO:
				return "Radio";
			case HAM_EMasterType.CAMP:
				return "Camp";
			case HAM_EMasterType.HIDEOUT:
				return "Planque";
			case HAM_EMasterType.CITY:
				return "Ville";
			case HAM_EMasterType.BASE:
				return "Base";
			case HAM_EMasterType.SUPPLY_VANILLA:
				return "Ressources";
			case HAM_EMasterType.SUPPLY_PERSONAL:
				return "Dépôt";
		}

		return "Zone";
	}
}

class HAM_MapDef
{
	string m_sKey;
	ref array<ref HAM_MapPoint> m_aPoints = {};

	//------------------------------------------------------------------------------------------------
	protected HAM_MapPoint Add(string name, HAM_ERole role, vector pos, float radius, string cell)
	{
		HAM_MapPoint point = new HAM_MapPoint();
		point.m_sName = name;
		point.m_eRole = role;
		point.m_eType = HAM_EMasterType.BASE;
		point.m_vPos = pos;
		point.m_fRadius = radius;
		point.m_sCell = cell;
		m_aPoints.Insert(point);
		return point;
	}

	//------------------------------------------------------------------------------------------------
	protected static void SplitNames(string packed, notnull array<string> names)
	{
		array<string> tokens = {};
		packed.Split(";", tokens, true);
		foreach (string token : tokens)
		{
			string clean = token.Trim();
			if (!clean.IsEmpty())
				names.Insert(clean);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! QG de départ du camp joueur. Le prefab de base Conflit le plus proche devient le QG.
	void Hq(string name, vector pos, float radius)
	{
		Add(name, HAM_ERole.HQ, pos, radius, name);
	}

	//------------------------------------------------------------------------------------------------
	//! Base principale ennemie : dernière cellule de la chaîne. Sa prise entière termine la partie.
	void EnemyHq(string name, vector pos, float radius, string neighbors)
	{
		HAM_MapPoint point = Add(name, HAM_ERole.ENEMY_HQ, pos, radius, name);
		SplitNames(neighbors, point.m_aNeighbors);
	}

	//------------------------------------------------------------------------------------------------
	//! Maître d'une cellule. neighbors : noms des maîtres collés, plus "QG" si la cellule touche le QG.
	void Master(string name, HAM_EMasterType type, vector pos, float radius, string neighbors)
	{
		HAM_MapPoint point = Add(name, HAM_ERole.MASTER, pos, radius, name);
		point.m_eType = type;
		SplitNames(neighbors, point.m_aNeighbors);
	}

	//------------------------------------------------------------------------------------------------
	//! type : FUEL, CAMP, HIDEOUT ou BASE. Sans type, l'esclave reste un point de combat.
	void Slave(string name, string master, vector pos, float radius, HAM_EMasterType type = HAM_EMasterType.CITY)
	{
		HAM_MapPoint point = Add(name, HAM_ERole.SLAVE, pos, radius, master);
		point.m_eType = type;
	}

	//------------------------------------------------------------------------------------------------
	void Deposit(string name, string master, vector pos)
	{
		Add(name, HAM_ERole.DEPOSIT, pos, 40, master);
	}

	//------------------------------------------------------------------------------------------------
	void Patrol(string name, string master, vector pos, float radius)
	{
		Add(name, HAM_ERole.PATROL, pos, radius, master);
	}

	//------------------------------------------------------------------------------------------------
	void Cqb(string name, string master, vector pos, float radius)
	{
		Add(name, HAM_ERole.CQB, pos, radius, master);
	}

	//------------------------------------------------------------------------------------------------
	void Ied(string name, string master, vector pos, float radius)
	{
		Add(name, HAM_ERole.IED, pos, radius, master);
	}

	//------------------------------------------------------------------------------------------------
	void Mines(string name, string master, vector pos, float radius)
	{
		Add(name, HAM_ERole.MINES, pos, radius, master);
	}

	//------------------------------------------------------------------------------------------------
	//! Civils en plus, au même titre qu'une patrouille : ils marchent dans le rayon quand un joueur approche.
	void Civilians(string name, string master, vector pos, float radius)
	{
		Add(name, HAM_ERole.CIVILIANS, pos, radius, master);
	}

	//------------------------------------------------------------------------------------------------
	//! Zone de ressources vanilla du Conflit : le prefab CampaignRemnantsSupplyDepot, hors eau / nourriture / munitions.
	void Supply(string name, vector pos)
	{
		Add(name, HAM_ERole.SUPPLY, pos, 20, name);
	}

	//------------------------------------------------------------------------------------------------
	HAM_MapPoint Find(string name)
	{
		foreach (HAM_MapPoint point : m_aPoints)
		{
			if (point.m_sName == name)
				return point;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	HAM_MapPoint FindRole(HAM_ERole role)
	{
		foreach (HAM_MapPoint point : m_aPoints)
		{
			if (point.m_eRole == role)
				return point;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Retire les lignes qui citent un maître absent, avec une ligne de log.
	void Validate()
	{
		for (int i = m_aPoints.Count() - 1; i >= 0; i--)
		{
			HAM_MapPoint point = m_aPoints[i];
			if (point.m_eRole == HAM_ERole.HQ || point.m_eRole == HAM_ERole.SUPPLY || point.IsCellMaster())
				continue;

			HAM_MapPoint master = Find(point.m_sCell);
			if (master && master.IsCellMaster())
				continue;

			Print(string.Format("[13RDPA H&M] Carte %1 : ligne %2 ignorée, maître %3 absent", m_sKey, point.m_sName, point.m_sCell), LogLevel.WARNING);
			m_aPoints.Remove(i);
		}
	}
}
