# Library overview

Sparkle 0.1.3 is a C++23 static library. Its CMake configuration defines two consumer-facing targets.

| CMake target | Purpose | Selected source domains |
| --- | --- | --- |
| `sparkle::core` | Headless engine and utility functionality | Containers, math, traits, Engine, networking, diagnostics, threading |
| `sparkle::sparkle` | Graphics/application functionality layered on Core | Application/window runtime, rendering, OpenGL, input, widgets |

The `SPARKLE_BUILD_GRAPHICS` CMake option controls the optional graphics target. The project currently documents Windows x64 as its validated public graphics distribution, while networking also contains Linux epoll support. Do not infer full cross-platform graphics support from the networking implementation.

Sparkle provides distinct umbrella headers `<sparkle_core.hpp>` and `<sparkle_graphics.hpp>`, but **use the specific header for a class when unsure**; some recently added APIs have not been added to the umbrella headers.

Continue to [installation](installation.md) or the [architecture overview](../architecture/README.md).
