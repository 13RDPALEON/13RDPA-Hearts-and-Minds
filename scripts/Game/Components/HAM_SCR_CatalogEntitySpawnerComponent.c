// ====================================================================================
// HAM_SCR_CatalogEntitySpawnerComponent.c — 13RDPA
//
// Générateurs du Conflit (petit et grand garage, hélipad) en H&M :
// - toutes les factions et le catalogue sans faction, comme le tableau de tri véhicules,
//   filtrés par les étiquettes du générateur et sans doublon ; les casernes gardent leur faction ;
// - le prefab n'a que quelques actions « Demander » : une seule reste, « Véhicules disponibles »,
//   qui ouvre la liste complète (HAM_VehicleSpawnMenu.c) ; la demande reste celle du Conflit ;
// - un véhicule masqué au tableau disparaît de la liste, donc aussi de la demande côté serveur ;
// - prix et grade du tableau : SCR_EntityCatalogSpawnerData ci-dessous ;
// - « Supprimer le rang minimum » retire le grade vanilla, mais un grade réglé au tableau reste exigé ;
// - aucun délai entre deux demandes ; plafonds d'IA et de recrues : HAM_NoLimits.c.
// ====================================================================================
modded class SCR_CatalogEntitySpawnerComponent
{
    override bool RankCheck(notnull SCR_EntityCatalogEntry entityEntry, notnull IEntity user)
    {
        if (!HAM_MilsimSettingsComponent.HAM_NoMinimumRank())
            return super.RankCheck(entityEntry, user);

        string prefab = entityEntry.GetPrefab();
        if (!HAM_VehicleBoardStore.HasRankOverride(prefab))
            return true;

        SCR_ECharacterRank rank = SCR_CharacterRankComponent.GetCharacterRank(user);
        if (rank == SCR_ECharacterRank.INVALID)
            return false;

        return rank >= HAM_VehicleBoardStore.GetRankOverride(prefab, SCR_ECharacterRank.PRIVATE);
    }

    //------------------------------------------------------------------------------------------------
    //! Pas de délai entre deux demandes : autant de véhicules et d'unités que voulu.
    override protected bool CooldownCheck(notnull IEntity user)
    {
        if (HAM_Mission.IsActive())
            return true;

        return super.CooldownCheck(user);
    }

    //------------------------------------------------------------------------------------------------
    //! Garage ou hélipad en H&M ; une caserne (personnages, groupes) reste vanilla.
    bool HAM_IsVehicleList()
    {
        if (SCR_Global.IsEditMode() || !HAM_Mission.IsActive() || !m_aCatalogTypes)
            return false;

        return m_aCatalogTypes.Contains(EEntityCatalogType.VEHICLE) && !m_aCatalogTypes.Contains(EEntityCatalogType.CHARACTER) && !m_aCatalogTypes.Contains(EEntityCatalogType.GROUP);
    }

    //------------------------------------------------------------------------------------------------
    int HAM_AssetCount()
    {
        if (!m_aAssetList)
            return 0;

        return m_aAssetList.Count();
    }

    //------------------------------------------------------------------------------------------------
    override void InitiateSpawn(notnull SCR_EntityCatalogEntry entityEntry, int userId, SCR_EntitySpawnerSlotComponent slot)
    {
        SCR_EntityCatalogSpawnerData data = SCR_EntityCatalogSpawnerData.Cast(entityEntry.GetEntityDataOfType(SCR_EntityCatalogSpawnerData));
        if (data && HAM_IsVehicleList())
        {
            string prefab = entityEntry.GetPrefab();
            string source = "vanilla";
            if (HAM_VehicleBoardStore.HasCostOverride(prefab))
                source = "tableau de tri";

            HAM_Util.Info(string.Format("Demande au garage : %1, prix %2 (%3), grade %4, fournitures de la base %5", HAM_Util.PrefabFileName(prefab), data.GetSupplyCost(), source, typename.EnumToString(SCR_ECharacterRank, data.GetMinimumRequiredRank()), GetSpawnerResourceValue()));
        }

        super.InitiateSpawn(entityEntry, userId, slot);
        if (HAM_IsVehicleList() && slot && slot.GetOwner())
            HAM_VehiclePersist.WatchSpawn(entityEntry.GetPrefab(), slot.GetOwner().GetOrigin());
    }

    //------------------------------------------------------------------------------------------------
    //! Une seule action reste visible : elle ouvre la liste complète au lieu d'un véhicule par action.
    override protected void AssignUserActions()
    {
        if (!HAM_IsVehicleList())
        {
            super.AssignUserActions();
            return;
        }

        if (!m_ActionManager)
            return;

        array<BaseUserAction> userActions = {};
        m_ActionManager.GetActionsList(userActions);
        bool assigned = false;
        foreach (BaseUserAction userAction : userActions)
        {
            SCR_CatalogSpawnerUserAction spawnerAction = SCR_CatalogSpawnerUserAction.Cast(userAction);
            if (!spawnerAction)
                continue;

            if (!assigned && !m_aAssetList.IsEmpty())
            {
                spawnerAction.SetSpawnerData(m_aAssetList[0]);
                assigned = true;
                continue;
            }

            spawnerAction.SetSpawnerData(null);
        }
    }

    //------------------------------------------------------------------------------------------------
    override protected void SetCurrentFactionCatalog()
    {
        if (!HAM_IsVehicleList())
        {
            super.SetCurrentFactionCatalog();
            return;
        }

        m_aAssetList.Clear();
        if (!GetFaction() || m_aCatalogTypes.IsEmpty())
        {
            AssignUserActions();
            return;
        }

        SCR_EntityCatalogManagerComponent catalogManager = SCR_EntityCatalogManagerComponent.GetInstance();
        array<Faction> factions = {};
        FactionManager factionManager = GetGame().GetFactionManager();
        if (factionManager)
            factionManager.GetFactionsList(factions);

        foreach (EEntityCatalogType catalogType : m_aCatalogTypes)
        {
            if (catalogManager)
            {
                SCR_EntityCatalog sharedCatalog = catalogManager.GetEntityCatalogOfType(catalogType);
                if (sharedCatalog)
                    AddAssetsFromCatalog(sharedCatalog);
            }

            foreach (Faction faction : factions)
            {
                SCR_Faction scrFaction = SCR_Faction.Cast(faction);
                if (!scrFaction)
                    continue;

                SCR_EntityCatalog catalog = scrFaction.GetFactionEntityCatalogOfType(catalogType, false);
                if (catalog)
                    AddAssetsFromCatalog(catalog);
            }
        }

        AssignUserActions();

        string ownerName = "?";
        if (GetOwner() && GetOwner().GetPrefabData())
            ownerName = HAM_Util.PrefabFileName(GetOwner().GetPrefabData().GetPrefabName());

        HAM_Util.Info(string.Format("Garage %1 : %2 véhicules toutes factions, une action « Véhicules disponibles »", ownerName, m_aAssetList.Count()));
    }

    //------------------------------------------------------------------------------------------------
    override protected void AddAssetsFromCatalog(notnull SCR_EntityCatalog entityCatalog, bool overwriteOld = false)
    {
        super.AddAssetsFromCatalog(entityCatalog, overwriteOld);

        if (SCR_Global.IsEditMode() || !m_aAssetList)
            return;

        set<string> seen = new set<string>();
        int i = 0;
        while (i < m_aAssetList.Count())
        {
            SCR_EntityCatalogEntry entry = m_aAssetList[i];
            string prefab;
            if (entry)
                prefab = entry.GetPrefab();

            if (!entry || seen.Contains(prefab) || HAM_VehicleBoardStore.IsHidden(prefab))
            {
                m_aAssetList.RemoveOrdered(i);
                continue;
            }

            seen.Insert(prefab);
            i++;
        }
    }

    //------------------------------------------------------------------------------------------------
    //! Reconstruit la liste après un changement du tri véhicules.
    void HAM_RefreshCatalog()
    {
        SetCurrentFactionCatalog();
    }

    //------------------------------------------------------------------------------------------------
    override protected void OnPostInit(IEntity owner)
    {
        super.OnPostInit(owner);
        HAM_VehicleBoardStore.RegisterSpawner(this);

        // Le mode de jeu peut naître après le générateur : la liste toutes factions est refaite ensuite.
        if (GetGame().InPlayMode())
            GetGame().GetCallqueue().CallLater(HAM_RefreshCatalog, 2000, false);
    }

    //------------------------------------------------------------------------------------------------
    override protected void OnDelete(IEntity owner)
    {
        GetGame().GetCallqueue().Remove(HAM_RefreshCatalog);
        HAM_VehicleBoardStore.UnregisterSpawner(this);
        super.OnDelete(owner);
    }
}

