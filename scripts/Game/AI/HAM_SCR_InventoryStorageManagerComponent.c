modded class SCR_InventoryStorageManagerComponent
{
    protected void HAM_OnWeaponStorageChanged(BaseInventoryStorageComponent storageOwner)
    {
        SCR_CharacterInventoryStorageComponent characterStorage = GetCharacterStorage();
        if (!characterStorage || storageOwner != characterStorage.GetWeaponStorage())
            return;

        IEntity owner = GetOwner();
        if (!owner)
            return;

        SCR_CharacterPerceivableComponent perceivable = SCR_CharacterPerceivableComponent.Cast(
            owner.FindComponent(SCR_CharacterPerceivableComponent));

        if (perceivable)
            perceivable.HAM_NotifyWeaponStorageChanged();
    }

    override protected void OnItemAdded(BaseInventoryStorageComponent storageOwner, IEntity item)
    {
        super.OnItemAdded(storageOwner, item);

        HAM_OnWeaponStorageChanged(storageOwner);
    }

    override protected void OnItemRemoved(BaseInventoryStorageComponent storageOwner, IEntity item)
    {
        super.OnItemRemoved(storageOwner, item);

        HAM_OnWeaponStorageChanged(storageOwner);
    }
}
