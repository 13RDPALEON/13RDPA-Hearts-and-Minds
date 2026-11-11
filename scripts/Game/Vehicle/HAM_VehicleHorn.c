//------------------------------------------------------------------------------------------------
// Le klaxon entendu est le signal HornState du véhicule (graphe Vehicles_*_Horn.acp).
// L'IA le pose sur le véhicule, pas sur OnApplyControls du personnage.
// Un joueur au volant, y compris un personnage possédé, laisse le signal intact.
//------------------------------------------------------------------------------------------------

modded class Vehicle
{
	protected int m_iHAM_Horn = -1;

	//------------------------------------------------------------------------------------------------
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		SetEventMask(EntityEvent.PHYSICSACTIVE | EntityEvent.FRAME);
	}

	//------------------------------------------------------------------------------------------------
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);
		HAM_MuteAiHorn();
	}

	//------------------------------------------------------------------------------------------------
	void HAM_MuteAiHorn()
	{
		if (!m_VehicleController)
			return;

		PilotCompartmentSlot slot = m_VehicleController.GetPilotCompartmentSlot();
		IEntity driver;
		if (slot)
			driver = slot.GetOccupant();

		if (driver)
		{
			PlayerManager players = GetGame().GetPlayerManager();
			if (players && players.GetPlayerIdFromControlledEntity(driver) > 0)
				return;
		}

		SignalsManagerComponent signals = SignalsManagerComponent.Cast(FindComponent(SignalsManagerComponent));
		if (!signals)
			return;

		if (m_iHAM_Horn < 0)
		{
			m_iHAM_Horn = signals.FindSignal("HornState");
			if (m_iHAM_Horn < 0)
				return;
		}

		if (signals.GetSignalValue(m_iHAM_Horn) == 0)
			return;

		signals.SetSignalValue(m_iHAM_Horn, 0);
	}
}

//------------------------------------------------------------------------------------------------
modded class SCR_VehicleSoundComponent
{
	//------------------------------------------------------------------------------------------------
	override void UpdateSoundJob(IEntity owner, float timeSlice)
	{
		Vehicle vehicle = Vehicle.Cast(owner);
		if (vehicle)
			vehicle.HAM_MuteAiHorn();

		// L'hélicoptère a la liste de signaux vide. Forcer cette passe la fait planter à chaque image.
		if (!m_aNormalizedSignal || m_aNormalizedSignal.IsEmpty())
			return;

		super.UpdateSoundJob(owner, timeSlice);
	}
}
