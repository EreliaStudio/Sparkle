# PR-03 — ShapeTransformationCache et OcclusionCache

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/03-shape-occlusion-caches`, créée depuis la tête courante de `V0.1.4` après fusion de PR-01. **Branche cible : `V0.1.4`.** Ne jamais cibler `main`.

**Objectif :** Fournir la transformation réutilisable des huit orientations et le calcul/cache d'occlusion directionnelle, sans Volume, Model, GPU ni VoxelMesher.

**Prérequis :** [PR-01](PR-01-voxel-foundations.md). PR-02 (Volume) peut être réalisée en parallèle après PR-01.

## Spécifications normatives

- [Modèle voxel](../02-modele-voxel.md)
- [Meshing et occlusion](../03-meshing-occlusion.md)
- [Contrats et validation](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)

## Périmètre exact à réaliser

- [ ] `ShapeTransformationCache` : pour une identité immuable de Shape et l'un des huit couples d'orientation, publier une géométrie transformée immuable et partageable.
- [ ] Une seule implémentation de l'algorithme de rotation/flip : positions, normales géométriques/d'éclairage, UV attachés aux sommets, boundaries et winding CCW conservant la face avant.
- [ ] `OcclusionCache` lié **explicitement et obligatoirement** à un `ShapeTransformationCache` externe ; aucun cache privé automatique et aucune transformation de secours.
- [ ] Algorithme CPU de clipping/occlusion directionnelle A→B réutilisable par le futur Mesher : polygones opaques frontaux, couverture partielle, union des surfaces, UV/normales interpolées selon les contrats.
- [ ] Clé de cache par identités Shape::Data A/B, orientations, direction, opacités pertinentes et tolérance ; `OcclusionResult` immuable, local à Shape A, avec fragments et slots géométriques.
- [ ] Opérations de consultation/calcul, `find/store/clear` selon l'API retenue, avec `ProtectedData<Data>` privé et calcul géométrique hors verrou.

## Contrat obligatoire de liaison

Un `OcclusionCache` ne peut exécuter aucune requête d'occlusion sans `ShapeTransformationCache` valide préalablement lié. Toute tentative d'usage sans liaison, et toute tentative de lier un pointeur nul, lève `spk::Exception`. La liaison est empruntée et non propriétaire : le cache de transformations doit vivre au moins autant que ses utilisations. Pas de recalcul implicite, pas de création automatique d'un cache de transformation.

La transformation ne doit plus être portée par `Cell` (ni publique, ni privée, ni via un accès `friend`). La Cell ne conserve que Definition et les deux enums d'orientation.

## Tests obligatoires

- [ ] Huit orientations sur cube, slab, slope et stair JSON, avec positions, UV, normales, boundaries et winding.
- [ ] Migrer les scénarios de tests géométriques retirés de PR-01 vers les tests du `ShapeTransformationCache` :
  - `EightOrientationsTransformPositionsAndNormals` : transformer la position (0.2, 0.3, 0.4) et le vecteur (1, 2, 3) dans les huit orientations ; comparer aux résultats attendus pour les quatre rotations et les quatre flips.
  - `WindingUVAndFrontFacesSurviveEveryVerticalFlip` : position et UV restent associés au même sommet sous flip ; normales géométriques et d'éclairage et winding CCW restent cohérents.
  - `FlippedTopAndBottomOuterFacesSwap` : le bord `PositiveY` devient `NegativeY` après inversion verticale.
  - `SixSidedCubeKeepsEachPhysicalFrontFaceUnderEveryOrientation` : conserver les six frontières distinctes et faces avant des six polygones.
  - `EveryShapeSupportsEightOrientedCellsWithoutUVOrWindingLoss` : reprendre les fixtures `cube.json`, `slab.json`, `slope.json`, `stair.json` depuis `tests/TU/Core/resources/voxel/shapes/` sans dupliquer leurs géométries, puis vérifier positions, UV, normales, slots locaux et winding.
  - Les tests appellent exclusivement la nouvelle API de transformation ; ne pas réintroduire d'adaptateur `CellTestAccess` ou de friend `VoxelMesher`.
- [ ] Identité partagée : même Shape/orientation réutilise le résultat, deux Shapes de contenu égal restent distinctes.
- [ ] Refus d'utilisation de `OcclusionCache` non lié ou lié à `nullptr` par `spk::Exception`.
- [ ] A→B différent de B→A, absence différente d'un résultat valide vide, opacités et tolérance dans la clé.
- [ ] Occlusion partielle, faces arrière/translucides/innerShell non occultantes, union des polygones opaques.
- [ ] Copie de résultat survivant à `clear()`, `store()` tardif après purge, lectures concurrentes, durée de vie des données Shape sans ABA.
- [ ] Compilation/test Core Windows et Linux Debug/Release ; aucune dépendance Engine ou OpenGL.

## Critères de livraison

La transformation et le calcul d'occlusion sont implémentés une seule fois ; la PR-04 consommera les résultats sans dupliquer les algorithmes. Exécuter `cmake --workflow --preset testDebug` et `testRelease` lorsque disponibles, déclarer les résultats réellement obtenus et ne pas fusionner sans validation du propriétaire.

**Hors périmètre :** Volume, VoxelMesher, Model, rendu, GPU, LRU, éviction automatique, ordonnanceur.
