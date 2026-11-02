# Sparkle v0.1.4 — Modèle articulé et représentation graphique

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6.1 Frontière Core / Graphics

La notion de `spk::Model` appartient au **Core**. Elle ne dépend d'aucun type de mesh, de palette, de GPU ou d'OpenGL. `Model` possède un `Model::Skeleton`, une transformation globale et un compteur de version propre à cette transformation.

Le `Skeleton` contient une `Skeleton::Part` racine et une hiérarchie de Parts reliées par des `Skeleton::Joint`. Une Part peut porter un `spk::Voxel::Volume` **optionnel**. Les Parts sans Volume sont des éléments logiques valides et ne produisent aucun mesh graphique.

Un Joint définit un repère rigide, une rotation mutable et, facultativement, une Part enfant et un repère d'attache dans celle-ci. Un Joint sans Part enfant est valide. La rotation est normalisée à l'affectation et vérifiée dans Core. Les transformations et les poses restent accessibles sans renderer pour les collisions, animations et simulations.

Les Models sont indépendants à la copie : les poses et hiérarchies mutables sont dupliquées, tandis que les Volumes et leurs ressources immuables restent partagés par valeur. L'Asset Manager peut charger les ressources source sans devenir propriétaire des occurrences mutables.

## 6.2 Versionnement

`Model::version()` fournit séparément les révisions `transform`, `pose`, `topology` et `geometry`.

- `transform` change lorsqu'on modifie la transformation globale du Model ;
- `pose` change lors d'une rotation ou d'une modification d'un repère de Joint ;
- `topology` change lorsque des Joints ou des Parts enfants sont ajoutés, remplacés ou détachés ;
- `geometry` change lorsque le Volume d'une Part change.

Une copie de Model reçoit des compteurs indépendants. Un Volume reste un snapshot immuable ; un changement de contenu passe par le remplacement de sa valeur dans une Part.

## 6.3 Représentation graphique

Les types `spk::Voxel::Geometry`, `Geometry::SurfaceMesh`, `spk::Mesh` et `spk::ShadowMap::Mesh` appartiennent uniquement à **Graphics**.

`spk::Model::View` référence un `spk::Model` mutable existant. Elle construit récursivement une `Model::Skeleton::Part::View` par Part attachée. Chaque Part View possède sa palette facultative et un cache de `Voxel::Geometry` créé **à la demande** depuis le `Voxel::Volume` de la Part avec `Voxel::Mesher::bake()`. Une Part sans Volume n'a pas de Geometry.

Une rotation de Joint ou une translation du Model ne reconstruit jamais les vertices : seule la matrice finale change. Le cache est reconstruit si le Volume d'une Part change. Le meshing est réalisé côté Graphics et jamais dans Core.

Les hiérarchies graphiques sont créées à la construction d'une View. Si la **topologie** du Skeleton change ensuite, cette View refuse son utilisation avec une exception explicite ; l'appelant doit créer une nouvelle View. Le remplacement d'un Volume ou la modification d'une pose ne demande pas de recréation de la View.

La View et les Part Views ne possèdent pas les Models ou les Parts Core qu'elles représentent : le Model doit rester vivant et stable pendant la durée de vie des Views. Les copies de Views ont des palettes et des caches graphiques indépendants, mais observent le même Model Core.

## 6.4 Palettes et transformations

`bind(palette)` affecte uniquement la Part View ciblée (`PalettePropagation::Self` par défaut). Une propagation récursive doit être demandée explicitement ; la compatibilité de tous les slots de toutes les géométries concernées est vérifiée avant de modifier les palettes. Les Materials supplémentaires restent autorisés.

La transformation d'une Part enfant est :

`M_child = M_parent * A_parent * R_joint * inverse(A_child)`

Le terme `inverse(A_child)` est appliqué seulement si le Joint désigne un repère d'attache présent dans la Part enfant.

`Model::View::visit()` parcourt les Parts possédant un Volume et fournit au renderer leur Geometry, leur Palette et leur matrice finale. Les Parts purement logiques restent dans le parcours hiérarchique mais ne déclenchent aucun draw. La View n'effectue pas directement d'appel OpenGL.

## 6.5 Ombres et collision

Dans cette PR, `Voxel::Mesher` produit ses meshes opaques et translucides et construit le `ShadowMap::Mesh` à partir du mesh opaque. **Aucun mesh GPU ne traverse la frontière Core.**

Le Core expose les Volumes, Shapes et transformations nécessaires à une future collision. Un éventuel `Collision::Mesh` sera un type CPU sans dépendance Graphics. Les meshes de collision et d'ombres n'ont pas nécessairement la même géométrie ou les mêmes règles ; ils ne doivent donc pas être confondus automatiquement.

L'harmonisation des indices de matériaux devient l'opération graphique explicite `Model::View::harmonize()`. Elle collecte les slots des Parts par nom et opacité, rejette les conflits, puis reconstruit les `Voxel::Geometry` des Part Views avec une table d'indices commune. Elle ne modifie ni les Volumes Core ni les Joints. Après remplacement d'un Volume, une nouvelle harmonisation est nécessaire pour rétablir cette table globale.
