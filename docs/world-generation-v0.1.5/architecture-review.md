# v0.1.5 — Architecture review: remaining decisions

> **Open questions only.** This checklist is deliberately smaller than the earlier speculative planning hierarchy. The agreed contracts are in [contracts.md](contracts.md). Do not silently promote recommendations below to approved design.

## Already agreed — do not re-open without a new requirement

- Generic inheritable `Chunk<SizeX,SizeY,SizeZ>`, spatial units, static coordinate conversions and **no stored coordinate**.
- `ChunkCollection<TChunk>` ordered by coordinates with one headless Entity3D per loaded chunk; replacement preserves entity; removal destroys entity and components.
- One immediate chunk-changed notification emitted by the collection; the renderer queues unique coordinates until the next render. Entity3D destruction is observed via `spk::LifeCycle` for neighbor invalidation, without a second generic collection event.
- `Generator<TChunk,TGenerationData>` with concrete `TChunk::Builder`; passes apply to builder and generation data; **`build()` returns TChunk by value and consumes the builder**. The Builder also exposes `load(const TChunk&)` for rebuilding from existing data, mirroring `Voxel::Volume::Builder`. Multiple seeds and shared mutable persistent data are developer-owned.
- Task-based swappable chunk requester; per-observer XYZ radius; Registry-based WorldObserver; multi-observer streaming with concurrency cap, timeouts/versions, nearest-first priority and no retention outside interest.
- Graphics is optional; voxel example uses existing Volume::Builder then immutable Volume, Mesher and Renderer.

## Essential decisions before the first API implementation

### A. Chunk Builder and build-result ownership (PR01 / PR03)

1. **Resolved:** `TChunk::Builder::build()` returns `TChunk` by value, as in `Voxel::Volume::Builder`; the generator moves it into the ownership required by the requester. A successful `build()` consumes the builder.
2. Is a Builder mandatory on *all* TChunk subclasses or only on concrete types used with a Generator? The agreed Generator signature requires a nested Builder; ChunkCollection itself can store externally obtained chunks.
3. Builder creation lifecycle: `createChunkBuilder(coordinate)` proposed to return nullable owning Builder; `createGenerationData(coordinate)` is fresh per call. Define exception/absence behavior precisely.
4. **Resolved:** match the existing `Voxel::Volume::Builder` API: a freshly constructed `TChunk::Builder` can `load(const TChunk&)` to start from an existing chunk. Edits occur in the builder, followed by a new `build()`. The old chunk/immutable Volume is untouched.

### B. Entity ownership and Engine binding (PR02)

**Resolved:** collection owns a root Entity3D with chunk children; explicit attach/detach/rebind to EngineHeadless preserves existing entities. Context-mismatched systems unbind and never reconnect automatically. Exact teardown ordering remains to validate.

1. **Resolved:** root Entity3D parents all chunk entities and can attach to the EngineHeadless root.
2. **Resolved:** explicit Engine binding/unbinding/rebinding preserves all chunk entities; context mismatch drops systems' subscriptions, requiring explicit system rebind.
3. **To verify in implementation:** replacement retains the Entity3D but destroys old chunk data; subscribed graphics code must not retain stale references.

### C. Map-like public API and update thread (PR02)

**Resolved:** `operator[]` and `at` return references and throw when absent, `tryGet` returns a nullable pointer, `contains` returns bool. Iteration yields `[coordinate, entry]`, with `chunk()` and `entity()` accessors. The collection emits immediately; render-specific deduplication occurs at render time.

1. **Resolved:** `operator[]` and `at` return `TChunk&` or throw; `tryGet` returns nullable pointer; `contains` returns bool. No implicit insertion.
2. **Resolved:** iteration exposes `[coordinate, entry]` with `entry.chunk()` and `entry.entity()` references; no owning pointer access.
3. **Resolved:** collection notifies immediately; renderer queues coordinates for sync on render. **To verify:** reentrant mutation and ordering during teardown.

## Essential decisions before streaming and Graphics implementation

### D. Requester completion and request lifecycle (PR04 / PR06)

1. Verify real `spk::Task` / `Answer` signatures and WorkerPool subscription semantics, including object lifetimes and exception propagation.
2. Timeout and retry policy: when an old task never completes, distinguish maximum logically *active requests* from actual worker jobs still running; avoid unlimited workers from repeated timeouts.
3. Where and how are successful-null (nonexistent), temporary failure and retryable timeout results retained for currently interested coordinates, without retaining unloaded chunk data?
4. Exact moment of publication on Engine update, and safe shutdown of in-flight callbacks.

### E. Neighbor-aware rendering without a second generic notification (PR02 / PR08)

1. **Resolved conceptually:** renderer uses the chunk Entity3D lifecycle-destruction subscription; callback captures coordinate, releases external graphics resources and queues six neighbors without reading the destructing entity.
2. On addition/replacement, the renderer invalidates boundary-dependent neighbors when needed; generic ChunkCollection has no voxel awareness.
3. Implement the adapter using actual Mesher/Renderer APIs and verify callback ordering in C++.

## Agreed Core lifecycle prerequisite

See [LifeCycle/Pool integration](life-cycle-design.md). The existing `Pool::Lease` weak-state mechanism already handles pool destruction with outstanding leases. We retain this ownership model and add protected lifecycle transitions at obtain/recycle/final destruction for lifecycle-aware elements. `Widget` and `Entity` migrate to the three independent events (Acquisition, Release, Destruction); `Entity` destruction enables the graphics neighbor invalidation above.

## Non-blocking: settle only when implementation needs it

- Public namespace spelling, helper spelling and optional Builder config syntax.
- Exact equality/tie-breaking distance metric for the streaming queue.
- Specific deterministic world-feature planning utilities. Cities, roads, terrain grammars and cave layouts remain **user-defined pass content**, not mandatory Core modules.
- Multiple simultaneously loaded world instances in one Engine and world UUID filtering: explicitly **future work**, not a hidden v0.1.5 dependency.
- Persistence of gameplay edits and disk save/load: user or future extension responsibility; no new network protocol.

## Proposed decision order

Builder return/rebuild behavior under **A** is settled; verify the remaining Builder factory/absence details, then prioritize **B (entity parenting and Engine binding)** and **C (collection API/events)**. With those stable, approve Core PR01–PR03. Next settle **D** before PR04–PR06 and **E** before PR08. A small headless voxel vertical slice should validate the chosen boundaries before adding feature algorithms.
