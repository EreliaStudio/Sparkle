# Sparkle v0.1.5 — Questions ouvertes : biomes et génération du monde

> Inventaire au 24 novembre 2026, réorganisé après validation des trois niveaux d'abstraction. Les identifiants A–F restent stables pour les discussions. Les décisions sont détaillées dans [l'architecture](biome-architecture.md). Aucune implémentation, PR de code ou fusion n'est autorisée par cet inventaire.

## 1. Périmètre validé

- Niveau générique `spk::World::Biome` : définition sans dimension imposée, fondations de passes sans logique spatiale spécifique.
- Niveau surfacique `spk::World::SurfaceBiome` : hauteur absolue X/Z, influences typées surfaciques, grille 2D, passes de distribution et de combinaison des hauteurs.
- Niveau voxel `spk::Voxel` : spécialisation surfacique avec strates et application concrète. Les héritages de classes et l'ordre d'exécution des passes sont deux choses distinctes.
- Le volumétrique sera une autre branche si nécessaire ; aucune API 3D, grille universelle ou classe VolumetricBiome à concevoir maintenant.
- Le générateur existant est conservé. Les algorithmes de placement, le relief, les matériaux et les décors restent séparés.

## A — Contrat de résultat de distribution surfacique (résolu)

**A1 — Résolue, révisée pour la couche surfacique.** Influence = pointeur non propriétaire vers un `SurfaceBiome` const et poids float. Agrégat modifiable, état vide `{nullptr, 0.0f}`. Le propriétaire garantit la stabilité et la durée de vie des définitions. Pas de registry ni ID obligatoires. Le nom imbriqué envisagé est `SurfaceBiome::Influence` ; mutualisation éventuelle avec un niveau générique à décider seulement si utile.

**A2 — Résolue, révisée pour la couche surfacique.** Grille 2D de cellules `std::array<Influence, TCapacity>` dans un vecteur contigu, capacité positive template choisie par le développeur. Écriture directe dans la cellule réelle, accès mutable/const, remise à zéro au recyclage. Pools statiques par spécialisation et puissance de deux du nombre de cellules, avec Lease. `CapacityPool` mutualise le mécanisme. Les détails spatiaux sont B3/B4.

**A3 — Résolue.** Poids négatif/non fini ou pointeur nul avec poids non nul : exception. Doublons de pointeur non nul : exception même à poids zéro. Biome non nul de poids zéro conservé. Somme/division en double puis stockage float, somme nulle inchangée, aucun ajustement artificiel ou correction silencieuse. Validation intégrale, puis normalisation en place.

**A4 — Résolue.** `dominant(position)` retourne un pointeur non propriétaire vers le premier maximum strictement positif ; nullptr sinon. Parcours linéaire sans cache ni allocation ; modifications valides immédiatement visibles. Une passe qui exige un biome utilisable lève une exception en son absence.

**A5 — Résolue.** Indices conservés ; aucune suppression, fusion, permutation, compaction ou tri. Les emplacements nuls et les poids nuls peuvent être intercalés. Aucun arrêt au premier nullptr.

