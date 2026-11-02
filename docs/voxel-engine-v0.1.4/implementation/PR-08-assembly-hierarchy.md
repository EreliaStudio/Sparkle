# PR-08 — Voxel::Mesh, Model immutable, Model::Builder et Model::View

## Contrats publics

- `Mesh::Vertex` et `Mesh::SurfaceMesh` sont des types imbriqués de `spk::Voxel::Mesh`.
- `Mesher::bake(volume, materialIndexOffset)` produit un Mesh immuable.
- `Model` possède `shared_ptr<const Data>` ; seules ses données immuables sont accessibles.
- `Model::Builder` configure le Mesh, les Anchors et leurs enfants via `setMesh`, `addAnchor`, `setAnchorTransform`, `removeAnchor`, `attach` et `detach`.
- `Builder::harmonize()`, appelé explicitement avant `build()`, unifie récursivement les slots par nom et opacité, avec ordre déterministe, réindexation des Meshes et rejet des conflits d'opacité.
- L'harmonisation ne dépend pas des palettes. Il n'existe plus de `Model::Harmonizer` ni de `Model::withMesh()`.
- L'Anchor enfant est facultatif, et `Anchor::NoAttachmentPoint = ""` signifie un alignement sur l'origine.
- `Model::View` gère palettes et rotations par occurrence, indépendamment du Model partagé.
- `Model::Anchor::Path` est un `std::vector<std::string>` ; chaque nom est un segment littéral, sans séparateur réservé.
- `Model::View::AnchorHandle` est lié à une View et détecte la destruction, le déplacement ou la réaffectation de celle-ci.

## Vérifications

Tester l'immuabilité des snapshots, la création des Anchors, les chemins typés et ambigus, l'absence de mutation en cas d'erreur de `attach` ou `harmonize`, l'idempotence de `harmonize`, la réindexation des indices de vertices, la propagation des palettes et l'invalidation des handles.

Le rendu GPU appartient aux PR ultérieures. La PR-08 reste en Draft jusqu'au succès de la compilation, des tests et du formatage CI.

## Conventions de nommage validées

- `Shape::Collection`, `Definition::Collection`, `Definition::SlotCollection` pour les associations par clés ; `Registry` et `SlotMapping` sont supprimés de l'API voxel.
- `Model::Builder::attach` / `detach` pour les enfants facultatifs des Anchors.
- `Volume::Builder::bind` et `Definition::Builder::bind` pour les catalogues nécessaires ; `Model::View::bind` pour l'association de palette au rendu.
- `Mesher::attachCache` / `detachCache` pour le cache facultatif.
- `Mesh::isCompatible`, à l'instar de `Model::isCompatible`.
- `Definition::Builder::loadFromJSON` remplace l'état, `Shape::Builder::addFromJSON` ajoute à la forme ; `Volume::Builder::copyFrom` copie un autre Volume.
