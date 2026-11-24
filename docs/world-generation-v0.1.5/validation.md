# Sparkle v0.1.5 — Matrice de validation (PR00–PR10)
La responsabilité des tests figure aussi dans chaque [fiche de PR](implementation/README.md). CI Windows + Linux, C++23, clang-format, GoogleTest ; comparaison de golden images par l'outil natif Sparkle.

| PR | Scénarios de preuve incontournables |
| --- | --- |
| 00 LifeCycle | acquisition/release idempotentes, ordre destruction, Widget/Entity, Pool<LifeCycle> et Pool<int>, Lease survivant à Pool, callbacks |
| 01 Chunk | coordonnées négatives et limites overflow, dimensions non cubiques, setters notifient, copie/move sans héritage d'abonnements, affectation garde ses abonnements |
| 02 Collection/Filter | stockage TChunk par valeur et pointeur stable, ChunkFilter correct, Entity3D headless, remplacement sans recréer les composants, événement relayé, suppression ordonnée |
| 03 Generator | _createChunkBuilder nullopt et aucune passe, Data fraîche, Pass::apply(builder,coord,data) dans l'ordre, build par valeur et consommant, exception sans publication |
| 04 Requester | Task<std::optional<TChunk>> succès non vide/vide/Failed ; copie depuis résultat const, callbacks déjà terminés, source externe factice, lifetime worker |
| 05 Observer | position/rayon XYZ, 405 coordonnées pour rayon 4/2/4, Registry EngineHeadless, mouvement intra-chunk sans modification, rayon et attach/detach |
| 06 Streaming | union et priorité Manhattan monde anisotrope, replanification seulement au changement centre/rayon/observers, budget réel sous timeout, retry, version stale, nullopt quitté puis réentré |
| 07 Voxel | dimensions Volume égales SX/SY/SZ, remplacement immuable, événement de setter, Builder::load, structure franchissant la frontière et génération déterministe |
| 08 Graphics | Voxel::ChunkMesher outsideCell et six voisins au bake, arrivée/suppression/édition => invalidation, translucent/opaque, un seul rebake par coord avant rendu, aucun OpenGL headless |
| 09 Multi-observateur | déplacements indépendants, union et éviction, latence et ordre mélangé des réponses, shutdown non bloquant du streaming, headless components |
| 10 Release | démo intégrale, goldens inclinés et variés, contraintes lifetime, données de test non payantes, docs finalisées, aucun protocole ni dépendance Graphics en Core |

## Cas transversaux critiques

### Copie et remplacement
Deux chunks initialement identiques doivent posséder des abonnements indépendants. Après remplacement d'un chunk dans ChunkCollection, le ChunkFilter doit encore désigner le chunk correct, sans invalidation des composants de son Entity3D. Les changements des champs via setters déclenchent la notification et sont relayés avec la bonne coordonnée.

### Nullopt, échecs et timeout
Task Completed avec optional vide = absence valide, sans répétition tant que la coordonnée reste désirée. La sortie efface ce statut, la réentrée demande à nouveau. Failed et timeout donnent des retries configurables, mais aucun timeout ne doit libérer fictivement une place tant que le travail soumis reste en cours. Une ancienne Task n'est jamais autorisée à écraser un nouveau chunk.

### Synchronisation
Le callback worker ne modifie ni collection ni Entity. Le streamer applique les résultats sur l'update Engine. Le renderer reçoit un événement immédiat et regroupe son travail jusqu'au prochain render. Toutes les suppressions passent par la destruction LifeCycle de l'entité et ne la déréférencent pas au milieu de sa destruction.

### Voxel et goldens
Mêmes coordonnées + mêmes données utilisateur => même contenu de démonstration même si la génération s'inverse ; volume exactement aux dimensions du template. Affichage de formes, volumes translucides et opaques adjacents, angles 3D, chunks négatifs, bords, déchargement, réapparition, édition et six voisins.

### Durée de vie
Ne pas bloquer la destruction du streamer pour attendre une tâche ; en revanche un WorkerPool externe peut bloquer sa propre destruction à cause de std::jthread. Les tests distinguent ces deux responsabilités et gardent les dépendances des closures vivantes.

## Validation à spécifier pour les nouvelles briques (non incluse dans PR00–PR10)

Cette section est un **inventaire de tests à rédiger**, et non une matrice normative déjà approuvée. Elle concerne les nouvelles passes sans modifier les critères historiques ci-dessus :

- **Distribution générique :** stratégies factices substituables, parcours et destination gérés par la passe, zéro/un/plusieurs biomes, poids relatifs et normalisation par Sparkle (3, 5, 2 → 0,3, 0,5, 0,2), validations des données invalides dès que les règles seront arrêtées.
- **Coordonnées et déterminisme :** mêmes résultats aux mêmes X/Z mondiaux, dans les chunks voisins, négatifs et générés dans un ordre quelconque ; aucune obligation de frontière alignée sur les chunks.
- **Sources de hauteur :** HeightMap et Grid (configurations, échantillons aux bords, interpolation, pool), orchestration de sources propres à plusieurs biomes et modification ultérieure de la grille dès que leur contrat sera défini.
- **Mémoire et concurrence :** références non propriétaires correctement maintenues en vie, réutilisation des buffers et Leases, thread safety des échantillons, absence d'allocations inutiles, benchmarks descriptifs.
- **Couche voxel spécialisée :** tests de consommation des influences, hauteurs et strates, puis régressions graphiques et golden images dans les TU dédiés ; la logique voxel reste hors des TU de distribution générique.

Voir [questions ouvertes](biome-open-questions.md), section F, pour les scénarios sans résultat attendu encore validé.

## Gate
Aucun test de rendu exclu sans accord, aucun merge automatique. Tous les modules compatibles CI et revue explicite avant release.
