# Prebuilt Windows package

`.github/workflows/prebuilt-windows-package.yml` produces Sparkle's reusable Windows package. It configures a clean Release build with clang-cl, static CRT, static Sparkle, and `SPARKLE_BUILD_TEST_LIBRARY=ON`, then installs through CMake into a clean prefix.

The archive is named:

`sparkle-<version>-g<full-commit>-windows-x64-clangcl-static-release.zip`

It is published with its SHA-256 file on the revision-specific GitHub Release `sparkle-v<version>-g<full-commit>`. Existing releases are never updated or overwritten. Feature-branch packages are prereleases; pushes to `main`, including pull-request merges, produce normal releases automatically. A push to either `main` or a `feat/prebuilt-*` branch rebuilds the package. The workflow can also be started manually for its selected immutable revision.

The archive is the unmodified shape of a `cmake --install` prefix plus `package-metadata.json`. It contains `sparkle::sparkle` and `Sparkle::TestLibrary`. GLEW and OpenGL remain normal external package dependencies resolved by `sparkleConfig.cmake`.

Before publication, CI deletes the isolated Sparkle source and build directories, relocates the install prefix, and builds two external consumers. One uses `find_package(sparkle CONFIG REQUIRED)` and `sparkle::sparkle`; the other requests `COMPONENTS TestLibrary` and links `Sparkle::TestLibrary`. Installed CMake metadata is also rejected if it embeds the isolated source or build path. Compiler debug records may contain diagnostic source names, but neither linking nor execution is allowed to depend on those paths.
