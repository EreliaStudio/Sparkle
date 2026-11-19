# Sparkle v0.1.5 — Proposed implementation PR roadmap

> **Documentation only, no coding authorized yet.** This replaces the previous 13-PR roadmap, which incorrectly mandated region planners, terrain/biome algorithms, settlements, roads and caves. The old issue series #78–#83 stays withdrawn.

Deliver a functioning **headless framework first**, then a voxel specialization and optional visual integration. PR briefs below are proposed slices, subject to explicit maintainer approval individually. No automatic merges.

| PR | Deliverable | Dependencies | Layer |
| --- | --- | --- | --- |
| [01](implementation/PR-01-chunk-coordinates.md) | Minimal inheritable chunk with world-unit dimensions and static signed XYZ conversions | — | Core |
| [02](implementation/PR-02-chunk-collection.md) | Sorted ChunkCollection, headless Entity3D ownership, add/replace/remove, iteration and single coalesced sync notification | 01 | Core |
| [03](implementation/PR-03-generator-passes.md) | Generator<TChunk,TData>, TChunk::Builder, fresh builder/workspace, ordered passes then final build; user seeds and persistent references | 01 | Core |
| [04](implementation/PR-04-chunk-requester.md) | Task-backed ChunkRequester interface and local generator implementation, null/failure differentiation | 03 | Core |
| [05](implementation/PR-05-interest-observer.md) | ChunkInterest radius and WorldObserver Component3D using the existing Engine-context Registry | 01 | Core |
| [06](implementation/PR-06-streaming-system.md) | Multi-observer union, nearest-first scheduling, request cap/timeout/version, update-thread publication, immediate unload | 02, 04, 05 | Core |
| [07](implementation/PR-07-voxel-example.md) | Concrete VoxelChunk and simple deterministic user-style generation passes, headless | 03, 06 | Voxel Core/example |
| [08](implementation/PR-08-graphics-adapter.md) | Graphics-bound chunk renderer: bind-time scan, reusable components, volume-to-geometry pipeline, neighbor invalidation | 02, 07 | Graphics |
| [09](implementation/PR-09-headless-multiobserver.md) | Server-like multi-observer scenario, fake remote requester, crossing borders, latency/timeout tests | 06, 07 | Core/integration |
| [10](implementation/PR-10-integration-validation.md) | End-to-end demo and regression/CI validation of framework, voxel sample, update and rendering | 07–09 | Integration |

## Milestones

- **A — Core data model:** 01–02. Chunk has no coordinate or rendering dependency. Ordered storage and entity lifetime verified.
- **B — Generation and acquisition:** 03–04. A concrete `TChunk::Builder` and ordered passes construct the final chunk headlessly without partial publication, with a swappable Task-based requester.
- **C — Streaming:** 05–06. One or multiple players drive chunk demand, scheduling and removal through EngineHeadless.
- **D — Concrete example and Graphics:** 07–08. Existing Voxel::Volume and rendering components are reused. Bind after chunks exist and replace volumes safely.
- **E — Validation:** 09–10. Multi-observer server-like usage, cross-chunk coherence, timeout/stale response behavior, visual updates and goldens.

The user-defined voxel sample may demonstrate a simple continuous terrain plus one multi-chunk feature. **It does not require shipping production-grade default terrain, cities, roads or caves.**

See [architecture decisions to finalize](architecture-review.md) before approving implementation signatures.

## Ground rules

Feature PRs target `V0.1.5` after design approval, not this draft docs branch. Use C++23, clang-format, Core and Graphics test infrastructure, Windows/Linux CI, Sparkle's built-in golden comparator. No paid APIs or external game servers. Do not introduce networking protocols or unrelated framework rewrites. Never merge without explicit approval.
