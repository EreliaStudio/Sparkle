# PR03 — Generator<TChunk,TGenerationData> and ordered virtual passes

**Layer:** Core. **Dependencies:** PR01. **Status:** design draft.

## Scope
- Inheritable `Generator<TChunk,TGenerationData>` deriving the mutable construction type from **`typename TChunk::Builder`**, not from a third template argument. User-overridable `createChunkBuilder(ChunkCoordinate)` and `createGenerationData(ChunkCoordinate)`.
- Nested polymorphic `Generator::Pass` with virtual `void apply(TChunk::Builder&,TGenerationData&)`; generator owns a configurable ordered list of passes, optionally configured through a separate generator Builder if that matches Sparkle conventions. Do not confuse the generator configuration Builder with `TChunk::Builder`.
- Fresh **chunk Builder and generation data** on every request, potentially containing mutable references to long-lived generator-owned seeds, settings, noise samplers, caches and plans.
- Passes may only update temporary data, or also modify the **chunk Builder**; ordering within a generation is fixed. They never operate on a published TChunk.
- `createChunkBuilder` can return null (finite-world out-of-bounds) and then passes are skipped. When every pass succeeds, **`builder.build()` returns the finished TChunk by value** (including immutable Volume snapshots), consuming the Builder. A fresh `TChunk::Builder` supports **`load(const TChunk&)`** to initialize from a previously published chunk, as does `Voxel::Volume::Builder`. The Generator can move the value into a `unique_ptr<TChunk>` for the requester. Exceptions leave no partially generated chunk published.
- The user is responsible for thread safety of mutable shared data and for deterministic rules; no framework-wide mutex or imposed seed derivation scheme. Multiple seeds configurable.

## Required tests
- Pass order, fresh chunk Builder/workspace lifetimes, null-builder behavior, by-value final build and one-shot consumed semantics, `load(existingChunk)` without modifying the original, exception safety and absence of partially published chunks.
- Two independent generation calls get separate mutable builders and workspaces but can reference shared generator-owned state.
- A deterministic example pipeline produces equal results for reversed generation order and for different scheduling using a user-controlled synchronized cache.

## Exclusions
No terrain/biome/city/road/cave algorithm, requester, Task scheduler or generic RegionPlan class.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
