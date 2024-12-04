# Development Norms

This repository follows a strict C++ style enforced by clang-format and clang-tidy. The same conventions extend to any GLSL shader code.

## Formatting
- Run `clang-format -i` on all modified C++ source (`*.cpp`), header (`*.hpp`), and GLSL (`*.glsl`, `*.vert`, `*.frag`) files before committing. The configuration in `.clang-format` must be used. It uses tabs (width 4), `BreakBeforeBraces: Custom` and a column limit of 150.
- Ensure files end with a newline.

## Linting
- Run `clang-tidy` using the options from `.clang-tidy`. This enforces camelBack naming with prefixes:
  - Parameters must be camelBack prefixed with `p_`.
  - Private/Protected members and methods must be camelBack prefixed with `_`.
- The same naming and prefix conventions apply to GLSL functions and parameters.
- **Boolean checks**:
  - Boolean variables must always be compared explicitly with `== true` or `== false`.
    ```cpp
    if (isVisible == true) { ... }
    if (hasUpdated == false) { ... }
    ```
  - Do **not** add `== true` or `== false` to:
    - Pointer/nullptr checks (`ptr != nullptr`, `ptr == nullptr`).
    - Value/object comparisons (`a == b`, `vec2A == vec2B`).
    - Integer/enum comparisons (`count > 0`, `mode == Mode::X`).
    ```cpp
    if (tile != nullptr) { ... }        // correct
    if (vecA == vecB) { ... }           // correct
    if ((_collisionRenderer != nullptr) == true) { ... } // not allowed
    ```
- Each header (`*.hpp`) must only contain function and class declarations. Their corresponding source (`*.cpp`) files must hold the implementations.
- Avoid forward declarations when a header with the full type definition can be included instead.
- Address any warnings that relate to the modified code.

## Building & Testing
- Configure the build with `cmake --preset test-debug` and build with `cmake --build --preset test-debug`.
- Run the test suite with `ctest --preset test-debug`.
- If these commands fail due to missing dependencies, note this in your PR message.