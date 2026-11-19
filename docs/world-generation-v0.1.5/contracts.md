# World-generation contracts and decisions (design draft)

## Invariants to test before implementation is accepted

**Deterministic generation**
- Same seed, settings, generator revision, global plan and spatial position
  yield bitwise-equal logical output across request order and supported
  Windows/Linux builds (including negative positions).
- Per-stage random streams are derived from stable identifiers; no
  iteration-order-dependent unordered containers are used for decisions.
- Floating-point sampling needs a documented reproducibility policy:
  bitwise cross-platform output where feasible, or an explicit fixed-point/
  quantized boundary for persisted terrain categories.

**Spatial correctness**
- Negative world positions map to exactly one chunk and local cell.
- Chunk dimensions are positive and total element counts are overflow checked.
- Every cross-region feature has one stable identity and may be queried from
  every intersecting chunk; no seams or duplicate structures.
- Large worlds use checked 64-bit coordinate arithmetic and bounded queries.
- Intersecting roads, buildings, scenery and caves have deterministic
  tie-breaking and placement exclusions.

**Ownership**
- Generated metadata is not the authoritative voxel data.
- Generic chunk payloads are owned by the generic runtime; voxel payloads
  are existing immutable Voxel::Volume snapshots.
- A requester may be local generation, disk or network, but no networking
  mechanism is implemented by the generation framework.
- Request completion is applied by the owning update loop; background
  workers do not mutate a live chunk collection or GPU state.
- Eviction cannot silently discard persistent user edits.

**Public API**
- Prefer small concrete types and value semantics over mandatory virtual
  factories and many framework classes.
- Use builders when assembling a meaningful mutable recipe or stage pipeline,
  not for trivial values with no invariants.
- Generic code does not depend on Voxel. Voxel Core does not depend on
  Graphics, and no generation module depends on OpenGL.
- Errors, cancellation, duplicate requests and invalid coordinates must
  have defined observable behavior.

## API sketches for review — not final signatures

An idiomatic result could look conceptually like:

~~~cpp
// Generic planning (names are illustrative, not committed):
World::Plan<WorldMeta> plan = planner.build(seed, bounds, settings);
auto descriptors = plan.featuresIntersecting(chunkBounds);

// Voxel realization (reuse current Sparkle API):
Voxel::Volume::Builder builder(chunkDimensions);
builder.bind(definitions);
voxelRecipe.realize(plan, chunkPosition, builder);
Voxel::Volume volume = builder.build();
~~~

It should be possible to use a generic generator without asking for a
Voxel::Volume, and to ask a voxel generator for a chunk without constructing
or rendering any Engine entities.

## Important unresolved choices for the maintainer

1. **Namespace:** spk::World, spk::Generation or another naming scheme?
2. **World metadata:** one typed immutable WorldPlan value, multiple typed
   RegionPlan values, or both? What is the minimal shared contract?
3. **Regions:** always 2D X/Z for surface worlds, or generic N-dimensional
   partitions with specialized 2D/3D views?
4. **Chunk data:** does a Chunk<TData> wrapper add enough meaning beyond
   (coordinate, TData, revision), or can Collection own that directly?
5. **Generation composition:** procedural function pipeline, compile-time
   stages, explicit Builder registry, or a small type-erased stage list?
6. **Feature conflicts:** fixed phase precedence plus stable feature IDs,
   or rule-driven reservations? How should bridges and tunnels override it?
7. **Generator ownership:** how are dependencies/region plans cached, and
   is the plan bound to the generator or passed as a stable input?
8. **Threading:** expose synchronous generation as the core primitive and
   put async acquisition in Requester, or support async stages internally?
9. **Existing edits:** store sparse overrides or copy-on-write chunks?
   This decision affects persistence and regeneration but should not force
   a second dense voxel representation.
10. **Release line:** which advanced algorithms (caves/cities/roads) must
    be shipped as reusable defaults, versus examples and extension hooks?

Recommended first pass: approve decisions 1–4 and the deterministic
contract, then implement one minimal end-to-end vertical slice before
finalizing the full public hierarchy.
