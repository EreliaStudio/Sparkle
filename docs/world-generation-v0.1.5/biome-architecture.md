# Sparkle v0.1.5 — Biome architecture: design record and validated principles

> **Status:** evolving design record. Validated decisions are explicitly identified; proposals and C++ signatures remain non-normative until approved.
> **Open questions:** see [biome-open-questions.md](biome-open-questions.md), maintained separately so later discussions can address each issue without implying a decision.
>
> **Branch:** `design/v0.1.5-biome-architecture` (based on `V0.1.5`).
>
> **Scope:** capture the biome-design discussion and its confirmed choices as of 2026-11-24. All API names below are conceptual unless they already exist in Sparkle. No code implementation, PR, or merge is authorized by this document.

## 1. Objective and guiding principle

Sparkle must provide reusable tools for placing, defining, and applying biomes without becoming an opinionated game-world generator. Games own their world-generation policies, but should be able to reuse biome definitions and basic passes.

A biome is an *environmental definition*, not the algorithm that distributes biome regions across the world. The distribution and application phases must be independent: different placement methods should emit the same kind of result, and downstream generation should not care which placement method produced it.

The user specifically wants:
- A first **biome placement pass** writing biome assignments into temporary generation data.
- Subsequent passes that **apply biome-driven height generation and voxel strata**, with later extension points for scenery and structures.
- Multiple interchangeable placement strategies, including manual layouts, Voronoi, environmental suitability, and competing noise fields.
- A reusable, representation-agnostic `spk::World` layer, **plus first-class, ready-to-use voxel-world building blocks** in `spk::Voxel`, with game-specific extensions where needed. Voxel is a primary intended Sparkle use case, not a restriction of `spk::World`.

Do not infer a fully finalized C++ API from these principles. References to voxel or particular spatial algorithms are welcome as examples and specialized components, but must not make the generic layer depend on them.

## 2. Three concrete target games

These cases are architectural tests, not engine-predefined biome types.

### Game A — Finite, tiered/plateau world

- Finite world generated once, deterministically from a seed; terrain is not intended to be editable.
- Several regions at discrete levels, commonly 3–4 voxels high, with cliffs and staircases at changes in elevation.
- A predefined number of chunk-sized/voxel regions, each with a biome, base elevation, ground strata, and eventually scenery, buildings, and towns.
- Chunk-scale ("chunky") biome assignment must be possible.

### Game B — Large floating island

- A large volume of terrain floating in void, containing dungeons, forests, swamps, mountains, etc.
- Each island has a main biome with a predefined collection of sub-biome variations.
- Placement and local variations should preserve the main biome's identity while allowing variety.
- A conventional heightmap does not fully represent floating-island undersides, caves, or overhangs; separate 3D/volumetric generation must remain possible.

### Game C — Minecraft-like world

- Streaming/deterministic voxel terrain; biome shape and ground materials must differ significantly.
- Examples include relatively flat deserts, canyon/mesa plateaus, and narrow, steep Chinese-style mountain pillars.
- Some terrain borders should be hard (cliffs), others smooth.

**Cross-case requirement:** terrain-shape algorithms, biome placement, biome content, and eventual decoration/structure generation must not be fused into a single monolithic algorithm.

## 3. Conceptual architecture

### 3.1 Generic world layer

- `spk::World::Biome`: an **empty generic base**, with no mandatory identifier or environmental fields. A polymorphic destructor may be needed, but its API has not been finalized.
- `spk::World::SurfaceBiome`: a generic specialization of `Biome` exposing a pure virtual `float groundHeight(const spk::Vector2 &worldPosition) const` (world X/Z). The biome chooses how to calculate its absolute continuous ground height. A separate `TerrainHeightGenerator` interface is no longer a mandatory dependency of the biome or combination pass; whether it has independent utility remains open (C1).
- A height generator evaluates the **absolute world elevation** for an X/Z position. Base altitude is a property of the generator's configuration, **not** a value that the biome is required to add.
- The generic placement concept associates spatial positions with biome definitions; no mandatory registry or numeric biome ID.
- The generic biome/placement layer should not require climate, a particular mesh algorithm, or a particular ground representation. A separately defined, concrete `spk::World::HeightMap` now offers a Perlin-based generic height source and transient sampled grids (see §4.4). A `SurfaceBiome` may use a concrete `HeightMap`, a constant, or a custom algorithm internally. The exact efficient point-sampling API of `HeightMap` remains to be settled (C1/C6).

### 3.2 Voxel layer: first-class consumer of generic world data

- A proposed `spk::Voxel::Biome` (exact namespace/name remains open; `spk::Voxel::World::Biome` was also mentioned) inherits the generic biome and holds:
  1. For surface terrain, the `SurfaceBiome` height contract; no mandatory height-generator member. The precise voxel specialization/inheritance arrangement remains open.
  2. An **ordered collection of voxel ground strata**.
- Strata define ground composition; height generators define terrain elevation and shape. A biome therefore influences both the shape and the material appearance of generated terrain.
- Sparkle should provide directly usable voxel-world generation facilities, not force games to reimplement them. This is a specialized consumer layer: the generic distribution and height contracts remain independent of voxel representation. Voxel generation must not be tied to greedy meshing; other voxel meshing methods should be able to use the same biome descriptions, subject to representation-specific adapters.

### 3.3 Game-specific layer

- Games can derive biome types with additional properties or executable behavior for terrain/scenery/towns/dungeons.
- The desired future architecture includes hooks/extensions for game-specific biome operations. **The hook signature, invocation frequency, ordering, lifetime, and interaction with multi-chunk structures remain undefined.**
- It is not decided whether scenery/structure behavior lives in biome virtual hooks, independent passes, or both.

## 4. Confirmed decisions

The numbered entries preserve the design conversation. **Question 20 was explicitly revisited and validated after decisions 21–24.** Decisions 1–24 remain recorded, including historical wording about a single dominant assignment; the later multi-influence decision supersedes that storage assumption. HeightMap-specific decisions appear in §4.4. **A newly confirmed decision is that Sparkle, rather than each strategy, normalizes raw biome influence weights**; see §4.5.

