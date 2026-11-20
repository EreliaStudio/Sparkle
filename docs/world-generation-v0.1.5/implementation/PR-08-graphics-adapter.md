# PR08 — Chunk graphics integration and voxel synchronization

**Layer:** Graphics only. **Dependencies:** PR02, PR07. **Status:** design draft.

## Scope
- A RenderingSystem/adaptor binds a ChunkCollection in a graphical Engine. On bind, it iterates **all existing entries** and synchronizes their existing Entity3D.
- Subscribe to the collection's one immediate chunk-changed event; callback only inserts the coordinate into a dirty `std::set`. Synchronize each pending coordinate at most once just before render, finding/reusing components on its Entity3D. Binding enqueues all existing coordinates.
- For voxel example: rebuild/reuse geometry via `Voxel::Mesher` and bind existing `Voxel::Renderer` with Geometry and Palette through current APIs. No fictional direct Volume::Renderer binding.
- Chunk removal destroys Entity3D and its components. Renderer observes its `spk::LifeCycle::Destruction` subscription, releases only separately owned graphics resources and queues the six neighbors for the next render; the chunk's coordinate was cached beforehand. No second collection removal event.
- Rebake affected geometry and neighbors when boundaries change. Skip coordinates absent by render time; detach all lifecycle and collection subscriptions when unbound or Engine context mismatches. Rebinding is always explicit.

## Required tests
- Bind renderer after chunks loaded: all visible and components created once.
- Repeated replacements reuse the same entity/component; multiple notifications before one render cause one synchronization, and absent dirty coordinates are safely skipped.
- Neighbor load/unload and translucent/opaque boundary tests; angled-camera golden images using Sparkle internal comparator.
- Generic Core build remains independent from Graphics; headless use has no rendering resources.

## Exclusions
No custom rendering engine, no forced 1:1 chunk-to-mesh relationship, no graphics dependencies in Chunk base or ChunkCollection.

## Delivery policy

Draft only; implementation requires maintainer approval. Target `V0.1.5` in a dedicated PR. C++23, clang-format, Windows/Linux CI, tests appropriate to layer. No automatic merges and no unrelated API rewrite.
