# Biome implementation — review branch

This branch is an implementation proposal, including decisions that were previously open.
The design branch remains unchanged. No merge is authorized by this record.

## Layers

- World::Biome / World::SurfaceBiome: identity and continuous absolute height.
- spk::Grid2D<T>: generic contiguous 2D storage with Vector2UInt dimensions, a static CapacityPool per element type, and RAII leases. Releasing a lease clears live elements while retaining capacity. Checked at() and operator[] accept Vector2UInt coordinates. Row-major iteration exposes read-only coordinates and mutable or const values through structured bindings (auto&& [coords, value]).
- World::SurfaceGrid<T>: composes a Grid2D lease with origin, scale, inclusive bounds and floor access. sampleSize() returns the number of samples on each axis, distinct from the physical extent in Configuration::size. Specialized surface wrappers reuse a regular Pool; releasing the wrapper immediately returns its storage lease. SurfaceGrid owns the Floor/Nearest/Bilinear interpolation enum and a protected weighted sample traversal shared by specialized grids.
- World::HeightMap: named Perlin layers, mutable construction grids, floor/nearest/bilinear sampling.
- World::SurfacePass<TSurfaceData>: abstract construction steps, receiving only X/Z.
- World::SurfaceGenerationPass: owns child passes and one entry per active horizontal column.
- Voxel::Biome: ordered strata, weighted/Perlin/custom material selection, finite/infinite thickness.
- Voxel::BiomeGenerator: ready-to-use abstract generator; implement _computeBiomePlacement.
  Additional surface steps run after the selected height combination and before strata publication.
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
non-copyable, so Pool::Lease cannot deep-copy them. Idle grid allocations are pooled by element type; wrapper objects are pooled independently. The dominant biome interpolation mode must be specified explicitly.
No LRU or trimming is introduced.

## Selected open-question resolutions

- Both grid kinds use inclusive endpoints, floor direct access, identical world origins.
  Influence and blended-height sample spacing are the same; voxel strata are sampled at
  unit spacing and use explicit bilinear dominance. Configuration is fixed
  during all active interests/jobs. No runtime invalidation API yet; detach and drain
  before changing definitions. Float world positions retain the existing Perlin precision limit.
- Height combination is explicitly selected at construction: Weighted sums normalized
  positive influences with double accumulation; Highest evaluates only the biome with
  the greatest positive weight (first slot wins ties). Both reject missing contributions
  and non-finite results. This choice is independent of spatial influence interpolation. Custom edits run inside the
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
deliberately fail rather than auto-approve a baseline. The five reference PNGs are now
committed under tests/TU/Graphics/resources/expectedImages/rendering/biome-world/.
They are byte-identical to the actual Graphics Release captures from CI run 45492661433.
These references remain subject to visual review; existing CI uploads results.

## Expanded validation

29 BiomeWorldGolden scenarios (5 initial + 24 additional):
- Exposed horizontal strata: low/high frequency Perlin, weighted speckles, custom
  bands, three weighted materials, variable thickness and infinite deep Perlin.
- Cutaways: variable thickness, independently seeded material and thickness Perlin,
  two independently distributed layers, finite strata and a single infinite stratum.
- Terrain: layered height noise, negative elevation, tall terrain, surface scales
  0.5/2/4, noisy biome islands, three-way blend, equal-weight tie, checkerboard,
  negative coordinates with reversed generation order, custom terraces.
- The cutaway fixtures only erase generated voxels to expose the real interior.
  No material or layer is generated by the visualization itself.
- Core tests compare exact material thresholds, thickness values, deterministic
  weighted output, entire vertical stacks across negative coordinates, finite and
  infinite layers, invalid settings, scale-dependent sampling counts, queued local
  requester lifetime, neighboring height grid endpoints, bilinear interpolation,
  recycled influences, height-combination failure atomicity and standalone height
  pass binding.

All additional references must originate from the actual Sparkle Graphics test
outputs. Existing reference PNGs are not replaced by this extension.

The 24 additional reference PNGs were imported from the Graphics Release captures in
CI run 45524081345. All 29 BiomeWorldGolden cases passed against the resulting
references before the import commit 705bd538ed6717d7f79a1e996bb38cd5f6a2e694.

## Noise and influence continuity

Perlin now uses 16 uniformly spaced unit gradients in 2D and 32 symmetric unit
gradients in 3D. Quintic interpolation is the default; Linear and SmoothStep
remain selectable. The seed and permutation remain deterministic, but existing
procedural worlds will change with the revised gradients and default fade.
DomainWarp is optional and does not silently alter any existing pipeline. It owns
three independently configurable Perlin fields and exposes warp1D, warp2D and
warp3D. Each axis samples the original input position at the requested dimension;
the 2D result is unchanged by the addition of the third field.
Influence grids interpolate by biome identity, not slot index, preserving up to
four cells' distinct biomes. Floor and Nearest copy the selected cell; Bilinear
combines neighboring influences without discarding positive contributions.
Both height and influence grids use SurfaceGrid's weighted traversal. Aligned
bilinear queries read only one cell at a grid node, or two cells on a grid line.
Height interpolation accumulates weighted samples in double precision before
converting to float; a single contributing sample is returned unchanged.
Empty cells remain empty. The Highest mode belongs to height combination only.
Stored and interpolated influences use Influence::Cell<N>, which owns a fixed-size
std::array and exposes dominant() independently of spatial sampling. Grid no longer
provides dominant(); both the height and strata passes use the cell method.
Exact ties retain first positive encounter order.
Seven raw-noise PNG cases isolate the scalar field, threshold, octaves, cubic
interpolation, 3D slices and optional coordinate warping from the voxel renderer.
Existing terrain goldens affected by this algorithm change must be refreshed
from actual test outputs only, with unchanged comparator tolerances.
