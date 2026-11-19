# Validation and release criteria

This is a planning document only; implementation PRs must add automated
tests that run without an external game server or graphics environment.

## Generic Core tests

- Signed coordinates and floor division: (-1) maps to previous chunk,
  boundary positions, large positive/negative positions, overflow, zero
  chunk dimensions and finite-world bounds.
- Copy/move and lifetime of chunk payloads, duplicate insertion and removal,
  revision monotonicity, change notification and deterministic iteration.
- Seed stability and local sampler determinism across process restarts and
  Windows/Linux (define the expected reproducibility boundary).
- Chunk request order permutations and parallel invocation produce exactly
  the same generated logical data.
- World/region metadata contains every feature intersecting a requested
  chunk; ownership is unique and results are stable across neighboring
  regions.
- Weighted biome transitions and boundary continuity; deterministic
  scenery placement with exclusions and minimum distances.
- Road segments and junctions spanning multiple regions, slope constraints,
  crossed streams, settlements with shared streets/plots.
- 3D cave tunnels spanning chunk boundaries and their interactions with
  surface/road/structure generation.
- Idempotent regeneration, invalid feature recipe, mutually exclusive
  placements and stable conflict resolution.
- Duplicate/failed/cancelled requests, priorities, stale asynchronous
  completions, memory budgets and resource release.
- Saved user modifications persist through eviction and regeneration;
  regeneration must not overwrite authoritative edits.

## Voxel-specific tests

- Generated payloads are **Voxel::Volume**, built through the existing
  Volume::Builder and bound to valid Definition::Registry IDs.
- Different chunk dimensions; negative X/Y/Z chunk coordinates.
- Exact voxel equality for same seed and plan, regardless of generation
  order; seam checks for terrain, roads, buildings and caves.
- Shape/orientation correctness and air/solid transitions.
- Volume serialization with existing ByteStream/Message round trips when
  world chunks travel between components; no new network protocol.
- Mesher renders visible boundary faces correctly when neighbor chunks
  are loaded, changed or evicted.

## Graphics and visual acceptance

No golden images for generic metadata or generation-only PRs. Once the
Graphics adapter is introduced, add extensive Sparkle golden tests:
seam-free neighboring chunks, terrains at different elevations, roads
crossing boundaries, buildings shared by chunks, cave entrance/interior,
unload/reload and partial updates. Use Sparkle's built-in golden
comparator; do not skip missing references or substitute Python image
checking.

## End-to-end acceptance scene

Deterministically create a seeded, finite sample world with:
- several biome zones and varied ground;
- at least two settlements connected by roads;
- objects/scenery distributed around reserved corridors;
- an underground cave crossing chunk boundaries;
- loading and unloading while traversing the map;
- data regenerated in a different request order with identical results;
- an edited cell that remains edited after unload and reload;
- headless generation plus optional visible Geometry/Engine rendering.

A single demo is a **final integration target**; individual PRs have
smaller acceptance targets in the roadmap and PR briefs.

## Performance and diagnostics

Record planning time per region, generation time per chunk,
peak allocations, world-plan memory, cancellation overhead and
render-bake latency. Use stable test seeds and representative worst cases.
No unbounded caching or global mutex around every cell operation.
Avoid performance promises before measurement.

## CI acceptance

C++23, Windows/Linux (Debug and Release where the pipeline provides it),
clang-format and the existing relevant Core/Graphics test matrices.
Do not mark a feature PR complete before its CI is green. Never merge
without the maintainer's explicit permission.
