# Sparkle V0.1.4 — Architecture cible du moteur voxel

> **Spécification d'architecture, préalable à l'implémentation.** Ce document décrit le comportement et les responsabilités attendus de la bibliothèque, indépendamment de l'ordre dans lequel les décisions ont été prises. Les exemples C++ représentent l'intention de l'API ; une signature illustrative ne constitue pas, à elle seule, une interface définitive. Aucune implémentation de ces nouveaux modules n'est présumée existante.
>
> **Objectif :** offrir au sein de Sparkle une infrastructure voxel publique, générique, utilisable pour des objets, personnages articulés et volumes de cellules, sans introduire de dépendance à un jeu, à un terrain, à Erelia ou à un catalogue global.

Ce fichier est le **sommaire normatif** des huit chapitres de Sparkle V0.1.4. Les chapitres sont répartis par domaine sans supprimer la géométrie voxel, les matériaux, les assets, les Assemblies ou les systèmes de rendu.

## Chapitres

| Chapitre | Spécification |
| --- | --- |
| 1 | [Vue d'ensemble](voxel-engine-v0.1.4/01-vue-ensemble.md) |
| 2 | [Modèle de données voxel](voxel-engine-v0.1.4/02-modele-voxel.md) |
| 3 | [Meshing, visibilité et occlusion](voxel-engine-v0.1.4/03-meshing-occlusion.md) |
| 4 | [Matériaux, textures et palettes](voxel-engine-v0.1.4/04-materiaux-textures-palettes.md) |
| 5 | [AssetManager générique](voxel-engine-v0.1.4/05-asset-manager.md) |
| 6 | [Assembly, composants et scène](voxel-engine-v0.1.4/06-assemblies-scene.md) |
| 7.1–7.2 | [RenderingEngine, EngineView et passes](voxel-engine-v0.1.4/07-vues-et-passes.md) |
| 7.3–7.4 | [Lumières et ombres](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) |
| 7.5–7.8 | [GPU, Forward et WBOIT](voxel-engine-v0.1.4/09-pipeline-gpu-transparence.md) |
| 8 | [Contrats et validation](voxel-engine-v0.1.4/10-contrats-et-validation.md) |

## Questions à résoudre avant l'implémentation

Le [document des questions encore ouvertes](voxel-engine-v0.1.4-points-a-clarifier.md) recense les arbitrages architecturaux **Q-ARCH** à valider avec le mainteneur et les précisions techniques **Q-TECH** à finaliser. Il n'annule aucune décision contenue dans les chapitres normatifs. Les points résolus seront intégrés dans les chapitres concernés puis retirés de ce suivi.

## Invariants de lecture

- **Éclairage :** Forward classique avec Top-N global Point/Spot par EngineView, N et hystérésis configurables ; toutes les Directional actives hors quota. `Light::Selection` partagée par Lighting/ShadowMap et par opaque/WBOIT ; aucune grille de culling de lumières par tile.
- **Moteur :** `RenderingEngine : Engine`, Engine logique headless, EngineView appartenant au moteur graphique, snapshots immuables, instances OpenGL distinctes sur chaque surface.
- **Ombres :** `ShadowMap::RenderingSystem` produit ; `Lighting::System` consomme via `Light::Channel` et `ShadowResults` par surface. `GPUResourceCollection` gère la durée de vie GPU.
- **Données :** Shape, Definition, Volume, Model, Material et VoxelPalette immuables ; Assemblies mutables et composants de rendu emprunteurs suivant les contrats.
- **Portée :** cette spécification ne constitue pas une autorisation d'implémenter. Aucun comportement public manquant n'est considéré comme validé sans décision du mainteneur.
