# PR03 — Generator<TChunk,TGenerationData> and ordered virtual passes

**Layer:** Core. **Dependencies:** PR01. **Status:** design draft.

## Scope
- Inheritable `Generator<TChunk,TGenerationData>` exposing user-overridable `createChunk(ChunkCoordinate)` and `createGenerationData(ChunkCoordinate)`.
- Nested polymorphic `Generator::Pass` with virtual `void apply(TChunk&,TGenerationData&)`; generator owns a configurable ordered list of passes, optionally assembled by a Builder if it matches current Sparkle conventions.
- Fresh generation data on **every** request, potentially containing mutable references to long-lived generator-owned seeds, settings, noise samplers, caches and plans.
- Passes may only update temporary data, or also change the chunk; ordering within a generation is fixed.
- `createChunk` can return null (finite-world out-of-bounds) and then passes are skipped. Errors propagate without publishing partially generated chunks.
- The user is responsible for thread safety of mutable shared data and for deterministic rules; no framework-wide mutex or imposed seed derivation scheme. Multiple seeds configurable.

## Required tests
- Pass order, work-data lifetime and identity, null chunk behavior and exception safety.
- Two independent generation calls get separate local workspaces but can reference a shared mock generator-owned state.
- A deterministic example pipeline produces equal results for reversed generation order and for different scheduling using a user-controlled synchronized cache.

## Exclusions
No terrain/biome/city/road/cave algorithm, requester, Task scheduler or generic RegionPlan class.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
