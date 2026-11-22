# PR08 — Chunk meshing and Graphics synchronization

**Layer:** Graphics only. **Dependencies:** PR00, PR02, PR07. **Status:** implementation in PR08.

## Public API

`spk::Voxel::ChunkMesher<SX, SY, SZ>` in `include/voxel/chunk_mesher.hpp` derives from the existing `Voxel::Mesher`. Construct it with a const reference to `World::ChunkCollection<Voxel::Chunk<SX,SY,SZ>>` and a chunk coordinate. It uses `outsideCell()` to look up boundary cells in the six adjacent chunks during each `bake()`; missing neighbors are treated as empty. The existing occlusion algorithm remains unchanged and handles opaque, translucent, and partially occluding shapes. No neighbor pointer is retained.

`spk::Voxel::ChunkRenderingSystem<SX, SY, SZ>` in `include/voxel/chunk_rendering_system.hpp` specializes the existing `Voxel::RenderingSystem`. Create it on a Graphics `spk::Engine`, attach a Core `World::ChunkCollection` to that same engine, then bind a palette explicitly:

```cpp
using Chunk = spk::Voxel::Chunk<16, 16, 16>;
spk::World::ChunkCollection<Chunk> chunks;
chunks.attach(engine);
auto &renderer = engine.addSystem<spk::Voxel::ChunkRenderingSystem<16, 16, 16>>();
renderer.bind(chunks, materials);
```

When the collection is bound, existing chunks are marked dirty. New chunks, changes through `setVolume`, and whole-chunk replacements mark that chunk and six neighbors dirty. Entity destruction marks its former neighbors dirty without accessing the entity during destruction.

The dirty set is processed before the existing voxel rendering system captures each frame. Multiple edits before one capture are coalesced: a chunk is rebuilt at most once. The system uses the collection-owned `Entity3D` and its existing `Voxel::Renderer` if present; otherwise it adds a renderer component. Its explicit `unbind()` detaches subscriptions and removes renderer components it created. The rendering system does not own chunks or replace the collection's headless entity tree.

Rendering requires a Graphics Engine. A Core-only headless world creates no renderer and does not require OpenGL.

## Measurements

`ChunkRenderingSystem::lastStatistics()` reports the last nonempty synchronization:

- Rebuilt chunks and processed voxels.
- Opaque and translucent vertex counts.
- Opaque and translucent triangle counts.
- CPU meshing time (`std::chrono::nanoseconds`), normalized as `millisecondsPerChunk()` and `nanosecondsPerVoxel()`.

A Graphics GoogleTest benchmark tests solid and sparse 8³, 16³ and 32³ workloads over five repetitions, printing vertex/triangle counts and measured timings. Results are written to `tests/TU/Graphics/results/metrics/chunk_meshing.csv` on the test runner and included in the `graphics-unit-tests-*` GitHub Actions artifacts.

Timing is for CPU `Mesher::bake()` (including geometry construction), not GPU rendering, uploads, world generation, streaming, or scheduling. There is no platform-dependent pass/fail timing threshold; timings vary by hardware, compiler, cache state, and workload.

## Tests and golden image evidence

`tests/TU/Graphics/srcs/voxel/chunk_rendering_golden_test.cpp` covers:

- Every axis and direction, including negative coordinates: two touching opaque cubes remove the internal faces (36 → 30 vertices per cube), and those faces return when a neighbor disappears.
- Translucent cells and partially occluding slab/slope boundaries.
- Multiple edits coalesced to one rebuild per chunk, with neighboring chunk invalidation and renderer cleanup.
- Three adjacent chunks, shown from an angled camera and rendered using the real Engine graphics pipeline.
- **Image comparison:** the chunked image is compared pixel-by-pixel with an equivalent, independently meshed single-volume scene in the same camera and lighting configuration. Both screenshots and any difference image are saved to `results/rendering/voxel-chunks` and uploaded to the visual comparison artifacts.

This image comparison uses a runtime-generated single-volume reference, not a checked-in static PNG baseline. Existing voxel-scene golden tests continue to validate the rendering pipeline against committed PNG goldens.

## Exclusions

No changes to the existing `Voxel::Mesher` or occlusion algorithm, persistence, world streaming policies, gameplay entity ownership, automatic material selection, networking, or Core-to-Graphics dependencies.
