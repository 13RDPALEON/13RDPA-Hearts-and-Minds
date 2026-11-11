// ====================================================================================
// HAM_SCR_CampaignMilitaryBaseComponent.c — 13RDPA (SERVEUR)
//
// Toggles GM « Stopper le supply auto » par catégorie de base (mode Conflit).
//
// Mécanique vanilla : chaque base a une boucle CallLater → SupplyIncomeTimer qui
// crédite périodiquement du supply (AddRegularSupplyPackage) aux bases capturées,
// couvertes par le réseau radio HQ (sauf SOURCE_BASE qui n'exige pas la radio) et
// pas en cours de capture. C'est le lien radio (antenne/relais de commandement) qui
// déclenche la régénération « toute seule » d'une base couverte.
//
// Blocage au point unique SupplyIncomeTimer, en reproduisant le chemin vanilla des
// bases inéligibles (m_iSupplyRegenAmount = 0 + BumpMe) → l'indicateur de revenu
// affiché aux joueurs tombe aussi à zéro. Le chemin reset (réarmement du timer à
// l'init/capture) reste vanilla. À la désactivation du blocage, le prochain tick
// crédite un paquet puis réarme le timer (même sémantique que la reprise vanilla).
//
// Catégories (priorité de classement : QG > point de réappro > source > base) :
//   - QG            : m_bIsHQ (base principale de faction)
//   - Réappro       : m_bIsSupplyHub (points de réapprovisionnement)
//   - Source        : m_eType == SOURCE_BASE (régénère sans lien radio)
//   - Bases         : tout le reste (relais inclus — ils ne régénèrent de toute
//                     façon pas, faute de spawn point)
//
// Chaîne modded : HAM_Conflict_Mod ne touche que SetAsHQ/EvaluateDefenders → libre.
// ====================================================================================
modded class SCR_CampaignMilitaryBaseComponent
{
    protected bool HAM_IsSupplyRegenBlocked()
    {
        if (m_bIsHQ)
            return HAM_MilsimSettingsComponent.HAM_NoSupplyRegenHQ();

        if (m_bIsSupplyHub)
            return HAM_MilsimSettingsComponent.HAM_NoSupplyRegenHubs();

        if (m_eType == SCR_ECampaignBaseType.SOURCE_BASE)
            return HAM_MilsimSettingsComponent.HAM_NoSupplyRegenSources();

        return HAM_MilsimSettingsComponent.HAM_NoSupplyRegenBases();
    }

    override protected void SupplyIncomeTimer(bool reset = false)
    {
        if (!reset && HAM_IsSupplyRegenBlocked())
        {
            m_iSupplyRegenAmount = 0;
            Replication.BumpMe();
            return;
        }

        super.SupplyIncomeTimer(reset);
    }
}
