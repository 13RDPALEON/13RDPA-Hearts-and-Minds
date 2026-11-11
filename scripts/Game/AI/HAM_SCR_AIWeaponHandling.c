modded class SCR_AIWeaponHandling
{
    static override void StartMagazineSwitchCharacter(CharacterControllerComponent controller,
        BaseMagazineComponent newMagazineComp)
    {
        super.StartMagazineSwitchCharacter(controller, newMagazineComp);

        if (!controller || !newMagazineComp)
            return;

        IEntity agentEntity = controller.GetOwner();
        if (!agentEntity)
            return;

        if (HAM_ConsumesMagazines(agentEntity))
            return;

        HAM_RefundMagazine(agentEntity, newMagazineComp);
    }

    protected static bool HAM_ConsumesMagazines(notnull IEntity agentEntity)
    {
        SCR_AIGroupUtilityComponent groupUtility = HAM_FindGroupUtility(agentEntity);

        if (groupUtility)
        {
            int groupChoice = groupUtility.HAM_GetMagazineConsumption();

            if (groupChoice == HAM_EAIMagazineConsumption.CONSUME)
                return true;

            if (groupChoice == HAM_EAIMagazineConsumption.INFINITE)
                return false;
        }

        return HAM_AISettingsComponent.HAM_ConsumeMagazines();
    }

    protected static SCR_AIGroupUtilityComponent HAM_FindGroupUtility(notnull IEntity agentEntity)
    {
        AIControlComponent controlComponent = AIControlComponent.Cast(
            agentEntity.FindComponent(AIControlComponent));

        if (!controlComponent)
            return null;

        AIAgent agent = controlComponent.GetAIAgent();
        if (!agent)
            return null;

        AIGroup group = agent.GetParentGroup();
        if (!group)
            return null;

        return SCR_AIGroupUtilityComponent.Cast(group.FindComponent(SCR_AIGroupUtilityComponent));
    }

    protected static void HAM_RefundMagazine(notnull IEntity agentEntity,
        notnull BaseMagazineComponent newMagazineComp)
    {
        InventoryStorageManagerComponent inventory = InventoryStorageManagerComponent.Cast(
            agentEntity.FindComponent(InventoryStorageManagerComponent));

        if (!inventory)
            return;

        IEntity magazineEntity = newMagazineComp.GetOwner();
        if (!magazineEntity)
            return;

        EntityPrefabData prefabData = magazineEntity.GetPrefabData();
        if (!prefabData)
            return;

        ResourceName prefabName = prefabData.GetPrefabName();
        if (prefabName.IsEmpty())
            return;

        Resource resource = Resource.Load(prefabName);
        if (!resource.IsValid())
            return;

        EntitySpawnParams params = new EntitySpawnParams();
        params.TransformMode = ETransformMode.WORLD;
        params.Transform[3] = agentEntity.GetOrigin();

        IEntity refund = GetGame().SpawnEntityPrefab(resource, null, params);
        if (!refund)
            return;

        if (inventory.CanInsertItem(refund) && inventory.TryInsertItem(refund))
            return;

        delete refund;
    }
}
