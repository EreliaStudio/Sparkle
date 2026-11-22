# PR07 — `spk::Voxel::Chunk` et démonstrateur headless

**Couche :** Voxel Core. **Dépendances :** PR01, PR02, PR03, PR06. **État :** implémenté sur la branche PR07.

## Type public

`include/voxel/chunk.hpp` fournit `spk::Voxel::Chunk<SX, SY, SZ>` héritant de `spk::Chunk<SX, SY, SZ>`.

- Chaque chunk possède exactement un `Voxel::Volume` aux dimensions fixes `SX × SY × SZ`. Le constructeur et `setVolume(Volume)` rejettent les dimensions incompatibles.
- `volume() const` permet de lire le volume sans exposer sa mutation interne ; `setVolume(Volume)` le remplace et notifie chaque remplacement valide, même lorsque le contenu est identique. Aucune comparaison de cellules n'est effectuée.
- Le volume lui-même reste un snapshot immuable et partagé, produit par `Volume::Builder`. Un chunk peut recevoir successivement plusieurs snapshots.
- Les copies et déplacements d'un chunk ne transfèrent pas les souscriptions d'édition ; les affectations conservent celles de la destination, conformément à `spk::Chunk`.
- Il n'existe **pas** de `Chunk::Builder` : seul `Volume::Builder` est nécessaire pour construire ou reconstruire le contenu voxel. Les futures métadonnées du chunk sont indépendantes de ce volume.

Exemple de création et de remplacement :

```cpp
using Chunk = spk::Voxel::Chunk<8, 16, 32>;
spk::Voxel::Volume::Builder builder({8, 16, 32});
builder.bind(definitions);
builder.set({7, 4, 5}, 2);
Chunk chunk(builder.build());

spk::Voxel::Volume::Builder edit(chunk.volume().dimensions());
edit.load(chunk.volume());
edit.erase({7, 4, 5});
chunk.setVolume(edit.build());
```

## Exemple de génération

`examples/world_generation/voxel_world.hpp` définit uniquement des types propres au démonstrateur :

- `GenerationData` : références aux définitions de cellules et seed du terrain.
- `TerrainPass` : hauteur déterministe en coordonnées mondiales, valable pour les X négatifs.
- `WallPass` : mur continu de X=-2 à X=10, Y=3 à Y=5, Z=5. Il traverse les frontières des chunks X=-1/0/1.
- `WorldGenerator` : enregistre les deux passes par `addPass<TerrainPass>()` puis `addPass<WallPass>()` ; renvoie `std::nullopt` hors de la zone de trois chunks.

`spk::Generator<TChunk, TGenerationData, TBuilder>` accepte un type de builder indépendant du chunk. Son paramètre `TBuilder` conserve `TChunk::Builder` par défaut pour compatibilité avec d'autres types de chunk. Ici, les passes reçoivent directement un `Voxel::Volume::Builder` ; après les passes, le générateur construit `Chunk` depuis le `Volume` résultant.

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

`tests/TU/Core/srcs/world/voxel_chunk_demo_test.cpp` couvre les dimensions non cubiques 8×16×32, les mauvaises dimensions, le remplacement systématiquement notifié du volume, les snapshots originaux inchangés après édition via `Volume::Builder`, le chargement et la consommation du `Volume::Builder`, l'absence de `Chunk::Builder`, la copie et le déplacement avec souscriptions isolées, le relais de `ChunkCollection`, les filtres après remplacement, `nullopt` hors zone, l'ordre inversé de génération, les coordonnées mondiales négatives, les frontières du mur et la publication asynchrone sur le thread du moteur.

## Exclusions

Aucun mesher ou voisinage graphique (PR08), OpenGL, système de biomes, villes, routes ou cavernes, sauvegarde, collision, networking ou changement du streaming Core au-delà de l'adaptation du template `Generator` n'est introduit.
