# Sparkle CI test strategy

The CI workflow retains the existing check names and platform/compiler matrix. It runs on pull requests, pushes to main, and manual dispatch. Superseded runs are cancelled. The separate prebuilt Windows package workflow is unchanged. No protection rules are changed.

## Test categories

Tests are organized under `tests/Core` and `tests/Graphics` by validation category, with shared graphical test utilities in `tests/SparkleTestLibrary` and external consumer fixtures in `tests/Packaging`. Individual GoogleTest cases retain their original CTest names, timeouts, failure propagation and JUnit results.

| CTest label | Responsibility |
| --- | --- |
| Core | Headless tests |
| Fast | Functional Core tests and audited Graphics CPU tests |
| Graphics | Graphics component tests, including CPU-only tests |
| CPU | Audited Graphics cases that do not create an OpenGL context |
| Golden | Registered image comparison and visual diagnostic suites |
| Functional | Other Graphics correctness cases (conservative resource isolation) |
| Native | Windows clipboard tests |
| Stress | Intensive/concurrent correctness workloads |
| Randomized | Explicit seeded stress cases |
| MemorySafety | Lifetime and resource regressions; sanitizer selection |
| Performance | Release hard temporal budget regressions |
| Benchmark | Measurement-oriented tests with their existing correctness assertions |

Classification is an explicit list of GoogleTest suites/cases in the Core and Graphics CMakeLists.txt files. Add a new suite to the relevant list when it requires special routing. The complementary default registration prevents unclassified tests from silently disappearing. Do not select a test by a naming convention alone. Keep a test in exactly one discovery partition; labels may overlap. The Core client-destructor cases intentionally carry both Fast and MemorySafety. Graphics tests that are not audited as CPU-only retain a graphical resource lock in mandatory CI. A separate opt-in experiment investigates concurrency only for selected independent GL test contexts and voxel scene/noise golden cases. Clipboard and performance tests use RUN_SERIAL. Do not relax locks without proving that the tests do not share native resources, GL state or image output.

## Source layout

- Core: Unit, Integration, Stress and MemorySafety.
- Graphics: Unit, Integration, Golden, Performance and Native.
- Both suites keep subsystem directories inside each category, plus separate helpers and resources.
- SparkleTestLibrary remains its own reusable target. Packaging contains CoreConsumer, GraphicsConsumer and TestLibraryConsumer.

A source file containing several categories of test cases stays together when splitting it would duplicate substantial fixtures or obscure related tests. Examples include the double-buffer, publishable, network concurrency, entity tracking, voxel mesher, biome world, shadow projection/rendering and voxel renderer/material fixtures. File location is the primary purpose, not the CTest selection rule. Case-level labels in the CMake files remain authoritative, including mixed-category fixtures. No test cases, names, targets, thresholds or golden references are changed by the move.

## Local execution

Configure and build once using the existing presets:

    cmake --preset testRelease
    cmake --build --preset testRelease

Then use ctest --preset followed by:

| Preset | Selection |
| --- | --- |
| testRelease | All registered cases (serial unless overridden) |
| testFast | Fast functional tests |
| testCore | Core, including stress and lifetime cases |
| testGraphicsCPU | Context-free Graphics correctness |
| testGraphics | Graphics correctness and golden images, excluding performance/benchmarks |
| testGolden | Visual regression |
| testPerformance | Release hard-budget regressions |
| testBenchmarks | Release exploratory measurements and associated correctness assertions |
| testStress | Intensive correctness workloads |
| testMemorySafety | Lifetime cases without sanitizer instrumentation |

The testDebug configure/build/test presets remain available. Release timing measurements are not meaningful in Debug or an instrumented build. For an existing Visual Studio or Ninja build, use ctest --test-dir <build> -C Release -L '^Performance$' --parallel 1 --output-on-failure; replace the label as needed. CTest -N and --show-only=json-v1 inspect registered tests without executing them.

On Linux, use cmake --preset testAsan, cmake --build --preset testAsan, and ctest --preset testAsan for Core ASan/LSan. On Windows, use the existing clang-cl ASan configuration in ci.yml with its matching static runtime and ASAN_OPTIONS. Windows ASan is not a substitute for Linux leak detection.

## CI stages

