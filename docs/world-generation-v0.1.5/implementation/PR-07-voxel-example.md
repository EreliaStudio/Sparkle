# PR07 — `spk::Voxel::Chunk` et génération headless testée

**Couche :** Voxel Core. **Dépendances :** PR01, PR02, PR03, PR06. **État :** implémenté sur la branche PR07.

## API du chunk

`include/voxel/chunk.hpp` fournit `spk::Voxel::Chunk<SX, SY, SZ>`, dérivé de `spk::World::Chunk<SX, SY, SZ>`.

- Le constructeur par défaut crée un `Volume` vide (air) aux dimensions fixes du chunk.
- `volume() const` donne accès au snapshot immuable du volume.
- `setVolume(Volume)` vérifie les dimensions, remplace le snapshot et notifie chaque remplacement valide, même si les cellules sont identiques. La première affectation utilise cette même API.
- La copie et le déplacement du chunk ne transfèrent pas les abonnements aux notifications ; l'affectation conserve les abonnements de la destination.
- Il n'existe pas de `Chunk::Builder`. Les voxels sont préparés par `Volume::Builder`, puis affectés au chunk.

```cpp
using Chunk = spk::Voxel::Chunk<8, 16, 32>;
Chunk chunk;

spk::Voxel::Volume::Builder builder({8, 16, 32});
builder.bind(definitions);
builder.set({7, 4, 5}, 2);
chunk.setVolume(builder.build());

spk::Voxel::Volume::Builder edit(chunk.volume().dimensions());
edit.load(chunk.volume());
edit.erase({7, 4, 5});
chunk.setVolume(edit.build());
```

## Cas pratique : un TU Core autonome

Le cas de génération procédurale se trouve **uniquement** dans `tests/TU/Core/srcs/world/voxel_chunk_test.cpp`. Il ne crée pas d'API d'exemple dans `include/`, de namespace `sparkle_world_generation_example`, ni d'exécutable de démonstration autonome.

Le TU définit localement :

- Deux définitions de voxels, pour le sol et le mur.
- `TerrainPass` : terrain déterministe dépendant des coordonnées mondiales et d'une seed.
- `WallPass` : mur entre X=-2 et X=10, Y=3 et Y=5, Z=5, qui traverse les frontières des chunks X=-1, 0 et 1.
- `WorldGenerator` : deux passes et zone finie de trois chunks ; hors zone, `generate()` renvoie `std::nullopt`.
- Un montage `EngineHeadless` + `WorldObserver` + `WorkerPool` + `LocalChunkRequester` + `WorldStreamingSystem` + `ChunkCollection` + `ChunkFilter`.

`spk::World::Generator<TChunk, TGenerationData, TBuilder>` accepte un builder distinct du chunk : par défaut `TChunk::Builder` pour les usages existants, et ici `Volume::Builder`. Après les passes, le générateur construit un `TChunk` par défaut et affecte le volume obtenu avec `setVolume()`. Les générateurs dont le builder retourne directement un `TChunk` conservent leur chemin précédent.

## Vérifications des TU

- Dimensions non cubiques 8×16×32, volume initial vide, lecture hors dimensions et rejet des volumes de dimensions incompatibles.
- Construction et modification par `Volume::Builder`, conservation du snapshot précédent, consommation du builder et absence d'un `Chunk::Builder`.
- Notifications à chaque `setVolume()` valide ; copies, déplacements, affectations et abonnements indépendants.
- Relais d'édition dans `ChunkCollection`, maintien de `ChunkFilter` lors du remplacement et absence de chunks hors de la zone.
- Déterminisme entre générations en ordre normal et inversé, coordonnées négatives et mur traversant plusieurs chunks.
- **Vérification exhaustive du type de chaque cellule** des trois chunks générés : air, terrain ou mur, selon la position mondiale.
- Chargement asynchrone des trois chunks par le streaming dans le moteur headless, puis validation des données reçues.

## Compilation et exécution

```bash
cmake -S . -B build/core-example -G Ninja \
  -DSPARKLE_BUILD_GRAPHICS=OFF -DSPARKLE_BUILD_TESTS=ON
cmake --build build/core-example --target SparkleCoreTestSuite
ctest --test-dir build/core-example -R 'VoxelChunkTest|VoxelChunkDemoTest' --output-on-failure
```

Les dépendances GoogleTest et les options de toolchain habituelles de Sparkle sont nécessaires.

## Exclusions

La PR07 n'introduit aucun rendu graphique, meshing inter-chunks, golden image, sauvegarde, réseau, biomes, villes, routes ou cavernes. Le rendu visuel et ses golden images restent prévus en PR08.
