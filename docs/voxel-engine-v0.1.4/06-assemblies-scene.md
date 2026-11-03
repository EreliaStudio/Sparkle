# Sparkle v0.1.4 — Modèles articulés et composition (en redéfinition)

Ce chapitre est en attente de la **PR-08.2**. Les anciennes descriptions de `Voxel::Assembly`, `Skeleton`, `Model`, `Section` et `Model::View` ne sont pas des décisions architecturales validées et ne doivent pas guider l'implémentation.

La **PR-08.1** conserve uniquement les fondations indépendantes : `Camera3D` dans Core, chargement transactionnel de `JSON::Catalog`, ainsi que `Voxel::Geometry` et `Voxel::Mesher` dans Graphics. `Voxel::Volume` reste une donnée Core utilisable sans GPU.

La définition des hiérarchies, transformations articulées, ressources graphiques d'occurrences et assemblages sera faite séparément en PR-08.2.
