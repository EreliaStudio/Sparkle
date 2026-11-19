# Architecture — ownership, phases and layering

> Agreed architecture; names and exact signatures remain subject to implementation review.

## Runtime flow

~~~mermaid
flowchart TD
    E["EngineHeadless (context)"] --> O["WorldObserver : Component3D (Registry)"]
    O --> I["ChunkInterest: XYZ radius"]
    I --> S["WorldStreamingSystem<TChunk>"]
    S --> R["ChunkRequester<TChunk>"]
    R --> L["Local requester"] & N["Remote/file/mock requester"]
    L --> G["Generator<TChunk, TGenerationData>"]
    G --> B["createChunkBuilder: TChunk::Builder"]
    B --> P["Ordered virtual passes: apply(Builder&, Data&)"]
    P --> F["builder.build(): finalized TChunk"]
    F --> L
    R --> T["One spk::Task answer per chunk"]
    T --> S
    S --> C["ChunkCollection<TChunk> (std::map)"]
    C --> X["TChunk + Entity3D per coordinate"]
    C --> Q["Coalesced sync notification on Engine update"]
    Q --> V["Optional Graphics-bound system"]
    X --> V
    V --> RC["Voxel::Mesher -> Voxel::Renderer or other components"]
~~~

## Generic chunk and coordinates

`Chunk<SizeX, SizeY, SizeZ>` is a minimal **inheritable** type. The three compile-time positive `size_t` dimensions represent **world-space units**, not voxel counts. They determine spatial partitioning, independent of the representation inside `TChunk`. It does not store its chunk coordinate, any entity, geometry or `Voxel::Volume`. `Erelia::Chunk` or a voxel example may derive from this type and add anything application-specific.

Static helpers on the chunk type:
- `worldToChunkCoordinate(Vector3 worldPosition) -> ChunkCoordinate` (floor division, including negatives).
- `chunkCoordinateToWorld(ChunkCoordinate) -> Vector3` (world position of chunk origin).
- `worldToLocalCoordinate(Vector3 worldPosition, ChunkCoordinate) -> Vector3` (world-unit position relative to that particular chunk).

`ChunkCoordinate` uses signed 32-bit XYZ components for v0.1.5; math and bounds must check overflow. The world is sparse and effectively very large, not mathematically infinite.

## Collection and headless entity ownership

`ChunkCollection<TChunk>` (the former working `World<TChunk>`) contains a lexicographically sorted `std::map<ChunkCoordinate, ChunkEntry>`; `ChunkEntry` owns a `std::unique_ptr<TChunk>` and a `std::unique_ptr<Entity3D>`. **The map key is the sole stored spatial identity**. Existing values can be modified, but consumers cannot mutate map keys/replace entries through iterators.

`addChunk(coordinate, unique_ptr<TChunk>)` creates the entry and its Entity3D when absent. If the key exists, it **replaces only the chunk while preserving the original Entity3D and attached components**. It marks the entry for synchronization. A direct data edit can call `notifyChunkChanged(coordinate)` (conceptual name). `removeChunk` destroys its owned entity and components; the collection never needs Graphics to do so.

The collection's Entity3D may be attached to the bound EngineHeadless scene hierarchy, including the headless server. The collection must have an explicit, safe Engine binding/lifetime contract; do not assume an unattached Entity3D is rendered or updated.

The collection exposes `begin/end/rbegin/rend` (and const variants), `at(coordinate)` throwing on a missing key, and `operator[](coordinate)` with non-inserting lookup semantics to be finalized. Never expose structural insertion/erase or entity ownership through public iterators. Iteration order is coordinate lexicographic, **not** distance priority.

## Generation

`Generator<TChunk, TGenerationData>` is the user-configured **synchronous generation recipe**. It owns an ordered list of `Generator::Pass` objects. Every generatable concrete chunk type provides a nested **`TChunk::Builder`**, which is the mutable construction target. This does **not** add a third Generator template parameter or force builder implementation into the generic Chunk base.

