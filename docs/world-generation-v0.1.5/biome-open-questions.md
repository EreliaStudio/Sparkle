# Sparkle v0.1.5 — Questions ouvertes : biomes et génération du monde

> **Statut :** inventaire documentaire pour des discussions futures, établi le 24 novembre 2026.
> **Ce document distingue les questions ouvertes des décisions explicitement validées.** Il ne demande pas d'implémenter un algorithme particulier et n'autorise ni PR de code ni fusion.
> **Source des décisions validées :** [Biome architecture](biome-architecture.md), surtout §§3, 4.4 et 4.5. Les anciens contrats de chunks et de génération restent documentés dans [contracts.md](contracts.md).

## Périmètre et architecture retenue

- `spk::World` fournit des composants de génération **génériques et réutilisables**, sans dépendre du type de terrain ou de son mode de représentation.
- `spk::Voxel` est un **cas d'utilisation majeur et une couche spécialisée de Sparkle** : à terme, il doit proposer un monde voxel 3D directement utilisable et des passes adaptées, sans qu'un jeu ait à les recoder.
- Les exemples de stratégies de placement (Voronoï fini, champs de bruit procéduraux, règles climatiques, placement manuel, etc.) servent à vérifier la généralité des interfaces. Leur conception interne et leur implémentation sont **reportées**.
- La sortie visée pour chaque position de l'espace X/Z est une **cellule de capacité fixe configurable à la compilation**, contenant des références vers des biomes avec leurs poids et des emplacements nuls inutilisés (A2). Un biome dominant peut être déduit pour les consommateurs qui en ont besoin.
- **Décision déjà validée :** les hooks de distribution produisent des poids *relatifs* ; la **passe générique normalise elle-même ces poids**. Exemple : 3/5/2 devient 0,3/0,5/0,2. Ne pas rouvrir la répartition de cette responsabilité.
- Le générateur existant enchaîne des `Pass::apply(builder, coordinate, TGenerationData&)` et fournit des données temporaires par demande. Son interface générale ne doit pas être refondue sans nécessité constatée.
- Les références aux définitions de biomes restent non propriétaires ; l'instance propriétaire est sous la responsabilité du développeur. Aucune registry ni ID numérique obligatoire.
- `HeightMap` est une source concrète de hauteur à couches Perlin, et `HeightMap::Grid` une grille temporaire et modifiable, avec son `Lease` et les pools de capacité détaillés dans la documentation principale.
- La distribution, la génération/combinaison des hauteurs et leur application dans un support concret restent des responsabilités séparées. Les matériaux, strates, voxels, meshing et prefabs **n'appartiennent pas** au contrat de base de distribution `spk::World`.

## A — Contrat de résultat de distribution (générique)

