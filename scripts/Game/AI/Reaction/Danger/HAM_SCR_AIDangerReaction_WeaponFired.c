[BaseContainerProps()]
modded class SCR_AIDangerReaction_WeaponFired
{
	override bool PerformReaction(notnull SCR_AIUtilityComponent utility, notnull SCR_AIThreatSystem threatSystem, AIDangerEvent dangerEvent, int dangerEventCount)
	{
		AIDangerEventWeaponFire eventWeaponFire = AIDangerEventWeaponFire.Cast(dangerEvent);
		if (!eventWeaponFire)
			return false;

		IEntity shooter = eventWeaponFire.GetObject();
		if (!shooter)
			return false;

		IEntity instigatorEntity = eventWeaponFire.GetInstigatorEntity();
		if (!instigatorEntity)
			return false;

		Faction instigatorFaction = SCR_AIFactionHandling.GetEntityPerceivedFaction(instigatorEntity);

		if (!instigatorFaction)
			return false;

		SCR_ChimeraAIAgent agent = SCR_ChimeraAIAgent.Cast(utility.GetOwner());

		bool myFactionIsMilitary = utility.IsMilitary();
		if (myFactionIsMilitary && !agent.IsEnemy(instigatorFaction))
			return false;

		vector shotPos = eventWeaponFire.GetPosition();
		vector shotDir = eventWeaponFire.GetDirection();
		bool isShotSuppressed = eventWeaponFire.IsSuppressed();

		vector myOrigin = utility.m_OwnerEntity.GetOrigin();
		float distance = vector.Distance(myOrigin, shotPos);

		bool isFlyby = IsFlyby(myOrigin, shotPos, shotDir, distance);

		float maxAudibleDistance;
		if (isShotSuppressed)
		{
			maxAudibleDistance = HAM_AISettingsComponent.HAM_SuppressedAudibleDistance();
			if (maxAudibleDistance < 0)
				maxAudibleDistance = AUDIBLE_DISTANCE_SUPPRESSED;
		}
		else
			maxAudibleDistance = AUDIBLE_DISTANCE_NORMAL;
		bool isAudible = distance < maxAudibleDistance;


		float timeTillFlyby_s = float.MAX;
		float timeTillGunshotHeard_s = float.MAX;
		if (isFlyby)
		{
			float projectileSpeed = eventWeaponFire.GetInitialSpeed();
			WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
			float flightTime_s = distance / projectileSpeed;
			WorldTimestamp flybyTimestamp = eventTimestamp.PlusSeconds(flightTime_s + PROJECTILE_FLYBY_DELAY_S);
			timeTillFlyby_s = flybyTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());

			if (timeTillFlyby_s < 0)
				OnProjectileFlyby(utility, dangerEventCount, shotPos);
			else
			{
				utility.GetCallqueue().CallLater(OnProjectileFlyby, 1000*timeTillFlyby_s, false,
					utility, dangerEventCount, shotPos);
			}
		}

		if (isAudible)
		{
			WorldTimestamp eventTimestamp = eventWeaponFire.GetTimestamp();
			float wavefrontTravelTime_s = distance / SOUND_SPEED_MS;
			WorldTimestamp wavefrontArrivalTimestamp = eventTimestamp.PlusSeconds(wavefrontTravelTime_s);
			timeTillGunshotHeard_s = wavefrontArrivalTimestamp.DiffSeconds(GetGame().GetWorld().GetTimestamp());

			bool ignoreGunshotHeard = isFlyby && timeTillFlyby_s < timeTillGunshotHeard_s;

			if (!ignoreGunshotHeard)
			{
				if (timeTillGunshotHeard_s < 0)
					OnGunshotHeard(utility, distance, dangerEventCount, shotPos);
				else
				{
					utility.GetCallqueue().CallLater(OnGunshotHeard, 1000*timeTillGunshotHeard_s, false,
						utility, distance, dangerEventCount, shotPos);
				}
			}
		}

		if (isFlyby || isAudible)
		{
			bool endangeringForGroup = isFlyby || distance < ENDANGERING_FOR_GROUP_RADIUS;

			AIGroup myGroup = utility.GetOwner().GetParentGroup();
			if (myGroup && myGroup.GetLeaderAgent() == agent)
			{
				float timeTillGroupNotified_s = Math.Min(timeTillFlyby_s, timeTillGunshotHeard_s);

				if (timeTillGroupNotified_s < 0)
					NotifyGroup(myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
				else
				{
					utility.GetCallqueue().CallLater(NotifyGroup, 1000*timeTillGroupNotified_s, false,
						myGroup, shooter, instigatorEntity, instigatorFaction, shotPos, endangeringForGroup);
				}
			}
		}

		return true;
	}
}
