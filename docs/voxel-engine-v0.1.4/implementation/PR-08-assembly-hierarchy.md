# PR-08 — Model Core, Skeleton articulé et Views graphiques

## Contrats publics

- `spk::Model` appartient à `sparkle_core`, sans inclusion de `spk::Mesh`, `Voxel::Geometry`, `spk::Palette` ou d'un en-tête OpenGL.
- `Model` porte un `Model::Skeleton` mutable, une transformation globale et des versions indépendantes de transformation, pose, topologie et géométrie.
- `Model::Skeleton::Part` possède un `std::optional<Voxel::Volume>` : aucune géométrie requise pour une Part purement logique.
- `Model::Skeleton::Joint` possède une rotation mutable validée, un repère rigide et éventuellement une Part enfant. Les Parts sont reliées par les Joints ; aucune déclaration parallèle de Joint dans Model.
- Les copies de Models et de Skeletons sont indépendantes pour les états mutables, avec partage des snapshots de Volumes.
- Les chemins de Joints sont des `Skeleton::Path = std::vector<std::string>` et les recherches courtes exigent un nom unique.
- `spk::Model::View` est définie dans Graphics (`voxel/model_view.hpp`) et référence un Model Core existant. Elle construit les Part Views correspondant à la hiérarchie.
- `Skeleton::Part::View` conserve la palette facultative et la `Voxel::Geometry` optionnelle. Elle appelle `Mesher::bake(volume)` à la première demande et sur remplacement de Volume.
- Le mesh de profondeur reste construit dans Graphics depuis le mesh opaque par le Mesher. Les collisions triangulaires CPU constituent une responsabilité séparée, non réalisée par cette PR.
- Les transformations finales sont calculées à partir du Core, sans duplication de quaternions dans les Views.
- `bind(palette)` est local par défaut ; `PalettePropagation::Recursive` doit être explicitement demandé et valide les slots avant mutation.
- Une modification de la hiérarchie Core impose de reconstruire la View. Les rotations et remplacements de Volumes sont pris en compte sans recréer la View.
- L'harmonisation devient `Model::View::harmonize()` côté Graphics : unification récursive des slots, indices déterministes et remplacement des meshes préparés, sans modifier les Volumes ou les poses Core.

## Vérifications

Tester la compilation de `sparkle_core` sans Graphics, les Parts sans Volume, les Joints sans enfant, la pose Core, le versionnement et les copies indépendantes. Tester côté Graphics le meshing à la demande, les caches, leur invalidation sur remplacement de Volume, les palettes indépendantes, les poses communes vues par plusieurs Views, les transformations de points d'attache et le rejet d'une View obsolète après modification de topologie.

La PR reste en Draft jusqu'au succès des tests et du formatage CI.
