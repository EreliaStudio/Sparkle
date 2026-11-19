# Sparkle v0.1.5 — Proposed implementation PRs

**Design-only** briefs. These are not implementation branches, approved public APIs, or GitHub issues.

The previous issues #78–#83 have been withdrawn. Each PR below targets V0.1.5 **only after** its API and dependencies are approved.

- [PR01 — World coordinates and spatial bounds](PR-01-world-coordinates.md) (Generic Core; depends on None).
- [PR02 — Generic chunk payload and loaded collection](PR-02-chunks-and-storage.md) (Generic Core; depends on PR01).
- [PR03 — Deterministic seed context and composable generation](PR-03-seeds-and-pipeline.md) (Generic Core; depends on PR01).
- [PR04 — World/region plans, metadata and spatial feature queries](PR-04-world-plans.md) (Generic Core; depends on PR01, PR03).
- [PR05 — First end-to-end headless voxel generation slice](PR-05-voxel-first-slice.md) (Voxel Core; depends on PR02–PR04).
- [PR06 — Terrain sampling, ground stratification and biome metadata](PR-06-ground-and-biomes.md) (Generic Core + Voxel Core; depends on PR03–PR05).
- [PR07 — Spatial placement, scenery and reservations](PR-07-scenery.md) (Generic Core + Voxel Core; depends on PR04–PR06).
- [PR08 — Route graphs and continuous roads](PR-08-paths-and-roads.md) (Generic Core + Voxel Core; depends on PR04, PR06–PR07).
- [PR09 — Settlements, city plans and multi-chunk structures](PR-09-settlements.md) (Generic Core + Voxel Core; depends on PR07–PR08).
- [PR10 — 3D underground features and tunnel generation](PR-10-caves.md) (Generic Core + Voxel Core; depends on PR04–PR06; coordinate with PR08–PR09).
- [PR11 — Interest zones, chunk requests and streaming](PR-11-requests-and-streaming.md) (Generic Core; depends on PR02, PR05).
- [PR12 — Voxel chunk meshing and renderer integration](PR-12-graphics-adapter.md) (Graphics; depends on PR05, PR11).
- [PR13 — Integrated sample world, reproducibility and regression suite](PR-13-validation-demo.md) (Integration; depends on PR06–PR12).

The recommended first implementation after design approval is PR01, followed by PR03/PR04 and a very small PR05 vertical slice. Do not create all 13 coding PRs at once.

No feature PR should be marked complete until C++23 Windows/Linux CI and relevant tests are green. Never merge without maintainer approval.