# PR-07 — Palette et compatibilité Geometry (livrée)

> **Fiche historique, implémentation fusionnée.** Voir [l'état consolidé](IMPLEMENTED-01-08.md) et [le chapitre 04](../04-materiaux-textures-palettes.md).

## API livrée

- `spk::Palette` est une collection immuable et générique de `spk::Material` nommés.
- `spk::Palette::Builder` possède un validateur optionnel appelé à chaque `setMaterial(name, material)`.
- `spk::Voxel::Palette` est un **namespace** dont `Builder` dérive de `spk::Palette::Builder` ; l'ajout impose un rôle `albedo` et la même source d'atlas pour tous les Materials.
- `spk::Voxel::Geometry::isCompatible(const spk::Palette&)` vérifie la présence de tous les slots sémantiques, y compris ceux dont les triangles sont occultés. Les slots supplémentaires sont acceptés.
- L'opacité appartient à `Voxel::Definition::Slot` et à la classification géométrique, jamais à une palette.

## Différences avec la conception d'origine

`Voxel::Model` n'existe plus. La méthode `Model::bind(palette)` ainsi que `Model::palette()` ont été supprimées lors de la séparation Core/Graphics. `Geometry` reste indépendante de l'apparence ; l'association d'une palette appartient à une vue/occurrence graphique, à implémenter avec PR-13/PR-14. Aucun renderer voxel n'est livré en PR-07.

## Validation historique

Tests de validation `albedo`/atlas au `setMaterial`, d'incomplétude des slots et de compatibilité `Geometry` intégrés. Les résultats sont ceux des PR fusionnées ; aucune campagne de tests nouvelle n'est revendiquée dans cette fiche.
