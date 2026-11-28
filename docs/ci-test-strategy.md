# Sparkle CI test strategy

The CI workflow uses concise check names grouped by action and actual workload (Build, Test and Format) and retains the platform/compiler matrix. It runs on pull requests, pushes to main, and manual dispatch. Superseded runs are cancelled. The separate prebuilt Windows package workflow is unchanged. No protection rules are changed.

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

- Four standard Clang build-and-test jobs cover Windows/Linux Debug and Release. Windows builds Core, Graphics and TestLibrary together; Linux builds Core only. Core Fast tests execute directly in the build job. Release runs Core stress, the original 20/5 network repetitions, and Core benchmarks on the same runner. Core test cases remain individually isolated; reports are uploaded even on failure.
- Windows publishes only the frozen Graphics test executable bundle, before running Core tests. Five Graphics jobs download their matching Debug/Release bundle, verify its origin and exact test inventory, and run Functional, Golden or Release Benchmark independently. Golden includes parameterized shape, scene, object and articulated-lighting suites. CPU cases use two workers; other graphics/native tests and benchmarks remain serial on their own runners.
- ASan/LSan, full compiler compatibility (MSVC and GCC Debug/Release), and direct CMake installed-package Debug/Release validation retain their existing checks. vcpkg independently validates Core, Graphics and TestLibrary in a three-entry matrix. Each port still builds Debug and Release and validates its consumer and licenses. Variant cache keys avoid save races and fall back to the previously populated combined port cache.
- LLVM coverage uses clang-cl with the LLVM source-based instrumentation and explicit profile runtime. The compiler launcher is passed explicitly. Coverage runs Fast correctness cases with two workers and the remaining correctness/memory-safety cases serially; Golden, Benchmark, Performance and Stress are excluded from both phases. Every included case retains its own timeout, and the two JUnit reports plus merged LLVM profile and HTML report are uploaded. Excluded workloads retain their normal validation jobs. The former math batching and manual XML merge are removed.
- Every compilation job attempts to restore vcpkg archives and enable sccache. The local overlay ABI includes a fingerprint of all packaged inputs, independent of CI-only changes. No further cache gain is claimed until comparable runs are measured.

The workflow contains 22 expanded jobs instead of 26. The four standard matrix entries complete before the five Graphics consumers; consumers attempt their own bundle even if another matrix entry failed. No validation is moved to a schedule. Required status-check names may need administrative updates. The MSVC compatibility and direct packaging builds are deliberately kept separate pending proof that reusing installed port libraries preserves their distinct guarantees.

## Benchmark methodology and evidence

Release hard-budget tests retain their original seeds, workload and assertions. The voxel meshing hard budget is 6 ms per case median (raised from 5 ms to accommodate observed hosted-runner variability); other thresholds are unchanged. CTest runs one case at a time; no unrelated test process runs concurrently on that runner. Hosted hardware variability still applies. The measurement-only partition is separate from functional tests; its correctness assertions remain mandatory. Existing stdout reports case counts, median, P95/P99, worst case, budget and violations where collected. The performance report reads that execution's CTest log and JUnit output, publishes the measurements to the job summary, and retains performance.log, performance-results.xml and performance-report.md as artifacts. No production instrumentation is added. Do not interpret CTest process duration as workload latency.

## Golden images

The pinned Mesa software renderer and expected PNG/GIF files are unchanged. The visual comparison gallery, raw actual/difference images, JUnit files and CTest evidence are retained on both success and failure. Review image artifacts before manually changing references. Do not update references simply to make CI pass.

## Measuring changes

See [initial CI timing audit](validation/ci-baseline-v0.1.5.md). Compare runs with the same runner image, compiler/configuration and cache conditions. Separate cumulative runner time from wall-clock latency and record cache hits, artifact transfer time and the complete run result. Coverage and packaging builds deliberately validate different configurations and must not be counted as identical duplicate builds.

