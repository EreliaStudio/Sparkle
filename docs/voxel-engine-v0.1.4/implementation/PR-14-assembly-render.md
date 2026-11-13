# PR-14 — Retirée du plan v0.1.4 (intégration voxel/Model3D)

> **Décision du 12 novembre 2026, après fusion de PR13.** Cette fiche reste consultable à titre historique : **pas de livraison PR14 prévue pour v0.1.4**.

## Pourquoi retirer cette PR ?

[PR13 (#63)](https://github.com/EreliaStudio/Sparkle/pull/63) a déjà livré le chemin de production : `Voxel::Geometry`, `spk::Palette`, `Voxel::Renderer`, `Voxel::RenderingSystem`, `DrawVoxelCommand`, instancing, transformations d'`Entity3D`, EngineView et golden tests.

Les parties d'un personnage et son arme peuvent être composées à l'aide des transformations et de la parenté ECS. Déplacer ou tourner un renderer réutilise son mesh sans rebake. **Un second chemin de rendu n'est pas nécessaire.**

## Limites explicites

- `Model3D::Blueprint`, `ObjectHeadless` et `Object<TGraphics>` (PR08) restent des API génériques disponibles, indépendantes du renderer voxel.
- **PR13 ne prend pas directement un `Model3D::Blueprint` en entrée.** Il rend `Voxel::Geometry` avec palette par `Voxel::Renderer` ; ne pas confondre les deux API.
- Les golden tests PR13 construisent certaines **poses du personnage comme volumes/géométries distincts** et attachent l'épée avec des parents ECS. Il ne s'agit ni d'un animateur à keyframes ni du rendu direct de joints `Model3D`.
- Ne pas introduire `Voxel::ModelElement`, `ModelElementView`, `ModelRenderer`, `RenderBatch` ou une deuxième architecture de rendu dans les PR15–PR26.
- PR19 étend le rendu opaque pour les lumières ; PR20–24 ajoutent des shadows explicitement configurées ; PR25 apporte les draws voxel translucides dans le WBOIT/HDR existant ; PR26 valide le tout.

## Branche expérimentale existante

La [PR GitHub #64](https://github.com/EreliaStudio/Sparkle/pull/64), fermée **sans fusion**, implémente l'ancien périmètre PR14. Elle ne devient **pas un prérequis technique** de la roadmap et ne doit pas être fusionnée au titre de ce plan. La PR expérimentale #64 a été fermée ; son périmètre n'est pas à réintroduire.

**Point de départ des PR futures :** [PR13 livrée](PR-13-voxel-render-first.md), sur la branche `V0.1.4`.
