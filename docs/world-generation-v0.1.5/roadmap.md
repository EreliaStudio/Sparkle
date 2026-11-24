# Sparkle v0.1.5 — Plan d'implémentation revu

> Plan historique des PR00–PR10, rédigé lors des premières étapes du socle de chunks. Il ne constitue **pas** le plan des futures PR biome/hauteur. Aucune fusion sans approbation explicite. Le travail de conception biome est maintenant suivi dans [biome-architecture.md](biome-architecture.md) sur la branche `design/v0.1.5-biome-architecture`.

## Déroulement
| Phase | PR | Démonstration attendue |
| --- | --- | --- |
| Fondation | [00](implementation/PR-00-lifecycle.md), [01](implementation/PR-01-chunk-coordinates.md) | Cycle de vie et Chunk copiable, éditable, coordonnées stables |
| Conteneur headless | [02](implementation/PR-02-chunk-collection.md) | ChunkCollection possédant TChunk, Entity3D et ChunkFilter non propriétaire, événements |
| Génération | [03](implementation/PR-03-generator-passes.md), [04](implementation/PR-04-chunk-requester.md) | Passes ordonnées, Builder optionnel, Task<std::optional<TChunk>> |
| Intérêt et streaming | [05](implementation/PR-05-interest-observer.md), [06](implementation/PR-06-streaming-system.md) | Plusieurs joueurs, priorité Manhattan, timeout non annulant, éviction |
| Surcouche voxel/Graphics | [07](implementation/PR-07-voxel-example.md), [08](implementation/PR-08-graphics-adapter.md) | Voxel::Chunk de dimensions exactes, ChunkMesher et six voisins, synchronisation |
| Preuves d'intégration | [09](implementation/PR-09-headless-multiobserver.md), [10](implementation/PR-10-integration-validation.md) | Headless, rendering, goldens, release gate |

## Vérifications de fin de phase
Phase Fondation : abonnements copiés = aucun, opération move sûre, LifeCycle destruction contrôlée.
Phase Conteneur : aucune Entity détruite lors du remplacement, même adresse de chunk si possible par affectation sur place, ChunkFilter jamais dangling.
Phase Génération : aucune allocation owning-pointer imposée au consommateur, retour nullopt valide, erreurs distinctes, API Task réellement respectée.
Phase Streaming : pas de travail recalculé sur mouvement intra-chunk, aucune boucle sur nullopt, pas de cache après sortie, pas de saturation WorkerPool par timeouts.
Phase Graphics : aucune dépendance Graphics dans Core, remeshing chunk+six voisins sur édition, pas de référence durable aux voisins, goldens angles variés.

## Prochaine phase de conception (sans PR d'implémentation planifiée)

L'objectif est de compléter les contrats de distribution pondérée, de raccordement aux sources de hauteur et de données temporaires génériques avant de découper des PR. La couche voxel doit ensuite pouvoir offrir des composants de génération directement réutilisables, tout en restant distincte de `spk::World`. Les exemples Voronoï/procéduraux ne sont pas à détailler dans cette phase. Les points restant à discuter sont recensés dans [biome-open-questions.md](biome-open-questions.md) : ce document n'approuve aucun calendrier ni arbitrage.

## Frontières de la release
Sparkle fournit des mécanismes, pas des règles de génération du monde. La génération de villages, routes, cavernes, structures géographiques, la physique et la persistance gameplay restent du code utilisateur ou des versions ultérieures.

Toutes les PR visent V0.1.5, avec C++23, clang-format, CI Windows/Linux, tests locaux gratuits. Ne créer aucun nouveau protocole ni infrastructure d'annulation de Task.
