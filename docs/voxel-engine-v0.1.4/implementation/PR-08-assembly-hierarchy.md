# PR-08 — Voxel::View : hiérarchie multi-Models, palette et remappage

**Pull request GitHub :** [PR #49](https://github.com/EreliaStudio/Sparkle/pull/49), Draft, branche `impl/v0.1.4/08-assembly-hierarchy`, cible `V0.1.4`. Ne jamais merger sans validation du propriétaire.

## Décision finale

L'ancienne `Voxel::Assembly` et l'idée séparée `Model::View` fusionnent dans **`spk::Voxel::View`**. Une View peut contenir une seule Part avec un seul Model ou un arbre de Parts contenant chacune **plusieurs Models**. `VoxelMesher` est renommé `spk::Voxel::Mesher` ; il continue à produire uniquement un `Model` immuable. `Model::bind` et `Model::palette` sont retirés ; la liaison de palette et toutes les transformations appartiennent à `View`.

## API

```cpp
spk::Voxel::Model torso = mesher.bake(torsoVolume);
spk::Voxel::View standalone(torso, palette);

spk::Voxel::View composed;
auto root = composed.addPart("torso", {}, {torso, armor});
auto arm = composed.addPart("arm", root, {armModel, sleeveModel});
composed.bind(palette);
composed.addModel(arm, detailModel);
auto remap = composed.materialIndices(arm, 0);
composed.setLocalTransform(arm, armPose);
composed.invalidate();
```

- `Part {id, name, parent, localTransform, vector<Model> models}` est exposé seulement en lecture `const`.
- `addPart`, `addModel`, `setModel(id,index,model)`, `removeModel`, `setName`, `setLocalTransform`, `setParent`, `removePart` contrôlent l'arbre.
- `View::bind(spk::Palette)` valide le contrat voxel (albedo, atlas unique) et tous les slots requis. `View::palette() const` renvoie `nullptr` tant que la View est sans palette.
- L'ajout de Models sans palette est toujours possible **avant** liaison ; après liaison, tous les ajouts doivent être compatibles avec la palette déjà choisie. Aucune fusion de palettes implicite.
- `insertView(parent, source, transform, name)` réalise une copie développée de Parts, de leurs Models, noms et transformations sous une Part d'ancrage, avec IDs nouveaux/remappés. La palette de la source n'est pas importée. L'opération est atomique y compris sur palette incompatible et ID épuisé.
- `materialIndices(partID, modelIndex)` mappe les indices de slots locaux du Model vers l'index lexicographique de Materials dans l'unique palette View ; le futur rendu transporte ce remapping jusqu'au shader, sans modifier les vertices.
- Les copies de View ont un `VersionedTrait` indépendant version 1 ; les mutations n'invalident pas automatiquement.

## Tests

Tests GoogleTest `tests/TU/Graphics/srcs/voxel/voxel_view_test.cpp` : constructeurs simple/composé, Parts multi-Models, animation, palette commune facultative, incompatibilités et rollback, remapping inter-Models, insertions répétées et sources indépendantes, versions, copies, contrôle de cycles, épuisement IDs. Tests de palette et mesher adaptés à `Model` sans palette et au nouveau nom `Mesher`.

## Hors périmètre

Rendu GPU et shaders (PR-13/PR-14) ; composant graphique emprunteur `ViewRenderingComponent`, contributions aux ombres et allocations GPU. Les meshes restent le résultat immuable du Mesher.

## Vérification de livraison

La CI GitHub fait foi pour la compilation et les tests sur l'infrastructure prise en charge. Les workflows locaux `cmake --workflow --preset testDebug/testRelease` ne sont pas exécutables depuis le conteneur de cette session. Ne pas fusionner la PR avant vérification des résultats.
