# PR-14 — Intégration du rendu voxel dans Model3D (à réaliser)

> **Branche prévue :** `impl/v0.1.4/14-assembly-render` depuis la tête courante de `V0.1.4`, PR vers `V0.1.4`. Le nom de branche peut rester historique ; l'API ne doit pas l'être.

**Objectif :** réutiliser le rendu voxel de PR-13 pour les éléments articulés de `Model3D::Object`, sans remailler lorsqu'un AttachmentPoint tourne.

**Prérequis :** [PR-08 — Model3D](PR-08-assembly-hierarchy.md), [PR-13 — rendu voxel](PR-13-voxel-render-first.md).

## Spécifications

- [Architecture actuelle Model3D](../06-assemblies-scene.md)
- [GPU, buckets et instancing](../09-pipeline-gpu-transparence.md)
- [État PR-01 à PR-08](IMPLEMENTED-01-08.md)
- [Contrats de validation](../10-contrats-et-validation.md)

## Périmètre attendu

- [ ] Définir un élément `Model3D::Blueprint::Element` portant ou référençant un `Voxel::Volume`, ou adapter un type utilisateur ; **`Voxel::Volume` n'hérite pas automatiquement de `Element`**.
- [ ] Définir la vue graphique de cet élément respectant `Model3D::ObjectGraphics` ou une extension explicitement validée, et réutiliser les commandes voxel PR-13.
- [ ] Capturer les transformations des `ElementNode` et sous-objets attachés avec la matrice commune fournie à `Object<TGraphics>::buildRenderSnapshot(builder, transform)`.
- [ ] Mettre en cache les `Voxel::Geometry` indépendamment des poses ; nouvelle rotation = nouvelle matrice d'instance uniquement.
- [ ] Configurer les palettes au niveau des occurrences graphiques, vérifier `Geometry::isCompatible`, et séparer les buckets lorsque nécessaire.
- [ ] Conserver des snapshots graphiques autonomes, sans lire la pose mutable durant `RenderCommand::execute()`.

## Tests obligatoires

- [ ] Golden 3D articulé avec **de vrais `Voxel::Volume`** (torse, bras, jambes, et objet attaché) ; comparer plusieurs poses.
- [ ] Rotations parent/enfant non commutatives, et vérification que le même mesh est partagé entre deux occurrences.
- [ ] Après rotation, aucun nouveau bake ; après remplacement de la géométrie, nouveau bake/capture nécessaires.
- [ ] Deux palettes compatibles distinctes sur une même géométrie ; buckets séparés si atlas/mapping incompatibles.
- [ ] Détachement/réattachement d'un sous-objet sans pixels fantômes ; anciens snapshots inchangés.

## Hors périmètre

Anciennes API `Voxel::Assembly`, `PartID`, reparentage par expansion, `AssemblyRenderingComponent` imposé, ombres automatiques, système d'animation temporelle.

**Définition de terminé :** une hiérarchie `Model3D` rend des voxels texturés articulés avec instancing et cache de géométrie.
