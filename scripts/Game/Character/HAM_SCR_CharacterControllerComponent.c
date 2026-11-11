modded class SCR_CharacterControllerComponent : CharacterControllerComponent
{
    override void OnConsciousnessChanged(bool conscious)
    {
        super.OnConsciousnessChanged(conscious);

        if (GetLifeState() != ECharacterLifeState.INCAPACITATED)
            return;

        AIControlComponent aiControl = AIControlComponent.Cast(GetOwner().FindComponent(AIControlComponent));
        if (aiControl && aiControl.IsAIActivated())
            return;

        ChimeraCharacter character = GetCharacter();
        if (!character)
            return;

        RplComponent rplComponent = character.GetRplComponent();
        if (rplComponent && !rplComponent.IsOwner())
            return;

        float dropChance = HAM_MilsimSettingsComponent.HAM_WeaponDropChance();
        if (dropChance >= 100)
            return;

        if (Math.RandomFloat(0, 100) < dropChance)
            return;

        BaseWeaponManagerComponent weaponManager = GetWeaponManagerComponent();
        if (!weaponManager || !weaponManager.GetCurrentWeapon())
            return;

        IEntity currentWeapon = weaponManager.GetCurrentWeapon().GetOwner();
        if (!currentWeapon)
            return;

        if (currentWeapon.FindComponent(GrenadeMoveComponent))
        {
            SCR_CharacterCommandHandlerComponent commandHandler = SCR_CharacterCommandHandlerComponent.Cast(GetAnimationComponent().GetCommandHandler());
            BaseTriggerComponent triggerComponent = BaseTriggerComponent.Cast(currentWeapon.FindComponent(BaseTriggerComponent));

            if ((triggerComponent && triggerComponent.WasTriggered()) || (commandHandler && commandHandler.IsThrowingAction()))
                return;
        }

        TryEquipRightHandItem(null, EEquipItemType.EEquipTypeUnarmedContextual, true);
    }

	//------------------------------------------------------------------------------------------------
	//! L'entrée du klaxon est lue pendant l'application des contrôles. On la coupe avant, seulement pour une IA.
	protected override void OnApplyControls(IEntity owner, float timeSlice)
	{
		HAM_ClearAiHorn(owner);
		super.OnApplyControls(owner, timeSlice);
	}

	//------------------------------------------------------------------------------------------------
	protected void HAM_ClearAiHorn(IEntity owner)
	{
		if (!owner)
			return;

		PlayerManager players = GetGame().GetPlayerManager();
		if (players && players.GetPlayerIdFromControlledEntity(owner) > 0)
			return;

		CompartmentAccessComponent access = CompartmentAccessComponent.Cast(owner.FindComponent(CompartmentAccessComponent));
		if (!access || !access.IsInCompartment())
			return;

		CharacterInputContext input = GetInputContext();
		if (!input)
			return;

		input.SetVehicleHorn(0);
	}
}
