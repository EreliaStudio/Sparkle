# Sparkle v0.1.5 — Fiches de PR révisées
Cahier des charges **documentation uniquement** ; les PR sont proposées, pas encore autorisées pour implémentation. Base de livraison : branche V0.1.5 ; validation explicite requise avant toute fusion.

## Découpage (11 PR de code prévues)
| PR | Mission | Dépendances |
| --- | --- | --- |
| [00](PR-00-lifecycle.md) | spk::LifeCycle, Entity/Widget et Pool::Lease | — |
| [01](PR-01-chunk-coordinates.md) | Chunk générique, conversions, événements d'édition et sémantique de copie | — |
| [02](PR-02-chunk-collection.md) | ChunkCollection, stockage par valeur, ChunkFilter, Entity3D headless, relais des événements | 00, 01 |
| [03](PR-03-generator-passes.md) | Generator, addPass, TChunk::Builder facultatif, std::optional et generation data | 01 |
| [04](PR-04-chunk-requester.md) | ChunkRequester par Task<std::optional<TChunk>>, adaptation WorkerPool | 03 |
| [05](PR-05-interest-observer.md) | WorldObserver, ChunkInterest et Registry headless | 01 |
| [06](PR-06-streaming-system.md) | WorldStreamingSystem, Manhattan, union, timeouts, retries, nullopt local à l'intérêt | 02, 04, 05 |
| [07](PR-07-voxel-example.md) | spk::Voxel::Chunk, Volume exact au template, Builder et exemple de passes | 01, 02, 03, 06 |
| [08](PR-08-graphics-adapter.md) | Voxel::ChunkMesher voisin-aware, rendu Graphics, dirty chunk + six voisins | 00, 02, 07 |
| [09](PR-09-headless-multiobserver.md) | Tests d'intégration de plusieurs observateurs, requesters, lifecycle headless | 05, 06, 07 |
| [10](PR-10-integration-validation.md) | Tests finaux, démo, golden images, docs et release gate | 00–09 |

## Principes transversaux
- La collection **possède** les TChunk comme valeurs stables ; une Entity3D par chunk et ChunkFilter non propriétaire donnent un accès rapide depuis n'importe quel système, y compris sans Graphics.
- Les setters de chunks notifient via notifyChunkEdition(), explicitement dans chaque méthode utilisateur. La collection relaie immédiatement ; Graphics regroupe les coordonnées au render.
- La copie/déplacement d'un chunk ne transfère jamais ses abonnements ; l'affectation conserve les abonnements de la destination.
- Génération et demande retournent std::optional<TChunk>, nullopt signifiant absence valide ; Task Failed/timeout restent distincts. L'absence est oubliée à la sortie de l'intérêt.
- Priorité Manhattan pondérée par dimensions du chunk ; file entièrement reconstruite à chaque changement de coordonnée centrale, de rayon ou d'observateurs, jamais pour un mouvement intra-chunk.
- Tasks non annulées : résultats expirés ignorés, budget comptabilisant les jobs non terminés. Arrêt du streamer non bloquant ; shutdown de WorkerPool potentiellement bloquant.
- Voxel::ChunkMesher redéfinit outsideCell() ; les six voisins sont résolus lors de chaque bake ; aucune extension du mesher générique.
- Tests dans chaque PR : unitaires et intégration, format clang, CI Windows/Linux ; golden images avec comparateur Sparkle uniquement.

## Exclusions explicites
Aucune implémentation de réseau, de collision, de sauvegarde automatique, de multi-world ID, de biomes, de routes, de villes ou de cavernes intégrés. Les données persistantes de génération et leur sécurité appartiennent au jeu.

Le détail des responsabilités et des critères de réussite est dans chaque fiche, complété par [architecture](../architecture.md), [contracts](../contracts.md), [review](../architecture-review.md) et [validation](../validation.md).
