# Feature model: ground, scenery, caves, towns and roads

> This describes **capabilities and algorithms**, not committed concrete
> implementations. World generation must remain game-agnostic.

## Meta-structures versus chunks

A *meta-structure* is a typed descriptor of something that exists in the
logical world, often spanning many chunks. It can represent a continuous
landform, biome region, reserved city footprint, multi-chunk building,
waterway, cave system or road segment.

A descriptor should expose (as appropriate) stable ID, world-space bounds,
feature parameters, seed or deterministic attributes, layer/priority, and
spatial query behavior. It should **not** contain the fully expanded voxels
of the feature. Requesting a chunk evaluates the intersecting descriptors
and realizes only the relevant local part.

No requirement to force roads, trees and caves into identical inheritance
chains; a small common spatial index may store handles to typed feature
groups or type-erased queries only when justified.

## Ground and geography

Generic utilities: reproducible scalar/vector fields, deterministic noise
sampling, blend masks, bounded interpolation and quantization, exclusion
regions and sampled height/density fields.

Voxel realization: terrain elevation, stratified ground layers, soil/rock
IDs, water level and slope/step placement. Surface worlds use continuous
world-coordinate sampling across chunk borders. A cave-capable world may
add three-dimensional density fields; not every world must start from a
heightmap.

## Climate and biomes

The planner can compute macro metadata (temperature, humidity, elevation,
ocean distance) and assign weighted biome transitions. Biomes should
provide constraints and distribution parameters; the generic framework
does not ship a fixed game-specific material table. Abrupt transitions
should be an explicit author choice rather than a boundary artifact.

## Scenery

Reusable deterministic placement: distributions, spacing constraints,
spatial exclusion masks, orientation/scale sampling, regional ownership.
Examples: trees, rocks, ruins, decor, vegetation. A generated scenery
object may become a separate Model3D entity **later**; world planning
does not require Model3D or Graphics.

Multi-chunk assets are represented once and clipped/projected per chunk.
Scatter points must not depend on load order or an unstable PRNG sequence.

## Roads and routes

A road exists as an explicit continuous spatial feature, e.g. polyline
or spline/corridor with width, surface, slopes and intersections. An
optional graph planner connects points of interest, settlements and
resource areas. Realization can reserve corridors, grade ground,
handle river crossings and select local voxel paving.

The graph is planned at a scope wider than a chunk; intersection and
seam correctness follow from shared world-space geometry. Road routing
algorithms (A*, graph heuristics, hand-authored routes) are interchangeable
data sources, not mandatory Sparkle policy.

## Towns, cities and buildings

A hierarchy of metadata can describe settlements, zones/districts,
street networks, lots/plots and building footprints. Deterministic
placement uses road connectivity, terrain suitability and exclusion
regions. A settlement and its buildings may span hundreds of chunks but
should not allocate voxel data until requested.

Core should supply generic spatial primitives, constraints and an
evaluation mechanism, not impose a single procedural-city grammar.
Voxel-specific realizers can draw foundations, walls, openings, paths
and scenery using existing Definition IDs.

## Caves and underground structures

Two compatible strategies: 3D density/threshold fields for volumetric
caves, and explicit connected graphs/tunnel descriptors for authored or
planned underground spaces. Realization carves or replaces voxel data
through local Builder operations. Cave generation must be continuous
at chunk boundaries and coordinate with foundations, roads and water.

Potential later extensions include tunnels connecting settlements,
subterranean roads, underground biomes and large caverns. They should
use the same spatial plan-query model rather than a separate cave-world
container.

## Collision and precedence

A deterministic realization order is proposed:

1. Evaluate shared metadata, base height/density and ground materials.
2. Apply major subtractive features (e.g. cave systems).
3. Reserve/fill terrain works for roads and settlements.
4. Apply infrastructure and buildings.
5. Place secondary details and scenery subject to exclusions.
6. Apply sparse authoritative/persisted world edits **last**.

This is **a starting point**, not universally correct: tunnels may need
to be cut through built terrain, bridges require additive geometry above
roads, and structures may intentionally intersect. Priority and conflict
resolution should be part of each feature's declared semantics.

## Extensibility and persistence

Persist seed + generator version + settings, coarse plans/feature metadata
if needed, and sparse edits. The generator base should be reproducible;
revising generation algorithms must define a migration policy or a
new world version. Avoid serializing thousands of procedurally generated
meshes and not-yet-loaded voxel chunks.
