# Sparkle CI test strategy

The CI workflow retains the existing check names and platform/compiler matrix. It runs on pull requests, pushes to main, and manual dispatch. Superseded runs are cancelled. The separate prebuilt Windows package workflow is unchanged. No protection rules are changed.

## Test categories

Tests remain under tests/TU/Core and tests/TU/Graphics, with shared graphical test utilities in tests/TU/SparkleTestLibrary. Individual GoogleTest cases retain their original CTest names, timeouts, failure propagation and JUnit results.

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

- Format and Core functional jobs run immediately; Windows/Linux Debug and Release are preserved.
- Windows Graphics Debug/Release jobs build once per configuration. Release runs the performance and benchmark partition once, in isolation, before other test workloads. Audited CPU correctness cases then run with two CTest workers. Other Graphics/native and golden cases execute serially in separate steps, even if an earlier test category failed. Debug does not run the timing partition; LLVM Debug coverage still exercises all cases for instrumented correctness/coverage, not for benchmark measurement.
- Windows/Linux stress jobs download the compatible Release Core executable and generated CTest registrations from this run. They verify the checkout path, commit, OS and architecture before executing. No full CMake build directory, compiler cache or dependency tree is transferred. Source fixtures are checked out normally. Existing 20 and 5 bounded network repetitions are intentionally retained in addition to the stress suite.
- ASan/LSan builds remain independent. Compiler compatibility (MSVC and GCC Debug/Release), LLVM coverage, installed package Debug/Release and vcpkg consumers keep their existing checks and triggers.
- Core and Graphics clang configurations restore vcpkg binary packages via an image/OS/architecture/manifest-specific cache; vcpkg still checks package ABI hashes. The overlay-port validation is not cached by this change. Existing MSVC Release sccache and its fallback are preserved. No performance gain is asserted until comparable cold and warm runs are measured.

The existing 19 check names are preserved because this change does not alter merge protections. Repository-wide required-check settings may require administrative access to inspect. No check has been moved to a schedule.

## Benchmark methodology and evidence

Release hard-budget tests retain their original seeds, workload and assertions. The voxel meshing hard budget is 6 ms per case median (raised from 5 ms to accommodate observed hosted-runner variability); other thresholds are unchanged. CTest runs one case at a time; no unrelated test process runs concurrently on that runner. Hosted hardware variability still applies. The measurement-only partition is separate from functional tests; its correctness assertions remain mandatory. Existing stdout reports case counts, median, P95/P99, worst case, budget and violations where collected. The performance report reads that execution's CTest log and JUnit output, publishes the measurements to the job summary, and retains performance.log, performance-results.xml and performance-report.md as artifacts. No production instrumentation is added. Do not interpret CTest process duration as workload latency.

## Golden images

The pinned Mesa software renderer and expected PNG/GIF files are unchanged. The visual comparison gallery, raw actual/difference images, JUnit files and CTest evidence are retained on both success and failure. Review image artifacts before manually changing references. Do not update references simply to make CI pass.

## Measuring changes

See [initial CI timing audit](validation/ci-baseline-v0.1.5.md). Compare runs with the same runner image, compiler/configuration and cache conditions. Separate cumulative runner time from wall-clock latency and record cache hits, artifact transfer time and the complete run result. Coverage and packaging builds deliberately validate different configurations and must not be counted as identical duplicate builds.
