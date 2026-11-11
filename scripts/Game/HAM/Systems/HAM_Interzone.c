//------------------------------------------------------------------------------------------------
// Vie entre les zones déjà posées. Les distances viennent du plan :
// joueur 400 à 1200 m, zone ennemie 400 m, zone joueur 2000 m pour les patrouilles, entre spawns 400 m.
// Un maître ou un esclave disponible doit être à 1000 m. À 800 m, capturé ou vidé, la paire s'arrête.
// Les civils continuent même si la paire est arrêtée.
//------------------------------------------------------------------------------------------------

class HAM_InterAnchor
{
	string m_sName;
	vector m_vPos;
	float m_fRadius;
	bool m_bCamp;
	bool m_bHeld;
	bool m_bEmptied;
}

class HAM_InterSpot
{
	int m_iKind;
	string m_sPair;
	vector m_vPos;
	bool m_bLive;
	ref array<SCR_AIGroup> m_aGroups = {};
	ref array<IEntity> m_aEntities = {};
	ref array<IEntity> m_aProps = {};
}

class HAM_Interzone
{
	protected static const float PLAYER_MIN = 400;
	protected static const float PLAYER_MAX = 1200;
	protected static const float ENEMY_ZONE = 400;
	protected static const float PLAYER_ZONE = 800;
	protected static const float PATROL_PLAYER_ZONE = 2000;
	protected static const float SEPARATION = 400;
	protected static const float HOST_NEAR = 1000;
	protected static const float HOST_STOP = 800;
	protected static const int KIND_PATROL = 0;
	protected static const int KIND_MINE = 1;
	protected static const int KIND_IED = 2;
	protected static const int KIND_CIVIL = 3;

	protected static ref array<ref HAM_InterAnchor> s_aAnchors;
	protected static ref array<ref HAM_InterSpot> s_aSpots;
	protected static ref array<string> s_aBanned;
	protected static ref array<int> s_aSeenId;
	protected static ref array<vector> s_aSeenPos;

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		if (!Replication.IsServer() || !HAM_Mission.IsRunning())
			return;

		if (!HAM_Mission.s_aPlayerBodies || HAM_Mission.s_aPlayerBodies.IsEmpty())
			return;

		Ensure();
		CollectAnchors();
		RefreshLive();
		BanStopped();

		int playerIndex = Math.RandomInt(0, HAM_Mission.s_aPlayerBodies.Count());
		IEntity body = HAM_Mission.s_aPlayerBodies[playerIndex];
		if (!body)
			return;

		int playerId = 0;
		if (HAM_Mission.s_aPlayerIds && playerIndex < HAM_Mission.s_aPlayerIds.Count())
			playerId = HAM_Mission.s_aPlayerIds[playerIndex];

		int kind = MissingKind(body);
		if (kind < 0)
			return;

		vector pos;
		string pair;
		if (!FindPoint(body, playerId, kind, pos, pair))
			return;

		HAM_InterSpot spot = SpotOf(pair, kind);
		if (!spot)
		{
			spot = new HAM_InterSpot();
			spot.m_iKind = kind;
			spot.m_sPair = pair;
			s_aSpots.Insert(spot);
		}

		if (spot.m_bLive)
			return;

		if (spot.m_vPos != vector.Zero && vector.DistanceXZ(spot.m_vPos, pos) < SEPARATION)
			return;

		if (!SpawnKind(kind, pos, spot))
			return;

