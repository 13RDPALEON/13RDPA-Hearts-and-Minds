class HAM_MilsimUserSettings : ModuleGameSettings
{
    [Attribute(defvalue: "100", uiwidget: UIWidgets.Slider, params: "50 150 5", desc: "Taille du compas, en pourcentage de sa taille d'origine")]
    float m_fCompassScale;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, params: "0 1 1", desc: "Position du compas : 0 en haut de l'ecran, 1 en bas")]
    int m_iCompassPosition;

    [Attribute(defvalue: "0", uiwidget: UIWidgets.EditBox, params: "0 7 1", desc: "Couleur du compas : 0 blanc, 1 noir, 2 rouge, 3 vert, 4 bleu, 5 jaune, 6 cyan, 7 orange")]
    int m_iCompassColor;
};