### 4.1 Voxel strata (decisions 1–12)

1. **Surface-relative strata.** Ground composition is measured downward from the terrain surface, not from fixed world Y intervals.
2. **Multiple voxel types per stratum.** Each stratum holds a weighted collection of candidate voxel IDs.
3. **Configurable material distribution per stratum.** Initially envisioned as independent weighted selection, 2D noise-based distribution, or an extensible custom strategy.
4. **Variable stratum thickness.** A stratum has minimum and maximum thickness/depth; normalized 2D Perlin noise selects thickness spatially. Fixed thickness is the degenerate min=max case.
5. **Independent Perlin configuration.** Each stratum's thickness variation has its own Perlin settings (e.g. seed/frequency), instead of forcing one shared noise field.
6. **Discrete material composition at biome borders.** Each terrain column uses only the *dominant* biome's strata; mixing different biomes' stratum definitions is not required.
7. **Numeric voxel IDs.** Weighted materials are referenced by voxel ID; no mandatory material registry or object-reference resolution.
8. **2D material distribution.** Selection uses world X/Z; within a given stratum, the selected material stays consistent along that column's Y extent. Material-distribution noise and thickness noise are separate.
9. **Optional unbounded final stratum.** A biome may have a final infinite stratum or terminate at finite depth. Beyond a finite final stratum, the biome specifies no material; that does *not* imply air.
10. **Solid materials only.** Empty/air voxel ID 0 is not a candidate stratum material. Caves and voids are handled by other generation processes.
11. **Relative material weights.** Weights need not sum to 100%; they are normalized for selection. At least one usable positive weight is needed. Exact weight validation and distribution mapping remain to be specified.
12. **No special surface layer type.** The first stratum is the surface; snow and similar overlays may be addressed separately by later passes.

Example (illustrative only):

| Stratum | Thickness | Candidate materials (relative weights) | Distribution |
| --- | --- | --- | --- |
| Surface | 1 | Grass: 9, Dirt: 1 | 2D weighted/random or noise |
| Subsurface | 2–6 | Dirt: 8, Gravel: 2 | 2D noise |
| Deep | Unbounded | Stone: 9, Granite: 1 | 2D noise |

A stratum's thickness Perlin configuration is independent from its material-selection distribution.

### 4.2 Biome identity, references, and temporary placement (decisions 13–19)

13. **Empty generic biome base.** `spk::World::Biome` has no mandatory data members. Biome identifiers, if ever used, would be external.
14. **Definitions are shared.** One biome definition can be reused in many regions. A mandatory ID-based collection was considered, then explicitly rejected in favor of the original shared-definition option.
15. **Direct references rather than IDs.** Placement data refers to shared biome objects directly, with no ID-resolution requirement.
16. **External ownership.** The developer/world setup owns biome instances. Placement results store **non-owning references**; referenced objects must outlive all relevant generation work.
17. **Temporary per-chunk storage.** The placement pass writes its results into the existing `TGenerationData` of `spk::World::Generator`. An external, persistent Voronoi/world map may still be the placement algorithm's *input*, without requiring persistent biome output storage.
18. **2D spatial sampling, historically recorded as one dominant assignment.** The original design stored one biome reference per X/Z position. This single-reference **storage model is superseded as a design target** by the multi-influence proposal in §4.5; sampling positions remain world-space and independent of vertical chunk coordinates. A single dominant biome may still be derived from those influences for consumers that need one.
19. **Unassigned positions are allowed.** A cell with all biome pointers null denotes no biome assignment (A2 revises the earlier empty-list representation); there is no mandatory fallback biome. A4 requires a downstream pass that needs a usable biome to throw if none exists; other consumer-specific handling remains open.

**Important nuance:** A game can deliberately return the same influences for every position in a coarse area, but the generic contract must not impose region/chunk-aligned boundaries. A per-position sampled 2D result remains the design target; coarser sampling remains open; the fixed-capacity cell storage is now approved in A2.

### 4.3 Terrain height and transitions (decisions 20–24)

20. **Dedicated terrain-height pass.** After biome placement, a separate terrain-height pass evaluates the selected biome's `spk::World::TerrainHeightGenerator` (and any neighboring influences needed for placement-defined transitions), and writes an **absolute surface-height map into the current chunk's `TGenerationData`**. The subsequent voxel biome application pass reads that stored map to apply surface-relative strata. Computing height inside the voxel application pass is not the selected design. The height-map storage representation has since been refined to a transient `HeightMap::Grid` held through a pooled `Lease` (see §4.4); A4 requires an exception when a consumer needs a biome and no positive contribution exists; other consumer-specific cases remain open.
21. **Biome-selected height-generator instance.** Each voxel biome references its own `spk::World::TerrainHeightGenerator` via a common abstract interface; multiple biomes may share one instance. The terrain pass invokes the selected biome's algorithm.
22. **Absolute, continuous height.** The generic height-generation algorithm produces an absolute **continuous** world elevation, not an integer voxel altitude or only a relative offset added to a biome-owned base height. Base height belongs to generator configuration. Voxel consumers are responsible for conversion to discrete voxel coordinates; the conversion and rounding rule remain undecided.
23. **Configurable height transitions.** Hard boundaries and smoothly blended biome elevations must both be supported. **Material selection stays discrete** even when height is blended.
24. **Transitions are controlled by placement.** The biome placement system provides transition mode/distance or equivalent boundary/influence information to a terrain-height application pass. Individual height generators do not know about biome boundaries.

Height selection/blending and material selection are distinct operations. Hard transitions permit tiered cliffs; smooth transitions permit plains-to-mountains blending.

#### C2 — Surface biome height contract (validated 2026-11-24)

