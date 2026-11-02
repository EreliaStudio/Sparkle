# PR-08 — Voxel::Geometry, Model immutable, Model::Builder et Model::View

## Contrats publics

- `Geometry::Vertex` et `Geometry::SurfaceMesh` sont des types imbriqués de `spk::Voxel::Geometry`.
- `Mesher::bake(volume)` produit une `Geometry` immuable.
- `Model` possède `shared_ptr<const Data>` ; seules ses données immuables sont accessibles.
- `Model::Builder` configure la `Geometry`, les Anchors et leurs enfants via `setGeometry`, `addAnchor`, `setAnchorTransform`, `removeAnchor`, `attach` et `detach`.
- `Builder::harmonize()`, appelé explicitement avant `build()`, unifie récursivement les slots par nom et opacité, avec ordre déterministe, réindexation des géométries et rejet des conflits d'opacité. Chaque appel reconstruit les indices, y compris après une harmonisation précédente.
- L'harmonisation ne dépend pas des palettes. Il n'existe plus de `Model::Harmonizer` ni de `Model::withMesh()`. `Geometry` n'enregistre ni décalage `offset` ni indicateur `harmonized` ; tous les indices sont locaux, à partir de zéro.
- L'Anchor enfant est facultatif, et `Anchor::NoAttachmentPoint = ""` signifie un alignement sur l'origine.
- Un Anchor conserve son enfant dans `std::optional<Model>` (absence : `std::nullopt`) et retourne `nullptr` depuis `model()` si l'optionnel est vide. `Model{}` reste un Model valide à géométrie vide. Un `attach()` copie légèrement le Model en partageant `shared_ptr<const Data>`, sans allouer de `shared_ptr<const Model>` et sans imposer de durée de vie à l'Asset Manager ; `detach()` rétablit l'absence. Les Models harmonisés sont conservés de la même façon dans la hiérarchie.
- `Model::View` représente une occurrence : Model immuable partagé, palette optionnelle, transformation locale et une map d'`Anchor::View`. À la construction, toutes les Anchor Views et les Model Views enfants sont créées récursivement. Chaque `Anchor::View` porte son quaternion, ainsi qu'un `unique_ptr<Model::View>` optionnel pour l'enfant. Les copies des Views ont des états de palette et de pose indépendants.
- `bind(palette)` valide et affecte uniquement la View concernée ; `PalettePropagation::Recursive` doit être demandé pour tous les enfants. L'opération récursive valide l'ensemble avant de remplacer les palettes.
- `View::isRenderable()` contrôle les palettes manquantes ou incompatibles sans avaler les exceptions inattendues.
- `spk::JSON::Catalog::load()` valide tout le fichier dans un état temporaire avant insertion. Un échec ne modifie ni les Shapes ni les Definitions déjà chargées, tout en conservant le support des éléments non copiables.
- `Model::Anchor::Path` est un `std::vector<std::string>` ; chaque nom est un segment littéral, sans séparateur réservé.
- `Model::View::anchor(name ou Anchor::Path)` donne une référence à l'`Anchor::View` (nom court uniquement non ambigu). `Anchor::View::child()` donne la View du Model enfant ou lève une exception s'il n'y en a pas. Les anciennes maps de palettes/rotations par chemins et `AnchorHandle` sont supprimés.

## Vérifications

Tester l'immuabilité des snapshots, la création récursive des Views et les Anchors sans enfant, les chemins typés et ambigus, les palettes et rotations indépendantes après copie, la composition de transformation racine/Anchor/enfant, l'absence de mutation en cas d'erreur de `attach` ou `harmonize`, la reconstruction effective lors de deux appels successifs à `harmonize`, la réindexation des indices de vertices, la propagation explicite des palettes, le chargement JSON atomique, la propagation des erreurs inattendues et l'invalidation des handles.

Le rendu GPU appartient aux PR ultérieures. La PR-08 reste en Draft jusqu'au succès de la compilation, des tests et du formatage CI.

## Conventions de nommage validées

- `Shape::Collection`, `Definition::Collection`, `Definition::SlotCollection` pour les associations par clés ; `Registry` et `SlotMapping` sont supprimés de l'API voxel.
- `Model::Builder::attach` / `detach` pour les enfants facultatifs des Anchors.
- `Volume::Builder::bind` et `Definition::Builder::bind` pour les catalogues nécessaires ; `Model::View::bind` pour l'association de palette au rendu.
- `Mesher::attachCache` / `detachCache` pour le cache facultatif.
- `Geometry::isCompatible`, à l'instar de `Model::isCompatible`.
- `Definition::Builder::loadFromJSON` remplace l'état, `Shape::Builder::addFromJSON` ajoute à la forme ; `Volume::Builder::copyFrom` copie un autre Volume.
