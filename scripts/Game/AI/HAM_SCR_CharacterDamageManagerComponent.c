modded class SCR_CharacterDamageManagerComponent
{
    protected bool m_bTRN_ExecutionResolved;

    override void UpdateConsciousness()
    {
        super.UpdateConsciousness();

        IEntity owner = GetOwner();
        if (!owner)
            return;

        CharacterControllerComponent controller = CharacterControllerComponent.Cast(
            owner.FindComponent(CharacterControllerComponent));

        if (!controller)
            return;

        if (!controller.IsUnconscious())
        {
            m_bTRN_ExecutionResolved = false;
            return;
        }

        if (m_bTRN_ExecutionResolved)
            return;

        m_bTRN_ExecutionResolved = true;

        int chance = HAM_AISettingsComponent.HAM_FinishOffChance();

        bool disarmed = true;
        if (chance > 0 && Math.RandomInt(0, 100) < chance)
            disarmed = false;

        PerceivableComponent perceivable = PerceivableComponent.Cast(
            owner.FindComponent(PerceivableComponent));

        if (perceivable)
            perceivable.SetDisarmed(disarmed);
    }
}
