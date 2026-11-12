# 13RDPA Hearts and Minds

Version **1.1.2**.

Campagne Hearts and Minds pour Arma Reforger, jouée sur Kolguyev. Les joueurs sont l'armée rouge du 13e RDPA. En face, les forces US tiennent l'île par cellules : un maître, ses points autour, et tout ce qui vit entre les deux.

Ce n'est pas une partie de Conflit où l'on pousse des bases à la chaîne. On découvre le pays, on parle aux civils, on prend un point, puis on le garde. L'eau, les vivres, les munitions et les hommes décident si la zone reste à nous.

La mission s'appelle **13RDPA Hearts and Minds — Kolguyev**. Le scénario est `Missions/HAM_Cain.conf`.

## Comment une partie avance

On part du QG. Autour, l'île est découpée en cellules. Chaque cellule a un maître et des points qui en dépendent : camp, carburant, radio, planque, ville, dépôt, base.

Un point ennemi se prend en restant dedans, sans ennemi vivant dans le rayon. Le temps est `capture.playerSeconds`. Un maître pris ouvre sa cellule : les esclaves deviennent prenables, la carte montre les stocks, et le délai de grâce commence.

L'ennemi reprend de la même façon. Contre-attaque, patrouille ou garnison : s'ils sont dans le rayon et qu'il n'y a plus ni joueur ni défenseur ami, le point retombe au bout de `capture.enemySeconds`. Un esclave perdu repasse seul. Le maître, lui, entraîne toute la cellule.

Derrière la ligne, s'enfoncer dans une cellule qui n'est pas encore au front coûte de la réputation.

## Tenir ce qu'on a pris

Prendre ne suffit pas. Le maître doit garder un plancher d'eau, de vivres, de munitions et d'hommes. En dessous, après le délai de grâce, la cellule est perdue.

Les hommes ne sortent pas de nulle part. Chaque cycle en consomme, dans la limite `hold.menMax`. Un camp pris en verse au stock. Le tableau de la zone, sur la carte, affiche les ressources et les hommes au-dessus de l'icône, et se met à jour à chaque évaluation.

Les dépôts personnels se remplissent tout seuls, dans leur plafond. Un camion charge les ressources perso dans le rayon de chargement, jusqu'à sa capacité, puis les dépose sur une zone. Ce ne sont pas les fournitures vanilla du Conflit : le compteur du jeu et le nôtre ne sont pas le même stock.

On peut échanger une ressource contre une autre. Le taux est `exchange.ratio` : une partie se perd dans l'échange.

## Ce qui bouge sur l'île

Les garnisons et les patrouilles à pied d'un type de zone sortent en groupes complets. Le nombre de groupes est `patrolUnits` dans `zoneTypes`, pas un budget d'hommes. `patrols.maxActive` limite le nombre de patrouilles à pied en même temps sur la carte.

Une patrouille, à pied ou en véhicule, ne naît pas à moins de 2 km d'une zone déjà tenue, QG compris. La garnison du point ennemi, elle, reste sur son point. Les mines, les IED et les civils restent sur leur distance propre, plus courte.

Les véhicules de patrouille roulent à la vitesse de croisière du bloc `ai`, en course légère. Ceux d'une contre-attaque avancent à 30 km/h. L'infanterie de la contre-attaque marche jusqu'à la zone, puis passe à la course.

Les hélicoptères patrouillent au-dessus des camps, des villes, des bases et des dépôts personnels. Le vol est celui d'AeroPilot : un point de défense au centre de la zone, dans le rayon réglé. Si un missile casse un rotor ou met le moteur hors service, l'appareil cesse de patrouiller et retombe.

Les civils marchent, s'arrêtent si on leur parle, et repartent. Un otage se libère par une action sur le personnage. Les parler assez souvent ouvre du renseignement. Un corps en a parfois aussi, et certains bâtiments en cachent.

Les mines se posent hors des routes, entre deux distances. Les IED restent sur une route assez large : un sentier n'en reçoit pas.

## Réputation et renseignement

On commence avec un capital de réputation. Tuer un civil, traîner derrière le front ou laisser une cellule pourrir le fait baisser. Prendre une zone, finir une cellule, ramener un renseignement le remonte.

En dessous du seuil local, la cellule d'à côté ne se révèle pas. En dessous du seuil des civils, ils ne parlent plus.

Le renseignement trouvé sur un corps ou dans un bâtiment découvre des points. Ce n'est pas une marque permanente posée à la main : c'est le front qui s'ouvre.

## Fichier de réglage

Tout ce qui se change sans recompiler est dans :

`$profile:13RDPA_HeartsAndMinds_Settings.json`

Sur un serveur dédié, c'est le profil du serveur, pas celui du Workbench. Le jeu le lit au démarrage. Le modifier pendant qu'une partie tourne ne change rien tant que le serveur n'a pas redémarré. S'il manque, il est écrit avec les valeurs par défaut du code, puis relu.