`spk::World::Biome` remains a generic base with no required terrain-height method. A new generic (non-voxel) `spk::World::SurfaceBiome` derives from it and exposes a pure virtual `float groundHeight(const spk::Vector2 &worldPosition) const` query, returning the absolute continuous elevation at world X/Z. Exact signature spelling follows Sparkle conventions. Each concrete surface biome is responsible for its own algorithm: it may use a `HeightMap`, a constant, or custom logic, with optional sharing or composition of height sources. No mandatory source member, adapter or registry is imposed. The combination pass consumes this contract and remains ignorant of the algorithm (C4). The historical mandatory `TerrainHeightGenerator` reference in decisions 20–21 is superseded by this interface. How a consumer obtains/checks the surface specialization from generic influence pointers remains open (C3/C5); the generic grid remains unchanged. The concrete `HeightMap` stays non-polymorphic; its point-evaluation API and any independent role for a separate abstract height source remain open (C1/C6).

The dimensionality of biome distribution is independent of terrain geometry: a future 2D distribution may feed a volumetric terrain generator, and a future 3D distribution may assign different biomes along one vertical column. Do not introduce a `VolumetricBiome`, density contract or 3D grid API now. These remain deferred under E5.

#### C4 — Weighted height combination (validated 2026-11-24)

Each contributing biome supplies its own absolute continuous terrain height `h_i(p)` at the same world-space position `p`, as though that biome were the only one present. The single generic combination rule is `H(p) = sum_i(w_i(p) * h_i(p))`, over the strictly positive normalized influences. For arbitrary relative weights the weighted mean would divide by their sum, but A3 already normalizes them; do not divide again by the number of biomes or by the grid capacity. Floating-point rounding follows A3. For example, heights 20 and 100 with normalized weights 0.75 and 0.25 yield 40.

The placement strategy controls the spatial evolution of the weights: smoothly varying weights can blend continuous terrain sources, while abrupt one-hot weight changes can yield hard transitions. No separate hard/smooth blending mode or transition parameter is required by the generic height combiner. The sources remain independent and unaware of neighboring biomes. Material strata still use only the dominant biome. Zero-weight and null slots make no contribution; if there is no positive influence, a height consumer that requires a biome throws under A4. The surface biome query is fixed in C2; reusable height-source integration (C1), orchestration (C3/C7), handling missing/invalid sources (C5), efficient evaluation (C6), and revalidation after downstream edits (E2) remain open.

### 4.4 HeightMap, Grid and generic generation pass — confirmed design (2026-11-24)

The later discussion supersedes older, exploratory alternatives elsewhere in this document. This section records **confirmed decisions**, not yet compiled or implemented code. Precise header/type names remain to be checked against existing Sparkle conventions.

#### 4.4.1 Separation of responsibilities

- `spk::World::HeightMap` is a **concrete, non-polymorphic procedural height source**, not an abstract base to derive for world-specific transformations.
- `HeightMap` owns a base altitude (default `0.0f`) and a **named associative collection** of independent Perlin noise layers. The collection is a `std::map<std::string, NoiseLayer>` (a collection, **not** an index-based vector/container); expose mutable and const references for direct insertion, lookup, modification and removal. No `addNoiseLayer()` API is necessary.
- Each `NoiseLayer` **owns a copy** of its `spk::Perlin`, together with `minimum` and `maximum` contribution bounds. Existing `spk::Perlin::sample2D(x, z, minimum, maximum)` supplies the remapping directly; do **not** modify `spk::Perlin` to give it persistent min/max values. An illustrative layer is `{perlin, -30.0f, 80.0f}`.
- Height at an absolute world X/Z position is `baseHeight + sum(layer.perlin.sample2D(worldX, worldZ, layer.minimum, layer.maximum))`. An empty collection yields flat ground at `baseHeight`. Height values stay continuous `float`; voxel-specific integer conversion belongs to voxel consumers.
- `HeightMap` is concerned only with **initial** height generation. Subsequent user-defined generator passes edit the resulting per-chunk `Grid`, e.g. lowering elevations toward the coast of a finite island. They do **not** mutate the procedural definition or require inheriting `HeightMap`.
- Configuration/layer edits are permitted **between** generation jobs, not concurrently with sampling from the same `HeightMap`.

#### 4.4.2 Grid::Configuration, world extent and resolution

`HeightMap::Grid::Configuration` exposes **only** the grid's two-dimensional world-space `size` and sampling `scale`. `size` means **world footprint/extent**, **not** the number of stored samples. `scale` means **world units between adjacent samples**, and must be strictly positive and finite. Configuration may be written illustratively as:

```cpp
struct Configuration
{
    spk::Vector2UInt size; // World-space extent in X/Z, not sample counts
    float scale = 1.0f;    // World-space spacing between samples
};
```

- **Both endpoints are always included.** The earlier `EndBoundaryExclusion` enum and its alternate boundary policies were explicitly **removed**. There is no optional outer-border mode.
- **Required invariant on each axis:** `size.x / scale` and `size.y / scale` must both be nonnegative **integers** (within a carefully specified floating-point comparison tolerance). Reject any configuration for which either quotient is fractional; **never** silently round it or introduce an uneven final spacing.
- **Sample counts:** `sampleCountX = size.x / scale + 1`, `sampleCountY = size.y / scale + 1`. This `+1` is **one full sample interval of `scale` in world coordinates**, not an assumed unit-distance sample. Its position coincides with the supplied world-space extent, not a position beyond it.
- **Valid local positions:** `[0, size.x]` and `[0, size.y]`, including both endpoints. Every `at()` overload rejects a position outside this rectangle; e.g. size=16, scale=1: 16 is valid; 16.1 throws. There is **no extended query region** beyond `size`.
- Examples per axis:
  - `size=16, scale=1` → `17` samples at `0,1,...,16`.
  - `size=16, scale=2` → `9` samples at `0,2,...,16`.
  - `size=16, scale=4` → `5` samples at `0,4,8,12,16`.
  - `size=16, scale=0.5` → `33` samples at `0,0.5,...,16`.
  - `size=16, scale=3` → **`spk::Exception`**, since `16/3` is not an integer.
  - `scale=0`, a negative or non-finite scale, invalid size, or overflow of sample counts/capacity → **`spk::Exception`**.
