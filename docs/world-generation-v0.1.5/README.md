# Sparkle v0.1.5 — Chunk-based world generation and streaming

> **Design plan, not an implementation authorization.** These documents supersede the original v0.1.5 world-planning proposal and reflect the architecture decisions discussed with the maintainer on 2026-11-19. API spellings are sketches until reviewed in a dedicated implementation PR. Never merge without explicit approval.

## Objective

Ship **reusable infrastructure** to generate, request, stream and optionally display chunk-based worlds. The library does **not** prescribe a game's geography, biomes, towns, roads, caves, vegetation, placement rules or other world content. Game developers create their own generation passes and chunk subclasses. An example based on the existing voxel API demonstrates the entire system.

A world is *practically unbounded*: signed 32-bit XYZ chunk coordinates cover a huge address space; no truly infinite or preallocated world is claimed. Finite worlds are supported by a requester/generator returning no chunk outside valid bounds.

## Agreed design

- **`Chunk<SizeX, SizeY, SizeZ>`**: minimal inheritable base defining static world-unit dimensions and coordinate conversions; **no coordinate, entity or voxel payload stored in the base**.
- **`ChunkCoordinate`**: signed three-dimensional integer key (reuse Sparkle's vector type where appropriate); floor-based negative coordinate conversions.
- **`ChunkCollection<TChunk>`** (previous working name: `World<TChunk>`): `std::map` indexed by coordinate, owning typed chunks and one headless-compatible `spk::Entity3D` per loaded chunk. Insert/replace preserves an existing entity; removal destroys its entity. Exposes controlled map-like iteration and lookup.
- **`Generator<TChunk, TGenerationData>`**: derives its mutable data type from **`typename TChunk::Builder`** (no third template argument). `createChunkBuilder(coordinate)` returns a new builder, or null for an absent chunk. Ordered polymorphic `Pass::apply(TChunk::Builder&, TGenerationData&)` edits the builder and fresh generation data, before **`builder.build()`** finalizes the chunk. A voxel chunk builder can own `Voxel::Volume::Builder`. Multiple seeds and synchronization of shared user data remain developer-controlled.
- **`ChunkRequester<TChunk>`**: interchangeable asynchronous source (local generator, remote client, file, mock). Exactly one `spk::Task` per requested coordinate. Successful `nullptr` means no chunk exists; failure/timeout are separate outcomes.
- **`ChunkInterest` and `WorldObserver`**: a symmetric XYZ radius around an observer's current chunk, supplied by a `Component3D` registered by existing `EngineHeadless*` context/Registry mechanisms. A player gets the component; ordinary monsters need not.
- **`WorldStreamingSystem<TChunk>`**: headless system bound to a collection and requester, aggregating all registered observers in the Engine, loading union of their interests, ordering requests by minimum observer distance, keeping a configurable maximum number of in-flight tasks, applying results on the Engine update thread, retrying timed-out requests using request versions, and unloading all chunks outside every interest.
- **Single chunk-synchronization notification**: insertion, replacement or explicitly marked changes cause one coalesced notification per chunk during Engine update (coordinate, chunk, entity). No dedicated rendering removal event; removing a chunk destroys its Entity3D and attached components.
- **Graphics adapter**: bound to the collection, synchronizes all existing chunks at bind time and listens for the same notification thereafter; reuses existing rendering components or adds them if absent. A voxel adapter uses `Voxel::Volume`, Mesher and Renderer. The generic Core remains headless.

## Scope and boundaries

Only **one ChunkCollection/World per Engine** is supported by the first observer-registration design. Multiple separate worlds in one Engine (overworld/dungeon) may later be disambiguated with a world UUID in observers and systems; **do not implement this in v0.1.5**.

No new multiplayer protocol or server framework. Several observers in one headless Engine, independent requester implementations and voxel volume serialization are sufficient integration targets. No global world/region planner class, mandatory meta-structure graph or city/road/cave algorithm is part of the core contract. Optional sample passes may show cross-chunk coherence through shared user-defined data.

## Reading order

1. [Architecture and ownership](architecture.md)
2. [API contracts and agreed decisions](contracts.md)
3. [Feature extensibility and examples](features.md)
4. [Remaining architecture review](architecture-review.md)
5. [Implementation roadmap](roadmap.md)
6. [Validation](validation.md)
7. [Individual draft PRs](implementation/README.md)

## Existing Sparkle APIs to reuse

`EngineHeadless`, `System`, `Component3D`, `Entity3D`, `Registry<T, EngineHeadless*>`, `ContextualizableTrait`, `WorkerPool`, `Task`, `ContractProvider`, `spk::Vector3Int`, `Voxel::Volume::Builder`, `Voxel::Mesher` and `Voxel::Renderer`. The voxel data/meshing pipeline already exists; do not create a duplicate dense voxel storage representation.

The planning PR is [#85](https://github.com/EreliaStudio/Sparkle/pull/85) (draft, documentation only), based on `V0.1.5`. Older candidate issues #78–#83 remain withdrawn and are not part of this plan.
