# PR08 — Chunk meshing and Graphics synchronization

**Layer:** Graphics only. **Dependencies:** PR00, PR02, PR07. **Status:** implementation in PR08.

## Public API

`spk::World::Voxel::Chunk<SX, SY, SZ>::Mesher` in `include/world/voxel/chunk_mesher.hpp` derives from the existing `Voxel::Mesher`. Construct it with a const reference to `World::ChunkCollection<World::Voxel::Chunk<SX,SY,SZ>>` and a chunk coordinate. It uses `outsideCell()` to look up boundary cells in the six adjacent chunks during each `bake()`; missing neighbors are treated as empty. The existing occlusion algorithm remains unchanged and handles opaque, translucent, and partially occluding shapes. No neighbor pointer is retained.

`spk::World::Voxel::Chunk<SX, SY, SZ>::RenderingSystem` in `include/world/voxel/chunk_rendering_system.hpp` specializes the existing `Voxel::RenderingSystem`. Create it on a Graphics `spk::Engine`, attach a Core `World::ChunkCollection` to that same engine, then bind a palette explicitly:

```cpp
using Chunk = spk::World::Voxel::Chunk<16, 16, 16>;
spk::World::ChunkCollection<Chunk> chunks;
chunks.attach(engine);
auto &renderer = engine.addSystem<spk::World::Voxel::Chunk<16, 16, 16>::RenderingSystem>();
renderer.bind(chunks, materials);
```

When the collection is bound, existing chunks are marked dirty. New chunks, changes through `setVolume`, and whole-chunk replacements mark that chunk and six neighbors dirty. Entity destruction marks its former neighbors dirty without accessing the entity during destruction.

The dirty set is processed before the existing voxel rendering system captures each frame. Multiple edits before one capture are coalesced: a chunk is rebuilt at most once. The system uses the collection-owned `Entity3D` and its existing `Voxel::Renderer` if present; otherwise it adds a renderer component. Its explicit `unbind()` detaches subscriptions and removes renderer components it created. The rendering system does not own chunks or replace the collection's headless entity tree.

Rendering requires a Graphics Engine. A Core-only headless world creates no renderer and does not require OpenGL.

## Measurements

`Chunk::RenderingSystem::lastStatistics()` reports the last nonempty synchronization:

- Rebuilt chunks and processed voxels.
- Opaque and translucent vertex counts.
- Opaque and translucent triangle counts.
- CPU meshing time (`std::chrono::nanoseconds`), normalized as `millisecondsPerChunk()` and `nanosecondsPerVoxel()`.

A **dedicated Graphics TU** — `tests/TU/Graphics/srcs/voxel/chunk_mesher_performance_test.cpp` — covers deterministic, realistic workloads: solid chunks at 8³/16³/32³; a hollow building with rooms and doorways; terraced terrain with caves; bridges and towers; a 3D checkerboard; non-cubic terrain (8×16×24); four neighboring cave chunks spanning negative coordinates; and an isolated mix of cube, slab, slope and stair shapes.

Every test **asserts vertex and triangle counts**, rather than just printing a number. For complex opaque-cube volumes, an **independent face-counting oracle** examines the occupied cells and computes how many quad faces must remain visible (six vertices and two triangles per face). For mixed geometric shapes, the expected vertex count is independently derived from the source shape polygons, with shapes isolated by air to make the expectation exact. The joined-world test counts faces across *all* chunks globally, detecting unremoved internal chunk boundaries. These are correctness checks; timing results are informational.

Each test warms the occlusion cache twice, then measures **seven meshing passes**, printing the median and the measured range in the **GoogleTest output**, together with chunks, voxels, occupied voxels, generated vertices and triangles, **ms/chunk** and **ns/voxel**. There is **no CSV file**. On Release CI, a dedicated `ctest --verbose` step prints these results directly in the Actions log for easy review. The full Graphics test suite also runs the TU in Debug and Release.

Timing covers CPU `Mesher::bake()` (including geometry construction), not GPU rendering, uploads, world generation, streaming or scheduling.

### Hard budget: randomized 16³ worst-case chunk meshing

The performance TU adds **two seeded stress tests with 256 distinct chunks each**: (1) randomized opaque cubes with two materials and high face exposure, and (2) randomized cubes/slabs/slopes/stairs with independent random horizontal/vertical orientations. All 4,096 cells in each chunk are randomized; occupancy varies from 35% to 80%, with additional near-checkerboard randomized cases (94% occupied on one parity, 3% on the other) that force more than 60,000 generated vertices. The generator uses a fixed 64-bit seed, so every run replays the identical 512 chunks. Each test independently validates mesh counts (including exact visible-face counts for cube-only chunks), and reports median, P95, P99, maximum per-case median, and maximum observed single bake.

**Hard CI contract:** each chunk's median across three consecutive bakes must be **no more than 5 ms** under the Release Windows CI configuration; one case above this limit fails its whole test. No results are hidden behind the overall median. The `Chunk::Mesher` reuses the shared occlusion cache, as the runtime adapter does; volume creation, random generation, and geometric assertions are not timed.

The **5 ms target is a design requirement**, not an existing demonstrated capability. A failing test is actionable evidence of a performance bottleneck; do not silently increase the threshold to get green CI. The maximum single elapsed bake is reported separately, since OS scheduling interruptions mean a wall-clock upper bound on arbitrary machines cannot be guaranteed by unit tests. The release build's fixed hardware/runners and per-case median provide a repeatable regression check.

Other deterministic geometry tests continue reporting timings without thresholds.

## Tests and golden image evidence

`tests/TU/Graphics/srcs/voxel/chunk_rendering_golden_test.cpp` remains focused on rendering and golden image correctness; it covers:

- Every axis and direction, including negative coordinates: two touching opaque cubes remove the internal faces (36 → 30 vertices per cube), and those faces return when a neighbor disappears.
- Translucent cells and partially occluding slab/slope boundaries.
- Multiple edits coalesced to one rebuild per chunk, with neighboring chunk invalidation and renderer cleanup.
- Three adjacent chunks, shown from an angled camera and rendered using the real Engine graphics pipeline.
- **Image comparison:** the chunked image is compared pixel-by-pixel with an equivalent, independently meshed single-volume scene in the same camera and lighting configuration. Both screenshots and any difference image are saved to `results/rendering/voxel-chunks` and uploaded to the visual comparison artifacts.

The test validates **both** the independently meshed runtime reference and the checked-in static PNG baseline at `tests/TU/Graphics/resources/expectedImages/rendering/voxel-chunks/three_chunks_oblique.png`. This 320×240 golden was captured from the passing Windows Release build and verified to match the passing Debug build exactly. Nonempty image assertions prevent an all-black result from passing.

## Exclusions

No changes to the existing `Voxel::Mesher` or occlusion algorithm, persistence, world streaming policies, gameplay entity ownership, automatic material selection, networking, or Core-to-Graphics dependencies.
