# PR-04 — Mesher et Geometry (livrée)

> **Fiche historique, implémentation mergée dans `V0.1.4`.** Voir [l'état consolidé](IMPLEMENTED-01-08.md) et [le chapitre 03](../03-meshing-occlusion.md).

**Livré :** `spk::Voxel::Mesher::bake(const Volume&)` génère une `spk::Voxel::Geometry` immuable, en utilisant `Voxel::Occlusion::{Algorithm,Cache,Result}` et les instances de Shape pré-orientées.

## API et périmètre effectifs

- `Mesher::attachCache(Occlusion::Cache&)`, `detachCache()`, `setTolerance(float)` et `bake(const Volume&)` ; mode sans cache autonome.
- `Geometry::Vertex {Vector3 position, Vector3 normal, Vector2 uv, uint32_t materialIndex}`.
- `Geometry::opaqueMesh()`, `transparentMesh()`, `depthMesh()`, `materialSlots()`, `materialSlot(index)` et `isCompatible(palette)`.
- Deux meshes visuels opaques/translucides et un mesh depth géométrique ; slots de Definition triés lexicalement, même occultés.
- L'opacité du `Definition::Slot` définit opaque/translucide ; alpha des textures sans effet sur le meshing ni la silhouette du depth mesh.
- Les conflits d'opacité pour un nom de slot, erreurs de géométrie et de culling doivent échouer sans publier de géométrie partielle.

## Évolutions depuis le plan initial

`VoxelMesher` est devenu `Voxel::Mesher`. `Voxel::Model` est devenu `Voxel::Geometry` ; aucune palette n'est attachée au résultat de bake. Le résultat n'a ni `bind(palette)` ni `palette()`. La création des RenderCommands GPU et le choix d'apparence seront traités en PR-13/PR-14.

## Validation historique

Tests Core/Graphics de géométrie, clipping, occlusion, orientations et meshes de profondeur intégrés. Pour les résultats précis, consulter les CI associées aux PR fusionnées ; cette fiche n'affirme pas une nouvelle exécution de tests.