- Validation is performed at the beginning of **`HeightMap::generate()`**, **before** obtaining/allocating a pooled grid. Constructing a `Configuration` alone is not expected to validate it.

#### 4.4.3 Grid local-coordinate access and interpolation

All public `Grid::at()` coordinate arguments are **`spk::Vector2` local world-distance coordinates**, never `Vector2UInt` sample indices. No implicit conversion to world coordinates is exposed by these methods. Suggested API:

```cpp
float& at(const spk::Vector2& localPosition);
const float& at(const spk::Vector2& localPosition) const;

enum class Interpolation { Nearest, Bilinear };

float at(
    const spk::Vector2& localPosition,
    Interpolation interpolation) const;
```

- **No enum:** validate the local position lies in `[0,size]` on both axes; select sample index by `floor(localPosition / scale)` componentwise; return a **mutable or const reference** to the stored height. For instance with scale=1, `at({15.5, z})` aliases the sample at X=15, while `at({16, z})` aliases X=16. Exception on an out-of-range position.
- **`Nearest` enum:** validate the same local world-space domain; return a **`float` by value** corresponding to the mathematically nearest stored sample. Exactly half-way ties choose the **lower** sample index. If the operation somehow requires an unavailable sample, throw. With the now-always-included final sample and the valid `[0,size]` domain, normal in-range nearest queries have the required samples.
- **`Bilinear` enum:** validate the same domain; interpolate using existing adjacent stored samples and return a **`float` by value**. At an exactly aligned sample, including the final edge/corner, that sample's height is a valid result; **do not** require a nonexistent sample beyond the edge with zero interpolation weight.
- Every overload uses `spk::Exception` for invalid positions or missing required samples; no automatic clamping or extrapolation. Non-finite positions are invalid.
- Using a shared `Vector2` interface avoids exposing integer sample indices, while the no-enum overload remains suitable for mutating per-column/per-sample heights.

#### 4.4.4 Generation, ownership and pool tiers

The intended public generation API is:

```cpp
[[nodiscard]] Grid::Lease generate(
    const Grid::Configuration& configuration,
    const spk::Vector2& worldOrigin) const;
```

- `HeightMap` receives the requested configuration and explicit **world origin**, validates it, computes sample dimensions, chooses a capacity tier, acquires/reconfigures a `Grid`, fills all height samples, and returns its RAII `Lease`.
- **The caller/pass never computes sample counts, selects a pool or resizes storage.** In particular, the generic generation pass only supplies configuration and origin, then stores the resulting lease.
- Pools are **static and shared by all `HeightMap` instances**, grouped by **power-of-two capacity measured in total number of stored samples**. E.g. `17×17 = 289` samples use the `512`-sample tier, `9×9 = 81` use the `128`-sample tier. Each tier's `spk::Pool<Grid>` reuses its grids and retained underlying allocations; a grid's logical dimensions, world origin and sample values are reset/overwritten each time.
- The common power-of-two tier selection and pool management will be factored into the generic `spk::CapacityPool<TElement>` approved in A2 (§4.5). Tier creation/lookups need synchronization; individual `spk::Pool` instances already implement thread-safe obtains/recycles. Protect lifetime and avoid mutable configuration during parallel generation.
- `Grid::Lease` denotes a `spk::Pool<Grid>::Lease` (exact alias location/name to settle). Existing `Pool<T>::Lease` is default-constructible and move-assignable: it can be held directly in `TGenerationData` and replaced without an `optional`.
- **No eviction, maximum idle count, or pool trimming** in v0.1.5. Pools retain freed grids; reconsider memory caps only if measurements justify it.

#### 4.4.5 Generic HeightMapGenerationPass (two hooks)

- A reusable **abstract `HeightMapGenerationPass<TChunk,TGenerationData,TBuilder>`** derives from the existing `spk::World::Generator<...>::Pass` and fully implements its `apply(builder, chunkCoordinate, data)`. Game/voxel-specific subclasses only supply information through **two pure virtual hooks**:
  1. `_gridConfiguration(chunkCoordinate, const TGenerationData&)` → `Grid::Configuration`.
  2. `_gridDestination(TGenerationData&)` → a **reference to the `Grid::Lease` storage** to replace in `TGenerationData`.
- The pass has a non-owning `bind(HeightMap&)` connection to the concrete height source. Its lifetime must cover pass execution, and rebinding while jobs are running is unsupported. A separate virtual hook for selecting the `HeightMap` was **not** chosen.
- The pass calculates the world-space origin from **chunk coordinates × configured world-space `size`** (componentwise). It does **not** need an origin hook, nor does it use a sample count for the origin. The resulting origin and configuration are passed to `HeightMap::generate()`.
- `apply()` then move-assigns the returned `Lease` into the referenced destination. The intermediate grid is subsequently accessible to custom passes that alter the terrain, followed by voxel/mesh application passes. No changes to the existing `Generator` template or a concept constraining `TGenerationData` are needed.
- Specific signatures for coordinates and error handling when unbound should be finalized against actual Sparkle headers before implementation.

### 4.5 Proposed generic biome-distribution contract — under discussion (2026-11-24)

> **Design proposal, not a validated C++ API or an implementation authorization.** This section captures the latest direction of the conversation and identifies precisely what remains to approve. The generic `spk::World` layer must not know about voxels, materials, Voronoi, Poisson-disk placement, climate, or particular noise algorithms.

#### General responsibility split

- A reusable, generic **distribution pass** owns all work common to distribution strategies: traversing a configured 2D world-space sampling domain, resolving sample positions from world-space origin and sampling interval, writing directly into pooled grid cells, validating influence results, providing results from which the grid can derive a deterministic dominant biome on request, and publishing sampled influences in `TGenerationData`.
- A strategy-specific subclass (or a game-defined implementation satisfying the same hook contract) supplies **only the distribution algorithm** through a hook that answers: for this absolute world X/Z position, which biome definitions have what relative influence?
- The pass does not compute terrain height, select materials, choose how a world is divided into regions, or subscribe to modification events.
- The current `World::Generator<TChunk,TGenerationData,TBuilder>::Pass::apply(builder, coordinate, data)` is the integration point; do not change the existing generator API merely to introduce biome placement.

