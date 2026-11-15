//------------------------------------------------------------------------------------------------
// Versement des récompenses : on remplit d'abord la cible, puis le report
// va au QG restant et aux maîtres tenus, du plus proche au plus loin.
// Les hommes s'arrêtent à menMax. Les fournitures Conflit restent sous le plafond du conteneur.
//------------------------------------------------------------------------------------------------

class HAM_SupplySpot
{
	string m_sLabel;
	vector m_vPos;
	SCR_CampaignMilitaryBaseComponent m_Base;
	HAM_MasterLinkComponent m_Link;
	bool m_bUsed;
}

class HAM_SpawnHold
{
	string m_sBase;
	int m_iMen;
}

class HAM_MenShare
{
	string m_sBase;
	int m_iCount;
}

class HAM_CampGrant
{
	string m_sPoint;
	ref array<ref HAM_MenShare> m_aShares;
}

class HAM_DeathMark
{
	int m_iPlayerId;
	string m_sIdentity;
}

class HAM_Credit
{
	protected static ref array<ref HAM_DeathMark> s_aDeaths;
	protected static ref array<ref HAM_CampGrant> s_aCampGrants;
	protected static ref map<int, ref HAM_SpawnHold> s_mHolds;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aDeaths = null;
		s_aCampGrants = null;
		s_mHolds = null;
	}

	//------------------------------------------------------------------------------------------------
	static int AvailableMen()
	{
		int total = 0;
		if (HAM_Mission.s_HqStock)
			total += HAM_Mission.s_HqStock.m_iMen;

		if (HAM_Mission.s_aCells)
		{
			foreach (HAM_Cell cell : HAM_Mission.s_aCells)
			{
				if (!cell || !cell.IsMasterPlayer() || !cell.m_Stock)
					continue;

				total += cell.m_Stock.m_iMen;
			}
		}

		if (HAM_Mission.s_aDeposits)
		{
			foreach (HAM_Deposit deposit : HAM_Mission.s_aDeposits)
			{
				if (!deposit || !deposit.m_Stock || !deposit.m_Def || !deposit.m_Cell || !deposit.m_Cell.IsMasterPlayer())
					continue;

				HAM_Point point = HAM_Mission.FindPoint(deposit.m_Def.m_sName);
				if (point && !point.m_bPlayer)
					continue;

				total += deposit.m_Stock.m_iMen;
			}
		}

		total += HAM_Convoys.MenInTransit();
		return total;
	}

	//------------------------------------------------------------------------------------------------
	static void CheckDefeat()
	{
		if (!HAM_Mission.IsRunning() || HAM_Mission.HasEnded())
			return;

		if (AvailableMen() > 0)
			return;

		HAM_Mission.Defeat();
	}

	//------------------------------------------------------------------------------------------------
	static void NoteDied(int playerId)
	{
		if (playerId <= 0 || !HAM_Mission.IsRunning())
			return;

		if (Owes(playerId))
			return;

		if (!s_aDeaths)
			s_aDeaths = {};

		HAM_DeathMark mark = new HAM_DeathMark();
		mark.m_iPlayerId = playerId;
		mark.m_sIdentity = DeathIdentity(playerId);
		s_aDeaths.Insert(mark);
	}

	//------------------------------------------------------------------------------------------------
	static bool AllowSpawn(int playerId, out SCR_ESpawnResult result)
	{
		result = SCR_ESpawnResult.OK;
		if (playerId <= 0)
			return true;

		if (HAM_Mission.HasEnded())
		{
			result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			return false;
		}

		if (!HAM_Mission.IsRunning())
			return true;

		if (AvailableMen() <= 0)
		{
			HAM_Mission.Defeat();
			result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			return false;
		}

		if (!Owes(playerId))
			return true;

		if (HoldOf(playerId))
			return true;

		HAM_Settings settings = HAM_Settings.Get();
		int cost = settings.m_iRespawnMen;
		if (cost < 0)
			cost = 0;

		string baseName = HAM_Mission.DesignatedHq();
		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		int have = 0;
		if (stock)
			have = stock.m_iMen;

		if (cost > 0 && have < cost)
		{
			result = SCR_ESpawnResult.SPAWN_NOT_ALLOWED;
			HAM_Net.NotifyPlayer(playerId, "Réapparition", "Le QG désigné n'a plus assez d'hommes. Il en reste ailleurs : réapparition refusée.", 8);
			return false;
		}

		if (cost > 0 && stock)
			stock.Take(HAM_EResource.MEN, cost);

		if (!s_mHolds)
			s_mHolds = new map<int, ref HAM_SpawnHold>();

		HAM_SpawnHold hold = new HAM_SpawnHold();
		hold.m_sBase = baseName;
		hold.m_iMen = cost;
		s_mHolds.Set(playerId, hold);
		HAM_Mission.MarkDirty();
		// Le retrait peut vider le total : l'échec part tout de suite, la demande reste acceptée.
		CheckDefeat();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void OnBody(int playerId)
	{
		HAM_SpawnHold hold = HoldOf(playerId);
		if (!hold)
			return;

		if (s_mHolds)
			s_mHolds.Remove(playerId);

		ClearOwes(playerId);
		HAM_Settings settings = HAM_Settings.Get();
		int loss = settings.m_iRespawnRep;
		if (loss < 0)
			loss = 0;

		if (loss > 0)
			HAM_Mission.AddGlobalRep(-loss, "réapparition");

		string menWord = "homme";
		if (hold.m_iMen > 1)
			menWord = "hommes";

		HAM_Net.NotifyPlayer(playerId, "Réapparition", string.Format("Coût : -%1 de réputation et %2 %3 pris sur %4.", loss, hold.m_iMen, menWord, hold.m_sBase), 8);
	}

	//------------------------------------------------------------------------------------------------
	static void RefundSpawn(int playerId)
	{
		HAM_SpawnHold hold = HoldOf(playerId);
		if (!hold)
			return;

		if (s_mHolds)
			s_mHolds.Remove(playerId);

		if (hold.m_iMen <= 0 || HAM_Mission.HasEnded())
			return;

		HAM_Stock stock = HAM_Mission.StockOf(hold.m_sBase);
		if (stock)
			stock.Add(HAM_EResource.MEN, hold.m_iMen);

		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	static void CreditMen(string primary, int amount, string reason)
	{
		array<ref HAM_MenShare> shares = {};
		CreditMenInto(primary, amount, reason, shares);
	}

	//------------------------------------------------------------------------------------------------
	//! Même versement qu'un crédit d'hommes, en gardant où les hommes sont réellement posés.
	static void CreditCamp(string pointName, string primary, int amount, string reason)
	{
		array<ref HAM_MenShare> shares = {};
		CreditMenInto(primary, amount, reason, shares);
		KeepCampGrant(pointName, shares);
	}

	//------------------------------------------------------------------------------------------------
	static void ReclaimCamp(HAM_Point point)
	{
		if (!point || !point.m_Def)
			return;

		HAM_CampGrant grant = PullCampGrant(point.m_Def.m_sName);
		if (grant)
		{
			if (grant.m_aShares)
			{
				foreach (HAM_MenShare share : grant.m_aShares)
					TakeMenShare(share);
			}

			return;
		}

		if (point.m_Cell && point.m_Cell.m_Stock)
			point.m_Cell.m_Stock.Add(HAM_EResource.MEN, -HAM_MasterBases.CampMen());
	}

	//------------------------------------------------------------------------------------------------
	protected static void CreditMenInto(string primary, int amount, string reason, notnull array<ref HAM_MenShare> shares)
	{
		if (amount <= 0 || primary.IsEmpty())
			return;

		int left = amount;
		int onPrimary = PourMen(primary, left);
		left -= onPrimary;
		if (onPrimary > 0)
			NoteShare(shares, primary, onPrimary);

		string placed;
		array<string> names = {};
		RememberMenTarget(names, primary, HAM_Mission.HQ_NAME);
		RememberMenTarget(names, primary, HAM_Mission.DesignatedHq());
		if (HAM_Mission.s_aCells)
		{
			foreach (HAM_Cell cell : HAM_Mission.s_aCells)
			{
				if (cell && cell.IsMasterPlayer())
					RememberMenTarget(names, primary, cell.m_sName);
			}
		}

		vector from = HAM_Mission.BasePos(primary);
		while (left > 0 && !names.IsEmpty())
		{
			string best = NearestName(names, from);
			names.RemoveItem(best);
			int given = PourMen(best, left);
			if (given <= 0)
				continue;

			left -= given;
			NoteShare(shares, best, given);
			placed = AppendPlace(placed, best, given);
		}

		HAM_Mission.MarkDirty();
		Report(reason, placed, left);
	}

	//------------------------------------------------------------------------------------------------
	static void CreditConflict(string primary, int amount, string reason)
	{
		if (amount <= 0 || primary.IsEmpty())
			return;

		SCR_CampaignMilitaryBaseComponent base = HAM_HqDesignation.ConflictBaseOf(primary);
		int left = amount;
		if (base)
			left -= PourBase(base, left);

		OverflowSupplies(HAM_Mission.BasePos(primary), left, reason, base);
	}

	//------------------------------------------------------------------------------------------------
	static void CreditMasterDepot(HAM_Cell cell, int amount, string reason)
	{
		if (!cell || amount <= 0)
			return;

		SCR_CampaignMilitaryBaseComponent home = HAM_Mission.FindBaseNear(cell.Pos(), 120);
		HAM_MasterLinkComponent link = LinkOn(home);
		int left = amount;
		if (link)
			left -= link.PourCappedSupplies(left);

		OverflowSupplies(cell.Pos(), left, reason, home);
	}

	//------------------------------------------------------------------------------------------------
	static void CreditBarracks(IEntity barracks, int amount, string reason)
	{
		if (amount <= 0)
			return;

		vector from = vector.Zero;
		SCR_CampaignMilitaryBaseComponent home;
		if (barracks)
		{
			from = barracks.GetOrigin();
			home = HAM_Mission.FindBaseNear(from, 120);
		}

		int added = PourAnyContainer(barracks, amount);
		OverflowSupplies(from, amount - added, reason, home);
	}

	//------------------------------------------------------------------------------------------------
	static int PourStorage(IEntity node, int amount)
	{
		if (!node || amount <= 0)
			return 0;

		IEntity storage = HAM_Mission.FindSupplyStorage(node);
		if (!storage)
			return 0;

		return PourContainer(ContainerOn(storage), amount);
	}

	//------------------------------------------------------------------------------------------------
	protected static int PourMen(string baseName, int amount)
	{
		if (amount <= 0 || baseName.IsEmpty())
			return 0;

		HAM_Stock stock = HAM_Mission.StockOf(baseName);
		if (!stock)
			return 0;

		int room = HAM_Settings.Get().m_iMenMax - stock.m_iMen;
		if (room <= 0)
			return 0;

		int take = amount;
		if (take > room)
			take = room;

		stock.Add(HAM_EResource.MEN, take);
		if (baseName != HAM_Mission.HQ_NAME)
		{
			HAM_Cell cell = HAM_Mission.FindCell(baseName);
			if (cell)
			{
				HAM_Logistics.CheckHold(cell);
				HAM_Mission.RefreshZoneProps(cell);
			}
		}

		return take;
	}

	//------------------------------------------------------------------------------------------------
	protected static int PourBase(SCR_CampaignMilitaryBaseComponent base, int amount)
	{
		if (!base || amount <= 0)
			return 0;

		float before = base.GetSupplies();
		float room = base.GetSuppliesMax() - before;
		if (room < 1)
			return 0;

		int take = amount;
		if (take > room)
			take = Math.Floor(room);

		if (take <= 0)
			return 0;

		base.AddSupplies(take);
		int added = Math.Round(base.GetSupplies() - before);
		if (added < 0)
			added = 0;

		if (added > take)
			added = take;

		return added;
	}

	//------------------------------------------------------------------------------------------------
	protected static int PourAnyContainer(IEntity node, int amount)
	{
		return PourContainer(FindContainer(node), amount);
	}

	//------------------------------------------------------------------------------------------------
	protected static int PourContainer(SCR_ResourceContainer stock, int amount)
	{
		if (!stock || amount <= 0)
			return 0;

		float room = stock.GetMaxResourceValue() - stock.GetResourceValue();
		if (room < 1)
			return 0;

		int take = amount;
		if (take > room)
			take = Math.Floor(room);

		if (take <= 0)
			return 0;

		float before = stock.GetResourceValue();
		stock.IncreaseResourceValue(take, true);
		int added = Math.Round(stock.GetResourceValue() - before);
		if (added < 0)
			added = 0;

		if (added > take)
			added = take;

		return added;
	}

	//------------------------------------------------------------------------------------------------
	protected static void OverflowSupplies(vector from, int amount, string reason, SCR_CampaignMilitaryBaseComponent skip)
	{
		if (amount <= 0)
			return;

		array<ref HAM_SupplySpot> spots = {};
		CollectSpots(spots, skip);
		string placed;
		int left = amount;
		while (left > 0)
		{
			HAM_SupplySpot spot = NearestSpot(spots, from);
			if (!spot)
				break;

			spot.m_bUsed = true;
			int given = 0;
			if (spot.m_Base)
				given = PourBase(spot.m_Base, left);
			else if (spot.m_Link)
				given = spot.m_Link.PourCappedSupplies(left);

			if (given <= 0)
				continue;

			left -= given;
			placed = AppendPlace(placed, spot.m_sLabel, given);
		}

		if (left != amount)
			HAM_Mission.MarkDirty();

		Report(reason, placed, left);
	}

	//------------------------------------------------------------------------------------------------
	protected static void CollectSpots(notnull array<ref HAM_SupplySpot> spots, SCR_CampaignMilitaryBaseComponent skip)
	{
		AddBaseSpot(spots, HAM_Mission.HQ_NAME, HAM_Mission.s_HqBase, skip);
		string designatedName = HAM_Mission.DesignatedHq();
		SCR_CampaignMilitaryBaseComponent designated = HAM_HqDesignation.ConflictBaseOf(designatedName);
		if (designated != HAM_Mission.s_HqBase)
			AddBaseSpot(spots, designatedName, designated, skip);

		if (!HAM_Mission.s_aCells)
			return;

		foreach (HAM_Cell cell : HAM_Mission.s_aCells)
		{
			if (!cell || !cell.IsMasterPlayer())
				continue;

			SCR_CampaignMilitaryBaseComponent near = HAM_Mission.FindBaseNear(cell.Pos(), 120);
			if (!near || near == skip || near == HAM_Mission.s_HqBase || near == designated)
				continue;

			HAM_MasterLinkComponent link = LinkOn(near);
			if (!link || !link.GetOwner())
				continue;

			HAM_SupplySpot spot = new HAM_SupplySpot();
			spot.m_sLabel = cell.m_sName;
			spot.m_vPos = cell.Pos();
			spot.m_Link = link;
			spots.Insert(spot);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void AddBaseSpot(notnull array<ref HAM_SupplySpot> spots, string label, SCR_CampaignMilitaryBaseComponent base, SCR_CampaignMilitaryBaseComponent skip)
	{
		if (!base || base == skip || label.IsEmpty() || !base.GetOwner())
			return;

		HAM_SupplySpot spot = new HAM_SupplySpot();
		spot.m_sLabel = label;
		spot.m_vPos = base.GetOwner().GetOrigin();
		spot.m_Base = base;
		spots.Insert(spot);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_MasterLinkComponent LinkOn(SCR_CampaignMilitaryBaseComponent base)
	{
		if (!base || !base.GetOwner())
			return null;

		return HAM_MasterLinkComponent.Cast(base.GetOwner().FindComponent(HAM_MasterLinkComponent));
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ResourceContainer ContainerOn(IEntity node)
	{
		if (!node)
			return null;

		SCR_ResourceComponent resources = SCR_ResourceComponent.Cast(node.FindComponent(SCR_ResourceComponent));
		if (!resources)
			return null;

		return resources.GetContainer(EResourceType.SUPPLIES);
	}

	//------------------------------------------------------------------------------------------------
	protected static SCR_ResourceContainer FindContainer(IEntity node)
	{
		if (!node)
			return null;

		SCR_ResourceContainer direct = ContainerOn(node);
		if (direct)
			return direct;

		IEntity storage = HAM_Mission.FindSupplyStorage(node);
		if (storage)
		{
			SCR_ResourceContainer stored = ContainerOn(storage);
			if (stored)
				return stored;
		}

		IEntity child = node.GetChildren();
		while (child)
		{
			SCR_ResourceContainer found = FindContainer(child);
			if (found)
				return found;

			child = child.GetSibling();
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void RememberMenTarget(notnull array<string> names, string primary, string candidate)
	{
		if (candidate.IsEmpty() || candidate == primary)
			return;

		if (names.Find(candidate) >= 0)
			return;

		names.Insert(candidate);
	}

	//------------------------------------------------------------------------------------------------
	protected static string NearestName(notnull array<string> names, vector from)
	{
		string best;
		float bestSq = -1;
		foreach (string name : names)
		{
			float distSq = vector.DistanceSqXZ(from, HAM_Mission.BasePos(name));
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = name;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_SupplySpot NearestSpot(notnull array<ref HAM_SupplySpot> spots, vector from)
	{
		HAM_SupplySpot best;
		float bestSq = -1;
		foreach (HAM_SupplySpot spot : spots)
		{
			if (!spot || spot.m_bUsed)
				continue;

			float distSq = vector.DistanceSqXZ(from, spot.m_vPos);
			if (bestSq >= 0 && distSq >= bestSq)
				continue;

			bestSq = distSq;
			best = spot;
		}

		return best;
	}

	//------------------------------------------------------------------------------------------------
	protected static string AppendPlace(string placed, string label, int amount)
	{
		string piece = string.Format("%1 (%2)", label, amount);
		if (placed.IsEmpty())
			return piece;

		return placed + ", " + piece;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Report(string reason, string placed, int lost)
	{
		if (placed.IsEmpty() && lost <= 0)
			return;

		string message = reason + " : ";
		if (!placed.IsEmpty())
			message = message + "le surplus est allé à " + placed + ".";

		if (lost > 0)
			message = message + " " + lost.ToString() + " n'ont trouvé aucune place.";

		HAM_Mission.Notify("Récompense", message, 10);
	}

	//------------------------------------------------------------------------------------------------
	protected static string DeathIdentity(int playerId)
	{
		string identity = HAM_GroupAccessHelper.GetPlayerIdentityString(playerId);
		if (HAM_GroupAccessHelper.IsInvalidIdentityString(identity))
			return string.Empty;

		return identity;
	}

	//------------------------------------------------------------------------------------------------
	//! Même mort si l'identité correspond, ou si le numéro de joueur correspond et que l'identité manquait d'un côté.
	//! Une identité différente sur le même numéro n'est pas cette mort : une reconnexion sans mort ne paie pas.
	protected static bool SameDeath(HAM_DeathMark mark, int playerId, string identity)
	{
		if (!mark)
			return false;

		bool sameIdentity = !identity.IsEmpty() && !mark.m_sIdentity.IsEmpty() && mark.m_sIdentity == identity;
		if (sameIdentity)
			return true;

		if (playerId <= 0 || mark.m_iPlayerId != playerId)
			return false;

		return mark.m_sIdentity.IsEmpty() || identity.IsEmpty();
	}

	//------------------------------------------------------------------------------------------------
	protected static bool Owes(int playerId)
	{
		if (!s_aDeaths || playerId <= 0)
			return false;

		string identity = DeathIdentity(playerId);
		foreach (HAM_DeathMark mark : s_aDeaths)
		{
			if (SameDeath(mark, playerId, identity))
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearOwes(int playerId)
	{
		if (!s_aDeaths)
			return;

		string identity = DeathIdentity(playerId);
		for (int i = s_aDeaths.Count() - 1; i >= 0; i--)
		{
			if (SameDeath(s_aDeaths[i], playerId, identity))
				s_aDeaths.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void NoteShare(notnull array<ref HAM_MenShare> shares, string baseName, int count)
	{
		if (baseName.IsEmpty() || count <= 0)
			return;

		foreach (HAM_MenShare listed : shares)
		{
			if (!listed || listed.m_sBase != baseName)
				continue;

			listed.m_iCount = listed.m_iCount + count;
			return;
		}

		HAM_MenShare created = new HAM_MenShare();
		created.m_sBase = baseName;
		created.m_iCount = count;
		shares.Insert(created);
	}

	//------------------------------------------------------------------------------------------------
	protected static void KeepCampGrant(string pointName, array<ref HAM_MenShare> shares)
	{
		if (pointName.IsEmpty())
			return;

		if (!s_aCampGrants)
			s_aCampGrants = {};

		for (int i = s_aCampGrants.Count() - 1; i >= 0; i--)
		{
			HAM_CampGrant existing = s_aCampGrants[i];
			if (existing && existing.m_sPoint == pointName)
				s_aCampGrants.Remove(i);
		}

		HAM_CampGrant grant = new HAM_CampGrant();
		grant.m_sPoint = pointName;
		grant.m_aShares = shares;
		s_aCampGrants.Insert(grant);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_CampGrant PullCampGrant(string pointName)
	{
		if (pointName.IsEmpty() || !s_aCampGrants)
			return null;

		for (int i = 0; i < s_aCampGrants.Count(); i++)
		{
			HAM_CampGrant grant = s_aCampGrants[i];
			if (!grant || grant.m_sPoint != pointName)
				continue;

			s_aCampGrants.Remove(i);
			return grant;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static void TakeMenShare(HAM_MenShare share)
	{
		if (!share || share.m_iCount <= 0 || share.m_sBase.IsEmpty())
			return;

		HAM_Stock stock = HAM_Mission.StockOf(share.m_sBase);
		if (!stock)
			return;

		stock.Add(HAM_EResource.MEN, -share.m_iCount);
		if (share.m_sBase == HAM_Mission.HQ_NAME)
			return;

		HAM_Cell cell = HAM_Mission.FindCell(share.m_sBase);
		if (!cell)
			return;

		HAM_Logistics.CheckHold(cell);
		HAM_Mission.RefreshZoneProps(cell);
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_SpawnHold HoldOf(int playerId)
	{
		if (!s_mHolds || playerId <= 0)
			return null;

		HAM_SpawnHold hold;
		if (!s_mHolds.Find(playerId, hold))
			return null;

		return hold;
	}
}
