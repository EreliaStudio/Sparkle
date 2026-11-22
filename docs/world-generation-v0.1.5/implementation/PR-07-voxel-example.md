# PR07 — `spk::Voxel::Chunk` et démonstrateur headless

**Couche :** Voxel Core. **Dépendances :** PR01, PR02, PR03, PR06. **État :** implémenté sur la branche PR07.

## Type public

`include/voxel/chunk.hpp` fournit `spk::Voxel::Chunk<SX, SY, SZ>` héritant de `spk::Chunk<SX, SY, SZ>`.

- Chaque chunk possède exactement un `Voxel::Volume` aux dimensions fixes `SX × SY × SZ`. Le constructeur et `setVolume(Volume)` rejettent les dimensions incompatibles.
- `volume() const` expose le snapshot immuable. `setVolume(Volume)` valide les dimensions, remplace systématiquement le volume et notifie chaque remplacement valide, même lorsque le contenu est identique. Aucune comparaison de voxels n'est effectuée.
- Le stockage immuable partagé du `Volume` existant n'est pas modifié. La copie / construction par déplacement du chunk n'importe aucune souscription ; l'affectation conserve les souscriptions de destination, conformément à `spk::Chunk`.
- Les mutations se font sur un nouveau `Volume` construit par le Builder. Ni `Chunk` ni `Volume` ne sont dotés de nouvelles couches de stockage.

Le `Chunk::Builder` construit son `Volume::Builder` aux dimensions exactes du chunk. `volumeBuilder()` donne accès directement aux opérations existantes `bind`, `set`, `erase` et aux autres opérations du Volume. `load(const Chunk&)` charge un snapshot existant, puis `std::move(builder).build()` retourne un chunk par valeur et consomme le Builder.

```cpp
using Chunk = spk::Voxel::Chunk<8, 16, 32>;
Chunk::Builder builder;
builder.volumeBuilder().bind(definitions);
builder.volumeBuilder().set({7, 4, 5}, 2);
Chunk chunk = std::move(builder).build();

Chunk::Builder edit;
edit.load(chunk);
edit.volumeBuilder().erase({7, 4, 5});
chunk.setVolume(std::move(edit).build().volume());
```

## Exemple de génération

`examples/world_generation/voxel_world.hpp` définit uniquement des types propres au démonstrateur :

- `GenerationData` : références aux définitions de cellules et seed du terrain.
- `TerrainPass` : hauteur déterministe en coordonnées mondiales, valable pour les X négatifs.
- `WallPass` : mur continu de X=-2 à X=10, Y=3 à Y=5, Z=5. Il traverse les frontières des chunks X=-1/0/1.
- `WorldGenerator` : enregistre les deux passes par `addPass<TerrainPass>()` puis `addPass<WallPass>()` ; renvoie `std::nullopt` hors de la zone de trois chunks.

Les passes reçoivent le même `GenerationData` par requête. Le mur utilise `worldToChunkCoordinate` et `worldToLocalCoordinate` du chunk pour décider et placer chaque cellule ; l'ordre des requêtes ne change donc jamais le résultat.

`examples/world_generation/headless_voxel_world.cpp` est un exécutable sans Graphics montrant la chaîne :

```
WorldGenerator
  → LocalChunkRequester<Chunk, GenerationData>
  → WorkerPool
  → WorldStreamingSystem<Chunk>
  → ChunkCollection<Chunk>
  → Entity3D / ChunkFilter<Chunk>
```

L'observateur surveille les chunks X=-1/0/1. Le streamer ne publie les réponses des workers que pendant `EngineHeadless::updateState()`. L'exécutable vérifie que le mur est présent aux frontières.

## Compilation et exécution

Le démonstrateur est construit avec les tests Core et ne nécessite aucune API réseau, OpenGL ou service externe.

```bash
cmake -S . -B build/core-example -G Ninja \
  -DSPARKLE_BUILD_GRAPHICS=OFF -DSPARKLE_BUILD_TESTS=ON
cmake --build build/core-example --target SparkleVoxelWorldDemo SparkleCoreTestSuite
ctest --test-dir build/core-example -R 'VoxelChunkTest|VoxelChunkDemoTest|VoxelWorldDemo' --output-on-failure
```

Les dépendances GoogleTest et les options de toolchain habituelles de Sparkle restent nécessaires pour le configure.

## Couverture automatisée

`tests/TU/Core/srcs/world/voxel_chunk_demo_test.cpp` couvre les dimensions non cubiques 8×16×32, les mauvaises dimensions, les snapshots inchangés après édition, `Builder::load`, la consommation du Builder, la copie et le déplacement avec souscriptions isolées, le relais de `ChunkCollection`, les filtres après remplacement, `nullopt` hors zone, l'ordre inversé de génération, les coordonnées mondiales négatives, les frontières du mur et la publication asynchrone sur le thread du moteur.

## Exclusions

Aucun mesher ou voisinage graphique (PR08), OpenGL, système de biomes, villes, routes ou cavernes, sauvegarde, collision, networking ou changement du streaming Core n'est introduit.
