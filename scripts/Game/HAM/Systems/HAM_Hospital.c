//------------------------------------------------------------------------------------------------
// Hôpital de campagne déjà présent dans le menu de construction.
// À la pose, une zone de 10 m au sol suit l'origine. Un joueur humain du même camp
// qui y entre est soigné une fois. Il ressort, le prochain passage le soigne à nouveau.
//------------------------------------------------------------------------------------------------

class HAM_HospitalZone
{
	IEntity m_Entity;
	string m_sFaction;
	ref array<int> m_aInside;
}

class HAM_Hospital
{
	protected static const float RADIUS = 10;

	protected static ref array<ref HAM_HospitalZone> s_aZones;

	//------------------------------------------------------------------------------------------------
	static void ResetSession()
	{
		s_aZones = null;
	}

	//------------------------------------------------------------------------------------------------
	static void Note(IEntity ent)
	{
		if (!Replication.IsServer() || !ent)
			return;

		string factionKey;
		if (!IsFieldHospital(ent, factionKey))
			return;

		if (Find(ent))
			return;

		if (!s_aZones)
			s_aZones = {};

		HAM_HospitalZone zone = new HAM_HospitalZone();
		zone.m_Entity = ent;
		zone.m_sFaction = factionKey;
		zone.m_aInside = {};
		s_aZones.Insert(zone);
	}

	//------------------------------------------------------------------------------------------------
	static void Forget(IEntity ent)
	{
		if (!s_aZones || !ent)
			return;

		for (int i = s_aZones.Count() - 1; i >= 0; i--)
		{
			HAM_HospitalZone zone = s_aZones[i];
			if (!zone || zone.m_Entity == ent)
				s_aZones.Remove(i);
		}
	}

	//------------------------------------------------------------------------------------------------
	static void Tick(array<int> ids, array<IEntity> bodies)
	{
		if (!Replication.IsServer() || !s_aZones)
			return;

		for (int i = s_aZones.Count() - 1; i >= 0; i--)
		{
			HAM_HospitalZone zone = s_aZones[i];
			if (!zone || !zone.m_Entity)
			{
				s_aZones.Remove(i);
				continue;
			}

			TickZone(zone, ids, bodies);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static void TickZone(HAM_HospitalZone zone, array<int> ids, array<IEntity> bodies)
	{
		array<int> still = {};
		float radiusSq = RADIUS * RADIUS;
		vector origin = zone.m_Entity.GetOrigin();
		int count = 0;
		if (ids && bodies)
		{
			count = ids.Count();
			if (bodies.Count() < count)
				count = bodies.Count();
		}

		for (int i = 0; i < count; i++)
		{
			int playerId = ids[i];
			IEntity body = bodies[i];
			if (!body || !SameFaction(playerId, zone.m_sFaction))
				continue;

			if (vector.DistanceSqXZ(body.GetOrigin(), origin) > radiusSq)
				continue;

			still.Insert(playerId);
			if (!zone.m_aInside || zone.m_aInside.Find(playerId) < 0)
				Heal(playerId, body);
		}

		zone.m_aInside = still;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool IsFieldHospital(IEntity ent, out string factionKey)
	{
		factionKey = string.Empty;
		if (!ent || !ent.GetPrefabData())
			return false;

		ResourceName prefab = ent.GetPrefabData().GetPrefabName();
		bool us = prefab.Contains("FieldHospital_M_US_01");
		bool ussr = prefab.Contains("FieldHospital_M_USSR_01");
		if (!us && !ussr)
			return false;

		factionKey = HAM_Util.FactionKey(ent);
		if (!factionKey.IsEmpty())
			return true;

		if (us)
			factionKey = "US";
		else
			factionKey = "USSR";

		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected static HAM_HospitalZone Find(IEntity ent)
	{
		if (!s_aZones || !ent)
			return null;

		foreach (HAM_HospitalZone zone : s_aZones)
		{
			if (zone && zone.m_Entity == ent)
				return zone;
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	protected static bool SameFaction(int playerId, string factionKey)
	{
		if (factionKey.IsEmpty())
			return false;

		Faction faction = SCR_FactionManager.SGetPlayerFaction(playerId);
		if (!faction)
			return false;

		return faction.GetFactionKey() == factionKey;
	}

	//------------------------------------------------------------------------------------------------
	protected static void Heal(int playerId, IEntity body)
	{
		if (!body || HAM_Util.IsDead(body))
			return;

		CharacterControllerComponent controller = CharacterControllerComponent.Cast(body.FindComponent(CharacterControllerComponent));
		if (!controller || controller.GetLifeState() == ECharacterLifeState.DEAD)
			return;

		SCR_CharacterDamageManagerComponent damage = SCR_CharacterDamageManagerComponent.Cast(body.FindComponent(SCR_CharacterDamageManagerComponent));
		if (!damage || damage.GetState() == EDamageState.DESTROYED)
			return;

		damage.FullHeal(true);
		ClearTourniquets(damage);
		damage.UpdateConsciousness();
		HAM_Net.NotifyPlayer(playerId, "Hôpital", "Vous êtes soigné.", 6);
	}

	//------------------------------------------------------------------------------------------------
	protected static void ClearTourniquets(SCR_CharacterDamageManagerComponent damage)
	{
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.HEAD, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.UPPERTORSO, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.LOWERTORSO, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.LEFTARM, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.RIGHTARM, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.LEFTLEG, false);
		damage.SetTourniquettedGroup(ECharacterHitZoneGroup.RIGHTLEG, false);
	}
}
