# PR-08 — Voxel::Mesh, Model::Anchor, Model::View et Harmonizer

> PR GitHub Draft #49 ; branche impl/v0.1.4/08-assembly-hierarchy, cible V0.1.4. Ne pas fusionner sans validation.

## Périmètre

- SurfaceMesh : mesh de vertices individuel, nommé autrement que le nouveau Voxel::Mesh.
- Mesh : ensemble immuable des meshes opaque, translucide, depth et slots sémantiques avec offset.
- Mesher : bake(Volume, materialIndexOffset=0) retourne Mesh, et offsetMaterialIndices transforme une copie d'une ressource existante.
- Model : un Mesh, des Model::Anchor nommés et enfants facultatifs ; aucune Palette.
- Model::View : palettes indépendantes ou propagation explicite, rotations de joints, matrice globale, visite pour futurs renderers.
- Model::Harmonizer : reindexation des meshes et génération d'une Palette harmonisée et d'un MaterialLayout commun.
- La stratégie définitive d'identifiants et chemins d'occurrence demeure volontairement ouverte ; les APIs provisoires rejettent les noms ambigus.

## Contrats

La géométrie, les noms/opacités des slots et leurs indices viennent exclusivement du Mesher. Un Anchor est un repère de repos rigide : le Model source est copié lors d'un bind. Pour les enfants M_child = M_parent * A_parent * R_joint * inverse(A_child), en préservant les points de pivot. Les rotations et palettes ne sont modifiables que dans la View. Une View non harmonisée supporte une palette par Model, y compris des atlas différents ; la sortie harmonisée exige un atlas commun. Les modèles harmonisés ne peuvent utiliser que des palettes au MaterialLayout identique.

## Tests

Géométrie, offsets, slots occultés, collisions, copies autonomes d'Anchors, rotations, propagations transactionnelles, palettes divergentes, harmonisation, invariance des sources et variants de palette.

## Hors périmètre

Rendu OpenGL / VoxelRenderingSystem, animations avancées, décision sur identifiants/chemins.

## Validation

CI Graphics/Core et clang-format requise avant validation humaine ; PR maintenue en Draft.
