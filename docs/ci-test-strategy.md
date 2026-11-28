# Sparkle CI test strategy

The CI workflow uses concise check names grouped by action and actual workload (Build, Test and Format) and retains the platform/compiler matrix. It runs on pull requests, pushes to main, and manual dispatch. Superseded runs are cancelled. The separate prebuilt Windows package workflow is unchanged. No protection rules are changed.

## Test categories

Tests are organized under `tests/Core` and `tests/Graphics` by validation category, with shared graphical test utilities in `tests/SparkleTestLibrary` and external consumer fixtures in `tests/Packaging`. Individual GoogleTest cases retain their original CTest names, timeouts, failure propagation and JUnit results.

| CTest label | Responsibility |
| --- | --- |
| Core | Headless tests |
| Fast | Functional Core tests and audited Graphics CPU tests |
| CoverageBatch | Audited pure math cases batched only during LLVM coverage |
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

Classification is an explicit list of GoogleTest suites/cases in the Core and Graphics CMakeLists.txt files. Add a new suite to the relevant list when it requires special routing. The complementary default registration prevents unclassified tests from silently disappearing. Do not select a test by a naming convention alone. Keep a test in exactly one discovery partition; labels may overlap. The Core client-destructor cases intentionally carry both Fast and MemorySafety. Graphics tests that are not audited as CPU-only retain a graphical resource lock. Clipboard and performance tests use RUN_SERIAL. Do not relax locks without proving that the tests do not share native resources, GL state or image output.

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
- Core and Graphics jobs download their matching bundles, verify the workspace, commit, OS, architecture, configuration and exact test inventory, and run without recompiling. A runner image mismatch is logged; Linux dynamic dependencies are checked. The existing Graphics test names and paths are preserved relative to the shared build directory. Windows Graphics uses a single five-entry matrix: Functional and Golden in Debug and Release, and Benchmark in Release only. Each entry downloads the same matching executable bundle. The functional job runs audited CPU cases with two CTest workers and other functional/native cases serially; golden and benchmark jobs run serially on independent runners. Image evidence is uploaded separately for each category. LLVM Debug coverage exercises all cases for instrumented correctness, not timing. It batches only the explicitly audited pure math suites in one process with a 120-second aggregate timeout and verifies exact individual XML test names against the CTest inventory. An incomplete, timed-out or invalid batch falls back to isolated execution for evidence and still fails coverage; ordinary assertion failures retain their original GoogleTest report and fail coverage. Other Fast cases execute with two workers; the non-Fast complement remains serial. All JUnit cases and raw profiles are merged. Normal CI retains individual CTest processes and timeouts for every case.
- Windows/Linux stress jobs download the same Release Core bundle as the functional jobs, directly from the standard builds. Existing 20 and 5 bounded network repetitions are intentionally retained in addition to the stress suite.
- ASan/LSan builds remain independent. Compiler compatibility (MSVC and GCC Debug/Release), LLVM coverage, installed package Debug/Release and vcpkg consumers keep their existing checks and triggers.
- Every compilation job attempts to restore vcpkg archives and enables sccache as a compiler launcher. The already-used sccache 0.20.1 version is pinned to avoid unexpected changes in cache behavior. Embedded Windows debug information permits cacheable objects without dropping debug information. Cache setup is optional; the MSVC compatibility build also retries without its launcher if needed. vcpkg verifies package ABI hashes; the local Sparkle overlay includes a fingerprint of all tracked packaged source trees, resource inputs, CMake files, port definitions and licenses in its port ABI and cache key. The fingerprint rejects dirty or untracked package inputs, but CI and ordinary documentation changes no longer invalidate the package. Independent packaging configurations and relocation checks remain unchanged. No performance gain is asserted until comparable cold and warm runs are measured. Recent interrupted runs may have only partially populated the Windows compiler cache; preserve existing compiler cache keys and record runner/toolchain provenance before diagnosing further. Cache quota and eviction cannot be inspected without the repository's Actions administration access.

The workflow contains 26 validation jobs (23 previous checks, with the two combined Graphics checks replaced by five independent category checks). Display names follow the action/component/purpose/platform/configuration convention, omitting redundant segments. Combined checks explicitly identify their combined scope: Windows standard builds include Core and Graphics, Linux builds Core only, and Graphics functional, golden and benchmark checks identify their independent categories. MSVC and GCC jobs are compile-only compatibility checks. GitHub may prefix these job names with the workflow name in its pull-request UI. Job IDs remain stable. Windows sanitizer validation uses ASan, while Linux also enables LSan. Required status-check contexts, if configured externally, must match the new display names. A single four-entry standard build matrix keeps the workflow compact. Consumers wait for the matrix to finish, then download only their matching artifacts. They still attempt their artifact if another configuration failed. Compiler compatibility, coverage, sanitizers and package jobs start independently. Artifacts are retained seven days to support job retries. This change does not alter merge protections. Repository-wide required-check settings may require administrative access to inspect. No check has been moved to a schedule.

## Benchmark methodology and evidence

Release hard-budget tests retain their original seeds, workload and assertions. The voxel meshing hard budget is 6 ms per case median (raised from 5 ms to accommodate observed hosted-runner variability); other thresholds are unchanged. CTest runs one case at a time; no unrelated test process runs concurrently on that runner. Hosted hardware variability still applies. The measurement-only partition is separate from functional tests; its correctness assertions remain mandatory. Existing stdout reports case counts, median, P95/P99, worst case, budget and violations where collected. The performance report reads that execution's CTest log and JUnit output, publishes the measurements to the job summary, and retains performance.log, performance-results.xml and performance-report.md as artifacts. No production instrumentation is added. Do not interpret CTest process duration as workload latency.

## Golden images

The pinned Mesa software renderer and expected PNG/GIF files are unchanged. The visual comparison gallery, raw actual/difference images, JUnit files and CTest evidence are retained on both success and failure. Review image artifacts before manually changing references. Do not update references simply to make CI pass.

## Measuring changes

See [initial CI timing audit](validation/ci-baseline-v0.1.5.md). Compare runs with the same runner image, compiler/configuration and cache conditions. Separate cumulative runner time from wall-clock latency and record cache hits, artifact transfer time and the complete run result. Coverage and packaging builds deliberately validate different configurations and must not be counted as identical duplicate builds.

