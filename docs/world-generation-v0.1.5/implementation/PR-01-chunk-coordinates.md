# PR01 — Chunk générique : dimensions, coordonnées et édition
**Couche :** Core. **Dépendances :** aucune. **État :** cahier des charges.

## API et invariants
- spk::Chunk<SX,SY,SZ>, base héritable, trois dimensions strictement positives fixées au compile-time et exprimées en unités du monde. Pas de coordonnée membre, pas d'Entity, pas de Volume ni de données imposées.
- ChunkCoordinate : trois entiers signés, s'appuyant sur les types vectoriels Sparkle ; conversion worldToChunkCoordinate en floor sur les coordonnées négatives ; chunkCoordinateToWorld donne l'origine ; worldToLocalCoordinate donne les coordonnées relatives.
- Calculs robustes aux limites int32 et aux débordements ; ne pas utiliser la troncature C++ pour les nombres négatifs.
- Chaque Chunk porte son propre fournisseur d'événement d'édition. subscribeToChunkEdition(...) et méthode protégée notifyChunkEdition() appelée explicitement dans chaque setter d'une classe dérivée. Les données sont modifiables ; aucune immutabilité obligatoire.
- Construction par copie ou déplacement = nouvelle instance avec fournisseur d'événements propre, sans importer d'abonnements. Affectation = ne copie jamais les abonnements de la source et préserve ceux déjà attachés à l'objet destination ; la collection signale séparément le remplacement.
- Les méthodes de construction/affectation ne notifient pas arbitrairement le contexte d'un autre chunk ; les setters utilisateur font les notifications à chaque modification réussie.

## Tests
Conversions et round trips avec (-1), zéro, frontières exactes, grands nombres et trois dimensions distinctes. Chunk générique non voxel. Copie, déplacement, affectation, indépendance et conservation des abonnements de la destination. Setters qui notifient exactement quand attendu.

## Exclusions
Pas de TChunk::Builder dans cette base, pas de stockage, de génération ni de Graphics.
