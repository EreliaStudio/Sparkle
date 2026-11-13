# Couverture de la spécification par les 26 PR

**État :** PR01–PR13 et PR15–PR18 fusionnées ; PR14 retirée. Le renderer voxel PR13 reste indépendant de `Model3D`. PR19–PR26 sont les objectifs futurs.

| Contrat fonctionnel obligatoire | Référence normative | Fiche(s) |
|---|---|---|
| Shape, Definition, Cell, enums d'orientation, Air | [02](../02-modele-voxel.md) | PR01 |
| Huit `Shape::Instance` préconstruites, partage entre Definitions, immutabilité après échec du Builder, winding/UV/normales | [02](../02-modele-voxel.md), [03](../03-meshing-occlusion.md) | PR01 |
| Volume dense et snapshots | [02](../02-modele-voxel.md) | PR02 |
| Mesher, assemblage des fragments, innerShell, voisins extérieurs | [03](../03-meshing-occlusion.md) | PR04 |
| `Voxel::Geometry` : meshes opaques, translucides et depth, sans palette | [03](../03-meshing-occlusion.md) | PR04 |
| `Occlusion::Cache` facultatif et clipping directionnel partagé avec le mode sans cache | [03](../03-meshing-occlusion.md) | PR03, PR04 |
| `Asset::Manager`, `Asset::Factory<T>`, JSON structurel et cycles | [05](../05-asset-manager.md) | PR05 |
| Material multi-rôles, texture, SpriteSheet, JSON strict | [04](../04-materiaux-textures-palettes.md) | PR06 |
| `Voxel::Palette::Builder`, compatibilité `Geometry` et identités d'albedo | [04](../04-materiaux-textures-palettes.md) | PR07 |
| `Model3D::Blueprint`, `ObjectHeadless`, `Object<TGraphics>` et attachments | [06](../06-assemblies-scene.md) | PR08 |
| RenderingEngine : Engine, Contributors, Camera, RGB/RGBA | [07](../07-vues-et-passes.md) | PR09 |
| EngineView nom, order, layers, CaptureContext, sous-snapshots | [07](../07-vues-et-passes.md) | PR10 |
| Cinq passes/ordres réservés, capture immuable historique | [07](../07-vues-et-passes.md) | PR10 |
| Texture render-target, Framebuffer attachments partagés, resize | [07](../07-vues-et-passes.md) | PR11 |
| GPUResourceCollection, multi-surfaces, récupération GL et fermeture | [08](../08-lumieres-et-ombres.md) | PR11, PR21 |
| EngineWidget rôles, fond opaque, exposition, Reinhard et sRGB | [07](../07-vues-et-passes.md) | PR12 |
| `Voxel::Renderer` + `Voxel::RenderingSystem` : géométrie et palettes par occurrence, opaques, SSBO `MaterialRemap` et instancing compatible | [09](../09-pipeline-gpu-transparence.md) | PR13 (livrée) |
| Scènes voxel avec poses et pièces attachées via transformations/parenté `Entity3D` | [06](../06-assemblies-scene.md), [10](../10-contrats-et-validation.md) | PR13 (livrée) |
| Adaptation automatique de `Model3D::Blueprint` au renderer voxel | [06](../06-assemblies-scene.md) | **Hors périmètre v0.1.4 ; PR14 retirée** |
| Material source EngineView, ordre/dépendances, sorties détachées | [07](../07-vues-et-passes.md) | PR15 (livrée #66) |
| Skybox six Materials unlit, composant, layers, UBO `CameraData` au binding 1 | [07](../07-vues-et-passes.md) | PR16 (livrée #67) |
| Point/Spot/Directional, Smoothstep, frustum, score, shadow params | [08](../08-lumieres-et-ombres.md) | PR17 (livrée #68) |
| Light::Channel, Top-N/hystérésis et mutation CPU puis notification | [08](../08-lumieres-et-ombres.md) | PR18 (livrée #69) |
| AmbientLight, Lighting::System et Forward opaque | [08](../08-lumieres-et-ombres.md) | PR12, PR19 |
| ShadowMap::Mesh, MeshInstance, Contributor et modes propres | [08](../08-lumieres-et-ombres.md) | PR04, PR20 |
| ShadowResult slots, admission/ordinal, fermeture/versions | [08](../08-lumieres-et-ombres.md) | PR21 |
| ShadowMap::RenderingSystem, Spot/Directional et caches | [08](../08-lumieres-et-ombres.md) | PR22 |
| Point cubemap depth et PCF 3x3 inter-faces | [08](../08-lumieres-et-ombres.md) | PR23 |
| Point sélection X/Y/epsilon/h, budget/surface et évictions | [08](../08-lumieres-et-ombres.md) | PR24 |
| EngineView HDR/WBOIT générique déjà livré ; géométrie voxel translucide à intégrer sans réinventer la composition | [09](../09-pipeline-gpu-transparence.md) | PR12 (baseline), PR25 (extension voxel) |
| Golden images, tests multi-surfaces, CI et packaging | [10](../10-contrats-et-validation.md) | Toutes ; PR26 intégration |

**Améliorations exclues :** éviction automatique des caches par orientation Shape, Tiled Forward+, gestionnaire GPU parallèle, raffinement progressif des ombres, LRU des assets, réordonnancement automatique des vues.

Les options techniques non arbitrées ne deviennent pas implicitement des contraintes nouvelles : l'IA fige localement la solution la plus simple permettant de satisfaire les comportements déjà validés et les tests du chapitre 10.

**Régression graphique PR13 :** les goldens des formes, palais de matériaux, personnages/armes ECS, maisons, perspectives intérieures et tailles de rendu différentes restent obligatoires. Atlas en `nearest` ; jamais de `skip` silencieux ni de remplacement automatique des références.
