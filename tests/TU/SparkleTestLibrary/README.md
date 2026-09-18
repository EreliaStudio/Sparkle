# Reusable test utilities

Build with `-DSPARKLE_BUILD_TEST_LIBRARY=ON -DSPARKLE_BUILD_TESTS=OFF`, then install Sparkle normally. This installs headers and the static library without requiring GTest or building SparkleTestSuite. The overlay vcpkg port offers the `test-library` feature. Enabling Sparkle's own tests also builds/installs the utilities.

```cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS TestLibrary)
target_link_libraries(my_tests PRIVATE Sparkle::TestLibrary)
```

```cpp
#include <sparkle_test.hpp>

// Configure once before tests/worker threads; these directories belong to your project.
sparkle_test::configurePaths(referenceResourceRoot, writableResultRoot);
// Defaults without configuration: resources/ and results/ beside the test executable.
auto result = sparkle_test::compareImages(actualPng, expectedPng, differencePng);
```

`expectedImagePath(category, name)` resolves under `resources/expectedImages`; `resultImagePath` resolves under the configured result root. No Sparkle checkout/build paths are embedded in the installed utilities. Configuration is process-wide and must not race with tests; empty roots throw without changing existing configuration.

PNG comparison does not initialize OpenGL. On success the comparator deletes actual/difference files; never use your reference path as the actual/result path. On mismatch it retains actual output and writes a red difference PNG. References are never automatically accepted. See `ImageComparisonOptions` for per-channel tolerances.

`OpenGLTestContext::instance()` creates a hidden Windows/WGL context lazily and must be used on its creating runner thread. The current capture framebuffer is 640×480; use that size. Exporting these utilities does not port Sparkle or GPU capture to Linux. No test framework is required by the public utilities.

## Installed-package smoke test

On the supported Windows toolchain, build/install Sparkle with the test library enabled and tests disabled, then configure `tests/package-consumer` with the installed prefix and the same dependency toolchain. Build it and run `ctest --test-dir <consumer-build> --output-on-failure`. Repeat after relocating the install prefix, with the original checkout/build inaccessible. The consumer uses only installed headers/targets, configures its own paths and checks both image-match and image-difference behavior without creating a GPU context.

Also verify that a default install omits the optional target and that `find_package(sparkle REQUIRED COMPONENTS TestLibrary)` rejects that install. Run SparkleTestSuite with tests enabled to validate its configured resource paths and existing GPU tests.

## Continuous integration

`.github/workflows/ci.yml` runs on pull requests (including drafts), pushes to
`Version0.1.1`/`main`, and manual dispatch. It builds and runs the full unit suite
in Debug and Release on Windows with LLVM and Mesa 23.3.4 software OpenGL.
CI sets Mesa GL/GLSL overrides to 4.6/460 to match Sparkle's requested core
context and shader versions; this is not a claim of full driver conformance.
CTest runs serially because window/clipboard tests share desktop state; missing
tests and failures are errors. Image references are never regenerated in CI.
Logs, JUnit results and image differences are downloadable workflow artifacts.

Independent Debug/Release packaging jobs build with tests disabled and GTest
explicitly unavailable. They verify that the default install omits TestLibrary,
that the base package still configures/builds/runs, and that requesting the absent
component fails for the expected reason. They then enable TestLibrary, run the
installed consumer, move the install into a path containing a space, remove the
producer source/build trees, and rebuild/run a fresh consumer. Third-party
vcpkg dependencies remain available outside those trees.

The consumer checks executable-relative defaults, custom paths, preservation of
both roots after invalid input, image match cleanup, mismatch output and reference
preservation. LLVM coverage runs separately and uploads its HTML/profile report;
test failures remain blocking, with no arbitrary coverage percentage gate yet.
Software rendering validates the Windows OpenGL path, not vendor GPU drivers.
There is no documentation generation, Pages deployment or release publishing.

### Reviewing failed images

Each Debug/Release test job publishes a dedicated `visual-diffs-Debug` or
`visual-diffs-Release` artifact even when tests fail. The job summary links directly
to its download. Extract the ZIP and open `index.html` to compare reference,
actual and difference images side by side at their native resolution. The HTML
embeds the images, so it can also be opened independently on a phone. Separate
PNGs are organized in `reference/`, `actual/` and `difference/` with matching paths.
Artifacts are retained for 30 days. GitHub downloads these reports; it does not
host the HTML as a webpage.

This report copies existing test output without updating references or changing
comparison tolerances. Intentional mismatch fixtures from comparator unit tests
are excluded. If no reference-backed outputs remain, the report says so and
points to the logs; this alone does not mean the tests passed.
