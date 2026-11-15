//------------------------------------------------------------------------------------------------
// Kolguyev (monde Cain). Carte de base de la mission.
// Les lignes sont écrites à partir des coordonnées relevées en jeu. Sans ligne, pas de front.
// Le QG et le QG ennemi doivent correspondre aux deux bases Conflit posées dans
// worlds/HAM_Cain/HAM_Cain_Layers/default.layer.
//
// Format :
//   def.Hq("QG", Vector(x, y, z), rayon);
//   def.Master("Nom", HAM_EMasterType.CITY, Vector(x, y, z), rayon, "Voisin1;Voisin2;QG");
//   def.Slave("Nom", "Maître", Vector(x, y, z), rayon);
//   def.Slave("Nom", "Maître", Vector(x, y, z), rayon, HAM_EMasterType.BASE);
//   Types d'esclave : FUEL, CAMP, HIDEOUT, BASE. La radio reste un type de maître.
//   def.Deposit("Nom", "Maître", Vector(x, y, z));
//   Un dépôt vanilla ou personnel est un esclave : pas d'icône, même temps de prise.
//   def.Slave("Nom", "Maître", Vector(x, y, z), rayon, HAM_EMasterType.SUPPLY_VANILLA);
//   def.Slave("Nom", "Maître", Vector(x, y, z), rayon, HAM_EMasterType.SUPPLY_PERSONAL);
//   def.Patrol("Nom", "Maître", Vector(x, y, z), rayon);
//   def.Cqb("Nom", "Maître", Vector(x, y, z), rayon);
//   def.Ied("Nom", "Maître", Vector(x, y, z), rayon);
//   def.Mines("Nom", "Maître", Vector(x, y, z), rayon);
//   def.Civilians("Nom", "Maître", Vector(x, y, z), rayon);
//   def.EnemyHq("Nom", Vector(x, y, z), rayon, "Voisin");
// Types de maître : FUEL, RADIO, CAMP, HIDEOUT, CITY, BASE.
//------------------------------------------------------------------------------------------------

