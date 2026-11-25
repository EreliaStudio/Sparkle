# Sparkle v0.1.5 — Matrice de validation (PR00–PR10)
La responsabilité des tests figure aussi dans chaque [fiche de PR](implementation/README.md). CI Windows + Linux, C++23, clang-format, GoogleTest ; comparaison de golden images par l'outil natif Sparkle.

| PR | Scénarios de preuve incontournables |
| --- | --- |
| 00 LifeCycle | acquisition/release idempotentes, ordre destruction, Widget/Entity, Pool<LifeCycle> et Pool<int>, Lease survivant à Pool, callbacks |
| 01 Chunk | coordonnées négatives et limites overflow, dimensions non cubiques, setters notifient, copie/move sans héritage d'abonnements, affectation garde ses abonnements |
| 02 Collection/Filter | stockage TChunk par valeur et pointeur stable, ChunkFilter correct, Entity3D headless, remplacement sans recréer les composants, événement relayé, suppression ordonnée |
| 03 Generator | données persistantes et scratch par demande, préparation false sans passe, apply(coord,data) ordonné, instanciation par valeur, nettoyage après succès ou exception |
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

## Gate
Aucun test de rendu exclu sans accord, aucun merge automatique. Tous les modules compatibles CI et revue explicite avant release.