#### Conceptual result type

- The public record is **`spk::World::Biome::Influence`**, a nested aggregate containing a non-owning `const Biome *biome` and a `float weight`. A1 is resolved and its null-pointer rule has been explicitly revised by A2 (see the validated contracts below).
- One sampled world-space position can return **zero, one, or multiple** influences. Each cell has a fixed compile-time capacity chosen by the developer, with no engine-imposed maximum or mandatory value; unused slots have null biome pointers.
- When several biomes contribute, downstream height evaluation combines their independent height sources by normalized weights (C4); a downstream consumer needing one biome can select the dominant influence. The generic distribution result itself is independent of what those consumers do.
- Prefer temporary, per-generation-request storage in `TGenerationData`. Strategy-specific preprocessing/state (e.g. precomputed finite-world regions) may be held separately by the strategy and shared safely across generation requests. Never require a world-wide per-position allocation.
- No mandatory numeric biome IDs, biome registry, climate fields, seed policy, or ownership transfer. Referenced biome definitions must outlive concurrent jobs.

#### A1 — Influence record (validated 2026-11-24)

The exact public record is nested directly inside the generic biome definition, not a standalone `World::BiomeInfluence` type. The following C++23 declaration illustrates the approved record only; the rest of the enclosing `Biome` API (including its destructor) is not settled here.

```cpp
namespace spk::World
{
    class Biome
    {
    public:
        struct Influence
        {
            const Biome *biome = nullptr;
            float weight = 0.0f;

            template <std::size_t TCapacity>
            class Grid;
        };
    };
}
```

- `Influence` is a simple aggregate, copyable and assignable by value, with public members. No custom constructor, accessor, ownership mechanism, subscription or internal validation. It does not depend on coordinates, voxels, a distribution algorithm or a biome registry.
- `biome` is a raw, non-owning pointer to an existing definition, read-only through the influence. The pointer itself and the weight may be reassigned. **Revised during A2:** a null pointer is valid and denotes an unused slot, not an error. The default empty slot is `{nullptr, 0.0f}`; a cell with only null biome pointers represents an unassigned position. Null slots are not biome contributions and are excluded from normalization. A3 requires the weight of every null slot to equal zero; any other value is an error. The aggregate itself performs no validation.
- `weight` is a `float`, representing a relative weight in raw hook output and a normalized weight in published output. The same record is used for both stages. The generic pass owns validation and normalization according to the approved A3 contract below.
- The developer owns the biome definitions and chooses their storage. Each referenced definition must remain alive at a stable address for as long as any influence referring to it may be used, including queued or executing generation work and downstream consumers. Definitions must not be modified while used by generation jobs. A timeout alone does not end the lifetime obligation if a job is still running. Sparkle does not track lifetimes or automatically manage ownership.
- A2 subsequently settles the container and direct-output shape. A1 does not choose an ordering, a dominance rule, the complete hook signature, a concrete biome destructor, or a global reconfiguration/lifetime protocol (B1, B6, E3 remain open; A4 and A5 resolve dominance and ordering below).

#### A2 — Fixed-capacity influence grid and pooling (validated 2026-11-24)

- The public type is `spk::World::Biome::Influence::Grid<TCapacity>`, nested in `Influence`. The developer chooses the positive compile-time cell capacity; 2, 4, 6 or 100 are examples, not engine defaults or limits. No runtime capacity selection is required.
- Each position is represented by a `std::array<Influence, TCapacity>` (public alias `Cell`); all cells are stored contiguously by value in a single `std::vector<Cell>`. The grid storage is preallocated/reused as a whole, not allocated separately per sample.
- The distribution pass and its grid use the same template capacity. The strategy receives a mutable reference to the actual destination cell and fills it directly; no intermediate vector, copy, truncation or generic top-N selection. Strategies are responsible for selecting at most the configured number of contributions. This supersedes the earlier temporary-vector proposal. Exact hook signatures and sampling configuration remain open (B1/B2).
- Before filling a recycled grid, every logical cell is reset to empty slots (`{nullptr, 0.0f}`). The grid retains its underlying reserved allocation across reuse. A cell with zero non-null entries denotes no biome. The generic pass normalizes the populated slots directly.
- Mutable and const direct cell access is approved. Subsequent passes may replace, add or remove influences within the fixed-size cell without an allocation. The precise coordinate/domain rules of `at()` remain open (B3/B4), as do the responsibilities for validation/normalization after downstream edits (E2). A3 defines the validation and normalization rules for distribution output, not the orchestration of later edits. References are usable only while the owning grid storage remains valid.
- Each grid specialization has a static capacity-tier pool, shared across its instances, returning a `Grid::Lease` based on `spk::Pool<Grid>::Lease`. The tier is measured in total sampled positions (cells), rounded up to a power of two, independently of the per-cell template capacity; e.g. 289 positions use tier 512. No per-position allocation is required.

Illustrative interface for the approved storage/access shape (not a complete implementation; the enclosing `Biome` declaration is above):

```cpp
template <std::size_t TCapacity>
class Biome::Influence::Grid
{
public:
    using Cell = std::array<Influence, TCapacity>;
    using Lease = typename spk::Pool<Grid>::Lease;

private:
    std::vector<Cell> _cells;
    // Shared capacity-tier pool and spatial configuration omitted.

public:
    Cell &at(const spk::Vector2 &position);
    const Cell &at(const spk::Vector2 &position) const;

    [[nodiscard]] const Biome *dominant(const spk::Vector2 &position) const;
};
```

##### Shared capacity-tier infrastructure — `spk::CapacityPool`