Voir [contrat des influences](biome-architecture.md#3-surface-layer--distribution-result-and-pass-responsibilities). Les règles A1–A5 sont conservées, seul leur rattachement à la couche surfacique est clarifié.

## B — Fondations de passes et distribution surfacique : hooks et coordonnées

**B1. Fondations de passes et signature du hook de calcul — PARTIELLEMENT RÉSOLUE (24 novembre 2026).** Répartition validée : la passe générique traite une cellule sans connaître l'espace (validation intégrale A3, rejet des doublons, normalisation en place, conservation des indices A5). La passe surfacique acquiert et réinitialise sa grille 2D, parcourt X/Z, appelle le hook sur la cellule de destination, délègue la validation/normalisation au socle et publie le résultat. La combinaison des hauteurs est une passe surfacique distincte. Aucune passe de distribution voxel supplémentaire n'est nécessaire par principe ; la spécialisation voxel intervient pour l'application des strates. Restent ouverts les noms, interfaces C++ et entrées exactes du hook (position mondiale X/Z, contexte éventuel, référence mutable vers la cellule surfacique de capacité template commune). Ne pas anticiper une API volumétrique. Voir [socle générique](biome-architecture.md#22-generic-pass-foundation--responsibilities-validated-b1) et [distribution surfacique](biome-architecture.md#32-reusable-surface-distribution-pass--responsibilities-validated-b1).

**B2. Hooks de configuration/destination.** De quelle manière une passe surfacique obtient-elle l'étendue 2D, le pas d'échantillonnage, l'origine et la destination dans `TGenerationData` sans imposer la structure de données du jeu ? Nombre et signatures des hooks à finaliser.

**B3. Coordonnées et résolution.** Quelle règle garantit une même distribution à une même position mondiale, y compris aux limites de chunks et aux coordonnées négatives ? Comment choisir la résolution de distribution indépendamment des algorithmes de placement, sans dégrader la précision requise par le consommateur ?

**B4. Règles d'accès aux résultats.** A2 a validé l'accès direct mutable et const à la cellule via `at(position)` ; Échantillons aux deux extrémités ou domaine semi-ouvert, accès hors limites, accès discret/interpolé et compatibilité éventuelle avec `HeightMap::Grid` : quelles règles sont nécessaires pour que deux passes puissent partager le même domaine ?

**B5. Initialisation facultative des stratégies.** Une stratégie ayant un état de monde fini peut-elle initialiser cet état séparément de `apply()` ? Quels hooks sont réellement génériques, et lesquels doivent rester une responsabilité de l'implémentation de stratégie ?

**B6. Garanties de calcul.** Quel niveau de déterminisme et de sécurité en concurrence est exigé d'un hook et de ses objets partagés ? Quelles modifications de configuration sont interdites pendant l'exécution de jobs ?

## C — Sources de hauteur et orchestration surfacique

**C1. Sources de hauteur réutilisables.** C2 écarte l'obligation d'une référence à `TerrainHeightGenerator` : `SurfaceBiome` fournit lui-même la hauteur. `HeightMap` reste concret et non polymorphe. Il reste à définir son API d'évaluation ponctuelle efficace, et à décider si une interface abstraite de source de hauteur garde une utilité indépendante.

**C2. Association biome/source — RÉSOLUE (24 novembre 2026).** `spk::World::Biome` reste une base générique sans hauteur obligatoire. `spk::World::SurfaceBiome` en hérite et expose une méthode virtuelle pure `float groundHeight(const spk::Vector2 &worldPosition) const` (position mondiale X/Z, hauteur absolue continue ; orthographe finale conforme aux conventions Sparkle). Le biome choisit son implémentation : `HeightMap`, constante ou algorithme personnalisé, éventuellement avec des sources partagées. Aucun membre de source obligatoire. La passe combine les hauteurs selon C4 ; les influences sont déjà typées SurfaceBiome ; aucune conversion depuis Biome n'est nécessaire pour les hauteurs. L'accès au type voxel plus riche reste E1. La distribution 2D/3D est indépendante de la représentation géométrique. Aucune classe `VolumetricBiome` ni contrat volumétrique n'est introduit maintenant (E5). Voir [contrat C2](biome-architecture.md#c2--surface-biome-height-contract-validated-2026-11-24).

**C3. Passe de hauteur unique ou distincte.** Comment conserver la passe documentée `HeightMapGenerationPass` liée à une seule `HeightMap`, tout en permettant une passe générique qui orchestre plusieurs sources pondérées ? Réutilisation, adaptation et responsabilités à préciser.

**C4. Fusion des hauteurs — RÉSOLUE (24 novembre 2026).** À une même position mondiale, chaque biome fournit sa hauteur absolue continue comme s'il était seul. La passe calcule `H = somme(poids_i * hauteur_i)` pour les influences de poids strictement positif. Les poids étant déjà normalisés selon A3, aucune division supplémentaire (ni par le nombre de biomes ni par la capacité) n'est nécessaire. Exemple : 20 × 0,75 + 100 × 0,25 = 40. Une évolution progressive des poids permet une transition douce entre sources continues ; un changement brutal de poids peut produire une falaise. La distribution contrôle ces transitions, sans mode de fusion distinct dans le cœur générique. Les strates restent celles du dominant. En l'absence de contribution positive, la passe qui nécessite un biome lève une `spk::Exception` (A4). Le contrat `SurfaceBiome` est validé en C2 ; les sources réutilisables, l'orchestration, l'évaluation et le traitement des erreurs restent ouverts (C1, C3, C5–C7, E2). Voir [contrat C4](biome-architecture.md#c4--weighted-height-combination-validated-2026-11-24).

**C5. Cas sans hauteur disponible.** A4 impose une `spk::Exception` lorsqu'une passe nécessite un biome utilisable et qu'aucune contribution positive n'existe. Quel traitement adopter pour une source absente, une hauteur non finie, ou une influence négligeable ? Qui décide de ne pas modifier une grille existante dans les autres cas ?

**C6. Échantillonnage efficace.** Faut-il générer des grilles complètes par source influente ou évaluer uniquement les positions ayant un poids utile ? Comment garantir un résultat identique, gérer les faibles poids et établir les benchmarks, sans anticiper l'optimisation par une règle prématurée ?

**C7. Contrat de sortie.** Comment la passe d'orchestration publie-t-elle la grille finale modifiable dans `TGenerationData`, afin que d'autres passes génériques puissent la transformer sans changer la source procédurale ?

## D — HeightMap::Grid : détails techniques résiduels (générique)

Les choix suivants sont **déjà arrêtés** et ne sont pas à rouvrir : taille du domaine en unités monde, pas `scale`, inclusion des deux extrémités, interpolation `Nearest`/`Bilinear`, valeurs continues, `Lease` RAII et pools partagés par classes de capacité en puissances de deux.

**D1. Types et stockage effectifs.** Confirmer les noms C++ et alias de `Grid`, `Lease`, stockage et dimensions, en les confrontant aux en-têtes Sparkle existants.

**D2. Robustesse numérique.** Définir la tolérance exacte de divisibilité `size / scale` et les limites d'overflow, sans modifier l'invariant d'espacement régulier.

**D3. Origine et alignement.** Vérifier le contrat d'origine déduit de la coordonnée de chunk et de la taille monde de la grille, notamment pour des dimensions variables et lors de la combinaison avec des influences échantillonnées.

**D4. Concurrence et recyclage.** Valider la durée de vie des pools statiques, les accès concurrents et la remise à zéro des grilles réutilisées ; pas de politique d'éviction prévue en v0.1.5 sans mesures.

## E — Frontières des couches, spécialisation voxel et durée de vie

**E1. Spécialisation voxel et accès typé.** Fixer le nom et les héritages exacts du biome voxel et des passes qui réutilisent le comportement surfacique ; distinguer héritage et ordre du pipeline. Comment la passe des strates obtient-elle un biome voxel compatible depuis la distribution de SurfaceBiome, sans contaminer World ni imposer de conversion au calcul des hauteurs ? Définir les erreurs d'incompatibilité, l'arrondi de hauteur/épaisseur, la représentation de profondeur infinie, la validation/sélection des matériaux et l'interaction avec les voxels préexistants, en préservant les règles de strates validées.

**E2. Interactions entre passes.** Comment les résultats temporaires se remplacent-ils ou se complètent-ils dans `TGenerationData` ? Préciser l'absence de résultat, l'ordre des passes et l'atomicité de l'échec.

**E3. Propriété et partage.** Définir les durées de vie de définitions de biome, sources de hauteur et données pré-calculées détenues par le jeu face aux requêtes asynchrones et aux échantillons temporaires.

**E4. Cache et invalidation.** Que stocker uniquement pendant une demande, quel état peut être partagé entre demandes et comment éviter les données obsolètes ? Pas de `ContractProvider` obligatoire en première version : ne l'introduire que si une vraie notification de modification le justifie.

**E5. Positionnement des extensions futures — reporté.** Une future distribution volumétrique sera une branche distincte du socle générique. Ne pas définir ses classes, grilles ou hooks maintenant. Les passes de décor/structures sont également reportées après validation du terrain de surface.

## F — Tests, validation et critères d'acceptation (à écrire avant les PR)

**F1. Normalisation et validation.** Jeux d'essais zéro/un/plusieurs biomes, emplacements nuls de poids zéro acceptés et de poids non nul rejetés, valeurs 3/5/2 → 0,3/0,5/0,2, conservation des biomes de poids nul et des cellules de somme nulle, rejet des poids négatifs/non finis et des doublons non nuls y compris de poids zéro, et normalisation avec somme/division en double de poids float élevés. A3 fixe les résultats attendus ; la tolérance numérique des tests reste à préciser lors de leur écriture. A4 fixe aussi les tests du dominant : premier maximum positif à égalité, absence de dominant dans les cellules entièrement nulles ou de poids tous nuls, prise en compte des modifications et exception du consommateur nécessitant un biome. A5 impose des tests de conservation exacte des indices, avec emplacements nuls intercalés, biomes de poids nul et maximums égaux.

**F2. Test de hook substituable.** Deux stratégies fictives de calcul, une passe surfacique commune ; vérifier que le parcours, la normalisation, le stockage, la publication et la libération ne sont pas réimplémentés par les stratégies.

**F3. Invariance spatiale.** Coordonnées positives/négatives, limites horizontales et verticales de chunks, ordre de génération inversé, réplication déterministe des résultats et absence de frontières artificiellement alignées sur les chunks.

**F4. HeightMap/combinaison.** Source unique, sources multiples, fusion pondérée (C4 : 20 × 0,75 + 100 × 0,25 = 40 ; aucune division par la capacité), valeurs continues, grilles intercalées, cas vides, exceptions et absence d'écriture partielle publiée.

**F5. Pools/performance.** Tester les capacités configurables, l'écriture directe, la remise à zéro et le recyclage des grilles ; compter les allocations, observer la réutilisation des `Lease`, comparer évaluations par position et par grilles et vérifier la concurrence. Les budgets chiffrés et scénarios représentatifs restent à définir.

**F6. Intégration spécialisée sans couplage du cœur.** Décrire séparément les TU Core génériques et les tests plus spécifiques (dont voxel et golden images) qui prouveront la réutilisation du résultat ; aucune dépendance de Graphics requise dans Core.

## Points expressément hors du périmètre actuel

La sélection géométrique des centres, la perturbation des frontières Voronoï, la largeur de transition spécifique à une paire de régions, la formule d'un algorithme de répartition infinie, ainsi que l'implémentation concrète des algorithmes de strates voxel, des prefabs et de la génération volumétrique ne sont pas à définir dans cette première série de discussions. Ils pourront avoir leurs propres documents et tests une fois les interfaces génériques stabilisées.

## Méthode pour les conversations futures

Chaque nouvelle discussion pourra choisir **un identifiant précis** (A1, B3, C2, etc.), examiner les contraintes et variantes, valider une décision, puis mettre à jour ce document et les sections normatives de [biome-architecture.md](biome-architecture.md). Aucun ordre ou date de PR d'implémentation n'est approuvé dans cet inventaire.

## Références

- [Biome architecture — décisions et propositions](biome-architecture.md)
- [World generation — README](README.md)
- [Contrats de génération existants](contracts.md)
- [Architecture des chunks et des passes](architecture.md)
- [Passes applicatives](features.md)
- [Matrice de validation historique PR00–PR10](validation.md)
