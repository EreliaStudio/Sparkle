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
