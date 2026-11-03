# Sparkle v0.1.4 — Modèle articulé et représentation graphique

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6.1 Frontière Core / Graphics

La notion de `spk::Model` appartient au **Core**. Elle ne dépend d'aucun type de mesh, de palette, de GPU ou d'OpenGL. `Model` possède un `Model::Skeleton`, une transformation locale et un compteur de version propre à cette transformation. La transformation de l'`Entity3D` reste extérieure au Model.

Le `Skeleton` contient une `Skeleton::Part` racine et une hiérarchie de Parts reliées par des `Skeleton::Joint`. Une Part peut porter un `spk::Voxel::Volume` **optionnel**. Les Parts sans Volume sont des éléments logiques valides et ne produisent aucun mesh graphique.

Un Joint définit un repère rigide et une rotation mutable. Il peut attacher soit une Part enfant, soit un Model autonome, soit aucun enfant. Les attachements sont exclusifs : `attach(Part)` et `attach(Model)` remplacent l'attachement précédent ; `detach()` le retire. Le repère d'attache facultatif référence un Joint de la Part enfant ou de la Part racine du Model attaché. Le Model enfant conserve sa transformation propre et son Skeleton indépendant. La rotation est normalisée à l'affectation et vérifiée dans Core. Les transformations et les poses restent accessibles sans renderer pour les collisions, animations et simulations.

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

Une rotation de Joint ou une translation du Model ne reconstruit jamais les vertices : seules les données de pose et de transformation Core changent. Les matrices cumulées du Core sont calculées et conservées en cache indépendamment des meshes. Le cache est reconstruit si le Volume d'une Part change. Le meshing est réalisé côté Graphics et jamais dans Core.

La View réconcilie la hiérarchie graphique avec la topologie du Core à la demande : les attachements et détachements de Parts ou de Models reconstruisent seulement les branches concernées, en conservant les palettes et caches des branches intactes. `partView.attachedModel(jointName)` donne accès à la View du Model enfant, qui possède ses propres palettes. Une View d'un Model attaché suit sa pose et sa transformation propres. Lorsque la Part racine du Model est remplacée, `Model::View` reconstruit sa racine graphique.

La View et les Part Views ne possèdent pas les Models ou les Parts Core qu'elles représentent : le Model doit rester vivant et stable pendant la durée de vie des Views. Les copies de Views ont des palettes et des caches graphiques indépendants, mais observent le même Model Core. Après détachement, il faut éviter de conserver une référence vers l'ancienne Part View ou la View du Model retiré.

## 6.4 Palettes et transformations

`bind(palette)` affecte uniquement la Part View ciblée (`PalettePropagation::Self` par défaut). Une propagation récursive doit être demandée explicitement ; la compatibilité de tous les slots de toutes les géométries concernées est vérifiée avant de modifier les palettes. Les Materials supplémentaires restent autorisés.

`Model::globalMatrix()`, `Skeleton::Part::globalMatrix()` et `Skeleton::Joint::globalMatrix()` renvoient une référence constante vers leur matrice cumulée **dans l'assemblage de Models**, sans la matrice de l'`Entity3D`.

- `M_joint = M_part * A_joint * R_joint` : le repère fixe du Joint précède sa rotation de pose.
- `M_partChild = M_jointParent * inverse(A_child)` : l'inverse du repère fixe d'attache facultatif repositionne la Part enfant.
- `M_modelChild = M_jointParent * inverse(A_childRoot) * M_modelLocal` : un Model attaché conserve sa transformation propre, appliquée après l'alignement de son repère d'attache.

L'inverse du repère d'attache ignore la rotation de pose du Joint utilisé comme ancre. Les matrices utilisent des `CachedData<Matrix4x4>` à génération paresseuse, sur le modèle de `Transform3D` : une mutation invalide le nœud et ses descendants, pas les branches indépendantes. Les modifications du repère fixe utilisé comme ancre invalident également la Part ou le Model attaché. Les copies et déplacements rétablissent les liens parentaux non propriétaires.

La matrice monde finale reste la composition explicite `entity.transform().modelMatrix() * model.globalMatrix()`. Aucun lien vers une Entity n'existe dans Core.

La PR-08 ne fournit pas de méthode `Model::View::visit()` ni de capture graphique de la hiérarchie. La future intégration `VoxelModelRenderer3D`, fondée sur `RenderingComponent3D`, parcourra les Part Views et les Views des Models attachés pour produire des contributions au `RenderSnapshot` en utilisant les matrices cumulées calculées par le Core et la matrice de l'Entity. Les Parts purement logiques ne produiront pas de commande de dessin.

## 6.5 Ombres et collision

Dans cette PR, `Voxel::Mesher` produit ses meshes opaques et translucides et construit le `ShadowMap::Mesh` à partir du mesh opaque. **Aucun mesh GPU ne traverse la frontière Core.**

Le Core expose les Volumes, Shapes et transformations nécessaires à une future collision. Un éventuel `Collision::Mesh` sera un type CPU sans dépendance Graphics. Les meshes de collision et d'ombres n'ont pas nécessairement la même géométrie ou les mêmes règles ; ils ne doivent donc pas être confondus automatiquement.

L'harmonisation des indices de matériaux devient l'opération graphique explicite `Model::View::harmonize()`. Elle collecte les slots des Parts par couple (nom, opacité), puis reconstruit les `Voxel::Geometry` des Part Views avec une table d'indices commune. Plusieurs opacités peuvent partager le même nom de matériau. Elle ne modifie ni les Volumes Core ni les Joints. Après remplacement d'un Volume, une nouvelle harmonisation est nécessaire pour rétablir cette table globale. L'harmonisation ne traverse jamais les Models attachés.

### 6.6 Models attachés aux Joints

Un Joint Core peut recevoir une Part ou un Model complet. L'appel `joint.attach(model, "grip")` copie le Model si l'argument est une lvalue ou le déplace si c'est une rvalue, et garantit son existence jusqu'au prochain remplacement ou détachement. Le Joint continue d'exposer `child()` pour la Part et propose `model()` pour le Model attaché ; un seul de ces pointeurs est non nul. Un échec de validation du repère d'attache conserve l'ancien attachement.

L'ensemble articulé peut être parcouru sans aucune dépendance Graphics. Dans la View, `partView.attachedModel("hand")` renvoie la View propre au sous-modèle, dont les palettes et les caches sont indépendants. Les changements de topologie sont détectés par les versions du Core et réconciliés lors du prochain accès à la View. Les Parts et Models qui n'ont pas été remplacés conservent leurs Views et leurs palettes. La propagation de palette récursive reste limitée aux Parts du Model courant ; elle ne traverse pas implicitement vers un Model attaché.

La matrice d'un Model attaché utilise le repère du Joint parent, sa rotation, l'inverse facultatif du repère d'attache de la racine enfant, puis la transformation propre du Model enfant. Les Models imbriqués se composent récursivement.
