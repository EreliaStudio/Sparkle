# Proposed microsoft/vcpkg submission: erelia-sparkle 0.1.3

## Scope
Copy the exact `ports/erelia-sparkle` directory from this preparation folder into a fork of `microsoft/vcpkg`.

- `erelia-sparkle`: Core C++23 static library, no graphics deps
- `erelia-sparkle[graphics]`: Core + Graphics, GLEW/OpenGL/stb
- `erelia-sparkle[test-library]`: Core + Graphics + reusable TestLibrary
- Supported: Windows x64 static CRT builds, as declared by `supports`
- Source: immutable tag `sparkle-v0.1.3-g5117c64a7052457ca2de1864bf23e0e29487bdcf`
- SHA512: `66d7432276d3d0ccb30f1297d187c0b230e5b6fbdec6ae8b07534f1e9b2b4f4d01fb021f30001f1abae04ca8b54d4ec3b597d054f2adf30c8d4f35220185128c`
- Licence: MIT for Sparkle and SIL OFL-1.1 for its bundled Liberation Sans font.

## Version database
Add this version to `versions/baseline.json` (alphabetically near other e- entries):

```json
"erelia-sparkle": { "baseline": "0.1.3", "port-version": 0 }
```

Add `versions/e-/erelia-sparkle.json` from this preparation folder. The exact `git-tree` is `c6c0378d0028a6f2a74a9be2adae0d8c0f0c8eb2`, calculated from the staged three files. Prefer rerunning `vcpkg x-add-version erelia-sparkle` in the target vcpkg fork to confirm/refresh.

## PR description
Add `erelia-sparkle` version 0.1.3, a C++23 library with independent Core target (`sparkle::core`), opt-in graphics (`sparkle::sparkle`), and optional test utilities (`Sparkle::TestLibrary`). The port builds from the published immutable source archive, tests all three consumption configurations, and installs CMake targets and MIT/OFL license files. Support is scoped to the official `x64-windows-static` triplet.

Sparkle's direct public predecessor `Hyarius/Sparkle` has been under public development since April 3, 2026. The current maintained upstream is `EreliaStudio/Sparkle`. Public upstream CI includes Core-only, graphical, and TestLibrary external-consumer tests.

## Remaining before official PR
1. Fork `microsoft/vcpkg` under Hyarius or EreliaStudio and grant the connected GitHub app access.
2. Copy these three port files, update version database, and run `vcpkg x-add-version erelia-sparkle`.
3. Open draft PR into `microsoft/vcpkg:master`; discuss feedback and pass vcpkg CI.
4. Official publication completes **only when maintainers merge the PR**.
