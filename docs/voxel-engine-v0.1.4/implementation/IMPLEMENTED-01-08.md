# Sparkle v0.1.4 — État consolidé des PR-01 à PR-08

> **Historique PR01–PR08 :** ces API sont intégrées. Depuis, PR09–PR13 ont également été fusionnées dans `V0.1.4`, PR14 a été retirée et PR15–PR26 restent prévues. Pour le renderer voxel, voir [PR13](PR-13-voxel-render-first.md).

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

Les golden tests OpenGL de PR08 utilisent `ColorVolume`/`ColorView` **uniquement définis dans les tests**. Le renderer voxel a depuis été livré en PR13 (`Voxel::Renderer`) ; la factory hétérogène et le lien direct à `Model3D::Blueprint` ne font pas partie de ce rendu.

**Conceptions abandonnées :** `Voxel::Model` avec palette liée, `Voxel::Assembly` mutable, `PartID`, reparentage/expansion d'Assemblies, `Skeleton`/`Puppet` et `VersionedTrait` spécifique. Ne pas réintroduire ces API en se basant sur les anciennes fiches.

## État depuis PR08

- **PR09–12 (livrées)** : `Engine`/`EngineView`, GPU, surfaces et HDR/WBOIT générique.
- **PR13 (livrée)** : `Voxel::Renderer`, `Voxel::RenderingSystem`, rendu opaque texturé/instancié, palettes et golden images.
- **PR14 (retirée)** : aucune conversion automatique `Model3D::Blueprint` → renderer voxel.
- **PR15–26 (prévues)** : sources de Materials, Skybox, éclairage, ombres, voxel translucide et intégration, en réutilisant le pipeline existant.
