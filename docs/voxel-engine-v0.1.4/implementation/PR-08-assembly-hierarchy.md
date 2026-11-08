# PR-08 — Model3D Blueprint, ObjectHeadless et Object<TGraphics> (livrée)

> **Implémentée et fusionnée dans `V0.1.4` via PR #49 et ses PR intermédiaires.** L'ancien intitulé « Voxel::Assembly par copie et expansion » est abandonné. Voir [l'état consolidé](IMPLEMENTED-01-08.md) et [le chapitre 06](../06-assemblies-scene.md).

## Périmètre livré

- `spk::Model3D::Blueprint` : définition immuable, Builder, `Element` polymorphes nommés, anchors, points d'attachement rigides et enfants structurels.
- `spk::Model3D::ObjectHeadless` : instances Core mutables, articulations, cache de matrices globales, sous-objets attachés/détachés, copies profondes des occurrences et mouvements.
- `spk::Model3D::Object<TGraphics>` : vues graphiques spécialisées, cache par `ElementNode`, `synchronize()` et `buildRenderSnapshot(builder, transform)`.
- Concept `Model3D::ObjectGraphics` : `ElementType`, constructeur depuis `const ElementType&`, contribution au `RenderSnapshot`.
- Tests Core/Graphics, y compris golden images 3D avec `ColorVolume`, `ColorView` et caméra, types définis seulement dans les tests.

## Contrats abandonnés et non livrés

- Pas de `spk::Voxel::Assembly` mutable, `PartID`, reparentage de Parts, import par copie/expansion ni `VersionedTrait` propre à l'Assembly.
- Pas de `Skeleton`, `Puppet`, `Model::View` ou `Assembly::Instance::View` dans l'API finale.
- Pas de factory hétérogène automatiquement enregistrée ni de renderer de `Voxel::Volume` fourni.
- Le mesher et la géométrie voxel existent, mais la vue associant `Voxel::Geometry` et `spk::Palette` relève de **PR-13** et son branchement sur Model3D de **PR-14**.

## Critère de clôture

Le socle articulé générique Core/Graphics est livré. Le rendu voxel spécialisé est expressément hors périmètre. Les futures PR doivent réutiliser l'architecture actuelle sans ressusciter l'ancienne Assembly.
