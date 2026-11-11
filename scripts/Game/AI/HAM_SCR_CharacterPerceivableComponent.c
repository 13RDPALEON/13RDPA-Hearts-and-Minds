modded class SCR_CharacterPerceivableComponent
{
    protected static const int HAM_INIT_REFRESH_DELAY_MS = 3000;

    protected BaseWeaponManagerComponent m_HAM_WeaponManager;

    override void EOnInit(IEntity owner)
    {
        super.EOnInit(owner);

        if (!owner)
            return;

        m_HAM_WeaponManager = BaseWeaponManagerComponent.Cast(
            owner.FindComponent(BaseWeaponManagerComponent));

        if (m_HAM_WeaponManager)
            m_HAM_WeaponManager.m_OnWeaponChangeCompleteInvoker.Insert(HAM_OnWeaponChanged);

        GetGame().GetCallqueue().CallLater(HAM_RefreshDisarmed, HAM_INIT_REFRESH_DELAY_MS, false);
    }

    void ~SCR_CharacterPerceivableComponent()
    {
        if (m_HAM_WeaponManager)
            m_HAM_WeaponManager.m_OnWeaponChangeCompleteInvoker.Remove(HAM_OnWeaponChanged);

        GetGame().GetCallqueue().Remove(HAM_RefreshDisarmed);
    }

    protected void HAM_OnWeaponChanged(BaseWeaponComponent weapon)
    {
        HAM_RefreshDisarmed();
    }

    void HAM_NotifyWeaponStorageChanged()
    {
        HAM_RefreshDisarmed();
    }

    override protected void OnLifeStateChanged(ECharacterLifeState previousLifeState, ECharacterLifeState newLifeState)
    {
        super.OnLifeStateChanged(previousLifeState, newLifeState);

        if (newLifeState == ECharacterLifeState.ALIVE)
            HAM_RefreshDisarmed();
    }

    static void HAM_RefreshAllCharacters()
    {
        AIWorld aiWorld = GetGame().GetAIWorld();
        if (aiWorld)
        {
            array<AIAgent> agents = {};
            aiWorld.GetAIAgents(agents);

            foreach (AIAgent agent : agents)
            {
                if (agent)
                    HAM_RefreshEntity(agent.GetControlledEntity());
            }
        }

        PlayerManager playerManager = GetGame().GetPlayerManager();
        if (!playerManager)
            return;

        array<int> players = {};
        playerManager.GetPlayers(players);

        foreach (int playerId : players)
        {
            HAM_RefreshEntity(playerManager.GetPlayerControlledEntity(playerId));
        }
    }

    protected static void HAM_RefreshEntity(IEntity character)
    {
        if (!character)
            return;

        SCR_CharacterPerceivableComponent perceivable = SCR_CharacterPerceivableComponent.Cast(
            character.FindComponent(SCR_CharacterPerceivableComponent));

        if (perceivable)
            perceivable.HAM_RefreshDisarmed();
    }

    protected void HAM_RefreshDisarmed()
    {
        if (!m_CharacterController || m_CharacterController.GetLifeState() != ECharacterLifeState.ALIVE)
            return;

        if (!HAM_AISettingsComponent.HAM_SpareUnarmed())
        {
            SetDisarmed(false);
            return;
        }

        SetDisarmed(!HAM_CarriesAnyWeapon());
    }

    protected bool HAM_CarriesAnyWeapon()
    {
        if (!m_HAM_WeaponManager)
            return false;

        array<WeaponSlotComponent> slots = {};
        m_HAM_WeaponManager.GetWeaponsSlots(slots);

        foreach (WeaponSlotComponent slot : slots)
        {
            if (slot && slot.GetWeaponEntity())
                return true;
        }

        return false;
    }
}
