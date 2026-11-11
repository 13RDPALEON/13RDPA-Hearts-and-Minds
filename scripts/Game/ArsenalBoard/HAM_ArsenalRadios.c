//------------------------------------------------------------------------------------------------
// Les sacs AN/PRC-119 ne sont plus insérés ici.
// SCR_EntityCatalog.InitCatalog est le seul passage qui fusionne les listes,
// appelle InitEntry et remplit m_mPrefabIndexes. Le refaire après coup
// (entrée créée à la main + InitEntry + réécriture de l'index) cassait
// le catalogue déjà construit : l'arsenal ne gardait plus que ces sacs.
// Les trois prefabs sont des entrées du catalogue ITEM de la faction
// joueur dans worlds/HAM_Cain/HAM_Cain_Layers/default.layer.
//------------------------------------------------------------------------------------------------
