# Sparkle v0.1.4 — État consolidé des PR-01 à PR-08

> **Source de vérité : API intégrée dans `V0.1.4` après fusion de PR #49.** Les fiches historiques contiennent des décisions ensuite remplacées. PR-09..26 restent à développer.

## PR-01 — Shape, Definition, Cell

`Voxel::Shape` fournit les `Shape::Instance::triangles()` immuables des huit orientations. `Voxel::Definition` relie des slots géométriques à `Definition::Slot {name, opacity}`. `Voxel::Cell` porte la définition et l'orientation. Aucune texture ni palette n'entre en jeu.

## PR-02 — Registries et Volume

`Shape::Registry` utilise des IDs texte, `Definition::Registry` des IDs `uint32_t`, avec Air = 0. Les Builders reposent sur `JSON::Catalog`. `Volume::Builder(dimensions)` propose `load(volume)`, `bind(registry)`, `set(position,id,orientation...)`, `erase`, `loadPacked(path)` et `build()`. `Volume` est immuable, avec `at`, `operator[]`, `atRaw` et `Volume::PackedCell` 32 bits.

## PR-03 — Occlusion

`spk::Voxel::Occlusion::{Algorithm,Cache,Result}` remplacent les anciens noms `OcclusionCache` et `OcclusionResult`. `Cache::get(key)` calcule en cas de manque et `clear()` purge ; pas de `find()`/`store()` publics.

## PR-04 — Meshing

`spk::Voxel::Mesher::bake(volume)` retourne `spk::Voxel::Geometry` (Graphics). Elle expose `opaqueMesh`, `transparentMesh`, `depthMesh`, `materialSlots`, `materialSlot(index)` et `isCompatible(const spk::Palette&)`. Son `Vertex` porte `Vector3 position`, `Vector3 normal`, `Vector2 uv` et `uint32_t materialIndex`. Elle ne conserve **aucune palette** et n'offre aucun `bind`.

## PR-05 — Assets

Namespace public **`spk::Asset`**, classes `Manager` et `Factory<T>`. Le cache différencie type, ID logique et configuration JSON. `load<T>`, `get<T>` et `setDefault<T>` permettent les variantes et les défauts. L'ID ne devient pas implicitement le chemin disque.

## PR-06 — Matériaux

`Material::Builder::add(role, Texture, section)` ou `add(role, SpriteSheet, sprite)` construit des bindings de handles/UV, sans référence à Asset::Manager. `Asset::Factory<Material>` charge la configuration multi-rôles depuis JSON et résout les ressources via l'Asset Manager.

## PR-07 — Palettes

`spk::Palette` est une collection immuable générique. `spk::Voxel::Palette` est un **namespace** ; son `Builder` contrôle `albedo` et l'atlas commun lors de `setMaterial`. La compatibilité de slots s'effectue par `Geometry::isCompatible(palette)` ; aucun binding de palette dans Geometry. L'opacité reste propriété des Definitions.

## PR-08 — Model3D

`spk::Model3D::Blueprint` est une hiérarchie structurelle immuable d'éléments polymorphes et de points d'attachement. `ObjectHeadless` est une occurrence Core avec pose, transformations globales cachées, attache/détache de sous-objets et copies autonomes. `Object<TGraphics>` ajoute le cache de vues graphiques par ElementNode et `buildRenderSnapshot`. Le concept `ObjectGraphics` exige `ElementType`, un constructeur depuis l'élément et la méthode de contribution graphique.

Les golden tests OpenGL de PR-08 utilisent `ColorVolume`/`ColorView` **uniquement définis dans les tests**. Il n'existe pas encore de renderer voxel spécialisé ni de factory hétérogène automatique.

**Conceptions abandonnées :** `Voxel::Model` avec palette liée, `Voxel::Assembly` mutable, `PartID`, reparentage/expansion d'Assemblies, `Skeleton`/`Puppet` et `VersionedTrait` spécifique. Ne pas réintroduire ces API en se basant sur les anciennes fiches.

## Travaux futurs (non livrés dans PR-01..08)

- PR-09 à PR-12 : cible `RenderingEngine`/`EngineView`, ressources GPU et affichage des vues.
- **PR-13** : dessiner réellement une `Voxel::Geometry` avec `spk::Palette`, shaders et commandes GPU.
- **PR-14** : brancher le rendu voxel sur les éléments articulés de `Model3D`, sans rebake lié aux rotations.
- PR-15..26 : éclairage, ombres, transparence et intégration selon les fiches adaptées au code présent.