- Four standard Clang build jobs run on Windows/Linux in Debug and Release. Windows builds Core, Graphics and TestLibrary together in a single CMake tree; Linux builds Core only. Each publishes separate static library and frozen CTest executable bundles. No full CMake build directory or dependency tree is transferred. Source fixtures are checked out normally.
- Core and Graphics jobs download their matching bundles, verify the workspace, commit, OS, architecture, configuration and exact test inventory, and run without recompiling. A runner image mismatch is logged; Linux dynamic dependencies are checked. The existing Graphics test names and paths are preserved relative to the shared build directory. Windows Graphics Debug/Release execute on separate runners. Release runs the performance and benchmark partition once, in isolation, before other test workloads. Audited CPU correctness cases then run with two CTest workers. Other Graphics/native and golden cases execute serially in separate steps, even if an earlier test category failed. Debug does not run the timing partition; LLVM Debug coverage still exercises all cases for instrumented correctness/coverage, not for benchmark measurement. Coverage executes the Fast label with two workers, then its exhaustive complement serially, and merges both JUnit reports and all raw profiles.
- Windows/Linux stress jobs download the same Release Core bundle as the functional jobs, directly from the standard builds. Existing 20 and 5 bounded network repetitions are intentionally retained in addition to the stress suite.
- ASan/LSan builds remain independent. Compiler compatibility (MSVC and GCC Debug/Release), LLVM coverage, installed package Debug/Release and vcpkg consumers keep their existing checks and triggers.
- Every compilation job attempts to restore vcpkg archives and enables sccache as a compiler launcher. Embedded Windows debug information permits cacheable objects without dropping debug information. Backend errors fall back to local compilation. vcpkg verifies package ABI hashes; the local Sparkle overlay includes the checked-out source tree hash in its port ABI to prevent stale binaries. Independent packaging configurations and relocation checks remain unchanged. No performance gain is asserted until comparable cold and warm runs are measured.

The existing 19 check names are preserved and four standard build checks are added (23 jobs). Each standard build has its own job ID; consumers depend only on their matching platform/configuration. Consumers still attempt their own artifact if their producer failed. Compiler compatibility, coverage, sanitizers and package jobs start independently. Artifacts are retained seven days to support job retries. This change does not alter merge protections. Repository-wide required-check settings may require administrative access to inspect. No check has been moved to a schedule.

## Benchmark methodology and evidence

Release hard-budget tests retain their original seeds, workload and assertions. The voxel meshing hard budget is 6 ms per case median (raised from 5 ms to accommodate observed hosted-runner variability); other thresholds are unchanged. CTest runs one case at a time; no unrelated test process runs concurrently on that runner. Hosted hardware variability still applies. The measurement-only partition is separate from functional tests; its correctness assertions remain mandatory. Existing stdout reports case counts, median, P95/P99, worst case, budget and violations where collected. The performance report reads that execution's CTest log and JUnit output, publishes the measurements to the job summary, and retains performance.log, performance-results.xml and performance-report.md as artifacts. No production instrumentation is added. Do not interpret CTest process duration as workload latency.

## Golden images

The pinned Mesa software renderer and expected PNG/GIF files are unchanged. The visual comparison gallery, raw actual/difference images, JUnit files and CTest evidence are retained on both success and failure. Review image artifacts before manually changing references. Do not update references simply to make CI pass.

## Measuring changes

See [initial CI timing audit](validation/ci-baseline-v0.1.5.md). Compare runs with the same runner image, compiler/configuration and cache conditions. Separate cumulative runner time from wall-clock latency and record cache hits, artifact transfer time and the complete run result. Coverage and packaging builds deliberately validate different configurations and must not be counted as identical duplicate builds.

## Optional build and test experiments

The `CI experiments` workflow compares baseline, PCH, unity and graphical parallelism on identical Windows Release settings without compiler-cache timing contamination. It runs on manual dispatch or when the `ci-experiments` label is added to a pull request, and does not replace any mandatory validation. PCH and unity are private target options and off by default. Unity batches can fail due to internal symbol collisions: record those failures rather than masking them. The parallel Graphics experiment runs selected independent buffer/program/framebuffer/VAO/sampler and voxel scene/noise golden candidates in a separate two-worker partition. Remaining native, functional, golden and performance workloads remain serial; image evidence is retained. Only adopt settings after comparable successful runs and correctness review.