The common tier-selection mechanism will be extracted into a generic `spk::CapacityPool<TElement>` (planned header `include/container/capacity_pool.hpp`), independent of World and Voxel. It composes existing `spk::Pool<TElement>` instances, reusing their Lease/recycling semantics; it selects/creates a pool by the next power of two of the requested capacity and enables a capacity-aware factory to reserve storage on creation. Tier lookup/creation must be thread-safe. Both HeightMap::Grid and Biome::Influence::Grid use this abstraction. The complete generic API, zero/overflow handling, tests and audit of other compatible uses will be addressed in a separate implementation PR; no existing pool is automatically refactored. No implementation is authorized by this document.

#### A3 — Weight validation and normalization (validated 2026-11-24)

The generic distribution pass validates each populated destination cell after the strategy hook fills it and before publishing the distribution. The strategy supplies relative weights; the pass normalizes them in place. `Influence` remains a plain aggregate with no internal validation.

| Slot | Validity and handling |
| --- | --- |
| Null biome, weight equal to zero | Valid unused slot; no contribution. |
| Null biome, any other weight (including NaN or infinity) | `spk::Exception`. |
| Non-null biome, negative or non-finite weight | `spk::Exception`. |
| Non-null biome, finite weight equal to zero | Valid; preserve both biome pointer and weight. |
| Non-null biome, finite positive weight | Valid; include in normalization. |
| Same non-null biome pointer appearing more than once in a cell | `spk::Exception`, regardless of either weight. Multiple null slots are valid. |

Biome uniqueness is by pointer identity, not by biome contents or a mandatory ID. Duplicate checking is required, but no particular search algorithm is prescribed. No silent removal, merging, clamping, or substitution of invalid contributions is allowed.

**Normalization:**
- Validate the entire cell, even when every weight is zero.
- Accumulate the weights of non-null biomes into a `double` sum.
- If the sum is zero, leave the entire valid cell unchanged, including its non-null zero-weight references. An all-null cell is valid and is also left unchanged.
- If the sum is positive, divide each non-null weight by the sum in `double` and convert the result back to `float`. Null slots remain unused. No arbitrary equal redistribution and no correction of the final weight to force an exact sum of 1. The normalized sum is 1 mathematically, subject to floating-point rounding; test comparisons must account for rounding.
- Perform the operation on the existing fixed-capacity cell without a second influence buffer. Ordering and dominant selection are settled by A4/A5 below; normalization changes weights only and preserves slot indices. Validation/normalization scheduling after arbitrary downstream edits remains E2.

Examples: weights 3, 5, 2 become approximately 0.3, 0.5, 0.2; weights 0, 0 for two distinct non-null biomes remain 0, 0; two references to the same non-null biome are rejected even if both weights are zero. The precise test tolerance may be chosen when the numerical tests are implemented; it is not an exact-sum invariant.

#### A4 — Dominant biome selection (validated 2026-11-24)

The public query belongs to `spk::World::Biome::Influence::Grid<TCapacity>`:

```cpp
[[nodiscard]] const Biome *dominant(const spk::Vector2 &position) const;
```

- The query reads the current cell at the supplied position and scans its slots in increasing array index order. Coordinate and bounds semantics follow the cell access contract to be finalized in B4.
- Only non-null biome pointers with strictly positive weights are eligible. The result is the non-owning pointer to the biome with the greatest stored weight. Exact equal positive weights select the first encountered eligible entry (strict `>`, not `>=`); no epsilon tie, ID, priority or pointer-address ordering is needed.
- If there is no eligible entry (all slots null or all weights zero), return `nullptr`. A valid distribution may have such a cell; absence of a dominant alone is not an exception from this query.
- A consumer that requires a usable biome to perform its task must throw `spk::Exception` if none is available. This includes biome-dependent height generation and voxel strata application when they require a contributing biome. Do not silently select a zero-weight biome or invent a fallback. C5 still settles other cases such as missing height sources and non-finite heights.
- Compute the result on demand in O(TCapacity) time, with constant auxiliary storage, no allocation, no sorting and no cached dominant. Direct valid cell edits are reflected in the next query. The query does not revalidate the entire distribution; validation/normalization after downstream edits remains E2.
- A5 preserves the slot order supplied by the strategy; ties are resolved using the current stored cell order, including any subsequent explicit edits.

Examples: weights 0.4, 0.4, 0.2 select the first 0.4 biome; all-zero weights with non-null pointers produce `nullptr`; an all-null cell also produces `nullptr`.

#### A5 — Influence ordering (validated 2026-11-24)

- The generic distribution pass preserves the exact array indices of every influence supplied by the strategy. Validation and normalization do not reorder, compact, remove or move slots; normalization only changes weights as specified in A3.
- Null slots may appear at any index, including between non-null biome pointers. Non-null zero-weight slots also remain at their original indices. Consumers must not assume a populated prefix, sorted weights or an early termination at the first null slot.
- No mandatory ordering invariant is imposed on a valid cell. A downstream pass may explicitly edit any slot, subject to the validity rules in A3; the scheduling of validation/normalization following edits remains E2.
- Dominant selection follows A4: scan increasing indices and retain the first strictly positive maximum. The strategy controls its tie preference through the placement of entries in the cell. A downstream edit can intentionally change that preference.
- There is no sorting or compaction cost in the generic distribution pipeline.

Example, in index order: `{forest, 3}, {nullptr, 0}, {mountain, 5}, {desert, 2}` becomes `{forest, 0.3}, {nullptr, 0}, {mountain, 0.5}, {desert, 0.2}` (weights approximate). Mountain is dominant; no entry changes index.

**A1–A5 are resolved.** The distribution result contract is settled; the pass hooks, spatial sampling/access, height orchestration and broader lifecycle questions remain open.

#### Extension hooks (illustrative, not signatures)

The generic reusable pass is responsible for **implementing** `apply()`; games should not have to copy a position loop, allocate a map, normalize weights, or write values manually. Reusable work belongs in Sparkle; hooks are reserved for genuinely strategy-specific choices. Minimum conceptual customization points:

1. **Sampling configuration/destination adapters** that supply the 2D world-space extent, sample spacing, and destination in the caller's `TGenerationData`, following existing configurable-pass patterns. How these are expressed (virtual hooks or a bound context) is open.
2. **A required distribution hook** such as `_sampleInfluences(worldPosition, cell)`. It fills the destination `Grid<TCapacity>::Cell` directly by mutable reference, with the same compile-time capacity as the pass (A2).
3. **Optional strategy initialization/preprocessing** for algorithms needing a world-level layout. Such state is prepared by the strategy's owner independently of chunk generation; no mandatory per-chunk or global precomputation hook is required in the generic pass.

**Confirmed shared normalization responsibility (2026-11-24):** the strategy hook may return arbitrary **relative** weights (e.g. 3, 5, 2); the reusable Sparkle pass converts the non-null contributions into normalized weights summing to 1 for a usable nonempty result (e.g. 0.3, 0.5, 0.2) before handing them to downstream consumers. Strategy code must not be required to repeat normalization. The validation and zero-sum rules are now settled in A3 above. Dominant selection and preservation of slot order are settled in A4/A5.

A `ContractProvider` subscription mechanism is **not required** for the initial generation pipeline. It can be added only if later designs genuinely require live configuration-change notification or cache invalidation; generation may assume stable definitions while jobs execute.

#### Why both example strategies fit

- A finite-world Voronoi strategy can precompute one or more sites per biome in externally held state and answer the same world-position sampling hook. This generic section does **not** define Poisson-disk placement, boundary distortion or transition distance.
- A seed-derived, infinite procedural strategy can compute influences directly from absolute coordinates and its own algorithm without holding a global map. It uses the **same** hook and output representation.
- Neither example is an implementation requirement or an approved concrete distribution-pass algorithm.

#### Generic contract tests to specify

- All-null, single, and multiple populated slots; direct cell output, configurable capacities, recycled-cell reset; A3 validation including invalid null-slot weights, negative/non-finite weights, duplicate non-null pointers even at zero weight, preserved zero-weight entries, zero-sum cells, and double-intermediate normalization (including large finite float weights); A4 tests include first equal positive maximum, null/zero entries, all-zero and all-null cells, edits reflected without a cache, and exceptions from consumers requiring a biome.
- Coordinates and sampling that cross chunk bounds and negative world positions without aligning biome boundaries to chunks.
- A5 tests verify exact index preservation through normalization, including interleaved null slots, non-null zero-weight slots and equal positive weights; the dominant scan does not stop at the first null slot.
- Different strategy hooks generating the same observable temporary result shape; stable results independent of chunk-generation order.
- Valid non-owning-reference lifetimes and read-only concurrent generation; no forced per-position heap allocation.
- A mock strategy proving that the base pass—not user code—performs traversal, transient storage, validation and publication.

**Pending decisions:** spatial configuration and access details (the record and cell/grid storage are settled by A1/A2), how the pass receives output storage and sample configuration, and how the multi-biome result connects to the separate generic height-combination pass. Validation and normalization are settled by A3; dominant selection by A4; slot order by A5. See [open questions](biome-open-questions.md).

## 5. Proposed pass flow (heightmap API confirmed; biome integration still open)

1. **Biome placement:** produce zero or more biome influences with weights at each configured world-space X/Z sample into `TGenerationData`; a dominant biome is derived when needed by downstream consumers. The generic contract proposal is detailed in §4.5 and is not yet final.
2. **Generic heightmap generation:** the bound `HeightMapGenerationPass` gets the grid world-space `Configuration` and lease destination through two subclass hooks. It derives the world origin from chunk coordinate × world-space grid size and calls `HeightMap::generate(configuration, origin)`, which handles all sampling and static pool selection. It stores the resulting `Grid::Lease` in `TGenerationData`.
3. **Optional custom grid-modification passes:** edit heights in the transient grid (e.g. lower the edge of a finite island, level a road/town) without subclassing or mutating the procedural `HeightMap`.
4. **Voxel strata/terrain application:** use final sampled heights and dominant biome strata to populate voxel columns; other consumers may use height interpolation for non-voxel surfaces.
5. **Future world passes:** vegetation, prefabs, caves, roads, buildings etc., with coherent biome input.

**Still unresolved:** C2 fixes the `SurfaceBiome` height query and C4 fixes the weighted combination formula. The concrete `HeightMap` remains non-polymorphic; its efficient point evaluation, any independent role for a separate abstract height generator, and pass orchestration/type compatibility remain open (C1/C3/C5/C6/C7). Neither the new contract nor blending has been implemented.

The existing `spk::World::Generator<TChunk,TGenerationData,TBuilder>` already calls passes in order with `apply(builder, coordinate, data)`. The current `spk::Voxel::PerlinTerrainPass` remains an implementation reference, not a constraint. No breaking change to `Generator` has been approved.

## 6. Candidate reusable biome-placement algorithms (not yet approved as final APIs)

All candidates should have the same observable, **multi-influence** output contract at each configured world-space X/Z sample; they differ only in how they compute influences. The fixed-capacity cell/grid output structure is approved in A2; coordinate and hook details remain open. The examples below inform generic requirements; they are not a request to implement or specify each algorithm now.

- **Manual/explicit regions:** regions or tiles predetermined by the game; useful for the finite tiered world.
- **Finite Voronoi regions:** sites distributed over a bounded map; Poisson-disk sampling is a candidate for well-spaced site positions; assign selected biome definitions to sites/cells. A **biome per chunk** behavior and a **biome per voxel column** behavior should both be expressible (potentially as different sampling granularities into the same per-column output).
  - **Coverage caveat:** Poisson-disk sampling only spaces points. It does **not** by itself guarantee every biome type appears. If coverage is required, biome-to-site assignment must guarantee at least one site per requested biome (and there must be enough sites).
