# Sparkle v0.1.5 — Plan d'implémentation revu

> Plan seulement : aucune PR d'implémentation ne peut être fusionnée sans approbation explicite. Les fichiers de conception vivent sur docs/v0.1.5-world-generation-design, base V0.1.5.

## Déroulement
| Phase | PR | Démonstration attendue |
| --- | --- | --- |
| Fondation | [00](implementation/PR-00-lifecycle.md), [01](implementation/PR-01-chunk-coordinates.md) | Cycle de vie et Chunk copiable, éditable, coordonnées stables |
| Conteneur headless | [02](implementation/PR-02-chunk-collection.md) | ChunkCollection possédant TChunk, Entity3D et ChunkFilter non propriétaire, événements |
| Génération | [03](implementation/PR-03-generator-passes.md), [04](implementation/PR-04-chunk-requester.md) | Passes ordonnées, Builder optionnel, Task<std::optional<TChunk>> |
| Intérêt et streaming | [05](implementation/PR-05-interest-observer.md), [06](implementation/PR-06-streaming-system.md) | Plusieurs joueurs, priorité Manhattan, timeout non annulant, éviction |
| Surcouche voxel/Graphics | [07](implementation/PR-07-voxel-example.md), [08](implementation/PR-08-graphics-adapter.md) | World::Voxel::Chunk de dimensions exactes, Chunk::Mesher et six voisins, synchronisation |
| Preuves d'intégration | [09](implementation/PR-09-headless-multiobserver.md), [10](implementation/PR-10-integration-validation.md) | Headless, rendering, goldens, release gate |

## Vérifications de fin de phase
Phase Fondation : abonnements copiés = aucun, opération move sûre, LifeCycle destruction contrôlée.
Phase Conteneur : aucune Entity détruite lors du remplacement, même adresse de chunk si possible par affectation sur place, ChunkFilter jamais dangling.
Phase Génération : aucune allocation owning-pointer imposée au consommateur, retour nullopt valide, erreurs distinctes, API Task réellement respectée.
Phase Streaming : pas de travail recalculé sur mouvement intra-chunk, aucune boucle sur nullopt, pas de cache après sortie, pas de saturation WorkerPool par timeouts.
Phase Graphics : aucune dépendance Graphics dans Core, remeshing chunk+six voisins sur édition, pas de référence durable aux voisins, goldens angles variés.

## Frontières de la release
Sparkle fournit des mécanismes, pas des règles de génération du monde. La génération de villages, routes, cavernes, structures géographiques, la physique et la persistance gameplay restent du code utilisateur ou des versions ultérieures.

Toutes les PR visent V0.1.5, avec C++23, clang-format, CI Windows/Linux, tests locaux gratuits. Ne créer aucun nouveau protocole ni infrastructure d'annulation de Task.
