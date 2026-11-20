# v0.1.5 — Agreed contracts and remaining API choices

> These decisions were made with the maintainer. Illustrative spelling is not a frozen ABI. Do not implement until a feature PR is approved.

## Agreed / normative for design

| Topic | Decision |
| --- | --- |
| Type model | Inheritable `Chunk<SizeX,SizeY,SizeZ>` base, compile-time positive dimensions in **world units**; arbitrary application-defined subclasses |
| Coordinates | Signed XYZ 32-bit `ChunkCoordinate`, floor division for negative positions; chunk **does not store** its key |
| Conversion | Static world-to-chunk, chunk-to-world-origin and world-to-local helpers on chunk type |
| Storage | `ChunkCollection<TChunk>` backed by ordered `std::map<ChunkCoordinate, ChunkEntry>`, chunk + headless Entity3D per entry |
| Access | `operator[]` and `at` return `TChunk&` or throw if missing; `tryGet` returns nullable `TChunk*`, `contains` checks presence, all with `const` forms. Iteration yields `[coordinate, entry]` with `entry.chunk()` and `entry.entity()`. No structural mutation through iterators. |
| `addChunk` | Coordinate + owned chunk; replace existing chunk and **retain** its Entity3D |
| Removal | Delete chunk and its Entity3D/components. Graphics observes entity destruction through `LifeCycle` and queues neighboring coordinates; no second generic collection notification. |
| Events | **One** immediate chunk-changed event per add/replace/explicit modification (Engine thread when attached). Rendering alone deduplicates dirty coordinates and updates on render; initial bind scans all existing entries. |
| Generation | `Generator<TChunk,TGenerationData>` uses `typename TChunk::Builder`, virtual `createChunkBuilder(coord)` and `createGenerationData(coord)`, ordered `Pass::apply(TChunk::Builder&,TGenerationData&)`; `builder.build()` returns `TChunk` **by value** |
| Builder lifetime | Like `Voxel::Volume::Builder`: `TChunk::Builder::load(const TChunk&)` populates a fresh builder from an existing chunk; successful `build()` consumes the builder; a published chunk stays unchanged | 
| Data lifetime | Fresh chunk Builder and TGenerationData per generation; TGenerationData can hold mutable references to persistent data; publish only a successfully built TChunk |
| Seeds | Multiple explicitly configurable seeds in user-defined generation configuration; no forced single-seed derivation |
| User concurrency | User-written generators/passes/caches responsible for their own thread safety and deterministic rules |
| Requester | Pluggable `ChunkRequester<TChunk>` returning a Task answer for **one** coordinate; null result = no chunk |
| Interest | `ChunkInterest` stores a nonnegative XYZ radius; observer position provides center; requested set is inclusive axis-aligned box |
| Observers | `WorldObserver : Component3D`, registered by Sparkle EngineHeadless context and Registry; support multiple players |
| Streaming | Union of interests, prioritize by nearest interested observer, configurable max concurrent Task requests, timeout, request version, ignore stale responses |
| Retention | Unload **all** chunks outside combined interest; no LRU/distance-based cache |
| Engine worlds | **One** collection/world per Engine in v0.1.5; future world UUID only if needed |
| Graphics | Optional system bound to collection; per-chunk Entity3D already exists. Renderer queues unique changed coordinates, synchronizes once per render and listens for entity destruction to invalidate neighbors. |

## Additional agreed lifecycle contract

`spk::LifeCycle` introduces three distinct subscription events (Acquisition, Release, Destruction), initially acquired objects, and protected idempotent `acquire()`/`release()` transitions. During final destruction, Release (if still acquired) always precedes Destruction. `Widget` and `Entity` will use the common primitive. `Pool<T>` adapts its existing obtain/return paths for lifecycle-aware elements **without replacing** the ownership/recycling logic in `Lease`: the pool's weak-state mechanism already handles a Lease outliving its pool. Ordinary element types remain supported. See [life-cycle-design.md](life-cycle-design.md) for precise ordering and tests.

A collection owns one root `Entity3D` with chunk entities beneath it. Explicit EngineHeadless attach/detach/rebind preserves chunks and their entities. Systems bound to a different Engine context automatically unbind and require an explicit new bind.

## Illustrative C++ API (not final)

~~~cpp
// Static geometry and application data; no coordinate stored on Chunk.
class GameChunk : public spk::Chunk<32, 16, 32>
{
public:
    class Builder; // load(const GameChunk&); GameChunk build(); (one-shot)
    spk::Voxel::Volume ground;
    // Other application-defined fields: foliage, structures, etc.
};