Le Game Master reprend une partie de ces curseurs. Certains champs restent dans le fichier parce que le maître de jeu les affiche, mais ne décident plus le nombre d'hommes qui sortent. C'est indiqué plus bas.

### Général

| Clé | Rôle |
| --- | --- |
| `debug` | Messages de suivi dans le journal. |
| `save.intervalMinutes` | Sauvegarde automatique du front, en minutes. |

### Distances communes

`garrison.activationDistance` et `garrison.despawnDistance` sont les distances de repli quand un type de zone n'a pas les siennes. Chaque entrée de `zoneTypes` peut les remplacer.

Une zone s'active quand un joueur entre dans le rayon d'activation. Elle se vide quand plus personne n'est dans le rayon de disparition.

### Types de zone

`zoneTypes` répète le même bloc pour `master`, `slave`, `fuel`, `radio`, `camp`, `hideout`, `city`, `supply`, `deposit`, `patrol`, `cqb`, `ied`, `mines`, `civilians` et `enemyHq`.

| Clé | Rôle |
| --- | --- |
| `activationDistance` / `despawnDistance` | Apparition et disparition, en mètres. |
| `patrolUnits` | Nombre de groupes entiers. Garnison et patrouilles à pied de ce type. |
| `vehicles` | Véhicules de la zone. |
| `percentLAV` / `percentM1025` | Part de LAV-25 et de M1025 parmi ces véhicules. |
| `civilians` | Civils seuls sur la zone. Chacun a son groupe. |
| `buildings` | Bâtiments fouillés pour le CQB et le renseignement. |
| `ieds` / `mines` | Engins posés pour ce type. |

`patrol`, `cqb`, `ied`, `mines` et `civilians` n'ont pas de véhicules. Le CQB est un homme par bâtiment, pris parmi les personnages hérités.

`units`, s'il traîne dans un vieux fichier, n'est plus le budget d'hommes. Le compte qui compte est `patrolUnits`.

### Prise et reprise

| Clé | Rôle |
| --- | --- |
| `capture.playerSeconds` | Secondes dans le rayon, sans ennemi, pour prendre le point. |
| `capture.enemySeconds` | Secondes dans le rayon, sans joueur ni défenseur ami, pour que l'ennemi le reprenne. |

### Dépôts et camions

| Clé | Rôle |
| --- | --- |
| `deposits.enabled` | Les dépôts personnels tournent. |
| `waterMax`, `foodMax`, `ammoMax` | Plafond de chaque ressource. |
| `refillPerMinute` | Ce qui revient tout seul, par minute. |
| `refillIntervalSeconds` | Espacement entre deux remplissages. |
| `loadRadius` | Rayon autour du camion pour charger ou décharger. |
| `vehicleCapacity` | Ressources perso transportées en une fois. |

### Tenue de la cellule

| Clé | Rôle |
| --- | --- |
| `waterThreshold`, `foodThreshold`, `ammoThreshold`, `menThreshold` | Plancher. En dessous, la grâce commence. |
| `graceMinutes` | Temps pour ravitailler avant la perte. |
| `cycleMinutes` | Durée d'un cycle d'entretien. |
| `upkeepWater`, `upkeepFood`, `upkeepAmmo` | Consommé à chaque cycle. |
| `menPerCycle` | Hommes gagnés par cycle, si les vivres suivent. |
| `menCostWater`, `menCostFood`, `menCostAmmo` | Coût d'un homme. |
| `menMax` | Plafond d'hommes sur la cellule. |
| `exchange.ratio` | Part gardée quand on change une ressource en une autre. `0.8` garde 80 %. |

### Réputation, civils, renseignement

| Clé | Rôle |
| --- | --- |
| `reputation.start` | Réputation au début de la partie. |
| `reputation.localRevealThreshold` | Sous ce seuil, la cellule voisine reste cachée. |
| `reputation.civilianTalkThreshold` | Sous ce seuil, les civils ne parlent plus. |
| `reputation.frontlinePenalty` | Coût si l'on s'enfonce derrière le front. |
| `intel.bodyChance` | Chance qu'un corps ennemi ait un renseignement. |
| `intel.buildingIntelPerCell` | Bâtiments à renseignement par cellule. |
| `civilians.maxGroups` | Groupes civils d'une zone qui n'est ni un camp ni une base. |
| `civilians.patrolVehicles` | Véhicules civils sur les routes. |

### Patrouilles, véhicules, hélicoptères

| Clé | Rôle |
| --- | --- |
| `patrols.enabled` | Les patrouilles nommées sortent. |
| `patrols.maxActive` | Patrouilles à pied en même temps, toutes zones confondues. |
| `patrols.vehicleShare` | Part des patrouilles qui prennent un véhicule au lieu de marcher. |
| `vehicles.patrolMax` | Véhicules ennemis en patrouille autour d'un point du front. |
| `helicopters.enabled` | Les UH-1H patrouillent. |
| `helicopters.radius` | Rayon du cercle, en mètres. En dessous de 50, le jeu remet 50. |
| `helicopters.altitudeMin` / `altitudeMax` | Ancienne fourchette. Le vol suit maintenant la hauteur de croisière AeroPilot, 100 m sur notre UH-1H. |

