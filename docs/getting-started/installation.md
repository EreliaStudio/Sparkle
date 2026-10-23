# Installing and building Sparkle

Requirements for the established graphics build: C++23, CMake 3.25+, supported Windows toolchain, OpenGL, GLEW and stb. The existing project uses vcpkg in its documented Windows build.

## Consume a packaged target

~~~cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS Core)
target_link_libraries(my_target PRIVATE sparkle::core)
~~~

For the graphical target when installed with its dependencies:

~~~cmake
find_package(sparkle CONFIG REQUIRED COMPONENTS Core Graphics)
target_link_libraries(my_target PRIVATE sparkle::sparkle)
~~~

The staged overlay port is named `erelia-sparkle`; it has not been described here as already accepted into the official vcpkg curated registry. See the [vcpkg publication plan](../internal/plans/vcpkg-publication.md).

## Build from source

Use the existing CMake presets or configure an explicit build. For a headless build:

~~~sh
cmake -S . -B build/core -G Ninja -DSPARKLE_BUILD_GRAPHICS=OFF
cmake --build build/core
~~~

For the established Windows/vcpkg graphics build, consult the [root README](../../README.md). Enable `SPARKLE_BUILD_TESTS=ON` to build Sparkle's test suite, and ensure dependencies for the tests feature are available. `SPARKLE_BUILD_TEST_LIBRARY` requires graphics.

## More

- [Prebuilt Windows packages](prebuilt-windows-package.md)
- [Building and testing as a contributor](../contributing/building-and-testing.md)
- [First Core application](first-core-application.md)