		spot.m_vPos = pos;
		spot.m_bLive = true;
		HAM_Util.Info(string.Format("Interzone : %1, paire %2", KindName(kind), pair));
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aAnchors)
			s_aAnchors = {};

		if (!s_aSpots)
			s_aSpots = {};

		if (!s_aBanned)
			s_aBanned = {};

		if (!s_aSeenId)
			s_aSeenId = {};

		if (!s_aSeenPos)
			s_aSeenPos = {};
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectAnchors()
	{
		s_aAnchors.Clear();
		if (HAM_Mission.s_HqPoint)
		{
			HAM_InterAnchor hq = new HAM_InterAnchor();
			hq.m_sName = HAM_Mission.s_HqPoint.m_sName;
			hq.m_vPos = HAM_Mission.s_HqPoint.m_vPos;
			hq.m_fRadius = HAM_Mission.s_HqPoint.m_fRadius;
			hq.m_bCamp = true;
			hq.m_bHeld = true;
			s_aAnchors.Insert(hq);
		}

		if (HAM_Mission.s_aPoints)
		{
			foreach (HAM_Point point : HAM_Mission.s_aPoints)
			{
				if (!point || !point.m_Def)
					continue;

				bool camp = point.IsMaster() || point.m_Def.m_eRole == HAM_ERole.SLAVE;
				if (!camp)
					continue;

				HAM_InterAnchor anchor = new HAM_InterAnchor();
				anchor.m_sName = point.m_Def.m_sName;
				anchor.m_vPos = point.Pos();
				anchor.m_fRadius = point.Radius();
				anchor.m_bCamp = true;
				anchor.m_bHeld = point.m_bPlayer;
				anchor.m_bEmptied = point.m_bGarrisonSpawned && point.m_iEnemyLeft == 0;
				s_aAnchors.Insert(anchor);
			}
		}

		if (!HAM_Mission.s_aLines)
			return;

		foreach (HAM_Line line : HAM_Mission.s_aLines)
		{
			if (!line || !line.m_Def)
				continue;

			HAM_InterAnchor marker = new HAM_InterAnchor();
			marker.m_sName = line.m_Def.m_sName;
			marker.m_vPos = line.Pos();
			marker.m_fRadius = line.m_Def.m_fRadius;
			s_aAnchors.Insert(marker);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void RefreshLive()
	{
		foreach (HAM_InterSpot spot : s_aSpots)
		{
			if (!spot)
				continue;

			spot.m_bLive = SpotAlive(spot);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void BanStopped()
	{
		foreach (HAM_InterSpot spot : s_aSpots)
		{
			if (!spot || spot.m_iKind == KIND_CIVIL || spot.m_sPair.IsEmpty())
				continue;

			if (!StoppedNear(spot.m_vPos))
				continue;

			if (s_aBanned.Find(spot.m_sPair) < 0)
				s_aBanned.Insert(spot.m_sPair);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static int MissingKind(IEntity body)
	{
		int patrols = CountKind(body, KIND_PATROL);
		if (patrols < 2)
			return KIND_PATROL;

		if (CountKind(body, KIND_MINE) < 1)
			return KIND_MINE;

		if (CountKind(body, KIND_IED) < 1)
			return KIND_IED;

		if (CountKind(body, KIND_CIVIL) < 1)
			return KIND_CIVIL;

		return -1;
	}

	//------------------------------------------------------------------------------------------------
	protected static int CountKind(IEntity body, int kind)
	{
		int count = 0;
		vector origin = body.GetOrigin();
		foreach (HAM_InterSpot spot : s_aSpots)
		{
			if (!spot || !spot.m_bLive || spot.m_iKind != kind)
				continue;

			float dist = vector.DistanceXZ(spot.m_vPos, origin);
			if (dist >= PLAYER_MIN && dist <= PLAYER_MAX)
				count++;
		}

		return count;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FindPoint(IEntity body, int playerId, int kind, out vector pos, out string pair)
	{
		pos = vector.Zero;
		pair = string.Empty;
		vector forward = TravelDir(body, playerId);
		bool forwardFirst = Math.RandomInt(0, 2) == 0;
		int start = Math.RandomInt(0, 4);
		for (int step = 0; step < 4; step++)
		{
			int index = start + step;
			if (index > 3)
				index = index - 4;

			float dist = 400;
			if (index == 1)
				dist = 700;
			else if (index == 2)
				dist = 1000;
			else if (index == 3)
				dist = 1200;

			for (int side = 0; side < 2; side++)
			{
				bool ahead = forwardFirst;
				if (side == 1)
					ahead = !forwardFirst;

				vector raw = body.GetOrigin() + forward * dist;
				if (!ahead)
					raw = body.GetOrigin() - forward * dist;

				vector sample;
				if (!HAM_Util.FindSite(raw, 4, sample) || HAM_Util.IsNearWater(sample))
					continue;

				string found;
				if (!Accept(sample, kind, found))
					continue;

				pos = sample;
				pair = found;
				return true;
			}
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Accept(vector sample, int kind, out string pair)
	{
		pair = string.Empty;
		if (!HAM_Mission.s_aPlayerBodies)
			return false;

		bool inBand = false;
		foreach (IEntity body : HAM_Mission.s_aPlayerBodies)
		{
			if (!body)
				continue;

			float dist = vector.DistanceXZ(body.GetOrigin(), sample);
			if (dist < PLAYER_MIN)
				return false;

			if (dist <= PLAYER_MAX)
				inBand = true;
		}

		if (!inBand)
			return false;

		foreach (HAM_InterAnchor anchor : s_aAnchors)
		{
			if (!anchor)
				continue;

			float dist = vector.DistanceXZ(anchor.m_vPos, sample);
			float playerClear = PLAYER_ZONE;
			if (kind == KIND_PATROL)
				playerClear = PATROL_PLAYER_ZONE;

			if (anchor.m_bHeld && dist < playerClear)
				return false;

			if (!anchor.m_bHeld && dist < ENEMY_ZONE)
				return false;
		}

		foreach (HAM_InterSpot spot : s_aSpots)
		{
			if (!spot)
				continue;

			if (vector.DistanceXZ(spot.m_vPos, sample) < SEPARATION)
				return false;
		}

		if (!Between(sample, pair))
			return false;

		if (kind == KIND_CIVIL)
			return true;

		if (s_aBanned.Find(pair) >= 0)
			return false;

		if (StoppedNear(sample))
			return false;

		return FreeHostNear(sample);
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Between(vector sample, out string pair)
	{
		pair = string.Empty;
		float best = -1;
		for (int i = 0; i < s_aAnchors.Count(); i++)
		{
			HAM_InterAnchor first = s_aAnchors[i];
			if (!first)
				continue;

			for (int j = i + 1; j < s_aAnchors.Count(); j++)
			{
				HAM_InterAnchor second = s_aAnchors[j];
				if (!second || first.m_sName == second.m_sName)
					continue;

				float span = vector.DistanceXZ(first.m_vPos, second.m_vPos);
				if (span < 50)
					continue;

				float extra = vector.DistanceXZ(sample, first.m_vPos) + vector.DistanceXZ(sample, second.m_vPos) - span;
				float slack = first.m_fRadius;
				if (second.m_fRadius < slack)
					slack = second.m_fRadius;

				if (extra > slack)
					continue;

				if (best >= 0 && extra >= best)
					continue;

				best = extra;
				pair = PairKey(first.m_sName, second.m_sName);
			}
		}

		return best >= 0;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool FreeHostNear(vector pos)
	{
		foreach (HAM_InterAnchor anchor : s_aAnchors)
		{
			if (!anchor || !anchor.m_bCamp || anchor.m_bHeld || anchor.m_bEmptied)
				continue;

			if (vector.DistanceXZ(anchor.m_vPos, pos) <= HOST_NEAR)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool StoppedNear(vector pos)
	{
		foreach (HAM_InterAnchor anchor : s_aAnchors)
		{
			if (!anchor || !anchor.m_bCamp)
				continue;

			if (!anchor.m_bHeld && !anchor.m_bEmptied)
				continue;

			if (vector.DistanceXZ(anchor.m_vPos, pos) <= HOST_STOP)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpawnKind(int kind, vector pos, HAM_InterSpot spot)
	{
		if (kind == KIND_PATROL)
			return HAM_Garrison.SpawnLoosePatrol(pos, spot.m_aGroups);

		if (kind == KIND_MINE)
			return HAM_Devices.PlaceLooseMines(pos, spot.m_aEntities, spot.m_aProps);

		if (kind == KIND_IED)
			return HAM_Devices.PlaceLooseIeds(pos, spot.m_aEntities, spot.m_aProps);

		if (kind == KIND_CIVIL)
			return HAM_Intel.SpawnPassCivilians(pos, spot.m_aEntities);

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SpotAlive(HAM_InterSpot spot)
	{
		if (!spot)
			return false;

		foreach (SCR_AIGroup group : spot.m_aGroups)
		{
			if (HAM_Util.AliveCount(group) > 0)
				return true;
		}

		foreach (IEntity ent : spot.m_aEntities)
		{
			if (!HAM_Util.IsDead(ent))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_InterSpot SpotOf(string pair, int kind)
	{
		foreach (HAM_InterSpot spot : s_aSpots)
		{
			if (spot && spot.m_sPair == pair && spot.m_iKind == kind)
				return spot;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static string PairKey(string first, string second)
	{
		if (first.Compare(second) > 0)
			return second + "|" + first;

		return first + "|" + second;
	}

	//------------------------------------------------------------------------------------------------
	protected static vector TravelDir(IEntity body, int playerId)
	{
		vector origin = body.GetOrigin();
		vector previous = origin;
		int index = s_aSeenId.Find(playerId);
		if (index >= 0)
			previous = s_aSeenPos[index];

		if (index < 0)
		{
			s_aSeenId.Insert(playerId);
			s_aSeenPos.Insert(origin);
		}
		else
			s_aSeenPos[index] = origin;

		vector dir = origin - previous;
		dir[1] = 0;
		if (dir.LengthSq() < 1)
		{
			vector angles = body.GetYawPitchRoll();
			dir = Vector(angles[0], 0, 0).AnglesToVector();
		}

		if (dir.LengthSq() < 0.01)
			dir = Vector(1, 0, 0);

		dir.Normalize();
		return dir;
	}

	//------------------------------------------------------------------------------------------------
	protected static string KindName(int kind)
	{
		if (kind == KIND_PATROL)
			return "patrouille";

		if (kind == KIND_MINE)
			return "mines";

		if (kind == KIND_IED)
			return "IED";

		return "civils";
	}
}
