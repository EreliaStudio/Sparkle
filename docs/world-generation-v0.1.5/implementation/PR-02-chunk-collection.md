# PR02 — ChunkCollection, Entity3D ownership and single sync event

**Layer:** Core/EngineHeadless. **Dependencies:** PR01. **Status:** design draft.

## Scope
- `ChunkCollection<TChunk>` using `std::map<ChunkCoordinate, ChunkEntry>` (lexicographic coordinate order). Each entry owns `unique_ptr<TChunk>` and one headless-compatible `unique_ptr<Entity3D>`.
- `addChunk(coordinate, chunk)` inserts or **replaces chunk content, preserving existing Entity3D and its components**. `removeChunk` destroys the chunk entity/components.
- Explicit collection-to-EngineHeadless binding and safe Entity3D attachment/parenting/lifetime; no graphics dependencies.
- Iteration `begin/end/rbegin/rend` and const variants, structured binding/view of coordinate + entry, no bypass of structural mutation. `at` throws on absence; `operator[]` lookup semantics to finalize without implicit insertion.
- One subscribe-able `synchronize this chunk` event carrying coordinate, chunk reference and entity reference. Insert, replacement and explicit modification mark dirty. Emit once per coordinate during Engine update; no mandatory removal event.
- Initial collection iteration must be available to late subscribers.

## Required tests
- Ordered forward/reverse traversal and missing-key lookup; direct editing does not mutate keys or entity ownership.
- First add creates Entity3D at chunk world origin. Same-coordinate replacement retains identical entity, existing mock component, updates data and emits one notification.
- Many modifications before one update => exactly one notification. Removal => owned Entity3D/component destructors called, no use-after-free.
- Headless engine attach/detach/destroy order and safe callbacks. Cross-thread publication is explicitly **not** allowed by this class.

## Exclusions
No generator, scheduling, OpenGL, disk persistence or graphics observer.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
