# PR10 — Démonstration complète, documentation et validation de release
**Couche :** intégration + tests Graphics. **Dépendances :** PR00–PR09. **État :** cahier des charges.

## Démonstrateur complet
Construire un exemple reproductible utilisant Chunk générique, Voxel::Chunk et son Builder, Generator + passes, ChunkRequester et WorkerPool, WorldObserver, WorldStreamingSystem, ChunkCollection + ChunkFilter, Entity3D headless, et adaptateur Graphic avec Voxel::ChunkMesher.

Montrer un joueur qui se déplace entre chunks, un monde limité qui retourne nullopt, deux observateurs en headless, une édition de Volume reconstruite via Builder et un setVolume qui notifie la collection. L'entité est conservée lors du remplacement et détruite lors du déchargement.

## Vérifications de publication
- Toutes les décisions documentées dans contracts.md correspondent au code et au comportement réellement testé, sans ancien unique_ptr<TChunk>, nullable Builder pointer, notifications coalescées dans Core, ni taille voxel indépendante pour Voxel::Chunk.
- Test des trois issues de demande (TChunk, nullopt, échec), du statut Pending et du timeout supervisé, de la protection contre retours tardifs, du budget physique et de la durée de vie des dépendances du WorkerPool.
- Test copie/déplacement des chunks sans transfert des abonnements, remplacement de la valeur stable dans ChunkFilter, abonnements collection et destruction Entity/Pool.
- Tester frontières XYZ, dimensions non cubiques, négatives, moteurs headless, composants non graphiques, rebake automatique chunk+six voisins.
- Golden images variées de blocs voisins, géométries opaques/translucides et angles de caméra ; comparateur interne Sparkle uniquement ; aucun golden ignoré.
- clang-format, tests GoogleTest, CI Windows/Linux, documentation des APIs finales avec exemples compilables.

## Hors scope
Aucun générateur géographique intégré (routes, villes, caves, biomes, macro-plans), aucune persistance automatique des modifications, aucun protocole réseau ni système de collision prêt à l'emploi.

## Validation
Release uniquement après validation explicite du mainteneur. Ne fusionner aucune PR automatiquement.
