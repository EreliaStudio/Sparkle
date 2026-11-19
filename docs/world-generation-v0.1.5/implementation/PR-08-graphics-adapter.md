# PR08 — Chunk graphics integration and voxel synchronization

**Layer:** Graphics only. **Dependencies:** PR02, PR07. **Status:** design draft.

## Scope
- A RenderingSystem/adaptor binds a ChunkCollection in a graphical Engine. On bind, it iterates **all existing entries** and synchronizes their existing Entity3D.
- Subscribe to **the single chunk synchronization event**. `synchronizeChunk(chunk,entity)` finds an existing rendering component or adds it, then refreshes its data.
- For voxel example: rebuild/reuse geometry via `Voxel::Mesher` and bind existing `Voxel::Renderer` with Geometry and Palette through current APIs. No fictional direct Volume::Renderer binding.
- Chunk removal destroys the corresponding Entity3D and components via ChunkCollection, not via a special rendering-remove callback.
- Correctly rebake chunk/adjacent neighbors when boundary geometry is affected by insertion, replacement or unloading. If a separate neighbor invalidation hook is needed, contain it inside Graphics integration without introducing a second mandatory World event.

## Required tests
- Bind renderer after chunks loaded: all visible and components created once.
- Repeated replacement reuses same entity/component, updates geometry; unchanged chunk not unnecessarily rebuilt.
- Neighbor load/unload and translucent/opaque boundary tests; angled-camera golden images using Sparkle internal comparator.
- Generic Core build remains independent from Graphics; headless use has no rendering resources.

## Exclusions
No custom rendering engine, no forced 1:1 chunk-to-mesh relationship, no graphics dependencies in Chunk base or ChunkCollection.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
