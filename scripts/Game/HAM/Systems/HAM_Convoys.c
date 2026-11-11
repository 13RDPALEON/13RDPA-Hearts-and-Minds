//------------------------------------------------------------------------------------------------
// Convoi d'hommes : le stock part tout de suite de l'origine, des véhicules du pool joueur roulent
// jusqu'à la cible. Arrivée : crédit. Véhicule détruit : ses hommes sont perdus.
// Convoi bloqué au-delà du délai : crédité quand même et retiré.
//------------------------------------------------------------------------------------------------

class HAM_Convoys
{
	static const float ARRIVAL_RADIUS = 120;

	protected static ref array<ref HAM_Convoy> s_aConvoys;
	protected static ref map<Vehicle, int> s_mMenByVehicle;
	protected static int s_iNextId = 1;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aConvoys = null;
		s_mMenByVehicle = null;
		s_iNextId = 1;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Ensure()
	{
		if (!s_aConvoys)
			s_aConvoys = {};

		if (!s_mMenByVehicle)
			s_mMenByVehicle = new map<Vehicle, int>();
	}

	//------------------------------------------------------------------------------------------------
	static array<ref HAM_Convoy> GetConvoys()
	{
		Ensure();
		return s_aConvoys;
	}

	//------------------------------------------------------------------------------------------------
	static bool Order(string fromBase, string toBase, int men, int playerId)
	{
		Ensure();
		HAM_Settings settings = HAM_Settings.Get();
		HAM_Stock source = HAM_Mission.StockOf(fromBase);
		if (!source || men < 1)
			return false;

		if (source.m_iMen < men)
		{
			HAM_Net.NotifyPlayer(playerId, "Convoi", string.Format("Pas assez d'hommes à %1 (%2 disponibles).", fromBase, source.m_iMen), 6);
			return false;
		}

		ResourceName vehiclePrefab = HAM_Units.PlayerConvoyVehicle();
		if (vehiclePrefab.IsEmpty())
		{
			HAM_Net.NotifyPlayer(playerId, "Convoi", "Aucun véhicule de convoi dans le pool joueur.", 6);
			return false;
		}

		int menPerVehicle = settings.m_iConvoyMenPerVehicle;
		int vehicles = men / menPerVehicle;
		int menRemainder = men - (men / menPerVehicle) * menPerVehicle;
		if (menRemainder != 0)
			vehicles++;

		vehicles = Math.Clamp(vehicles, 1, settings.m_iConvoyMaxVehicles);

		source.Add(HAM_EResource.MEN, -men);
		HAM_Cell sourceCell = HAM_Mission.FindCell(fromBase);
		if (sourceCell)
			HAM_Logistics.CheckHold(sourceCell);

		HAM_Convoy convoy = new HAM_Convoy();
		convoy.m_iId = s_iNextId;
		s_iNextId++;
		convoy.m_sFrom = fromBase;
		convoy.m_sTo = toBase;
		convoy.m_iMen = men;
		convoy.m_fStarted = HAM_Util.Now();
		convoy.m_vTarget = HAM_Mission.BasePos(toBase);

		vector origin = HAM_Mission.BasePos(fromBase);
		vector park = HAM_Util.ParkSpot(origin);
		vector roadEnd;
		bool dryRoad = HAM_Util.RoadPoint(convoy.m_vTarget, 250, roadEnd) && !HAM_Util.IsNearWater(roadEnd);
		if (!dryRoad)
			roadEnd = HAM_Util.ParkSpot(convoy.m_vTarget);

		vector heading = roadEnd - park;
		heading[1] = 0;
		float yaw = 0;
		if (heading.Length() > 1)
			yaw = heading.ToYaw();

		vector along = Vector(yaw, 0, 0).AnglesToVector();
		int remaining = men;
		for (int i = 0; i < vehicles; i++)
		{
			int load = settings.m_iConvoyMenPerVehicle;
			if (i == vehicles - 1 || load > remaining)
				load = remaining;

			remaining -= load;
			vector prefer = park;
			if (i > 0)
				prefer = park - along * (i * 16.0);

			vector slot = HAM_Util.ParkSpot(prefer);
			array<vector> route = {roadEnd};
			Vehicle vehicle = HAM_Garrison.SpawnCrewedVehicle(vehiclePrefab, slot, yaw, false, route, HAM_Units.WP_MOVE, 30, convoy.m_aGroups, false, HAM_Units.GROUP_PLAYER_CONVOY);
			if (!vehicle)
			{
				remaining += load;
				continue;
			}

			convoy.m_aVehicles.Insert(vehicle);
			s_mMenByVehicle.Set(vehicle, load);
		}

		if (convoy.m_aVehicles.IsEmpty())
		{
			source.Add(HAM_EResource.MEN, men);
			HAM_Net.NotifyPlayer(playerId, "Convoi", "Le convoi n'a pas pu être formé.", 6);
			return false;
		}

		if (remaining > 0)
		{
			Vehicle last = convoy.m_aVehicles[convoy.m_aVehicles.Count() - 1];
			int lastLoad;
			s_mMenByVehicle.Find(last, lastLoad);
			s_mMenByVehicle.Set(last, lastLoad + remaining);
		}

		s_aConvoys.Insert(convoy);
		HAM_Util.Debug(string.Format("Convoi %1 : %2 hommes, %3 véhicules, %4 -> %5", convoy.m_iId, men, convoy.m_aVehicles.Count(), fromBase, toBase));
		HAM_Mission.RefreshBaseProp(fromBase);
		HAM_Mission.MarkDirty();
		HAM_Mission.Notify("Convoi", string.Format("Convoi de %1 hommes en route : %2 → %3.", men, fromBase, toBase), 8);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static void Tick()
	{
		Ensure();
		if (s_aConvoys.IsEmpty())
			return;

		float stuckSeconds = HAM_Settings.Get().m_fConvoyStuckMinutes * 60;
		float now = HAM_Util.Now();
		for (int i = s_aConvoys.Count() - 1; i >= 0; i--)
		{
			HAM_Convoy convoy = s_aConvoys[i];
			int lostMen = 0;
			for (int v = convoy.m_aVehicles.Count() - 1; v >= 0; v--)
			{
				Vehicle vehicle = convoy.m_aVehicles[v];
				if (vehicle && !HAM_Util.IsDead(vehicle))
					continue;

				int men;
				if (vehicle && s_mMenByVehicle.Find(vehicle, men))
				{
					lostMen += men;
					s_mMenByVehicle.Remove(vehicle);
				}

				convoy.m_aVehicles.Remove(v);
			}

			if (lostMen > 0)
			{
				convoy.m_iMen = Math.Max(0, convoy.m_iMen - lostMen);
				HAM_Mission.Notify("Convoi", string.Format("Un véhicule du convoi vers %1 est détruit : %2 hommes perdus.", convoy.m_sTo, lostMen), 8);
			}

			if (convoy.m_aVehicles.IsEmpty() || convoy.m_iMen <= 0)
			{
				Finish(convoy, false, "convoi détruit");
				s_aConvoys.Remove(i);
				continue;
			}

			bool arrived = false;
			foreach (Vehicle moving : convoy.m_aVehicles)
			{
				if (vector.DistanceXZ(moving.GetOrigin(), convoy.m_vTarget) <= ARRIVAL_RADIUS)
					arrived = true;
			}

			if (arrived)
			{
				Finish(convoy, true, "arrivé");
				s_aConvoys.Remove(i);
				continue;
			}

			if (now - convoy.m_fStarted >= stuckSeconds)
			{
				Finish(convoy, true, "bloqué, crédité");
				s_aConvoys.Remove(i);
			}
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void Finish(HAM_Convoy convoy, bool credit, string reason)
	{
		HAM_Util.Debug(string.Format("Convoi %1 terminé : %2, %3 hommes", convoy.m_iId, reason, convoy.m_iMen));
		if (credit && convoy.m_iMen > 0)
			Credit(convoy.m_sTo, convoy.m_iMen, reason);
		else
			HAM_Mission.Notify("Convoi", string.Format("Le convoi vers %1 est perdu.", convoy.m_sTo), 8);

		foreach (Vehicle vehicle : convoy.m_aVehicles)
		{
			s_mMenByVehicle.Remove(vehicle);
		}

		HAM_Util.DeleteGroups(convoy.m_aGroups);
		foreach (Vehicle leftover : convoy.m_aVehicles)
		{
			HAM_Util.Delete(leftover);
		}

		convoy.m_aVehicles.Clear();
		HAM_Mission.MarkDirty();
	}

	//------------------------------------------------------------------------------------------------
	//! Les hommes vont à la cible si elle est encore tenue, sinon au QG.
	static void Credit(string toBase, int men, string reason)
	{
		string target = toBase;
		if (!HAM_Mission.IsAlliedBase(target))
			target = HAM_Mission.HQ_NAME;

		HAM_Stock stock = HAM_Mission.StockOf(target);
		if (!stock)
			return;

		stock.Add(HAM_EResource.MEN, men);
		HAM_Cell cell = HAM_Mission.FindCell(target);
		if (cell)
			HAM_Logistics.CheckHold(cell);

		HAM_Mission.RefreshBaseProp(target);
		HAM_Mission.MarkDirty();
		HAM_Mission.Notify("Convoi", string.Format("%1 hommes arrivés à %2 (%3).", men, target, reason), 8);
	}

	//------------------------------------------------------------------------------------------------
	//! Au redémarrage, un convoi encore en route est crédité à sa cible.
	static void CreditPending(string toBase, int men)
	{
		if (men > 0)
			Credit(toBase, men, "convoi repris après redémarrage");
	}
}