- **Environmental suitability:** evaluate world variables such as temperature, moisture, elevation, or other game-defined conditions; choose the biome most favored by each location. Whether preferences belong in base biome data, game-derived biome data, or placement rules remains open.
- **Competing noise fields:** one Perlin/noise field per candidate biome, with the highest-scoring field selecting the region.
- **Cellular automata:** a previously suggested option for iterative, neighborhood-driven distributions; requires special thought for determinism and chunk independence.
- **Hierarchical/main and sub-biome placement:** needed for the island game, with a constrained set of variations per parent environment; no mandatory inheritance or fixed hierarchy has been approved.

A placement algorithm may consult a persistent finite map or deterministic world-scale state, but should provide temporary per-chunk results to downstream passes. Chunk-order independence, seams, and negative coordinates must be tested.

## 7. Scope boundaries and compatibility

- The core biome concept is not inherently voxel-specific.
- A heightmap is a reusable terrain-surface model, **not** a universal model for caves, overhangs, floating islands, or arbitrary 3D shapes. Volumetric terrain should remain a separate, compatible extension.
- Voxel strata describe **material composition**; they do not decide air/solid topology, caves, or void geometry.
- Voxel biomes should remain independent of rendering and meshing choices. Marching cubes or alternative volume encodings may need an adapter; representation compatibility is not yet demonstrated.
- `TGenerationData` is temporary per generation request. Non-owning biome and height-generator references require valid external lifetime and read-safe configuration during concurrent chunk generation.
- The design should preserve deterministic generation across independent chunks and avoid recomputing the same biome decision in unrelated downstream passes.
- Mutable-after-setup behavior and cache invalidation are not designed yet.

### Approved initial ground-generation scope (2026-11-24)

The first implementation phase will focus on **heightmaps, biome placement/selection, biome-driven terrain-height generators, and voxel strata**. This phase is meant to develop reusable world-generation infrastructure while progressively enriching the existing Sparkle chunk-rendering/golden-image validation world. The validation scene is an evolving example of a fuller world, **not** a special-purpose permanent substitute for production APIs.

Only **after** these ground-generation building blocks are defined, implemented, and validated will the discussion move to **prefab placement passes** (trees, rocks, scenery, and subsequently buildings or other structures). Arbitrary 3D/volumetric terrain, such as cave geometry, overhangs, and floating-island undersides, is intentionally deferred; the generic interfaces should not preclude such future extensions.

This scope decision does not settle the previously proposed alternatives concerning whether to replace the existing Perlin fixture immediately or retain it as a parallel regression baseline. Golden-image and chunk-streaming regression coverage should be preserved.

**HeightMap representation selected:** `spk::World::HeightMap` is a concrete procedural height generator, with transient, mutable and pooled `HeightMap::Grid` sample storage and both reference-returning direct `at(position)` and value-returning interpolated `at(position, mode)` queries. Subsequent passes modify the sampled grid rather than the procedural generator. See the detailed, later-approved contract in §4.4.

## 8. Outstanding architectural questions — not decisions

This is a historical summary kept for context. The **current, categorized and independently actionable inventory** is [biome-open-questions.md](biome-open-questions.md). It deliberately leaves details unanswered for future focused discussions. **Weight normalization by the generic pass is confirmed**, not an open choice.

1. **Multi-influence storage and hook contract:** A1–A5 settled the individual `Biome::Influence` record, pooled fixed-capacity grid, distribution validation/normalization, dominant query and preservation of slot order; resolve adapter hooks and spatial access. Downstream consumers may derive a dominant biome but must not force a single entry in the distribution result.
2. **Remaining HeightMap integration details:** The primary procedural/grid contract is **confirmed in §4.4**. Still to settle: exact C++ type/alias names; representation of `Grid`'s storage; numeric edge cases for floating-point divisibility tolerance; reference semantics for `at()` at boundaries; consumer-specific handling beyond the A4 requirement to throw when a usable biome is required but absent; voxel integer rounding; and how biome-selected height algorithms and blending integrate with the current `HeightMap` design.
3. **Application pass API:** Concrete interfaces and `TGenerationData` access for the already-separated height and voxel application passes; behavior of finite strata and remaining consumer-specific cases beyond A4; interaction with preexisting voxels.
4. **Stratum semantics:** Thickness-to-integer rounding, unlimited-depth representation, per-stratum noise offsets, validation of weights, and how weighted noise maps to proportions.
5. **Type names and extensibility:** Exact `spk::Voxel::Biome` vs `spk::Voxel::World::Biome` name; polymorphism and lifetime-safe references; custom behaviors for biome-specific decoration and multi-chunk structures.
6. **Placement strategies:** Which reusable algorithms ship first, detailed APIs, finite-map boundaries and seed management; how environmental preferences are represented without making climate mandatory.
7. **World topology:** Surface-relative layers and 2D biome maps fit ground surfaces. Define how (or whether) a future generic 3D environment/biome placement extension coexists.
8. **Testing:** chunk seams including negative coordinates, vertical-chunk consistency, fixed-seed determinism, hard/smooth heights, distinct biome shapes, empty assignments, and future visual golden references.

## 9. Suggested next design discussion

Future design discussions should take one item at a time from [the open-questions inventory](biome-open-questions.md), rather than reopen settled requirements. The generic multi-influence distribution contract (§4.5) and concrete Perlin-layered `HeightMap` (§4.4) need an explicit integration contract. The voxel-specific implementation is a supported first-class destination for those generic results, not a dependency of the generic types.

No implementation PR breakdown has been approved yet.

## References

- [World generation overview](README.md)
- [Open questions: generic distribution, height orchestration, voxel integration and tests](biome-open-questions.md)
- [Existing generation contracts](contracts.md)
- [Existing world architecture](architecture.md)
- [Existing terrain generation implementation](../../include/voxel/pass/perlin_terrain_pass.hpp)
- [Existing world generator implementation](../../include/world/generator.hpp)
- [PR #99: Perlin voxel world and golden validation](https://github.com/EreliaStudio/Sparkle/pull/99)
