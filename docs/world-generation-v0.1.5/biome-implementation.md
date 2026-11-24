# Biome implementation — review branch

This branch is an implementation proposal, including decisions that were previously open.
The design branch remains unchanged. No merge is authorized by this record.

## Layers

- World::Biome / World::SurfaceBiome: identity and continuous absolute height.
- World::SurfaceGrid<T>: pooled contiguous 2D samples, inclusive bounds, floor access.
- World::HeightMap: named Perlin layers, mutable construction grids, nearest/bilinear sampling.
- World::SurfacePass<TSurfaceData>: abstract construction steps, receiving only X/Z.
- World::SurfaceGenerationPass: owns child passes and one entry per active horizontal column.
- Voxel::Biome: ordered strata, weighted/Perlin/custom material selection, finite/infinite thickness.
- Voxel::BiomeGenerator: ready-to-use abstract generator; implement _computeBiomePlacement.
  Additional surface steps run after weighted height combination and before strata publication.
  Existing PerlinTerrainPass and existing golden tests are not replaced.

## Lifetime and threading

InterestPlan compares sorted coordinate keys; retains precede releases. Requester hooks
are optional. A failing retain must leave its own state unchanged; prior retains are
rolled back. Release and endJob are noexcept. Generator and requester must outlive the
streamer and all submitted jobs. Configure the pipeline before observing or submitting
jobs. Biome definitions, callbacks and height sources must remain immutable and alive
for the duration of use; hooks running across different columns must be thread-safe.

Interest and job reservations are independent counts. LocalChunkRequester reserves before
submitting to WorkerPool; direct generate also reserves. A per-column mutex serializes
construction; unrelated columns compute independently. An exception publishes nothing,
and the next attempt retries. A timeout does not cancel a worker or release its reservation.
After all interest and job reservations end, the map entry is erased. A result handle
may retain the data independently until its last consumer releases it. Grids are
non-copyable, so Pool::Lease cannot deep-copy them. Idle grid allocations are pooled.
No LRU or trimming is introduced.

## Selected open-question resolutions

- Both grid kinds use inclusive endpoints, floor direct access, identical world origins.
  Influence and blended-height sample spacing are the same; voxel strata are sampled at
  unit spacing and consume the floor-selected influence cell. Configuration is fixed
  during all active interests/jobs. No runtime invalidation API yet; detach and drain
  before changing definitions. Float world positions retain the existing Perlin precision limit.
- Height blending evaluates only positive influences, uses double accumulation, rejects
  missing positive contributions and non-finite heights. Custom edits run inside the
  construction pipeline, before the result is published as shared_ptr<const SurfaceData>.
- Voxel height is floored. Depth zero means the top voxel. Finite strata have positive
  integer thickness; the final stratum may be infinite. Missing finite layers leave the
  builder unchanged; solid strata overwrite the corresponding existing voxels.
- Incompatible dominant biome throws using a dynamic cast once per column sample, not per voxel.
  Material weights are finite/nonnegative, air and out-of-range packed IDs rejected.
  Independent hash-based weighted selection, Perlin selection and custom [0,1] selectors
  are supported; thickness noise is independent. Registry compatibility is checked by
  the existing volume builder.
- Pool tiers are next powers of two; zero and overflow are rejected; no trimming.

## Visual review

Five GPU-rendered cases are registered in BiomeWorldGolden: smooth overview, hard
boundaries, top-down distribution, visible strata/cliffs, and custom height modification.
They use the existing OpenGL scene and palette, generate two vertical chunks per column,
and save actual PNGs to the normal Graphics results directory. Missing expected images
deliberately fail rather than auto-approve a baseline. Review actual captures, then
promote the five PNGs to tests/TU/Graphics/resources/expectedImages/rendering/biome-world/.
Do not substitute fabricated images for rendered baselines. Existing CI uploads results.
