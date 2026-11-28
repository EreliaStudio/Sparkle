# CI baseline — v0.1.5

Audit of the CI workflow at merge commit eb42209a6b753f5af0fb0b7af52591c8ad8cf99f (#105).

## Recorded before measurements

The completed [PR #102 run 45985130614](https://github.com/EreliaStudio/Sparkle/actions/runs/45985130614) had 19 jobs and failed two Graphics Release tests. The data below is historical, not a fully green baseline at the new branch head. The [PR #105 run 46075940547](https://github.com/EreliaStudio/Sparkle/actions/runs/46075940547) was still active during the audit. Job step durations are GitHub timestamps rounded to seconds; CTest wall time comes from logs.

| Stage | #102 Windows Release Core | #105 Windows Release Core | #105 Windows Release stress |
| --- | ---: | ---: | ---: |
| Configure, including vcpkg | 71 s | 68 s | 99 s |
| vcpkg install (subset of configure) | not extracted | 59.7 s | 79.7 s |
| Compile | 231 s | 216 s | 310 s |
| Functional CTest real time | 14–16 s step | 14.56 s (1,482 tests) | — |
| Stress suite and repetitions | — | — | about 3.8 s CTest real time |
| Artifact upload step | 2 s | 1 s | 1 s |

In #105 Linux Release Core, configure was 37 s, compilation 228 s, and 1,482 tests took 11.92 s. Its separate stress job compiled for another 224 s. Both Core Release configurations and stress configurations use the same compiler, build type, triplet, static CRT selection where relevant, and test target; the extra two builds are redundant.

| Completed historical run #102 | Before |
| --- | ---: |
| Time to first functional Core completion (including queue/setup, Windows Release) | 5 min 46 s |
| Time to first compiler check (GCC Debug) | 3 min 29 s |
| Explicit compilation steps (excludes bundled coverage/packaging) | 4,675 s cumulative |
| Core functional CTest steps (four jobs) | 73 s cumulative |
| Graphics Debug CTest | 595.35 s real |
| Graphics Release CTest (failed) | 328.59 s real |
| Dedicated performance invocation | not reached because Release failed |
| PR validation elapsed | 20 min 47 s |
| Extended LLVM coverage job | 19 min 18 s |
| Cumulative job execution | 10,137 s = 168.95 runner minutes |
| Independent jobs | 19 |
| Exact duplicate Core Release test builds | 2 |
| Extra Release voxel benchmark cases (when full suite succeeds) | 12 at #105 head |

The old Release Graphics job runs every Graphics case, then invokes VoxelChunkPerformanceTest a second time for reporting. Its Debug run and the coverage run also execute the benchmark fixture without Release budgets. The dedicated second invocation is skipped if the first CTest step fails. The Core stress repeats are intentional reliability coverage; ASan and coverage use different instrumentation.

## Slow suites

From the #102 Graphics Debug CTest log (sum of reported individual durations, not total job duration): BiomeWorldGolden 105.33 s, ShadowRenderingGolden 65.03 s, BiomeWorldShadowDiagnostics 56.19 s, VoxelChunkPerformanceTest 21.81 s. Slow individual tests included CoarseFourVoxelSurfaceGrid (38.29 s), CustomTerracedHeightPass (17.90 s), the two randomized voxel hard-budget fixtures (11.81 s and 7.79 s), and exhaustive mesher fixture geometry (7.71 s). The Graphics Debug build took 447 s, configure 110 s, and Mesa installation 5 s; Release build 336 s, configure 94 s, Mesa 5 s. The vcpkg port consumers took 20 min 39 s total and LLVM coverage 19 min 18 s. These historical values vary with renderer workload and runner performance; #105 changed the rendering implementation.

The #105 MSVC Release compiler compatibility job recorded 526 sccache hits (100%) and a 66 s build step. Core and Graphics vcpkg installation logs restored zero binary packages on the inspected runners. Retain the existing sccache fallback; measure the new binary cache cold and warm before making speed claims.

## After

Pending a complete CI run of the new branch. Record the same metrics, per-category JUnit counts, cache conditions and runner images before claiming gains. The planned elimination of duplicate invocations is structural, not a measured runtime improvement. The separate prebuilt release workflow, packaging isolation, compiler compatibility and instrumentation-specific builds are intentionally retained.