`patrols.maxGroups` et `patrols.intervalSeconds` sont lus et montrés au Game Master. Ils ne décident plus combien de groupes sortent ni à quel rythme. C'est `patrolUnits`, plus la limite `maxActive`.

### Contre-attaques et renforts

Une contre-attaque part d'une zone encore ennemie collée à une zone joueur, vers la zone joueur la plus proche dont le délai après prise est passé. Jamais depuis une zone déjà à nous, jamais depuis l'arrière.

| Clé | Rôle |
| --- | --- |
| `counterAttack.enabled` | Les vagues existent. |
| `counterAttack.intervalMinutes` | Temps entre deux tirages. |
| `counterAttack.chance` | Chance qu'une vague parte. `1` veut dire à chaque fois. |
| `counterAttack.groups` | Groupes à pied de la vague. |
| `counterAttack.vehicles` | Véhicules de la vague. |
| `counterAttack.delayAfterCaptureMinutes` | Une zone fraîchement prise n'est pas visée avant ce délai. |
| `counterAttack.minFromPlayer` | La vague ne naît pas plus près d'un joueur. |
| `counterAttack.minFromBase` | Ni plus près d'une base. |

`counterAttack.groupUnits` et `reinforcements.groupUnits` sont encore dans le fichier et dans le Game Master. Les groupes qui sortent sont des groupes complets, pas un effectif découpé par cet entier.

| Clé | Rôle |
| --- | --- |
| `reinforcements.enabled` | Les renforts des zones ennemies du front continuent. |
| `reinforcements.intervalMinutes` | Espacement. |
| `reinforcements.chance` | Chance à chaque échéance. |

### Mines et IED

| Clé | Rôle |
| --- | --- |
| `mines.zonesEnabled` | Mines et IED sont posés. |
| `mines.mineMinDistance` / `mineMaxDistance` | Couronne autour du point, en mètres. |
| `mines.iedRoadMinWidth` | En dessous de cette largeur, la route est un sentier : pas d'IED. |

### IA, santé, radio, partie

Le bloc `ai` règle la perception, la distance de tir, la navigation et la vitesse des véhicules conduits par l'IA. `vehicleCruiseSpeed` est en km/h. `0` retombe sur la vitesse d'origine du véhicule. Les hélicoptères ne passent pas par ce curseur.

`stayInVehicleMode` : `1` tout le monde reste à bord, `2` seul l'équipage (conducteur et tireurs), `3` l'IA décide.

`footNavPreset` et `vehicleNavPreset` choisissent le rapport aux routes. `1` à pied est le mélange routes et terrain. `1` en véhicule préfère le bitume.

`health.dotScale` réduit les dégâts dans le temps. `health.regenScale` accélère la récupération.

`holdBreath` limite l'apnée et le souffle au viseur.

`teleport.playerToPlayer` et `teleport.betweenBases` ouvrent les deux téléportations.

`vehicleDepot.radiusMeters` est le rayon de l'atelier véhicule.

`milsim` coupe ou garde les habitudes du jeu : noms au-dessus des têtes, voix à proximité, épaves, cadavres, mode photo, boussole, première personne.

`time` et `weather` posent l'heure, la date et le ciel au lancement. `game` reprend les options de partie du Conflit : arsenal, ravitaillement de réapparition, tirs alliés, déguisement.

`groupAccess.whitelistIds` liste qui peut ouvrir les outils réservés. On y met un identifiant Steam ou un identifiant de compte, un par ligne. Les valeurs livrées avec le mod, `STEAM64_EXEMPLE` et `IDENTIFIANT_COMPTE_EXEMPLE`, sont des exemples : on les remplace par les vrais identifiants dans le JSON du profil. Ce n'est pas un réglage de difficulté.

`gm.enableBudgets` laisse les budgets du Game Master s'appliquer. Décoché, ils ne bloquent pas les poses.

## Lancer

Le projet Workbench est `addon.gproj`, identifiant `8896CB4B7C4A242A`. La mission à mettre dans la liste du serveur est `Missions/HAM_Cain.conf`.

AeroPilot AI (atelier, identifiant `69D0BD2BAA5B26BF`) doit être dans les mods lancés avec la partie, pas seulement déclaré dans le projet. Sans lui, l'hélicoptère hérité n'a plus son pilote automatique.

Le reste des dépendances est celui du Conflit vanilla et des modules déjà branchés sur cette mission. Hearts and Minds ne charge pas le mod Conflit du 13e comme une dépendance : la campagne tient seule.

Après une mise à jour du code, on republie le mod. Copier le dossier du Workbench par-dessus un `.pak` déjà publié ne met pas le serveur à jour.
