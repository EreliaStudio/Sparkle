# Sparkle v0.1.5 — Questions ouvertes : biomes et génération du monde

> **Statut :** inventaire documentaire pour des discussions futures, établi le 24 novembre 2026.
> **Ce document ne tranche aucune question**, ne fixe pas de nouvelle API C++, ne demande pas d'implémenter un algorithme particulier et n'autorise ni PR de code ni fusion.
> **Source des décisions validées :** [Biome architecture](biome-architecture.md), surtout §§3, 4.4 et 4.5. Les anciens contrats de chunks et de génération restent documentés dans [contracts.md](contracts.md).

## Périmètre et architecture retenue

- `spk::World` fournit des composants de génération **génériques et réutilisables**, sans dépendre du type de terrain ou de son mode de représentation.
- `spk::Voxel` est un **cas d'utilisation majeur et une couche spécialisée de Sparkle** : à terme, il doit proposer un monde voxel 3D directement utilisable et des passes adaptées, sans qu'un jeu ait à les recoder.
- Les exemples de stratégies de placement (Voronoï fini, champs de bruit procéduraux, règles climatiques, placement manuel, etc.) servent à vérifier la généralité des interfaces. Leur conception interne et leur implémentation sont **reportées**.
- La sortie visée pour chaque position de l'espace X/Z est une **collection de références vers des biomes avec leurs poids**, éventuellement vide. Un biome dominant peut être déduit pour les consommateurs qui en ont besoin.
- **Décision déjà validée :** les hooks de distribution produisent des poids *relatifs* ; la **passe générique normalise elle-même ces poids**. Exemple : 3/5/2 devient 0,3/0,5/0,2. Ne pas rouvrir la répartition de cette responsabilité.
- Le générateur existant enchaîne des `Pass::apply(builder, coordinate, TGenerationData&)` et fournit des données temporaires par demande. Son interface générale ne doit pas être refondue sans nécessité constatée.
- Les références aux définitions de biomes restent non propriétaires ; l'instance propriétaire est sous la responsabilité du développeur. Aucune registry ni ID numérique obligatoire.
- `HeightMap` est une source concrète de hauteur à couches Perlin, et `HeightMap::Grid` une grille temporaire et modifiable, avec son `Lease` et les pools de capacité détaillés dans la documentation principale.
- La distribution, la génération/combinaison des hauteurs et leur application dans un support concret restent des responsabilités séparées. Les matériaux, strates, voxels, meshing et prefabs **n'appartiennent pas** au contrat de base de distribution `spk::World`.

## A — Contrat de résultat de distribution (générique)

**A1. Type d'une influence.** Quelle représentation publique adopter pour `BiomeInfluence` (pointeur const non possédant, référence enveloppée, autre) ? La collection peut-elle contenir une influence nulle, ou seulement être vide ?

**A2. Collection et stockage.** Quel conteneur représente les influences à une position (vecteur réutilisable, petite capacité inline, autre), et quel conteneur porte ces listes sur tout le domaine échantillonné ? Comment permettre zéro, un ou plusieurs biomes sans limite fixe ni allocation par échantillon imposée ?

**A3. Nettoyage/validation des poids bruts.** Quelle règle appliquer aux poids négatifs, NaN, infinis, aux poids nuls, aux doublons d'un même biome et à une liste non vide dont la somme est nulle ? Quelles situations entraînent `spk::Exception`, suppression ou agrégation d'entrées ? La **normalisation par Sparkle n'est pas en question** ; ses règles de validation détaillées le sont.

**A4. Sélection du dominant.** Quel contrat d'égalité pour des poids identiques : premier élément fourni, ordre stable explicite, règle de comparaison indépendante de l'ordre ? Éviter d'imposer un ID de biome qui a déjà été rejeté.

**A5. Ordre des influences.** Le résultat normalisé doit-il préserver l'ordre du hook, trier les poids, ou fournir un accès non ordonné ? Quels invariants de l'API publique en découlent ?

## B — Passes de distribution : hooks et coordonnées (générique)

