# PR02 — ChunkCollection, Entity3D ownership and single sync event

**Layer:** Core/EngineHeadless. **Dependencies:** PR01. **Status:** design draft.

## Scope
- `ChunkCollection<TChunk>` using `std::map<ChunkCoordinate, ChunkEntry>` (lexicographic coordinate order). Each entry owns `unique_ptr<TChunk>` and one headless-compatible `unique_ptr<Entity3D>`.
- `addChunk(coordinate, chunk)` inserts or **replaces chunk content, preserving existing Entity3D and its components**. `removeChunk` destroys the chunk entity/components.
- Own a collection root Entity3D with child chunk entities. Explicit EngineHeadless attach/detach/rebind preserves loaded chunks and entities. Context-mismatched systems unbind and must reconnect explicitly; no Graphics dependencies.
- Iteration `begin/end/rbegin/rend` and const variants, yielding `[coordinate, entry]` with `entry.chunk()` and `entry.entity()` references; never expose owning pointers. `operator[]` and `at` return `TChunk&` or throw, `tryGet` returns nullable pointer, and `contains` returns bool, all const-correct and non-inserting.
- One immediate chunk-changed event on add, replacement and explicit modification (Engine thread when attached). No batching in the collection. Render-side batching occurs in PR08; no mandatory removal event.
- Initial collection iteration must be available to late subscribers.

## Required tests
- Ordered forward/reverse traversal and missing-key lookup; direct editing does not mutate keys or entity ownership.
- First add creates Entity3D at chunk world origin. Same-coordinate replacement retains identical entity, existing mock component, updates data and emits one notification.
- Many modifications => immediate event per operation. Removal destroys owned Entity3D/components, triggers lifecycle destruction for subscribers, and causes no use-after-free.
- Headless engine attach/detach/destroy order and safe callbacks. Cross-thread publication is explicitly **not** allowed by this class.

## Exclusions
No generator, scheduling, OpenGL, disk persistence or graphics observer.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