//------------------------------------------------------------------------------------------------
//! En H&M, l'action restante d'un garage ou d'un hélipad ouvre la liste complète des véhicules.
modded class SCR_CatalogSpawnerUserAction
{
    //------------------------------------------------------------------------------------------------
    protected bool HAM_IsListAction()
    {
        return m_EntitySpawner && m_EntitySpawner.HAM_IsVehicleList();
    }

    //------------------------------------------------------------------------------------------------
    override bool GetActionNameScript(out string outName)
    {
        if (!HAM_IsListAction())
            return super.GetActionNameScript(outName);

        outName = string.Format("Véhicules disponibles (%1)", m_EntitySpawner.HAM_AssetCount());
        return true;
    }

    //------------------------------------------------------------------------------------------------
    override bool CanBeShownScript(IEntity user)
    {
        if (!HAM_IsListAction())
            return super.CanBeShownScript(user);

        if (!m_EntityData)
            return false;

        SCR_ChimeraCharacter character = SCR_ChimeraCharacter.Cast(user);
        return character && character.GetFaction() == m_EntitySpawner.GetFaction();
    }

    //------------------------------------------------------------------------------------------------
    override bool CanBePerformedScript(IEntity user)
    {
        if (!HAM_IsListAction())
            return super.CanBePerformedScript(user);

        return true;
    }

    //------------------------------------------------------------------------------------------------
    override void OnActionSelected()
    {
        if (!HAM_IsListAction())
        {
            super.OnActionSelected();
            return;
        }

        m_bIsSelected = true;
    }

    //------------------------------------------------------------------------------------------------
    override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
    {
        if (!HAM_IsListAction())
        {
            super.PerformAction(pOwnerEntity, pUserEntity);
            return;
        }

        HAM_VehicleSpawnUI.OpenForSpawner(m_EntitySpawner);
    }
}

