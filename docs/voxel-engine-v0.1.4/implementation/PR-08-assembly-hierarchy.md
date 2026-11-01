# PR-08 — Voxel::Mesh, Model::Anchor, Model::View et Harmonizer

> PR GitHub Draft #49 ; branche impl/v0.1.4/08-assembly-hierarchy, cible V0.1.4. Ne pas fusionner sans validation.

## Périmètre

- SurfaceMesh : mesh de vertices individuel, nommé autrement que le nouveau Voxel::Mesh.
- Mesh : ensemble immuable des meshes opaque, translucide, depth et slots sémantiques avec offset.
- Mesher : bake(Volume, materialIndexOffset=0) retourne Mesh, et offsetMaterialIndices transforme une copie d'une ressource existante.
- Model : un Mesh, des Model::Anchor nommés et enfants facultatifs ; aucune Palette.
- Model::View : palettes indépendantes ou propagation explicite, rotations de joints, matrice globale, visite pour futurs renderers.
- Model::Harmonizer : reindexation des meshes et génération d'une Palette harmonisée et d'un MaterialLayout commun.
- Model::AnchorPath : segments explicites de noms, sans caractère séparateur réservé ; recherches courtes uniquement si uniques.
- Model::View::AnchorHandle : handle non propriétaire avec contrôle de validité après destruction, déplacement et réaffectation de la View.

## Contrats

La géométrie, les noms/opacités des slots et leurs indices viennent exclusivement du Mesher. Un Anchor est un repère de repos rigide : le Model source est copié lors d'un bind. Pour les enfants M_child = M_parent * A_parent * R_joint * inverse(A_child), en préservant les points de pivot. Les rotations et palettes ne sont modifiables que dans la View. Une View non harmonisée supporte une palette par Model, y compris des atlas différents ; la sortie harmonisée exige un atlas commun. Les modèles harmonisés ne peuvent utiliser que des palettes au MaterialLayout identique.

## Accès aux articulations

`view.anchor(Model::AnchorPath{"leftShoulder", "elbow"})` retourne un handle permettant de modifier l'articulation de cette View sans modifier les autres instances. `Model::AnchorPath{"leftShoulder/elbow"}` est un chemin distinct composé d'un seul segment. Les identités sont déterministes dans la hiérarchie et ne nécessitent aucun ID numérique.

## Tests

Géométrie, offsets, slots occultés, collisions, copies autonomes d'Anchors, rotations, propagations transactionnelles, palettes divergentes, harmonisation, invariance des sources et variants de palette. Vérification des chemins imbriqués, noms contenant `/`, noms courts ambigus, palettes par chemin et handles invalides après destruction ou déplacement de View.

## Hors périmètre

Rendu OpenGL / VoxelRenderingSystem, animations avancées, décision sur identifiants/chemins.

## Validation

CI Graphics/Core et clang-format requise avant validation humaine ; PR maintenue en Draft.
