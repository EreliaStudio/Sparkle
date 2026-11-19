# PR06 — Headless WorldStreamingSystem<TChunk>

**Layer:** Core/EngineHeadless. **Dependencies:** PR02, PR04, PR05. **Status:** design draft.

## Scope
- `WorldStreamingSystem<TChunk>` binds one ChunkCollection and one ChunkRequester; queries all WorldObserver components registered in its Engine context.
- Expands each observer's XYZ box, unions coordinates, and orders missing requests by **minimum distance to an interested observer**; updates priorities when observers move.
- Uses a configurable **maximum number of concurrently in-flight Task requests**, replenishing slots as they complete (not rigid wait-for-entire-pack batches).
- Tracks coordinates with task status, request timestamp, and monotonically new request ID/version; timeout allows superseding request without requiring old-task cancellation. Ignore late completions when version mismatches or coordinate no longer desired.
- Completion callbacks never directly edit live scene state. Accepted Task results are inserted with `collection.addChunk(coordinate,move(chunk))` on the Engine update thread.
- Remove **every chunk outside all active interests**; no LRU or max-retained-chunk cache. Respect successful-null results so finite-world boundaries do not continuously retry.
- Same design works with one observer in Graphics and multiple observers on a headless server.

## Required tests
- Two overlapping observers => union, no duplicate requests, nearest-of-interested priority.
- Throttling and slot refill; observer movement reprioritizes pending queue; request transitions/retry after timeout.
- Task from generation 1 completes after generation 2: result 1 ignored; leaving/reentering interest cannot accept stale responses.
- Null/no-chunk and failed request are not confused, unloaded chunks disappear, no-observer case empties loaded set.
- Worker cannot cause collection or Entity3D mutation outside Engine update.

## Exclusions
No Graphics, network protocol, auto-save, cross-world UUID, cancel Task implementation or gameplay entity persistence.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
