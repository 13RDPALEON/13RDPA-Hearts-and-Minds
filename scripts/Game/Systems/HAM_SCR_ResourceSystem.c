// ====================================================================================
// HAM_SCR_ResourceSystem.c — 13RDPA (SERVEUR)
//
// Curseurs GM « quantité » et « délai » de régénération des DÉPÔTS de supply.
//
// Pourquoi ici et pas dans le modded SCR_CampaignMilitaryBaseComponent avec les quatre
// cases QG / réappro / sources / bases : ce sont deux mécaniques différentes. Les quatre
// cases bloquent SupplyIncomeTimer (régime des bases, soumis au point de respawn et à la
// couverture radio). Un dépôt, lui, tient sa régénération du CONTENEUR lui-même
// (SCR_ResourceContainer : m_bEnableResourceGain / m_fResourceGain / m_fResourceGainTickrate),
// sans aucune de ces conditions — SupplyIncomeTimer ne le voit même pas.
//
// AUCUNE DÉPENDANCE À SNK CONFLICT : on ne demande pas « qui est un dépôt », on applique
// à tout conteneur dont le gain est DÉJÀ actif. Ce sont exactement les dépôts, qu'ils
// viennent des caisses vanilla SupplyCache_*_Campaign posées sur la carte ou d'une base
// SNK cochée « point de supply ». Milsim fonctionne donc seul, SNK aussi, et ensemble
// Milsim prime — mais seulement si sa case est cochée.
//
// SCR_ResourceSystem est un GameSystem (instancié par le moteur, jamais désérialisé
// depuis les données du monde) : le modder est sûr, contrairement à SCR_ResourceContainer
// qui est un [BaseContainerProps] utilisé comme type de propriété dans les prefabs —
// le modder casserait le chargement du monde en silence.
// ====================================================================================
modded class SCR_ResourceSystem
{
    //! Un conteneur vient de s'enregistrer (au spawn, ou quand EnableGain l'active) :
    //! il prend les réglages Milsim au passage. Couvre les dépôts apparus en cours de
    //! partie sans avoir à balayer quoi que ce soit périodiquement.
    override void RegisterContainer(notnull SCR_ResourceContainer container)
    {
        super.RegisterContainer(container);
        HAM_ApplyDepotRegenTo(container);
        HAM_Mission.NoteSupplyContainer(container);
    }

    override void UnregisterContainer(notnull SCR_ResourceContainer container)
    {
        if (m_aTRN_StoppedContainers)
            m_aTRN_StoppedContainers.RemoveItem(container);

        super.UnregisterContainer(container);
    }

    //! Ré-application à tout le monde — appelée quand le GM bouge un curseur.
    void HAM_ApplyDepotRegenToAll()
    {
        foreach (SCR_ResourceContainer container : m_aContainers)
        {
            HAM_ApplyDepotRegenTo(container);
        }
    }

    protected void HAM_ApplyDepotRegenTo(SCR_ResourceContainer container)
    {
        if (!container) return;

        // En H&M le remplissage 10 / 20 s est celui des dépôts vanilla, pas celui de chaque caisse.
        if (HAM_Mission.IsActive())
            return;

        // Un dépôt se reconnaît à son gain — actif à l'origine, ou coupé par nous.
        // Les conteneurs ordinaires (camions, caisses inertes, stocks de base) n'ont
        // jamais de gain et ne sont donc jamais touchés.
        if (!container.IsResourceGainEnabled() && !m_aTRN_StoppedContainers.Contains(container))
            return;

        // 1. Arrêt / reprise de la régénération
        if (HAM_MilsimSettingsComponent.HAM_NoSupplyRegenDepots())
        {
            if (container.IsResourceGainEnabled())
            {
                container.EnableGain(false);
                m_aTRN_StoppedContainers.Insert(container);
            }
            return;   // inutile de régler une cadence sur un dépôt à l'arrêt
        }

        if (m_aTRN_StoppedContainers.Contains(container))
        {
            container.EnableGain(true);
            m_aTRN_StoppedContainers.RemoveItem(container);
        }

        // 2. Cadence — 0 = on laisse la valeur d'origine du dépôt
        float amount = HAM_MilsimSettingsComponent.HAM_DepotRegenAmount();
        if (amount > 0)
            container.SetResourceGain(amount);

        float tickrate = HAM_MilsimSettingsComponent.HAM_DepotRegenTickrate();
        if (tickrate > 0)
            container.SetResourceGainTickrate(tickrate);
    }

    //! Dépôts que NOUS avons arrêtés : sans ce registre, un gain coupé serait
    //! indiscernable d'un conteneur ordinaire et ne pourrait jamais repartir.
    protected ref array<SCR_ResourceContainer> m_aTRN_StoppedContainers = {};

    //! Raccourci pour les setters GM, qui n'ont pas de référence au système.
    static void HAM_RefreshDepotRegen()
    {
        ChimeraWorld world = ChimeraWorld.CastFrom(GetGame().GetWorld());
        if (!world) return;

        SCR_ResourceSystem system = SCR_ResourceSystem.Cast(world.FindSystem(SCR_ResourceSystem));
        if (system)
            system.HAM_ApplyDepotRegenToAll();
    }
}
