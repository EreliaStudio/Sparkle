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

## État consolidé de l'éclairage (Q-TECH-002)

Les arbitrages suivants sont **validés** et décrits de façon normative dans le [chapitre Lumières et ombres](voxel-engine-v0.1.4/08-lumieres-et-ombres.md) et ses [tests](voxel-engine-v0.1.4/10-contrats-et-validation.md) :

- **API lumineuse :** Point, Spot et Directional ont une couleur `spk::RGB` linéaire à trois canaux bornés `[0,1]` et une intensité scalaire finie `>=0` ; Point/Spot ont une portée `range` directement définie en `U`, Spot possède un seul angle, Directional aucune portée. L'ancienne API `spk::Color` (RGBA) conserve son alias lors du renommage `spk::RGBA`.
- **Shading :** Point et Spot utilisent `A(x)=1−3x²+2x³` (`x=d/range` borné à `[0,1]`), extinction à `range`, sans inverse du carré ni `d_floor` ; Spot multiplie par un Smoothstep angulaire en cosinus. Directional utilise `RGB×intensity` sans atténuation radiale.
- **Sélection :** score Point/Spot `intensity×range²×priorityWeight/max(distanceCamera,minLightDistance)`, calculé après culling ; sélection Forward classique, sans Tiled Forward+.
- **Ombres :** Point/Spot `near=0,001×range`, `far=range` ; Point six faces à 90°, PCF manuel 3×3 inter-faces (±1 texel sur face centrale, reprojection individuelle), règle d'égalité X > Y > Z, biais automatique par tap fondé sur la projection historique ; centre exact non ombré, tap avant near non ombré. Cartes historiques géométriquement cohérentes ; RGB/intensity courants.
- **Directional :** une seule shadow map orthographique par lumière et par vue ; couverture automatique à partir du frustum de caméra, **plafonnée par `shadowDistance` configurable sur la lumière**, en unités `U` (far infini accepté grâce au plafond fini). Ce paramètre limite uniquement les ombres, jamais l'éclairage ; aucune saisie manuelle des bornes, aucun CSM. La valeur initiale de `shadowDistance` et les détails des bornes de profondeur et casters hors frustum restent à préciser.
- **Statut :** les précisions non validées (certaines résolutions/formats, limites numériques, ombres directionnelles et conventions exactes de plans) restent dans [Q-TECH-002](voxel-engine-v0.1.4-points-a-clarifier.md). La conception n'autorise pas encore l'implémentation.

## Questions à résoudre avant l'implémentation

Les **sept arbitrages architecturaux Q-ARCH-001 à Q-ARCH-007 sont clos**, ainsi que **Q-TECH-001** (Top-N et transaction synchronisée des `Light::Channel`) et leurs décisions figurent désormais dans les chapitres normatifs. Le [document des précisions encore ouvertes](voxel-engine-v0.1.4-points-a-clarifier.md) conserve **six Q-TECH** à consolider avant l'implémentation, sans annuler ni rouvrir les contrats validés. Toute précision susceptible de changer un comportement public doit être soumise explicitement au mainteneur avant rédaction de ce nouveau choix.

## Invariants de lecture

- **Éclairage :** Forward classique avec Top-N global Point/Spot par EngineView, N et hystérésis configurables ; toutes les Directional actives hors quota. `Light::Selection` partagée par Lighting/ShadowMap et par opaque/WBOIT ; aucune grille de culling de lumières par tile.
- **Moteur :** `RenderingEngine : Engine`, Engine logique headless, EngineView appartenant au moteur graphique, snapshots immuables, instances OpenGL distinctes sur chaque surface.
- **Ombres :** `ShadowMap::RenderingSystem` produit ; `Lighting::System` consomme via `Light::Channel` et `ShadowResults` par surface. `GPUResourceCollection` gère la durée de vie GPU.
- **Données :** Shape, Definition, Volume, Model, Material et VoxelPalette immuables ; Assemblies mutables et composants de rendu emprunteurs suivant les contrats.
- **Portée :** cette spécification ne constitue pas une autorisation d'implémenter. Aucun comportement public manquant n'est considéré comme validé sans décision du mainteneur.
