# PR09 — Headless multi-observer and pluggable requester validation

**Layer:** Core/integration. **Dependencies:** PR06, PR07. **Status:** design draft.

## Scope
- Exercise one EngineHeadless with multiple independently moving Entity3D players each bearing WorldObserver, plus entities lacking the component.
- A fake remote requester returns chunks with shuffled/delayed completions; optionally emulate no-chunk outside a finite domain, failed Task and timeouts. Do **not** add a new networking protocol.
- Verify union of observers, minimum interested-distance prioritization, deduplication, request supersession and immediate unload.
- Document extension path for later multiple worlds/UUID without implementing it.

## Required tests
- Two observers share chunks, move apart, reunite and detach; no missing shared chunks or incorrect early unload.
- Late old response after timeout and after exit/re-entry rejected.
- Stress test concurrent completions and Engine-thread publication; no invalid Entity3D accesses.
- Stable deterministic voxel sample through fake requester across different ordering.

## Exclusions
No multiplayer transport, player network synchronization, world UUID plumbing, save system or authoring of city/road algorithms.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
