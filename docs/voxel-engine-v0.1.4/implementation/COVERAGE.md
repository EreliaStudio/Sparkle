# Couverture de la spécification par les 26 PR

| Contrat fonctionnel obligatoire | Référence normative | Fiche(s) |
|---|---|---|
| Shape, Definition, Cell, enums d'orientation, Air | [02](../02-modele-voxel.md) | PR01 |
| Huit `Shape::Instance` préconstruites, partage/immutabilité, winding/UV/normales | [02](../02-modele-voxel.md), [03](../03-meshing-occlusion.md) | PR01 |
| Volume dense et snapshots | [02](../02-modele-voxel.md) | PR02 |
| Mesher, assemblage des fragments, innerShell, voisins extérieurs | [03](../03-meshing-occlusion.md) | PR04 |
| Model : meshes opaques, translucides et d'ombre | [03](../03-meshing-occlusion.md) | PR04 |
| OcclusionCache facultatif et clipping directionnel partagé avec le mode sans cache | [03](../03-meshing-occlusion.md) | PR03, PR04 |
| AssetManager, AssetFactory, JSON structurel et cycles | [05](../05-asset-manager.md) | PR05 |
| Material multi-rôles, texture, SpriteSheet, JSON strict | [04](../04-materiaux-textures-palettes.md) | PR06 |
| VoxelPalette, compatibilité Model et identités d'albedo | [04](../04-materiaux-textures-palettes.md) | PR07 |
| Assembly, PartID, copie développée et reparentage | [06](../06-assemblies-scene.md) | PR08 |
| RenderingEngine : Engine, Contributors, Camera, RGB/RGBA | [07](../07-vues-et-passes.md) | PR09 |
| EngineView nom, order, layers, CaptureContext, sous-snapshots | [07](../07-vues-et-passes.md) | PR10 |
| Cinq passes/ordres réservés, capture immuable historique | [07](../07-vues-et-passes.md) | PR10 |
| Texture render-target, Framebuffer attachments partagés, resize | [07](../07-vues-et-passes.md) | PR11 |
| GPUResourceCollection, multi-surfaces, récupération GL et fermeture | [08](../08-lumieres-et-ombres.md) | PR11, PR21 |
| EngineWidget rôles, fond opaque, exposition, Reinhard et sRGB | [07](../07-vues-et-passes.md) | PR12 |
| ModelRenderingComponent, VoxelRenderingSystem, SSBO/buckets | [09](../09-pipeline-gpu-transparence.md) | PR13 |
| AssemblyRenderingComponent, instancing et poses | [06](../06-assemblies-scene.md) | PR14 |
| Material source EngineView, ordre/dépendances, sorties détachées | [07](../07-vues-et-passes.md) | PR15 |
| Skybox six Materials unlit, composant, layers | [07](../07-vues-et-passes.md) | PR16 |
| Point/Spot/Directional, Smoothstep, frustum, score, shadow params | [08](../08-lumieres-et-ombres.md) | PR17 |
| Light::Channel, Top-N/hystérésis, transactions CPU | [08](../08-lumieres-et-ombres.md) | PR18 |
| AmbientLight, Lighting::System et Forward opaque | [08](../08-lumieres-et-ombres.md) | PR12, PR19 |
| ShadowMap::Mesh, MeshInstance, Contributor et modes propres | [08](../08-lumieres-et-ombres.md) | PR04, PR20 |
| ShadowResult slots, admission/ordinal, fermeture/versions | [08](../08-lumieres-et-ombres.md) | PR21 |
| ShadowMap::RenderingSystem, Spot/Directional et caches | [08](../08-lumieres-et-ombres.md) | PR22 |
| Point cubemap depth et PCF 3x3 inter-faces | [08](../08-lumieres-et-ombres.md) | PR23 |
| Point sélection X/Y/epsilon/h, budget/surface et évictions | [08](../08-lumieres-et-ombres.md) | PR24 |
| WBOIT MRT RGBA16F/R8, depth partagé, transparence HDR | [09](../09-pipeline-gpu-transparence.md) | PR25 |
| Golden images, tests multi-surfaces, CI et packaging | [10](../10-contrats-et-validation.md) | Toutes ; PR26 intégration |

**Améliorations exclues :** éviction automatique des caches par orientation Shape, Tiled Forward+, gestionnaire GPU parallèle, raffinement progressif des ombres, LRU des assets, réordonnancement automatique des vues.

Les options techniques non arbitrées ne deviennent pas implicitement des contraintes nouvelles : l'IA fige localement la solution la plus simple permettant de satisfaire les comportements déjà validés et les tests du chapitre 10.
