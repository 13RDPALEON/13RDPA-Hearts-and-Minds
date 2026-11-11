[BaseContainerProps(configRoot: true)]
class HAM_AIStylePreset
{
    [Attribute("0", UIWidgets.ComboBox, "Formation de chaque sous-formation (équipe de combat)", "", ParamEnumArray.FromEnum(SCR_EAIGroupFormation), category: "Style")]
    SCR_EAIGroupFormation m_eTeamFormation;

    [Attribute("1.0", UIWidgets.EditBox, "Échelle d'espacement de la formation en déplacement. 1 = espacement d'origine du jeu", category: "Style")]
    float m_fMoveFormationScale;

    [Attribute("-1", UIWidgets.EditBox, "Plancher d'approche (m) : distance sous laquelle l'IA cesse d'avancer vers sa cible. -1 = calcul d'origine du jeu (~15 m)", category: "Style")]
    float m_fApproachFloor;

    [Attribute("5.0", UIWidgets.EditBox, "Immobilité entre les bonds (s), cas de référence à couvert. 5 = jeu d'origine ; les autres cas s'ajustent proportionnellement", category: "Style")]
    float m_fHoldTimeS;

    [Attribute("1", UIWidgets.CheckBox, "Exiger un couvert à l'arrivée pour accepter un déplacement de combat", category: "Style")]
    bool m_bRequiresCover;

    [Attribute("0", UIWidgets.CheckBox, "Riposte depuis sa position au premier contact, au lieu de décrocher d'abord vers un couvert (correctif du réflexe vanilla)", category: "Style")]
    bool m_bFiresBeforeSeekingCover;

    [Attribute("0", UIWidgets.ComboBox, "Regard donné aux IA des groupes de ce style laissés sur « Suivre le réglage général ». FROM_GLOBAL = le style ne touche pas au regard, le réglage général du scénario s'applique", "", ParamEnumArray.FromEnum(HAM_EAILookMode), category: "Style")]
    HAM_EAILookMode m_eLookMode;

    [Attribute("3", UIWidgets.EditBox, "Rayon (m) de la zone Defend donnée à chaque IA postée en intérieur par la garnison de bâtiment (clic droit « Garnir le bâtiment »). Minimum effectif 2 m : en dessous, le système de défense ne peut allouer aucune position et l'IA décroche. Les postes de sentinelle (fenêtres) restent à 2 m fixes.", category: "Style")]
    float m_fDefendRadius;
}