User-overridden `createChunkBuilder(coordinate)` creates a fresh mutable `TChunk::Builder` (conceptually an owned nullable pointer, to support nonexistent chunks in finite worlds); `createGenerationData(coordinate)` constructs **fresh temporary data per call**, optionally holding mutable references to generator-owned seeds, noise samplers, caches and plans.

Every pass receives **`(TChunk::Builder&, TGenerationData&)`**. It can modify the builder (including nested `Voxel::Volume::Builder`) and/or prepare temporary data for later passes. **Only after every pass succeeds does `builder.build()` produce the finalized `TChunk`.** A null builder skips passes; errors leave no partially built chunk published. Exact build return ownership remains an implementation API choice. Published `Voxel::Volume` snapshots are immutable; this does not mandate immutability for all possible fields of every application chunk.

**Sparkle does not synchronize user-defined persistent state**: passes and shared caches must be thread-safe when concurrent requests are used. Seed determinism is the algorithm author's responsibility; no cross-request completion order is guaranteed.

`Generator` is not a requester and does not edit the live collection. A local `ChunkRequester` wraps it in WorkerPool/Task.

## Acquisition and streaming

`ChunkRequester<TChunk>` exposes one asynchronous request per chunk key; implementations may use generation, disk or networking. A successful null result means the requested coordinate has no chunk, rather than a transient failure. A failed/timed-out task may be retried under the streamer's policy.

The streaming system is bound to the collection and one requester. It finds `WorldObserver` components through the existing `EngineHeadless*` Registry (no duplicate registration layer), uses `TChunk` static coordinate conversion to find each observer's center and expands the interest radius into an inclusive XYZ box.

All observers' interests are combined by union. For each candidate coordinate, priority is its **minimum distance to an observer requesting it**. Queue missing chunks nearest first, with a configurable **limit on in-flight tasks**; replenish slots as tasks complete rather than waiting for rigid batches. Reevaluate priorities on observer movement. Never request the same coordinate twice concurrently except intentional timeout/retry supersession.

A monotonically issued **requestId/requestVersion** accompanies each pending request. On completion, accept the result only if the coordinate is still desired and the version matches the current request. Previous timed-out tasks can finish; ignore obsolete results without requiring actual task cancellation. Apply accepted results by `addChunk` on the Engine update thread; worker callbacks cannot mutate live scene state.

Every update unloads chunks outside the union of current interests. **No LRU, distance cache or maximal retained-chunk budget** in v0.1.5. Max concurrent requests controls work submitted, not retention. Handle null results explicitly so an out-of-bounds coordinate is not retried in an endless loop.

## Rendering and notifications

The collection has **one event**, meaning *synchronize this coordinate*; it carries (coordinate, `TChunk&`, `Entity3D&`). Insert/replace and explicit edit notifications are coalesced into one event per coordinate per update. Its callback runs on the Engine update thread, not the worker.

A Graphics-only rendering system binds to a collection and first synchronizes **every chunk already present**. Then it subscribes to the same event. Its `synchronizeChunk` logic finds an existing rendering component on the Entity3D or adds one, then rebinds updated chunk-derived resources. There is no separate add/update event and no required remove event: collection removal destroys the entity and its attached components. Code that retains external references must respect that lifetime.

For voxels, **`Voxel::Volume` is not a renderer input by magic**: the adapter produces/reuses `Voxel::Mesher` geometry (including boundary-neighbor handling) and binds `Voxel::Renderer` using its actual Geometry/Palette APIs. Visual updates should invalidate adjacent meshing when boundary conditions change, without changing the generic collection contract.

## Core/Graphics boundary

- **Core/headless:** Chunk, coordinate math, collection, Entity3D, generator/passes, requester/Task, interest/observer, streaming and synchronization notification.
- **Voxel Core example:** concrete chunk containing immutable `Voxel::Volume` and a nested Chunk Builder wrapping `Voxel::Volume::Builder`; example passes mutate only the builder. World-unit chunk dimensions remain independent of voxel size.
- **Graphics optional:** rendering adapter, Mesher/Geometry/Palette/Renderer components, golden tests.

One collection/world per Engine is the supported v0.1.5 assumption. A later feature may use `UUID` to identify multiple worlds inside one Engine and filter observers accordingly, without changing the Engine's existing context type.
