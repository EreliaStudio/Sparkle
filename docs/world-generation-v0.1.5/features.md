# Generation extensibility — user-defined passes, not built-in worlds

## Philosophy

Sparkle v0.1.5 provides **how** a chunk is created, loaded, stored and shown, not **what** is in the world. Terrain, caves, cities, roads, vegetation, geometry files and gameplay content are written by the consuming game. A single chunk may contain voxel terrain plus non-voxel foliage and any other data. Chunk contains **no mandatory Entity or Volume** and owns no coordinate.

The prior mandatory `WorldPlan`/`RegionPlan`, universal meta-structure index, city grammar, routing and cave-planning implementation are **withdrawn from the v0.1.5 core plan**. They may reappear later as generic optional utilities if specific user-written passes demonstrate a need.

## Pass collaboration

`Generator<TChunk, TGenerationData>` has ordered virtual passes. Each generation creates a new `TGenerationData`. Passes can write intermediate plans into this local data for later passes, or read/write **persistent** generator-owned data via references held in it.

A game might define:

~~~cpp
struct MyGenerationData {
    ChunkCoordinate coordinate;
    WorldSeeds& seeds;                // Persisted configuration
    RegionalCache& cache;             // Optional mutable shared data
    HeightSamples heights;            // Per-request intermediate result
    std::vector<BuildingPlan> plans;  // Per-request intermediate result
};

// Example sequence, defined by the game rather than supplied by Sparkle:
// HeightPlanningPass -> CityPlanningPass -> TerrainVoxelPass
// -> BuildingRealizationPass -> VegetationPass
~~~

The contract does not oblige every pass to touch the chunk Builder. Stages can prepare data only. Users choose ordering, noise implementation, seed scopes, cache schemes, tie-breaking rules and how cross-chunk structures are described. They also ensure shared caches are thread-safe when needed.

## Cross-chunk continuity

If a tree, cave or building crosses a boundary, passes must derive the same underlying spatial feature regardless of requested chunk order. A user-defined cache or deterministic field can be shared through the generator. Spatial descriptors should be **queryable by every intersecting chunk**, rather than belonging only to the chunk requested first.

This is a **sample design requirement**, not an imposed `RegionPlan` API. The demonstration should include at least one feature crossing a chunk boundary to catch request-order bugs.

## Finite and virtually unbounded worlds

The framework supports the entire signed 32-bit chunk coordinate domain, limited by numeric type and checked conversions. A game can constrain its world arbitrarily: `createChunkBuilder` may return `nullptr` outside bounds. The requester reports that as a successful no-chunk result; streamer must not spin forever requesting the same absent coordinate.

## Example voxel integration

A concrete `VoxelChunk` derives from the generic chunk type, holds one or more existing immutable `spk::Voxel::Volume` values, and may add unrelated game data. **`VoxelChunk::Builder`** holds the mutable draft, including a `Voxel::Volume::Builder` on which the generator passes operate. Only its outer `build()` finalizes the Volume(s) and the chunk. Voxels can be 0.5 world units or any chosen size; **chunk dimensions always describe world units**. Do not duplicate existing voxel storage.

The optional graphical adapter bakes Geometry with `Voxel::Mesher` and binds a `Voxel::Renderer` on the chunk's Entity3D; other components/child entities may be added by game-specific systems. The generic framework has no rule that one chunk means one mesh or one graphic.

## Not in the required implementation

- Full procedural biomes, climate/weather or noise libraries.
- Automated road or city layout, building architecture, pathfinding, cave synthesis.
- Global hierarchical world/region planner classes and mandatory feature graphs.
- Runtime save/load of edited terrain, character persistence or ownership semantics beyond the chunk entity.
- A multiplayer transport protocol or a novel graphics pipeline.

If later PRs add reusable spatial tools, they must be optional, motivated by demonstrated needs and independently testable; they must not be prerequisites for basic chunk generation.
