# v0.1.5 — Agreed contracts and remaining API choices

> These decisions were made with the maintainer. Illustrative spelling is not a frozen ABI. Do not implement until a feature PR is approved.

## Agreed / normative for design

| Topic | Decision |
| --- | --- |
| Type model | Inheritable `Chunk<SizeX,SizeY,SizeZ>` base, compile-time positive dimensions in **world units**; arbitrary application-defined subclasses |
| Coordinates | Signed XYZ 32-bit `ChunkCoordinate`, floor division for negative positions; chunk **does not store** its key |
| Conversion | Static world-to-chunk, chunk-to-world-origin and world-to-local helpers on chunk type |
| Storage | `ChunkCollection<TChunk>` backed by ordered `std::map<ChunkCoordinate, ChunkEntry>`, chunk + headless Entity3D per entry |
| Access | begin/end/rbegin/rend and const forms, subscript lookup and exception-throwing `at`; no bypass of structural mutations |
| `addChunk` | Coordinate + owned chunk; replace existing chunk and **retain** its Entity3D |
| Removal | Delete chunk and its Entity3D/components; no separate rendering-removal notification |
| Events | **One** coalesced synchronization notification per dirty chunk, during Engine update; initial bind scans all existing entries |
| Generation | `Generator<TChunk,TGenerationData>` uses `typename TChunk::Builder`, virtual `createChunkBuilder(coord)` and `createGenerationData(coord)`, ordered `Pass::apply(TChunk::Builder&,TGenerationData&)`, then `builder.build()` |
| Data lifetime | Fresh chunk Builder and TGenerationData per generation; TGenerationData can hold mutable references to persistent data; publish only a successfully built TChunk |
| Seeds | Multiple explicitly configurable seeds in user-defined generation configuration; no forced single-seed derivation |
| User concurrency | User-written generators/passes/caches responsible for their own thread safety and deterministic rules |
| Requester | Pluggable `ChunkRequester<TChunk>` returning a Task answer for **one** coordinate; null result = no chunk |
| Interest | `ChunkInterest` stores a nonnegative XYZ radius; observer position provides center; requested set is inclusive axis-aligned box |
| Observers | `WorldObserver : Component3D`, registered by Sparkle EngineHeadless context and Registry; support multiple players |
| Streaming | Union of interests, prioritize by nearest interested observer, configurable max concurrent Task requests, timeout, request version, ignore stale responses |
| Retention | Unload **all** chunks outside combined interest; no LRU/distance-based cache |
| Engine worlds | **One** collection/world per Engine in v0.1.5; future world UUID only if needed |
| Graphics | Optional system bound to collection; per-chunk Entity3D already exists; find/create components and rebind data |

## Illustrative C++ API (not final)

~~~cpp
// Static geometry and application data; no coordinate stored on Chunk.
class GameChunk : public spk::Chunk<32, 16, 32>
{
public:
    class Builder; // Owns mutable draft data, including Volume::Builder.
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
    // After all passes: builder.build() creates the final TChunk.
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
- Direct mutation of a chunk's own fields cannot be detected automatically; callers explicitly signal that it changed. The collection deduplicates dirty coordinates per Engine update.
- A bound rendering adapter must scan loaded chunks before depending on future notifications. Removal uses entity destruction; external resource holders must not retain dangling references.
- Request timeouts and stale results are handled without requiring a cancellation API in Task. A fresh monotonically unique request version prevents old completions from superseding new requests even if interests are removed/re-added.
- Because user-defined persistent generator state may be mutable, **Sparkle guarantees only pass order within a generation**, not deterministic outputs from nondeterministic user rules or scheduling across generations.

## Deliberately not part of v0.1.5

No built-in WorldPlan/RegionPlan, procedural city/road/cave generator hierarchy, universal metadata schema, persistence of player edits, auto-save/reload, network protocol, multi-world UUID, smart frustum/neighbor-flood interest, general object-entity persistence or LRU cache. These may be built by users through passes or designed later.

## API details to settle during implementation review

1. Exact namespace and public class names, including whether a future `World` will wrap `ChunkCollection`.
2. Whether `operator[]` returns a nullable pointer on missing keys (recommended) and whether `at` returns TChunk or ChunkEntry; it **must not implicitly create a chunk**. `at` must throw for absence.
3. Whether iteration exposes a read-only Entry view or a stable pair of references; ensure structural map/entry ownership cannot be mutated directly.
4. Precise Engine binding/Entity3D parentage and safe teardown order, especially when the collection is destroyed or rebound.
5. Exact `Task::Answer` return syntax, retry/error/terminal-null bookkeeping and timeout configuration, based on existing Sparkle APIs.
6. Whether synchronization events are flushed by `ChunkCollection::updateState` or explicitly by its binding system during Engine update; keep one callback per coordinate per frame.
7. Exact nested `TChunk::Builder` creation/ownership, `build()` return (value or owning pointer) and whether optional rebuilding-from-existing is supported.
8. Minimal concrete voxel example and renderer resource binding strategy, including neighboring-chunk rebakes.
9. How neighboring voxel graphics are invalidated on removal while retaining the sole generic chunk-synchronization event.

These are **implementation details, not permission to revert the architectural decisions above**.
