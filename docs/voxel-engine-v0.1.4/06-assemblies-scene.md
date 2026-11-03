# Sparkle v0.1.4 — Modèles articulés et Bodies indépendants

[Sommaire de l'architecture](../voxel-engine-v0.1.4-specification.md)

## 6.1 Séparation Core / Graphics

`spk::Body` est une interface polymorphe **Core**, actuellement limitée à un destructeur virtuel. Elle identifie une description tridimensionnelle réutilisable ; elle ne prescrit ni rendu, ni masse, ni intégration physique. `spk::Voxel::Volume` hérite directement de `spk::Body`. D'autres sources (paramétriques, meshes CPU, etc.) pourront l'implémenter.

`spk::Skeleton` est une classe **Core autonome**, sans dépendance aux voxels ou à Graphics. Il possède une `Skeleton::Part` racine et des `Skeleton::Joint` articulés. Chaque Part peut référencer un corps par `const spk::Body *` : pointeur nu, facultatif, non propriétaire. **Le Skeleton ne possède pas les Bodies et ne prolonge pas leur durée de vie**. Leur propriétaire doit garantir leur validité pendant leur utilisation. Les copies de Skeleton reproduisent les mêmes pointeurs sans copier ni retenir les Bodies. Une Part peut être purement logique, sans Body.

`spk::Model` possède un Skeleton mutable, une transformation propre et les compteurs de versions. Les Models et Skeletons sont copiables avec leurs poses indépendantes. Les Parts peuvent référencer le même Body, ou des Bodies de types différents. L'Asset Manager peut gérer les ressources, mais n'est jamais propriétaire implicite via les pointeurs des Parts.

Un Joint porte un repère rigide fixe et une rotation de pose. Il peut contenir une Part enfant, un Model complet ou aucun enfant ; `attach(Part)`, `attach(Model)` et `detach()` restent exclusifs. L'attachement facultatif par nom choisit un Joint de référence dans la Part ou la racine du Model enfant. Un Model attaché conserve son Skeleton et sa transformation propres.

## 6.2 Versions et mises à jour

`Model::version()` expose `transform`, `pose`, `topology` et `geometry` :
- `transform` : transformation locale du Model ;
- `pose` : rotation ou transformation fixe d'un Joint ;
- `topology` : ajout, remplacement ou retrait de Joints, Parts ou Models ;
- `geometry` : changement de pointeur de Body via `Part::setBody()`.

`Part::bodyVersion()` permet aux consommateurs de savoir quand reconstituer leurs représentations. Réaffecter un Body par `setBody()` notifie ce changement même si l'adresse reste identique ; modifier directement les données d'un Body référencé ne déclenche **aucune invalidation automatique**. Les descriptions destinées à cet usage doivent donc rester immuables ou être réassignées explicitement.

## 6.3 Matrices cumulées

`Model::globalMatrix()`, `Skeleton::Part::globalMatrix()` et `Skeleton::Joint::globalMatrix()` renvoient des matrices d'assemblage calculées paresseusement et mémorisées via `CachedData<Matrix4x4>`.

- `M_joint = M_part * A_joint * R_joint` : repère fixe, puis rotation de pose.
- `M_partChild = M_jointParent * inverse(A_child)` : alignement facultatif du repère d'attache de la Part.
- `M_modelChild = M_jointParent * inverse(A_childRoot) * M_modelLocal` : alignement puis transformation propre du Model attaché.

L'inversion d'ancre utilise le repère fixe du Joint, pas sa rotation animée. Une modification invalide les caches concernés et ceux de leurs descendants, sans invalider les autres branches. Les liens parentaux non propriétaires sont rétablis lors des copies, mouvements et remplacements. Les Joints sans Body restent des articulations valides.

Les transformations du Model ne comprennent **pas** celles d'une `Entity3D` : le futur renderer composera explicitement `entity.transform().modelMatrix() * model.globalMatrix()`.

## 6.4 Représentation graphique et palettes

`spk::Voxel::Geometry`, `spk::Mesh` et `spk::ShadowMap::Mesh` restent dans **Graphics**. Le Core ne contient aucun mesh ni appel OpenGL.

`spk::Model::View` (Graphics) suit la hiérarchie Core, avec une `Skeleton::Part::View` par Part. La Part View conserve uniquement sa géométrie graphique en cache, créée à la demande avec `Voxel::Mesher::bake()` si le Body référencé est un `Voxel::Volume`. Une Part sans Body n'a pas de géométrie. À ce stade, un Body d'un autre type est explicitement refusé par ce chemin de meshing voxel ; sa prise en charge nécessitera un provider graphique adapté dans une PR ultérieure. Une rotation n'entraîne pas de nouveau meshing.

**Une seule palette appartient à chaque `Model::View`.** `bind(palette)` valide les matériaux nécessaires à toutes les Parts du Model courant avant modification. Les Parts du même Model la partagent ; les Models attachés possèdent leurs propres Views et palettes. Aucune propagation de palette vers des Models attachés.

`Model::View::harmonize()` et `PalettePropagation` sont supprimés. Chaque Geometry conserve ses indices de matériaux locaux ; le futur système de rendu doit résoudre ces slots vers la palette du Model, sans imposer une harmonisation globale préalable.

La View se réconcilie à la demande avec les modifications de topologie. Elle conserve les caches des branches inchangées, sans devenir propriétaire des Models, Parts ou Bodies. Les vues d'enfants retirés ne doivent plus être utilisées.

## 6.5 Collisions et évolution

Les mêmes données `spk::Body` et matrices Core peuvent être utilisées pour produire une forme de collision CPU sans imposer que cette forme soit identique au mesh de rendu ou d'ombres. `spk::Body` ne devient pas automatiquement un corps dynamique de simulation physique. Un futur `Physics::RigidBody` sera une responsabilité indépendante.

La conversion générique Body → géométrie ou forme de collision reste volontairement hors de la PR-08. Le prochain `VoxelModelRenderer3D`, composant de rendu, utilisera les matrices de Core, les caches de Part Views et les palettes propres aux Views. Il n'y a pas de méthode `Model::View::visit()`.