// Spatial ownership belongs to the collection, not the data.
spk::ChunkCollection<GameChunk> chunks;
chunks.addChunk({14, 16, 18}, std::make_unique<GameChunk>());
GameChunk& chunk = chunks.at({14, 16, 18}); // Throws if missing.

// Accessor spellings and exact Entry iteration representation remain under review.
for (const auto& [coordinate, entry] : chunks) { /* inspect */ }
~~~

~~~cpp
template<typename TChunk, typename TGenerationData>
class Generator
{
public:
    using ChunkBuilder = typename TChunk::Builder;
    class Pass
    {
    public:
        virtual ~Pass() = default;
        virtual void apply(ChunkBuilder&, TGenerationData&) = 0;
    };
protected:
    // Illustrative factory return type; confirm during API review.
    virtual std::unique_ptr<ChunkBuilder> createChunkBuilder(
        const ChunkCoordinate&) = 0;
    virtual TGenerationData createGenerationData(const ChunkCoordinate&) = 0;
    // After all passes: TChunk result = builder.build(); // by value
    // Then move result to the requester's unique_ptr<TChunk>.
};
~~~

~~~cpp
// Concept, not verified exact Task syntax / method spelling:
ChunkRequester<TChunk>::request(coordinate)
    -> spk::Task<std::unique_ptr<TChunk>>::Answer;

// Concept:
// collection.subscribeToChunkSynchronization(
//     [](const ChunkCoordinate&, TChunk&, spk::Entity3D&) {...});
~~~

## Operational details

- `worldToChunkCoordinate` must handle negative values by floor, not C++ truncation. For (70,-2,35) in 32-unit chunks: key (2,-1,1), world origin (64,-32,32), local (6,30,3). Coordinate conversions use world units, *not number of voxels*.
- Generators publish no partially constructed chunk: passes only receive a fresh Builder; `builder.build()` finalizes the TChunk before handing it to the requester.
- Replacing a chunk retains its entity and only rebinds its attached component(s) on the next synchronization.
- Direct mutation of a chunk's own fields cannot be detected automatically; callers explicitly signal that it changed. The collection emits notifications immediately; the rendering system deduplicates coordinates and rebuilds once at render.
- A bound rendering adapter must scan loaded chunks before depending on future notifications. Removal uses entity destruction; external resource holders must not retain dangling references.
- Request timeouts and stale results are handled without requiring a cancellation API in Task. A fresh monotonically unique request version prevents old completions from superseding new requests even if interests are removed/re-added.
- Because user-defined persistent generator state may be mutable, **Sparkle guarantees only pass order within a generation**, not deterministic outputs from nondeterministic user rules or scheduling across generations.

## Deliberately not part of v0.1.5

No built-in WorldPlan/RegionPlan, procedural city/road/cave generator hierarchy, universal metadata schema, persistence of player edits, auto-save/reload, network protocol, multi-world UUID, smart frustum/neighbor-flood interest, general object-entity persistence or LRU cache. These may be built by users through passes or designed later.

## API details to settle during implementation review

1. Exact namespace and public class names, including whether a future `World` will wrap `ChunkCollection`.
2. **Resolved:** `operator[]` and `at` return `TChunk&` and throw for missing keys; `tryGet` returns nullable pointer; `contains` tests presence. None inserts.
3. **Resolved:** iteration provides `[coordinate, entry]` with `entry.chunk()` and `entry.entity()` references, without exposing unique ownership.
4. Precise Engine binding/Entity3D parentage and safe teardown order, especially when the collection is destroyed or rebound.
5. Exact `Task::Answer` return syntax, retry/error/terminal-null bookkeeping and timeout configuration, based on existing Sparkle APIs.
6. **Resolved:** collection notifies immediately; renderer queues coordinates and flushes them once before each render.
7. Builder return and reconstruction are **decided**: `TChunk::Builder::build()` returns `TChunk` by value and consumes the builder; `load(const TChunk&)` can initialize a fresh builder from an existing chunk. Only precise constructors and ownership of `createChunkBuilder()` remain to confirm.
8. Minimal concrete voxel example and renderer resource binding strategy, including neighboring-chunk rebakes.
9. **Resolved conceptually:** subscribe to Entity3D lifecycle destruction, release external graphics resources and queue six neighbors to update at next render. Validate notification order during derived destruction.

These are **implementation details, not permission to revert the architectural decisions above**.
