# Biome implementation — review branch

This branch is an implementation proposal, including decisions that were previously open.
The design branch remains unchanged. No merge is authorized by this record.

## Layers

- World::Biome / World::SurfaceBiome: identity and continuous absolute height.
- spk::Grid2D<T>: generic contiguous 2D storage with Vector2UInt dimensions, a static CapacityPool per element type, and RAII leases. Releasing a lease clears live elements while retaining capacity. Checked at() and operator[] accept Vector2UInt coordinates. Row-major iteration exposes read-only coordinates and mutable or const values through structured bindings (auto&& [coords, value]).
- World::SurfaceGrid<T>: composes a Grid2D lease with origin, scale, inclusive bounds and floor access. sampleSize() returns the number of samples on each axis, distinct from the physical extent in Configuration::size. Specialized surface wrappers reuse a regular Pool; releasing the wrapper immediately returns its storage lease. SurfaceGrid owns the Floor/Nearest/Bilinear interpolation enum and a protected weighted sample traversal shared by specialized grids.
- World::HeightMap: named Perlin layers, mutable construction grids, floor/nearest/bilinear sampling.
- World::SurfacePass<TGenerationData>: X/Z operations receiving the full generation data; specialized passes access their inputs and destinations via coordinate-aware hooks.
- Voxel::BiomeGenerationData: owns persistent column entries and request-local voxel builders; the ordinary voxel column pass only orchestrates construction.
- Voxel::Biome: ordered strata, weighted/Perlin/custom material selection, finite/infinite thickness.
- Voxel::BiomeGenerator: ready-to-use abstract generator; implement _computeBiomePlacement.
  Additional surface steps run after the selected height combination and before strata publication.
  Existing PerlinTerrainPass and existing golden tests are not replaced.

## Lifetime and threading

Generator owns a single persistent GenerationData instance, without imposing a lock,
spatial hierarchy or eviction policy. Each derived generator decides how to initialize,
synchronize and clean its data. Streaming requests no longer retain or release generator
data. The generator must outlive all submitted jobs.

Voxel::BiomeGenerationData owns two maps directly: ColumnData keyed by X/Z and
ChunkData keyed by the full chunk coordinate. Both types are nested in the generation
data. ChunkData contains the request-local Volume::Builder; ColumnData contains
influences, heights and strata. There are no shared pointers or separate collection
types for generation data.

The map mutex protects insertion, lookup, cleanup and flush. A mutex per column
serializes its first construction without blocking other columns. The concrete generation data runs surface passes under the column lock and marks the grids available only after all passes succeed; failed attempts clear partial grids. Failed builds are retryable. Only one active generation per chunk coordinate
is supported; a duplicate preparation throws without modifying the existing entry.
Distinct coordinates may be generated concurrently.

Active chunks protect their X/Z column from eviction. References to chunk or column
data are valid only until the calling chunk is cleaned. The generator calls cleanup
after successful preparation, on success or failure; preparation itself must be
exception-safe. Surface callbacks may access the current X/Z column through the construction-only accessors but must not recursively prepare columns. Queued jobs
need no reservations. Completed columns remain cached until flush(), which derives
the active X/Z keys from the chunk map and evicts only unused columns. A duplicate
streaming request after timeout can fail temporarily; retry policy is unchanged.
Flush returns leases to their pools, not necessarily memory to the OS.

Configure passes before submitting jobs. Biome definitions, callbacks and height
sources must remain immutable and alive throughout use. Drain workers and flush
before changing settings or definitions. No LRU or automatic eviction is implemented.

## Selected open-question resolutions

- Both grid kinds use inclusive endpoints, floor direct access, identical world origins.
  Influence and blended-height sample spacing are the same; voxel strata are sampled at
  unit spacing and use explicit bilinear dominance. Configuration is fixed
  during all active jobs. Drain workers and flush before changing definitions. Float world positions retain the existing Perlin precision limit.
- Height combination is explicitly selected at construction: Weighted sums normalized
  positive influences with double accumulation; Dominant evaluates only the biome with
  the greatest positive weight (first slot wins ties). Both reject missing contributions
  and non-finite results. This choice is independent of spatial influence interpolation. Custom edits run inside the
  construction pipeline, before the column is published for read-only access.
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
Empty cells remain empty. The Dominant mode belongs to height combination only.
Stored and interpolated influences use Influence::Cell<N>, which owns a fixed-size
std::array and exposes dominant() independently of spatial sampling. Grid no longer
provides dominant(); both the height and strata passes use the cell method.
Exact ties retain first positive encounter order.
Seven raw-noise PNG cases isolate the scalar field, threshold, octaves, cubic
interpolation, 3D slices and optional coordinate warping from the voxel renderer.
Existing terrain goldens affected by this algorithm change must be refreshed
from actual test outputs only, with unchanged comparator tolerances.

### Generator contract (current PR)

- `Generator<TChunk, TGenerationData>` owns persistent generation data and has no Builder template parameter or spatial hierarchy.
- Optional `_prepareGenerationData(coordinate, data)` returns false for an absent chunk; it must leave no request-local state on rejection or failure.
- Chunk passes receive only coordinate and mutable data; `_instantiateChunk(coordinate, data)` constructs the final chunk.
- `_cleanGenerationData(coordinate, data) noexcept` runs exactly once after successful preparation, including pass and instantiation failures.
- Generic SurfaceGenerationPass and streaming retention hooks are removed. SurfacePass receives the complete TGenerationData and remains a reusable XZ operation without its own cache or cleanup policy.
- Voxel data owns its column cache and request-local Volume::Builder, accessed with `data.builder(coordinate)`. Generation data is private to the generator and passed to hooks and passes; no public accessor is provided. `Generator::flush()` is available only when the data provides `flush()`, and forwards directly to it; voxel data releases idle cached columns.

### Validation after persistent-generation refactoring

CI run 45636410109 compiled both Graphics configurations and executed 1,369 tests
per configuration. Debug and Release each reported the same 43 golden failures:
7 PerlinNoiseGolden, 13 PerlinWorldGolden and 23 BiomeWorldGolden. All remaining
Graphics tests passed. Core unit and stress tests and AddressSanitizer checks
passed on both platforms. LLVM coverage failed on the same golden comparisons;
the separate MSVC Debug compatibility build exceeded the COFF section limit.

Commit f6e54b170406acd147cdb9232aa09dcb85075c69 adds /bigobj to the MSVC
graphics test target only. Commit 456b3612ce9275d3b7f53256b79282ef332c8406
applies clang-format 21.1.0. Commit 7a85d98a249ea0f0b867c26a5ddfb544227554ba
refreshes the 43 affected references from CI 2417. No comparator changes were made.
The refreshed references still require a successful independent Graphics run.

### Visual review follow-up

The hard-boundary overview hid the transition behind the elevated mountain biome.
The hard, smooth and custom-height overview cameras now share a more distant
view from the negative X/Z lowland side, so the transitions can be inspected
without changing terrain generation or the lighting setup. Their three expected
PNGs must be refreshed from actual CI captures after visual review, with the
existing comparison tolerances. The reported incomplete-looking shadows are
explicitly deferred to PR #102; no shadow implementation or lighting settings
are changed in this PR for that issue.