//------------------------------------------------------------------------------------------------
// Classe de config chargée depuis les catalogues : l'attribut vanilla doit être répété sur la version moddée.
[BaseContainerProps(configRoot: true), BaseContainerCustomCheckIntTitleField("m_bEnabled", "SpawnerData", "DISABLED - SpawnerData", 1)]
modded class SCR_EntityCatalogSpawnerData
{
    //------------------------------------------------------------------------------------------------
    //! Prix du tableau véhicules s'il en a un : affichage, demande et paiement aux générateurs du Conflit.
    override int GetSupplyCost()
    {
        if (HAM_VehicleBoardStore.HasCostOverride(m_sDefaultPrefab))
            return HAM_VehicleBoardStore.BoardPrice(m_sDefaultPrefab);

        return super.GetSupplyCost();
    }

    //------------------------------------------------------------------------------------------------
    override SCR_ECharacterRank GetMinimumRequiredRank()
    {
        if (HAM_VehicleBoardStore.HasRankOverride(m_sDefaultPrefab))
            return HAM_VehicleBoardStore.GetRankOverride(m_sDefaultPrefab, super.GetMinimumRequiredRank());

        return super.GetMinimumRequiredRank();
    }

    //------------------------------------------------------------------------------------------------
    int HAM_VanillaSupplyCost()
    {
        return m_iSupplyCost;
    }

    //------------------------------------------------------------------------------------------------
    SCR_ECharacterRank HAM_VanillaRank()
    {
        return m_eMinimumRequiredRank;
    }
}
