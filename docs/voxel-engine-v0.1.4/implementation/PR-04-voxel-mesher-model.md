# PR-04 — VoxelMesher, clipping et Model

> **Fiche de préparation pour une future PR GitHub, pas une PR déjà créée.** Ne développer que cette fiche.
>
> **Branche source :** `impl/v0.1.4/04-voxel-mesher-model`, créée depuis la tête courante de `V0.1.4` après fusion de ses prérequis. **Branche cible : `V0.1.4`.** Ne jamais cibler `main`.

**Objectif :** Produire un Model voxel immuable à partir d'un Volume via les services de transformation et d'occlusion déjà disponibles.

**Prérequis :** [PR-02 — Volume](PR-02-voxel-volume.md) et [PR-03 — ShapeTransformationCache et OcclusionCache](PR-03-shape-occlusion-caches.md).

## Spécifications normatives

- [Modèle voxel](../02-modele-voxel.md)
- [Meshing, visibilité et occlusion](../03-meshing-occlusion.md)
- [Ombres](../08-lumieres-et-ombres.md)
- [Contrats et validation](../10-contrats-et-validation.md)
- [Spécification principale](../../voxel-engine-v0.1.4-specification.md)

## Périmètre exact à réaliser

- [ ] `VoxelMesher` : bake synchrone complet d'un Volume, requérant **obligatoirement** un `OcclusionCache` externe explicitement lié.
- [ ] Aucune voie de bake sans cache d'occlusion, aucun cache interne fabriqué automatiquement, aucun algorithme de clipping ou de transformation dupliqué.
- [ ] Voisinage extérieur `Empty`, `Occupied`, `Unknown`; visibilité directionnelle et règle globale des `innerShell`.
- [ ] Tolérance de clipping configurable `1e-5` par défaut et incluse dans les clés.
- [ ] `Model` immuable : meshes visuels opaques/translucides, mesh de profondeur `ShadowMap::Mesh` et slots sémantiques triés lexicographiquement.
- [ ] Utilisation de `spk::Mesh<TVertex>` existant, UV et normales conservés/interpolés, indices et winding corrects.
- [ ] Opacité du slot géométrique, non alpha de texture, utilisée pour culling, classification et ombres.

## Contrats de dépendance explicites

`VoxelMesher` ne peut lancer `bake()` sans `OcclusionCache` valide lié : lever `spk::Exception`. De même, un `OcclusionCache` non lié à un `ShapeTransformationCache` doit refuser son utilisation avec `spk::Exception`. Les deux bindings sont non propriétaires et leur durée de vie relève de l'appelant. `unbind()`, s'il est conservé, n'autorise jamais une exécution silencieuse sans cache.

## Tests obligatoires

- [ ] `bake()` sans cache lève `spk::Exception` ; lier `nullptr` est rejeté.
- [ ] `bake()` avec OcclusionCache non lié à ShapeTransformationCache lève `spk::Exception`.
- [ ] Meshing identique entre cache initialement vide et déjà renseigné (pas de mode sans cache).
- [ ] Clipping partiel, union des occultants et respect des faces avant, opacités, UV et normales.
- [ ] Six voisins, trois états extérieurs, règle globale des `innerShell`.
- [ ] Opacité 1 / inférieure à 1, absence d'inférence depuis les textures, mesh d'ombre purement géométrique.
- [ ] Slots masqués conservés, collision de nom/opacité rejetée et ordre lexicographique déterministe.
- [ ] Deux Meshers peuvent partager un cache ; anciens Models restent immuables.
- [ ] Tests Core Debug/Release sous Windows et Linux ; aucune dépendance Engine/OpenGL.

## Critères de livraison

Premier Model voxel vérifiable numériquement sans contexte GL. Déclarer les validations réellement exécutées, corriger les violations de contrat, obtenir la validation du propriétaire avant fusion.

**Hors périmètre :** rendu GPU, greedy meshing, rebake incrémental, framework de cache supplémentaire et toute PR ultérieure.