**A1. Type d'une influence — RÉSOLUE (24 novembre 2026).** Le type public est `spk::World::Biome::Influence`, une structure imbriquée simple, copiable et affectable, composée de `const Biome *biome` et `float weight`. Le pointeur brut est non propriétaire. **Révision validée pendant A2 :** `nullptr` représente un emplacement inutilisé, et non une erreur ; une cellule entièrement nulle représente une position sans biome. L'état vide par défaut est `{nullptr, 0.0f}` ; les emplacements nuls ne contribuent pas à la normalisation. Le développeur garantit la durée de vie, la stabilité d'adresse et l'absence de modification des définitions pendant leur utilisation par les jobs. Aucun contrôle de durée de vie ni validation interne de la structure. Le poids reste relatif avant normalisation et normalisé après la passe générique. Voir [le contrat validé A1](biome-architecture.md#a1--influence-record-validated-2026-11-24). A2 (stockage) est également résolue. A3 (validation des poids) est également résolue. A4 (dominant) est également résolue. A5 (ordre) est également résolue. E3 (propriété et partage au niveau du système) reste ouverte.

**A2. Collection et stockage — RÉSOLUE (24 novembre 2026).** `spk::World::Biome::Influence::Grid<TCapacity>` stocke un vecteur contigu de cellules `std::array<Biome::Influence, TCapacity>`. La capacité positive est choisie à la compilation par le développeur et partagée avec la passe de distribution, sans maximum arbitraire ni valeur par défaut imposée. Le hook écrit directement dans la cellule mutable, sans vecteur temporaire ni transfert. Les emplacements inutilisés sont nuls ; les cellules recyclées sont réinitialisées avant calcul. Accès direct mutable et const via `at(position)` ; règles spatiales encore ouvertes (B3/B4). Grilles recyclées via des pools statiques par spécialisation et par capacité en puissances de deux (nombre total de cellules), avec `Lease`. Le mécanisme commun sera extrait dans `spk::CapacityPool<TElement>`, utilisé aussi par HeightMap::Grid, avec une PR indépendante à définir. Voir [contrat A2](biome-architecture.md#a2--fixed-capacity-influence-grid-and-pooling-validated-2026-11-24).

**A3. Nettoyage/validation des poids bruts — RÉSOLUE (24 novembre 2026).** La passe générique valide chaque cellule : biome nul avec poids différent de zéro (NaN et infinis inclus), poids négatif/non fini pour un biome non nul ou doublon de pointeur de biome non nul (même de poids nul) entraînent une `spk::Exception`. Les emplacements nuls de poids zéro et les biomes non nuls de poids nul sont valides et conservés. La somme est calculée en `double` ; si elle est nulle, la cellule valide reste inchangée ; sinon, la division est effectuée en `double` et le résultat stocké en `float`, sans correction artificielle pour obtenir une somme exactement égale à 1. Aucune suppression, fusion ni correction silencieuse. Voir [le contrat A3](biome-architecture.md#a3--weight-validation-and-normalization-validated-2026-11-24). La sélection du dominant est résolue en A4 ; l'ordre est résolu en A5 ; la validation après les modifications par d'autres passes (E2) reste ouverte.

**A4. Sélection du dominant — RÉSOLUE (24 novembre 2026).** `Biome::Influence::Grid<TCapacity>::dominant(const spk::Vector2 &position) const` retourne un `const Biome *` non propriétaire. Parcours en ordre croissant des indices de la cellule, sans cache, tri ni allocation ; seuls les biomes non nuls de poids strictement positif sont éligibles. Maximum de poids stocké, premier rencontré en cas d'égalité exacte (comparaison stricte). Retourne `nullptr` si aucun candidat positif. Une passe qui nécessite un biome utilisable doit lever une `spk::Exception` si aucun n'est disponible ; la requête de dominant n'est pas responsable de cette exception. Les modifications valides de la cellule sont immédiatement reflétées. Voir [contrat A4](biome-architecture.md#a4--dominant-biome-selection-validated-2026-11-24). A5 fixe la conservation de l'ordre de stockage ; B4 reste ouverte pour les coordonnées et E2 pour la validation après édition.

**A5. Ordre des influences — RÉSOLUE (24 novembre 2026).** La passe générique conserve exactement les indices des influences renseignées par le hook. La validation et la normalisation ne trient, ne compactent ni ne déplacent les entrées ; seuls les poids sont normalisés. Les emplacements nuls et les biomes de poids nul peuvent se trouver à n'importe quel indice. Aucun ordre obligatoire ni préfixe contigu de biomes renseignés ; les consommateurs parcourent les cellules sans s'arrêter au premier pointeur nul. Le premier maximum strictement positif reste dominant selon A4. Une passe ultérieure peut modifier explicitement les indices ; la validation après édition relève d'E2. Voir [contrat A5](biome-architecture.md#a5--influence-ordering-validated-2026-11-24).

**Le groupe A (A1–A5) est entièrement résolu.** C4 (fusion des hauteurs) est également résolue ; les autres questions des groupes B à F et le contrat détaillé de `CapacityPool` restent à traiter.

## B — Passes de distribution : hooks et coordonnées (générique)

**B1. Signature minimale du hook de calcul.** Quelles entrées exactes (position mondiale X/Z, contexte optionnel) ? A2 impose une référence mutable vers la cellule de destination de capacité template commune ; la signature complète reste ouverte.

**B2. Hooks de configuration/destination.** De quelle manière une passe générique obtient-elle l'étendue 2D, le pas d'échantillonnage, l'origine et la destination dans `TGenerationData` sans imposer la structure de données du jeu ? Nombre et signatures des hooks à finaliser.

**B3. Coordonnées et résolution.** Quelle règle garantit une même distribution à une même position mondiale, y compris aux limites de chunks et aux coordonnées négatives ? Comment choisir la résolution de distribution indépendamment des algorithmes de placement, sans dégrader la précision requise par le consommateur ?

**B4. Règles d'accès aux résultats.** A2 a validé l'accès direct mutable et const à la cellule via `at(position)` ; Échantillons aux deux extrémités ou domaine semi-ouvert, accès hors limites, accès discret/interpolé et compatibilité éventuelle avec `HeightMap::Grid` : quelles règles sont nécessaires pour que deux passes puissent partager le même domaine ?

**B5. Initialisation facultative des stratégies.** Une stratégie ayant un état de monde fini peut-elle initialiser cet état séparément de `apply()` ? Quels hooks sont réellement génériques, et lesquels doivent rester une responsabilité de l'implémentation de stratégie ?

**B6. Garanties de calcul.** Quel niveau de déterminisme et de sécurité en concurrence est exigé d'un hook et de ses objets partagés ? Quelles modifications de configuration sont interdites pendant l'exécution de jobs ?

## C — Sources de hauteur et orchestration (générique)

**C1. Contradiction de modèles.** Réconcilier le concept de `World::TerrainHeightGenerator` abstrait avec le `World::HeightMap` concret, non polymorphe, déjà validé : conserver deux abstractions, employer un adaptateur, ou une autre composition ? Ne pas assimiler silencieusement les deux.

**C2. Association biome/source.** Où se trouve l'association entre une définition générique de biome, qui reste minimale, et une ou plusieurs sources de hauteur ? Comment plusieurs biomes peuvent-ils partager une source sans imposer Perlin ou des champs dans la classe de base ?

**C3. Passe de hauteur unique ou distincte.** Comment conserver la passe documentée `HeightMapGenerationPass` liée à une seule `HeightMap`, tout en permettant une passe générique qui orchestre plusieurs sources pondérées ? Réutilisation, adaptation et responsabilités à préciser.

**C4. Fusion des hauteurs — RÉSOLUE (24 novembre 2026).** À une même position mondiale, chaque biome fournit sa hauteur absolue continue comme s'il était seul. La passe calcule `H = somme(poids_i * hauteur_i)` pour les influences de poids strictement positif. Les poids étant déjà normalisés selon A3, aucune division supplémentaire (ni par le nombre de biomes ni par la capacité) n'est nécessaire. Exemple : 20 × 0,75 + 100 × 0,25 = 40. Une évolution progressive des poids permet une transition douce entre sources continues ; un changement brutal de poids peut produire une falaise. La distribution contrôle ces transitions, sans mode de fusion distinct dans le cœur générique. Les strates restent celles du dominant. En l'absence de contribution positive, la passe qui nécessite un biome lève une `spk::Exception` (A4). Les interfaces des sources, leur évaluation et le traitement des erreurs restent ouverts (C1–C3, C5–C7, E2). Voir [contrat C4](biome-architecture.md#c4--weighted-height-combination-validated-2026-11-24).

**C5. Cas sans hauteur disponible.** A4 impose une `spk::Exception` lorsqu'une passe nécessite un biome utilisable et qu'aucune contribution positive n'existe. Quel traitement adopter pour une source absente, une hauteur non finie, ou une influence négligeable ? Qui décide de ne pas modifier une grille existante dans les autres cas ?

**C6. Échantillonnage efficace.** Faut-il générer des grilles complètes par source influente ou évaluer uniquement les positions ayant un poids utile ? Comment garantir un résultat identique, gérer les faibles poids et établir les benchmarks, sans anticiper l'optimisation par une règle prématurée ?

**C7. Contrat de sortie.** Comment la passe d'orchestration publie-t-elle la grille finale modifiable dans `TGenerationData`, afin que d'autres passes génériques puissent la transformer sans changer la source procédurale ?

## D — HeightMap::Grid : détails techniques résiduels (générique)

Les choix suivants sont **déjà arrêtés** et ne sont pas à rouvrir : taille du domaine en unités monde, pas `scale`, inclusion des deux extrémités, interpolation `Nearest`/`Bilinear`, valeurs continues, `Lease` RAII et pools partagés par classes de capacité en puissances de deux.

**D1. Types et stockage effectifs.** Confirmer les noms C++ et alias de `Grid`, `Lease`, stockage et dimensions, en les confrontant aux en-têtes Sparkle existants.

**D2. Robustesse numérique.** Définir la tolérance exacte de divisibilité `size / scale` et les limites d'overflow, sans modifier l'invariant d'espacement régulier.

**D3. Origine et alignement.** Vérifier le contrat d'origine déduit de la coordonnée de chunk et de la taille monde de la grille, notamment pour des dimensions variables et lors de la combinaison avec des influences échantillonnées.

**D4. Concurrence et recyclage.** Valider la durée de vie des pools statiques, les accès concurrents et la remise à zéro des grilles réutilisées ; pas de politique d'éviction prévue en v0.1.5 sans mesures.

## E — Frontières des couches et durée de vie

**E1. Adaptateurs vers une représentation concrète.** Quel contrat minimal reçoit un consommateur spécialisé (notamment voxel) depuis `World` ? La spécialisation doit pouvoir appliquer le biome dominant sans contaminer les structures génériques avec des types voxel.

**E2. Interactions entre passes.** Comment les résultats temporaires se remplacent-ils ou se complètent-ils dans `TGenerationData` ? Préciser l'absence de résultat, l'ordre des passes et l'atomicité de l'échec.

**E3. Propriété et partage.** Définir les durées de vie de définitions de biome, sources de hauteur et données pré-calculées détenues par le jeu face aux requêtes asynchrones et aux échantillons temporaires.

**E4. Cache et invalidation.** Que stocker uniquement pendant une demande, quel état peut être partagé entre demandes et comment éviter les données obsolètes ? Pas de `ContractProvider` obligatoire en première version : ne l'introduire que si une vraie notification de modification le justifie.

**E5. Positionnement des extensions futures.** Préserver la possibilité d'une distribution 3D/volumétrique et de passes de décor/structures, sans leur imposer dès maintenant une API ou une surcharge générique.

## F — Tests, validation et critères d'acceptation (à écrire avant les PR)

**F1. Normalisation et validation.** Jeux d'essais zéro/un/plusieurs biomes, emplacements nuls de poids zéro acceptés et de poids non nul rejetés, valeurs 3/5/2 → 0,3/0,5/0,2, conservation des biomes de poids nul et des cellules de somme nulle, rejet des poids négatifs/non finis et des doublons non nuls y compris de poids zéro, et normalisation avec somme/division en double de poids float élevés. A3 fixe les résultats attendus ; la tolérance numérique des tests reste à préciser lors de leur écriture. A4 fixe aussi les tests du dominant : premier maximum positif à égalité, absence de dominant dans les cellules entièrement nulles ou de poids tous nuls, prise en compte des modifications et exception du consommateur nécessitant un biome. A5 impose des tests de conservation exacte des indices, avec emplacements nuls intercalés, biomes de poids nul et maximums égaux.

**F2. Test de hook substituable.** Deux stratégies fictives de calcul, une passe générique commune ; vérifier que le parcours, la normalisation, le stockage, la publication et la libération ne sont pas réimplémentés par les stratégies.

**F3. Invariance spatiale.** Coordonnées positives/négatives, limites horizontales et verticales de chunks, ordre de génération inversé, réplication déterministe des résultats et absence de frontières artificiellement alignées sur les chunks.

**F4. HeightMap/combinaison.** Source unique, sources multiples, fusion pondérée, valeurs continues, grilles intercalées, cas vides, exceptions et absence d'écriture partielle publiée.

**F5. Pools/performance.** Tester les capacités configurables, l'écriture directe, la remise à zéro et le recyclage des grilles ; compter les allocations, observer la réutilisation des `Lease`, comparer évaluations par position et par grilles et vérifier la concurrence. Les budgets chiffrés et scénarios représentatifs restent à définir.

**F6. Intégration spécialisée sans couplage du cœur.** Décrire séparément les TU Core génériques et les tests plus spécifiques (dont voxel et golden images) qui prouveront la réutilisation du résultat ; aucune dépendance de Graphics requise dans Core.

## Points expressément **hors** de cet inventaire d'architecture générique

La sélection géométrique des centres, la perturbation des frontières Voronoï, la largeur de transition spécifique à une paire de régions, la formule d'un algorithme de répartition infinie, ainsi que l'implémentation concrète des strates voxel, des prefabs et de la génération volumétrique ne sont pas à définir dans cette première série de discussions. Ils pourront avoir leurs propres documents et tests une fois les interfaces génériques stabilisées.

## Méthode pour les conversations futures

Chaque nouvelle discussion pourra choisir **un identifiant précis** (A1, B3, C2, etc.), examiner les contraintes et variantes, valider une décision, puis mettre à jour ce document et les sections normatives de [biome-architecture.md](biome-architecture.md). Aucun ordre ou date de PR d'implémentation n'est approuvé dans cet inventaire.

## Références

- [Biome architecture — décisions et propositions](biome-architecture.md)
- [World generation — README](README.md)
- [Contrats de génération existants](contracts.md)
- [Architecture des chunks et des passes](architecture.md)
- [Passes applicatives](features.md)
- [Matrice de validation historique PR00–PR10](validation.md)
