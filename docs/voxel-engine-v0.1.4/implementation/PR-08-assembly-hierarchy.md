# PR-08 — Model Core, Skeleton articulé et Views graphiques

## Contrats publics

- `spk::Model` appartient à `sparkle_core`, sans inclusion de `spk::Mesh`, `Voxel::Geometry`, `spk::Palette` ou d'un en-tête OpenGL.
- `Model` porte un `Model::Skeleton` mutable, une transformation globale et des versions indépendantes de transformation, pose, topologie et géométrie.
- `Model::Skeleton::Part` possède un `std::optional<Voxel::Volume>` : aucune géométrie requise pour une Part purement logique.
- `Model::Skeleton::Joint` possède une rotation mutable validée, un repère rigide et soit une Part enfant, soit un `spk::Model` autonome, soit aucun enfant. `attach(Part)` et `attach(Model)` sont exclusifs, `detach()` retire l'enfant, et l'attribut `attachment` sélectionne un Joint de référence chez l'enfant.
- Les copies de Models et de Skeletons sont indépendantes pour les états mutables, avec partage des snapshots de Volumes.
- Les chemins de Joints sont des `Skeleton::Path = std::vector<std::string>` et les recherches courtes exigent un nom unique.
- `spk::Model::View` est définie dans Graphics (`voxel/model_view.hpp`) et référence un Model Core existant. Elle construit les Part Views correspondant à la hiérarchie.
- `Skeleton::Part::View` conserve la palette facultative et la `Voxel::Geometry` optionnelle. Elle appelle `Mesher::bake(volume)` à la première demande et sur remplacement de Volume.
- Le mesh de profondeur reste construit dans Graphics depuis le mesh opaque par le Mesher. Les collisions triangulaires CPU constituent une responsabilité séparée, non réalisée par cette PR.
- Les transformations et les rotations restent dans le Core, sans duplication de quaternions dans les Views. `Model`, `Skeleton::Part` et `Skeleton::Joint` exposent `globalMatrix()` : matrices cumulées dans l'assemblage, excluant la transformation de l'`Entity3D`, avec caches `CachedData<Matrix4x4>` invalidés par branche à chaque modification pertinente.
- Les liens parentaux non propriétaires sont restaurés après copies, déplacements et remplacements. Les changements de repères d'attache invalident également les matrices des enfants alignés sur ces repères.
- La collecte des contributions au `RenderSnapshot` relève d'une PR ultérieure, via un `VoxelModelRenderer3D` dérivé de `RenderingComponent3D`. Aucune API `Model::View::visit()` n'est exposée.
- `bind(palette)` est local par défaut ; `PalettePropagation::Recursive` doit être explicitement demandé et valide les slots avant mutation.
- Les modifications de topologie Core sont réconciliées à la demande par la View, sans recréer les branches inchangées. Les Views de Models attachés sont accessibles via `Part::View::attachedModel(name)` ; elles possèdent leur état graphique indépendant. Les rotations et remplacements de Volumes sont pris en compte sans recréer la View.
- L'harmonisation devient `Model::View::harmonize()` côté Graphics : unification récursive des slots des Parts du Model courant par (nom, opacité), indices déterministes et remplacement des meshes préparés, sans modifier les Volumes ou les poses Core ni traverser les Models attachés.

## Vérifications

Tester la compilation de `sparkle_core` sans Graphics, les Parts sans Volume, les Joints sans enfant, les Models attachés par valeur, la pose Core, le versionnement, les matrices cumulées mises en cache, leurs invalidations, les repères d'attache et les copies/déplacements indépendants. Tester côté Graphics le meshing à la demande, les caches, les palettes indépendantes, les repères d'attache conservés dans le Core et la réconciliation dynamique lors de l'équipement, du remplacement et du détachement d'un Model ou d'une Part.

La PR reste en Draft jusqu'au succès des tests et du formatage CI.