class HAM_Map_Cain
{
	static void Build(notnull HAM_MapDef def)
	{
		def.Hq("QG", Vector(2602.366, 24.219, 5384.033), 150);
		// Le QG ennemi s'ouvre quand la cellule LeRocher est complète, pas dès le départ.
		def.EnemyHq("QG ennemi", Vector(12029.178, 2.902, 10492.373), 100, "LeRocher");

		// Udarnik, première cellule après le QG joueur. Les noms des lignes ne sont pas affichés.
		def.Master("Udarnik", HAM_EMasterType.CITY, Vector(4415.882, 40.289, 3544.781), 300, "QG");
		def.Slave("Udarnik West", "Udarnik", Vector(3801.861, 5.024, 3738.642), 200, HAM_EMasterType.HIDEOUT);
		def.Slave("Base de Udarnik", "Udarnik", Vector(4367.679, 55.188, 3219.023), 150, HAM_EMasterType.CAMP);
		def.Slave("Base Est de Udarnik", "Udarnik", Vector(5337.222, 64.969, 3370.426), 200, HAM_EMasterType.FUEL);
		def.Slave("Ressources", "Udarnik", Vector(3430.24, 21.721, 4290.794), 200, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Port de Udarnik", "Udarnik", Vector(4858.461, 4.62, 2935.403), 200, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Slave("Tour de la Hauteur de Udarnik", "Udarnik", Vector(4964.035, 253.203, 4398.712), 300, HAM_EMasterType.RADIO);

		def.Cqb("Udarnik CQB 1", "Udarnik", Vector(4366.721, 22.688, 2916.182), 100);
		def.Cqb("Udarnik CQB 2", "Udarnik", Vector(4298.874, 40.451, 3577.434), 80);
		def.Cqb("Udarnik CQB 3", "Udarnik", Vector(4722.666, 56.698, 3678.883), 50);

		def.Mines("Udarnik mines 1", "Udarnik", Vector(4372.615, 22.776, 2937.426), 100);
		def.Mines("Udarnik mines 2", "Udarnik", Vector(4294.005, 40.407, 3577.27), 80);
		def.Ied("Udarnik IED 1", "Udarnik", Vector(4372.615, 22.776, 2937.426), 100);

		def.Patrol("Udarnik patrouille 1", "Udarnik", Vector(4364.754, 23.578, 2929.435), 100);
		def.Patrol("Udarnik patrouille 2", "Udarnik", Vector(5054.934, 88.925, 3431.66), 100);
		def.Patrol("Udarnik patrouille 3", "Udarnik", Vector(5453.272, 45.75, 3218.629), 100);
		def.Patrol("Udarnik patrouille 4", "Udarnik", Vector(3626.309, 10.327, 3935.866), 100);

		def.Civilians("Udarnik civils 1", "Udarnik", Vector(2721.704, 49.614, 5474.847), 100);

		// Ugoldar, collée à Udarnik. Coordonnées des marqueurs posés dans les calques.
		def.Master("Ugoldar", HAM_EMasterType.CITY, Vector(6644.696, 25.821, 3905.052), 500, "Udarnik");
		def.Slave("Ugoldar West", "Ugoldar", Vector(6515.242, 24.668, 3709.261), 200, HAM_EMasterType.HIDEOUT);
		def.Slave("Port d'Ugoldar", "Ugoldar", Vector(6648.376, 2.648, 3404.199), 200, HAM_EMasterType.FUEL);
		def.Slave("Ressources d'Ugoldar", "Ugoldar", Vector(7659.757, 5.098, 4511.684), 200, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Dépôt d'Ugoldar", "Ugoldar", Vector(5900.229, 188.937, 4490.22), 300, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Slave("Base radio d'Ugoldar", "Ugoldar", Vector(5974.035, 9.303, 3113.396), 200, HAM_EMasterType.RADIO);
		def.Slave("Base militaire de Ugoldar", "Ugoldar", Vector(7668.438, 23.161, 5080.069), 200, HAM_EMasterType.BASE);
		def.Slave("Camp de Ugoldar", "Ugoldar", Vector(7478.902, 3.196, 4196.831), 200, HAM_EMasterType.CAMP);

		def.Cqb("Ugoldar CQB 1", "Ugoldar", Vector(7427.526, 22.05, 4861.751), 80);
		def.Cqb("Ugoldar CQB 2", "Ugoldar", Vector(6794.294, 27.644, 4023.215), 100);
		def.Cqb("Ugoldar CQB 3", "Ugoldar", Vector(6993.198, 5.223, 3851.927), 80);
		def.Cqb("Ugoldar CQB 4", "Ugoldar", Vector(6507.437, 7.594, 2536.282), 80);
		def.Cqb("Ugoldar CQB 5", "Ugoldar", Vector(7611.186, 19.776, 4825.583), 200);

		def.Mines("Ugoldar mines 1", "Ugoldar", Vector(7000.586, 4.909, 3849.348), 80);
		def.Mines("Ugoldar mines 2", "Ugoldar", Vector(6375.318, 6.58, 3251.746), 200);
		def.Mines("Ugoldar mines 3", "Ugoldar", Vector(6284.42, 3.969, 2864.983), 300);
		def.Mines("Ugoldar mines 4", "Ugoldar", Vector(6507.437, 7.594, 2536.282), 80);
		def.Mines("Ugoldar mines 5", "Ugoldar", Vector(7611.186, 19.776, 4825.583), 200);
		def.Mines("Ugoldar mines 6", "Ugoldar", Vector(5862.045, 90.775, 3583.308), 250);
		def.Mines("Ugoldar mines 7", "Ugoldar", Vector(5871.59, 171.142, 4183.754), 200);
		def.Mines("Ugoldar mines 8", "Ugoldar", Vector(6815.721, 56.453, 4403.33), 250);

		def.Ied("Ugoldar IED 1", "Ugoldar", Vector(6375.318, 6.58, 3251.746), 200);
		def.Ied("Ugoldar IED 2", "Ugoldar", Vector(6284.42, 3.969, 2864.983), 300);
		def.Ied("Ugoldar IED 3", "Ugoldar", Vector(6815.721, 56.453, 4403.33), 250);

		def.Patrol("Ugoldar patrouille 1", "Ugoldar", Vector(7427.053, 21.99, 4865.29), 80);
		def.Patrol("Ugoldar patrouille 2", "Ugoldar", Vector(6375.318, 6.58, 3251.746), 200);
		def.Patrol("Ugoldar patrouille 3", "Ugoldar", Vector(6284.42, 3.969, 2864.983), 300);
		def.Patrol("Ugoldar patrouille 4", "Ugoldar", Vector(6507.437, 7.594, 2536.282), 80);
		def.Patrol("Ugoldar patrouille 5", "Ugoldar", Vector(7611.186, 19.776, 4825.583), 200);
		def.Patrol("Ugoldar patrouille 6", "Ugoldar", Vector(5862.045, 90.775, 3583.308), 250);
		def.Patrol("Ugoldar patrouille 7", "Ugoldar", Vector(5871.59, 171.142, 4183.754), 200);
		def.Patrol("Ugoldar patrouille 8", "Ugoldar", Vector(6815.721, 56.453, 4403.33), 250);

		// Ulfid, voisine du QG.
		def.Master("Ulfid", HAM_EMasterType.CITY, Vector(2230.251, 126.125, 7355.555), 200, "QG");
		def.Slave("Camp de Ulfid", "Ulfid", Vector(1928.308, 132.501, 7402.713), 300, HAM_EMasterType.CAMP);
		def.Slave("Carburant de Ulfid", "Ulfid", Vector(2699.387, 221.312, 6991.725), 200, HAM_EMasterType.FUEL);
		def.Slave("Fard de Ulfid", "Ulfid", Vector(1650.525, 110.719, 7396.865), 200, HAM_EMasterType.RADIO);
		def.Slave("Mine de Ulfid", "Ulfid", Vector(5550.407, 1.964, 8209.319), 200, HAM_EMasterType.RADIO);
		def.Slave("Sceirie de Ulfid", "Ulfid", Vector(2729.641, 110.695, 7474.375), 200, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Mines("Mines de Ulfid", "Ulfid", Vector(1418.114, 0.381, 7690.279), 200);
		def.Patrol("Patrouille de Ulfid", "Ulfid", Vector(1418.114, 0.381, 7690.279), 200);

		// Hamertin. Voisin : Ulfid (le calque écrit VUIlfid).
		def.Master("Hamertin", HAM_EMasterType.CITY, Vector(3286.324, 268.281, 6792.042), 400, "Ulfid");
		def.Slave("Tour de Hamertin", "Hamertin", Vector(3219.995, 266.591, 6747.567), 200, HAM_EMasterType.RADIO);
		def.Slave("Nord de Hamertin", "Hamertin", Vector(4326.48, 448.688, 6272.091), 200, HAM_EMasterType.RADIO);
		def.Slave("Port de Hamertin", "Hamertin", Vector(3672.575, 1.718, 7942.549), 300, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Mine de Hamertin", "Hamertin", Vector(3688.265, 180.143, 5191.224), 300, HAM_EMasterType.SUPPLY_PERSONAL);

		// Cain, voisine d'Ulfid.
		def.Master("Cain", HAM_EMasterType.CITY, Vector(6327.804, 37.599, 8004.253), 400, "Ulfid");
		def.Slave("Mine de Sel de Cain", "Cain", Vector(5903.291, 2.569, 8280.392), 500, HAM_EMasterType.CAMP);
		def.Slave("Carburant de Cain", "Cain", Vector(6120.542, 39.246, 7976.936), 300, HAM_EMasterType.FUEL);
		def.Slave("Plage de Cain", "Cain", Vector(4400.554, 2.487, 8148.061), 300, HAM_EMasterType.RADIO);
		def.Slave("Camp de Cain", "Cain", Vector(5078.44, 176.486, 7586.357), 300, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Cqb("CQB 1 de Cain", "Cain", Vector(6210.534, 37.224, 8012.943), 100);
		def.Mines("Mines de Cain", "Cain", Vector(5891.775, 2.641, 8407.846), 200);
		def.Ied("IED de Cain", "Cain", Vector(5891.775, 2.641, 8407.846), 200);
		def.Patrol("Patrouille de Cain", "Cain", Vector(5891.775, 2.641, 8407.846), 200);

		// Soumont, voisine d'Udarnik.
		def.Master("Soumont", HAM_EMasterType.CITY, Vector(5272.756, 407.197, 5326.531), 100, "Udarnik");
		def.Slave("Soumont West", "Soumont", Vector(5049.709, 410.467, 5309.485), 400, HAM_EMasterType.HIDEOUT);
		def.Slave("Scierie de Soumont", "Soumont", Vector(5375.938, 403.065, 5299.106), 200, HAM_EMasterType.FUEL);
		def.Slave("Bassin de Soumont", "Soumont", Vector(4807.688, 439.946, 5775.667), 300, HAM_EMasterType.SUPPLY_VANILLA);

		// PikdeCain. Voisins : Cain, Hamertin, Soumont. Ugold n'est pas cité.
		def.Master("PikdeCain", HAM_EMasterType.CITY, Vector(5089.824, 688.671, 6347.482), 100, "Cain;Hamertin;Soumont");
		def.Mines("Mines de PikdeCain", "PikdeCain", Vector(5082.457, 689.37, 6339.071), 100);
		def.Patrol("Patrouille de PikdeCain", "PikdeCain", Vector(5082.457, 689.37, 6339.071), 100);

		// Ugold. Viille du calque est Ville. Voisin unique : PikdeCain.
		def.Master("Ugold", HAM_EMasterType.CITY, Vector(6524.823, 493.293, 6614.982), 200, "PikdeCain");
		def.Slave("Ugold West", "Ugold", Vector(6084.18, 418.514, 6241.288), 400, HAM_EMasterType.HIDEOUT);
		def.Slave("Caserne de Ugold", "Ugold", Vector(6943.775, 436.688, 6768.651), 500, HAM_EMasterType.CAMP);
		def.Slave("Base militaire de Ugold", "Ugold", Vector(7132.497, 314.322, 6037.294), 300, HAM_EMasterType.BASE);
		def.Slave("Camp militaire de Ugold", "Ugold", Vector(5660.638, 421.506, 6846.87), 400, HAM_EMasterType.RADIO);
		def.Slave("Reserve de Ugold", "Ugold", Vector(5665.448, 420.233, 6933.698), 100, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Lac de Ugold", "Ugold", Vector(5438.6, 441.356, 6016.511), 100, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Cqb("CQB 1 de Ugold", "Ugold", Vector(6780.034, 441.812, 6752.478), 100);

		// Gorshkov, voisine de PikdeCain.
		def.Master("Gorshkov", HAM_EMasterType.CITY, Vector(7754.825, 312.131, 8172.653), 400, "PikdeCain");
		def.Slave("Ville de Gorshkov", "Gorshkov", Vector(8452.417, 21.219, 6494.947), 400, HAM_EMasterType.HIDEOUT);
		def.Slave("Camp militaire de Gorshkov", "Gorshkov", Vector(8029, 182.719, 7756.903), 400, HAM_EMasterType.CAMP);
		def.Slave("Rafinerie de Gorshkov", "Gorshkov", Vector(8307.77, 46.906, 6714.747), 300, HAM_EMasterType.FUEL);
		def.Slave("Base militaire de Gorshkov", "Gorshkov", Vector(8739.86, 9.344, 7460.356), 500, HAM_EMasterType.BASE);
		def.Slave("Port de Gorshkov", "Gorshkov", Vector(7011.203, 1.094, 8772.475), 300, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Mine de Gorshkov", "Gorshkov", Vector(7652.942, 208.184, 6809.952), 400, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Cqb("CQB 1 de Gorshkov", "Gorshkov", Vector(8402.425, 22.719, 6421.382), 100);

		// Komsomo, voisine de Gorshkov.
		def.Master("Komsomo", HAM_EMasterType.CITY, Vector(8623.405, 95.977, 9284.329), 500, "Gorshkov");
		def.Slave("Ville de Komsomo", "Komsomo", Vector(8597.298, 3.938, 10176.605), 400, HAM_EMasterType.HIDEOUT);
		def.Slave("Base militaire de Komsomo", "Komsomo", Vector(9610.193, 11.563, 8716.397), 500, HAM_EMasterType.BASE);
		def.Slave("Village de Komsomo", "Komsomo", Vector(9260.736, 93.489, 9499.706), 300, HAM_EMasterType.RADIO);
		def.Slave("Port de Komsomo", "Komsomo", Vector(9223.82, 4.948, 8259.13), 300, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Depot de Komsomo", "Komsomo", Vector(8253.001, 10.521, 9890.739), 200, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Mines("Mines de Komsomo", "Komsomo", Vector(9163.171, 120.897, 9087.095), 200);
		def.Mines("Mines de Komsomo", "Komsomo", Vector(8474.379, 196.745, 8994.898), 200);
		def.Ied("IED de Komsomo", "Komsomo", Vector(9163.171, 120.897, 9087.095), 200);
		def.Ied("IED de Komsomo", "Komsomo", Vector(8474.379, 196.745, 8994.898), 200);
		def.Patrol("Patrouille de Komsomo", "Komsomo", Vector(9163.171, 120.897, 9087.095), 200);
		def.Patrol("Patrouille de Komsomo", "Komsomo", Vector(8474.379, 196.745, 8994.898), 200);

		// Rowan, voisine de Komsomo.
		def.Master("Rowan", HAM_EMasterType.CITY, Vector(9644.172, 132.483, 9996.863), 500, "Komsomo");
		def.Slave("Ville de Rowan", "Rowan", Vector(9838.98, 56.536, 9483.446), 400, HAM_EMasterType.HIDEOUT);
		def.Slave("Champ de Tir de Rowan", "Rowan", Vector(9651.238, 87.281, 9707.756), 200, HAM_EMasterType.CAMP);
		def.Slave("Tour de l'Ile de Rowan", "Rowan", Vector(9423.245, 20.238, 12533.658), 300, HAM_EMasterType.RADIO);
		def.Slave("Tour du Bois de Rowan", "Rowan", Vector(10615.652, 129.858, 10300.276), 300, HAM_EMasterType.RADIO);
		def.Slave("Port de Rowan", "Rowan", Vector(9863.099, 1.125, 10806.646), 200, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Avant Poste de Rowan", "Rowan", Vector(10515.298, 22.489, 9762.387), 300, HAM_EMasterType.SUPPLY_PERSONAL);

		// LeRocher, dernière cellule avant le QG ennemi. Voisins : Rowan et le QG ennemi.
		def.Master("LeRocher", HAM_EMasterType.CITY, Vector(11519.273, 106.065, 10730.259), 300, "Rowan;QG ennemi");
		def.Slave("Planque de la Ville de LeRocher", "LeRocher", Vector(11390.128, 86.537, 10511.521), 300, HAM_EMasterType.HIDEOUT);
		def.Slave("Planque de la Ville de Kamensk", "LeRocher", Vector(11904.275, 6.363, 10770.85), 300, HAM_EMasterType.HIDEOUT);
		def.Slave("Planque de la Centreville de Kamensk", "LeRocher", Vector(11832.164, 9.505, 10639.734), 300, HAM_EMasterType.HIDEOUT);
		def.Slave("Camp militaire de Kamensk", "LeRocher", Vector(11863.208, 8.093, 10730.962), 300, HAM_EMasterType.CAMP);
		def.Slave("Tour de le Port de Kamensk", "LeRocher", Vector(11850.306, 3.727, 10951.258), 300, HAM_EMasterType.RADIO);
		def.Slave("Depot de LeRocher", "LeRocher", Vector(11227.36, 94.519, 10262.796), 200, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Plage de LeRocher", "LeRocher", Vector(11238.663, 0.156, 10860.158), 200, HAM_EMasterType.SUPPLY_PERSONAL);
		def.Cqb("CQB 1 de LeRocher", "LeRocher", Vector(11902.579, 7.032, 10627.271), 100);
		def.Cqb("CQB 2 de LeRocher", "LeRocher", Vector(11753.231, 34.968, 10422.848), 80);
		def.Mines("Mines de LeRocher", "LeRocher", Vector(9333.521, 92.935, 9522.323), 200);
		def.Ied("IED de LeRocher", "LeRocher", Vector(9333.521, 92.935, 9522.323), 200);
		def.Patrol("LeRocher_Mines_IED_Patrouille_R200 Patrouille", "LeRocher", Vector(9333.521, 92.935, 9522.323), 200);
		def.Patrol("LeRocher_CQB_Patrouille_R100 Patrouille", "LeRocher", Vector(11902.579, 7.032, 10627.271), 100);

		// Marqueurs QGEN : lignes du QG ennemi, pas des cellules séparées.
		def.Cqb("QGEN_CQB_Patrouille_R100 CQB", "QG ennemi", Vector(12070.707, 2.157, 10569.948), 100);
		def.Cqb("QGEN_CQB_Patrouille_2_R100 CQB", "QG ennemi", Vector(12042.385, 3.066, 10930.212), 100);
		def.Cqb("QGEN_Mine_CQB_Patrouille_R100 CQB", "QG ennemi", Vector(12228.09, 2.376, 10914.596), 100);
		def.Cqb("QGEN_CQB_Patrouille_3_R100 CQB", "QG ennemi", Vector(12095.796, 2.156, 10454.21), 100);
		def.Mines("QGEN_Mine_CQB_Patrouille_R100 Mines", "QG ennemi", Vector(12228.09, 2.376, 10914.596), 100);
		def.Patrol("QGEN_CQB_Patrouille_R100 Patrouille", "QG ennemi", Vector(12070.707, 2.157, 10569.948), 100);
		def.Patrol("QGEN_CQB_Patrouille_2_R100 Patrouille", "QG ennemi", Vector(12042.385, 3.066, 10930.212), 100);
		def.Patrol("QGEN_Mine_CQB_Patrouille_R100 Patrouille", "QG ennemi", Vector(12228.09, 2.376, 10914.596), 100);
		def.Patrol("QGEN_CQB_Patrouille_3_R100 Patrouille", "QG ennemi", Vector(12095.796, 2.156, 10454.21), 100);

		// Patrouilles entre les zones. 500 m entre elles, 300 m du centre des zones, 800 m des ports et des plages.
		def.Patrol("Patrouille terrain QG 1", "QG", Vector(2500, 49.614, 6000), 100);
		def.Patrol("Patrouille terrain QG 2", "QG", Vector(3000, 49.614, 5000), 100);
		def.Patrol("Patrouille terrain QG 3", "QG", Vector(3000, 49.614, 6000), 100);
		def.Patrol("Patrouille terrain QG 4", "QG", Vector(3500, 180.143, 5500), 100);
		def.Patrol("Patrouille terrain Udarnik 1", "Udarnik", Vector(4000, 40.407, 3500), 100);
		def.Patrol("Patrouille terrain Udarnik 2", "Udarnik", Vector(4000, 21.721, 4500), 100);
		def.Patrol("Patrouille terrain Udarnik 3", "Udarnik", Vector(4500, 56.698, 4000), 100);
		def.Patrol("Patrouille terrain Udarnik 4", "Udarnik", Vector(4500, 253.203, 4500), 100);
		def.Patrol("Patrouille terrain Udarnik 5", "Udarnik", Vector(5000, 253.203, 4000), 100);
		def.Patrol("Patrouille terrain Ugoldar 1", "Ugoldar", Vector(6500, 56.453, 5000), 100);
		def.Patrol("Patrouille terrain Ulfid 1", "Ulfid", Vector(2000, 132.501, 7000), 100);
		def.Patrol("Patrouille terrain Ulfid 3", "Ulfid", Vector(2500, 110.695, 8000), 100);
		def.Patrol("Patrouille terrain Hamertin 1", "Hamertin", Vector(2500, 221.312, 6500), 100);
		def.Patrol("Patrouille terrain Hamertin 2", "Hamertin", Vector(3000, 266.591, 6500), 100);
		def.Patrol("Patrouille terrain Hamertin 3", "Hamertin", Vector(3000, 221.312, 7000), 100);
		def.Patrol("Patrouille terrain Hamertin 4", "Hamertin", Vector(3500, 266.591, 6000), 100);
		def.Patrol("Patrouille terrain Hamertin 5", "Hamertin", Vector(3500, 268.281, 6500), 100);
		def.Patrol("Patrouille terrain Hamertin 6", "Hamertin", Vector(4000, 448.688, 6000), 100);
		def.Patrol("Patrouille terrain Hamertin 7", "Hamertin", Vector(4000, 448.688, 6500), 100);
		def.Patrol("Patrouille terrain Hamertin 8", "Hamertin", Vector(4000, 268.281, 7000), 100);
		def.Patrol("Patrouille terrain Cain 3", "Cain", Vector(5500, 176.486, 7500), 100);
		def.Patrol("Patrouille terrain Cain 7", "Cain", Vector(6000, 39.246, 7500), 100);
		def.Patrol("Patrouille terrain Cain 10", "Cain", Vector(6500, 37.599, 7500), 100);
		def.Patrol("Patrouille terrain Cain 11", "Cain", Vector(7000, 436.688, 7500), 100);
		def.Patrol("Patrouille terrain Soumont 1", "Soumont", Vector(4000, 180.143, 5000), 100);
		def.Patrol("Patrouille terrain Soumont 2", "Soumont", Vector(4000, 180.143, 5500), 100);
		def.Patrol("Patrouille terrain Soumont 3", "Soumont", Vector(4500, 410.467, 5000), 100);
		def.Patrol("Patrouille terrain Soumont 4", "Soumont", Vector(4500, 439.946, 5500), 100);
		def.Patrol("Patrouille terrain Soumont 5", "Soumont", Vector(5000, 410.467, 5000), 100);
		def.Patrol("Patrouille terrain Soumont 6", "Soumont", Vector(5500, 403.065, 5000), 100);
		def.Patrol("Patrouille terrain Soumont 7", "Soumont", Vector(6000, 188.937, 5000), 100);
		def.Patrol("Patrouille terrain Soumont 8", "Soumont", Vector(6000, 403.065, 5500), 100);
		def.Patrol("Patrouille terrain PikdeCain 1", "PikdeCain", Vector(4500, 448.688, 6000), 100);
		def.Patrol("Patrouille terrain PikdeCain 2", "PikdeCain", Vector(4500, 448.688, 7000), 100);
		def.Patrol("Patrouille terrain PikdeCain 3", "PikdeCain", Vector(5000, 176.486, 7000), 100);
		def.Patrol("Patrouille terrain Ugold 1", "Ugold", Vector(6000, 420.233, 7000), 100);
		def.Patrol("Patrouille terrain Ugold 2", "Ugold", Vector(6500, 314.322, 5500), 100);
		def.Patrol("Patrouille terrain Ugold 3", "Ugold", Vector(6500, 418.514, 6000), 100);
		def.Patrol("Patrouille terrain Ugold 4", "Ugold", Vector(6500, 441.812, 7000), 100);
		def.Patrol("Patrouille terrain Ugold 5", "Ugold", Vector(7000, 314.322, 5500), 100);
		def.Patrol("Patrouille terrain Ugold 6", "Ugold", Vector(7500, 23.161, 5500), 100);
		def.Patrol("Patrouille terrain Ugold 7", "Ugold", Vector(7500, 314.322, 6000), 100);
		def.Patrol("Patrouille terrain Ugold 8", "Ugold", Vector(7500, 208.184, 6500), 100);
		def.Patrol("Patrouille terrain Ugold 9", "Ugold", Vector(8000, 23.161, 5500), 100);
		def.Patrol("Patrouille terrain Ugold 10", "Ugold", Vector(8000, 22.719, 6000), 100);
		def.Patrol("Patrouille terrain Ugold 11", "Ugold", Vector(8000, 46.906, 6500), 100);
		def.Patrol("Patrouille terrain Gorshkov 1", "Gorshkov", Vector(7500, 182.719, 7500), 100);
		def.Patrol("Patrouille terrain Gorshkov 2", "Gorshkov", Vector(7500, 312.131, 8000), 100);
		def.Patrol("Patrouille terrain Gorshkov 3", "Gorshkov", Vector(8000, 208.184, 7000), 100);
		def.Patrol("Patrouille terrain Gorshkov 4", "Gorshkov", Vector(8000, 312.131, 8500), 100);
		def.Patrol("Patrouille terrain Gorshkov 5", "Gorshkov", Vector(8500, 46.906, 7000), 100);
		def.Patrol("Patrouille terrain Rowan 8", "Rowan", Vector(10000, 11.563, 8500), 100);
		def.Patrol("Patrouille terrain LeRocher 3", "LeRocher", Vector(11000, 94.519, 10000), 100);
	}
}
