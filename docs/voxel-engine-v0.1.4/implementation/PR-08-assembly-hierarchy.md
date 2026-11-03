# PR-08 — Body Core, Skeleton articulé autonome et Model::View

## Contrats publics

- `spk::Body` : interface polymorphe Core minimale, sans dépendance Graphics ou physique ; `Voxel::Volume : spk::Body` expose toujours ses données voxel habituelles.
- `spk::Skeleton` : type Core autonome. Une `Skeleton::Part` contient un `const Body *` facultatif et **non propriétaire**. `setBody()`, `body()` et `bodyVersion()` remplacent les méthodes spécifiques aux Volumes.
- Les Bodies doivent rester vivants tant qu'ils sont utilisés ; ni la Part ni le Model ne contrôlent leur durée de vie. Les copies de Parts et Models conservent les pointeurs nus.
- `Skeleton::Joint` possède un repère fixe, une rotation mutable et peut attacher une Part ou un Model par valeur. Les Models attachés restent indépendants et disposent de leurs propres palettes graphiques.
- `spk::Model` : Skeleton et transformation locale ; versions `transform`, `pose`, `topology`, `geometry` indépendantes.
- `Model::globalMatrix()`, `Skeleton::Part::globalMatrix()` et `Skeleton::Joint::globalMatrix()` : matrices d'assemblage Core avec `CachedData<Matrix4x4>` et invalidation de branche. La matrice de l'Entity est externe.
- `spk::Model::View` : classe Graphics qui référence un Model Core. Ses Part Views construisent des `Voxel::Geometry` depuis les Bodies `Voxel::Volume` et les gardent en cache selon `bodyVersion()`. Les autres types de Body attendent leurs providers graphiques futurs.
- **Une seule `spk::Palette` par `Model::View`** ; `bind(palette)` vérifie tous les slots du Model courant, sans toucher aux Models attachés. `harmonize()`, `PalettePropagation` et les palettes de Part Views sont supprimés.
- Les indices de matériaux restent locaux aux géométries. Le futur renderer devra associer ces slots à la palette du Model.
- Pas de `Model::View::visit()` ni de renderer dans cette PR ; le futur `VoxelModelRenderer3D` étendra `RenderingComponent3D`.

## Tests requis

- Compilation et installation de `sparkle_core` sans Graphics, indépendamment des voxels.
- Skeleton sans Body et avec Body générique, pointeurs non propriétaires, copies et déplacements, invalidations des versions de Body.
- Tests de matrices **avec valeurs de référence indépendantes** : rotations, ancrages orientés, Models imbriqués, chaînes articulées, mise en cache et changement de pose.
- Côté Graphics : génération voxel à la demande, remplacement de Body, mise en cache, palette unique et validation atomique, indépendance des palettes des Models attachés, pas de réindexation implicite.
- Les formes de collision et providers graphiques génériques sont hors de cette PR.

La PR reste Draft jusqu'à validation complète par CI.
