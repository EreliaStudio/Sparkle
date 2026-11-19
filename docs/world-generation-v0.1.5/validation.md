# v0.1.5 — Acceptance criteria and test plan

## Core spatial model

- Positive compile-time world-unit dimensions on `Chunk<SX,SY,SZ>`; arbitrary derived payload, no required position member.
- `worldToChunkCoordinate`, `chunkCoordinateToWorld`, `worldToLocalCoordinate` round trips in signed XYZ, including -1, exact negative boundaries, zero and large positive/negative values.
- Overflow checks at int32 chunk-coordinate limits and during conversion to world-unit values.
- Different voxel sizes inside equal world-unit chunk sizes do not change generic coordinate math.

## ChunkCollection ownership and lookup

- Lexicographically ordered `std::map` iteration via begin/end/rbegin/rend and const forms; `at` throws when absent; subscript does not silently insert.
- `addChunk` initially creates the correct Entity3D at the coordinate origin; replacing the same key changes chunk data but preserves the **same** Entity3D and its components.
- `removeChunk` destroys chunk-owned Entity3D/components (including in EngineHeadless); valid ownership and teardown order.
- Insert and explicit modification dirty the entry; repeated modifications within a frame produce **one sync callback** on Engine update, not from a worker.
- Binding a consumer to a pre-filled collection synchronizes all entries once; subsequent notifications update only relevant entries.
- Generic non-voxel chunk and headless Engine tests; external raw references cannot outlive their entity/chunk.

## Generation and Requester

- User subclass generator overrides `createChunkBuilder` and `createGenerationData`; passes execute in configured order on `typename TChunk::Builder`, never directly on a finalized chunk.
- Some passes modify only fresh local generation data; later passes consume it. Different chunk generations do not share temporary builders or data, and each generatable concrete chunk supplies a nested Builder.
- Local generation data can hold user-controlled mutable references to generator-owned state; test with a user-provided synchronized cache and document that **Sparkle provides no implicit locks**.
- Multiple configured seeds influence independent features; fixed seed/rules produce identical logical chunks regardless of request order **for deterministic example passes**.
- Null `createChunkBuilder` skips passes and yields a successful no-chunk result. Exceptions in passes or `builder.build()` publish no partial chunk; Task failures stay distinct from absence.
- One Task answer per requested chunk; local requester can use Sparkle WorkerPool; mock remote/file requesters use same abstraction.

## Observer and Streaming

- `WorldObserver` registered by existing EngineHeadless context and Registry; removal/reattachment updates the observed set. Non-observer monsters do not trigger loading.
- Given center and (X,Y,Z) radii, interest expands to exactly an inclusive axis-aligned box.
- Two observers with overlapping interests load **union** once, using minimum interested-observer distance to prioritize each coordinate.
- Configurable maximum in-flight requests; replenish slots upon completion, not as rigid all-or-nothing batches.
- Repositioning observers reprioritizes pending work; chunks outside every interest are removed immediately.
- Timeout causes new request version; late old results are ignored even if the old Task completes afterward; reentering an interest cannot reuse a stale request ID.
- Failed requests, temporary absence, no-chunk (finite edge), fast observer movement and disappearance of all observers; **no infinite retry of absent chunks**.
- Worker finishes but only Engine update installs chunks and emits sync events; no worker-side Entity mutation.

## Voxel and Graphics example

- Derived voxel chunk holds immutable `Voxel::Volume` snapshots; its nested `TChunk::Builder` owns a mutable `Voxel::Volume::Builder`. Passes update the latter and only the outer `build()` finalizes the Volume and chunk. Chunk physical size in world units remains independent of voxel count/size.
- Adjacent and negative-coordinate volumes have coherent seam samples even when generated in reverse order. A failed build must not change an already published immutable Volume or chunk.
- Bind Graphics renderer after chunks were loaded; initial sync creates the component, later replacement reuses it and changes bound geometry/palette.
- New, updated and removed neighbors invalidate the relevant boundary geometry; use current Mesher semantics and existing Voxel::Renderer API, without inventing Volume->Renderer direct binding.
- Golden images: several adjacent chunks with angled camera, variety of shapes, updated ground, empty neighbors, unload/reload, and replacement; Sparkle internal comparator only.

## Delivery

C++23, clang-format, Windows/Linux CI and applicable Core/Graphics tests must pass. Prefer deterministic fake requesters over real networks. Check runtime lifetime problems and no use-after-free across Task callbacks, observers and chunk/entity removal. No implementation PR is merged without maintainer approval.
