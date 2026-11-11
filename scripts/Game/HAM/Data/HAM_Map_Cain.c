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
		// Le QG ennemi s'ouvre quand une cellule voisine est complète, pas dès le départ.
		def.EnemyHq("QG ennemi", Vector(12029.178, 2.902, 10492.373), 100, "");

		// Udarnik, première cellule après le QG joueur. Les noms des lignes ne sont pas affichés.
		def.Master("Udarnik", HAM_EMasterType.CITY, Vector(4415.882, 40.289, 3544.781), 300, "QG");
		def.Slave("Udarnik West", "Udarnik", Vector(3801.861, 5.024, 3738.642), 200, HAM_EMasterType.HIDEOUT);
		def.Slave("Base de Udarnik", "Udarnik", Vector(4367.679, 55.188, 3219.023), 150, HAM_EMasterType.CAMP);
		def.Slave("Base Est de Udarnik", "Udarnik", Vector(5337.222, 64.969, 3370.426), 200, HAM_EMasterType.FUEL);
		def.Slave("Ressources", "Udarnik", Vector(3430.24, 21.721, 4290.794), 200, HAM_EMasterType.SUPPLY_VANILLA);
		def.Slave("Port de Udarnik", "Udarnik", Vector(4858.461, 4.62, 2935.403), 200, HAM_EMasterType.SUPPLY_PERSONAL);

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
	}
}