**B1. Signature minimale du hook de calcul.** Quelles entrées exactes (position mondiale X/Z, contexte optionnel) et quelle sortie réutilisable ? Le passage d'un conteneur de sortie mutable est envisagé pour éviter une allocation par position, mais la signature n'est pas validée.

**B2. Hooks de configuration/destination.** De quelle manière une passe générique obtient-elle l'étendue 2D, le pas d'échantillonnage, l'origine et la destination dans `TGenerationData` sans imposer la structure de données du jeu ? Nombre et signatures des hooks à finaliser.

**B3. Coordonnées et résolution.** Quelle règle garantit une même distribution à une même position mondiale, y compris aux limites de chunks et aux coordonnées négatives ? Comment choisir la résolution de distribution indépendamment des algorithmes de placement, sans dégrader la précision requise par le consommateur ?

**B4. Règles d'accès aux résultats.** Échantillons aux deux extrémités ou domaine semi-ouvert, accès hors limites, accès discret/interpolé et compatibilité éventuelle avec `HeightMap::Grid` : quelles règles sont nécessaires pour que deux passes puissent partager le même domaine ?

**B5. Initialisation facultative des stratégies.** Une stratégie ayant un état de monde fini peut-elle initialiser cet état séparément de `apply()` ? Quels hooks sont réellement génériques, et lesquels doivent rester une responsabilité de l'implémentation de stratégie ?

**B6. Garanties de calcul.** Quel niveau de déterminisme et de sécurité en concurrence est exigé d'un hook et de ses objets partagés ? Quelles modifications de configuration sont interdites pendant l'exécution de jobs ?

## C — Sources de hauteur et orchestration (générique)

**C1. Contradiction de modèles.** Réconcilier le concept de `World::TerrainHeightGenerator` abstrait avec le `World::HeightMap` concret, non polymorphe, déjà validé : conserver deux abstractions, employer un adaptateur, ou une autre composition ? Ne pas assimiler silencieusement les deux.

**C2. Association biome/source.** Où se trouve l'association entre une définition générique de biome, qui reste minimale, et une ou plusieurs sources de hauteur ? Comment plusieurs biomes peuvent-ils partager une source sans imposer Perlin ou des champs dans la classe de base ?

**C3. Passe de hauteur unique ou distincte.** Comment conserver la passe documentée `HeightMapGenerationPass` liée à une seule `HeightMap`, tout en permettant une passe générique qui orchestre plusieurs sources pondérées ? Réutilisation, adaptation et responsabilités à préciser.

**C4. Fusion des hauteurs.** Quel contrat de fusion après normalisation ? Quels modes de transition sont nécessaires dans le cœur générique (dure, pondérée), où vivent les paramètres éventuels, et que faire lorsque plusieurs sources contribuent à la même position ? Ne pas y intégrer de mathématique Voronoï.

**C5. Cas sans hauteur disponible.** Quel résultat produire pour une distribution vide, une source absente, une hauteur non finie, ou une influence négligeable ? Qui décide de ne pas modifier une grille existante et qui signale l'erreur ?

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

**F1. Normalisation et validation.** Jeux d'essais zéro/un/plusieurs biomes, valeurs 3/5/2 → 0,3/0,5/0,2, somme nulle, valeurs invalides, doublons et égalités ; la décision du moteur de normaliser est établie, les autres résultats attendus attendent les règles correspondantes.

**F2. Test de hook substituable.** Deux stratégies fictives de calcul, une passe générique commune ; vérifier que le parcours, la normalisation, le stockage, la publication et la libération ne sont pas réimplémentés par les stratégies.

**F3. Invariance spatiale.** Coordonnées positives/négatives, limites horizontales et verticales de chunks, ordre de génération inversé, réplication déterministe des résultats et absence de frontières artificiellement alignées sur les chunks.

**F4. HeightMap/combinaison.** Source unique, sources multiples, fusion pondérée, valeurs continues, grilles intercalées, cas vides, exceptions et absence d'écriture partielle publiée.

**F5. Pools/performance.** Compter les allocations, observer la réutilisation des `Lease`, comparer évaluations par position et par grilles et vérifier la concurrence. Les budgets chiffrés et scénarios représentatifs restent à définir.

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
