# spk::RenderSnapshot

**Header:** [`rendering/render_snapshot.hpp`](../../../include/rendering/render_snapshot.hpp)  
**Tests:** `tests/TU/Graphics/srcs/rendering/render_snapshot_test.cpp`

A RenderSnapshot groups `RenderPass` instances to be executed with a `RenderContext`.

`RenderSnapshot::Builder::renderPass(key)` gets or creates a pending pass, and `build()` produces the resulting snapshot. `RenderSnapshot::execute(renderContext)` executes it. Commands and their resource lifetimes should be designed with the snapshot's execution environment in mind; do not assume mutable borrowed update-thread resources remain valid.

See [rendering architecture](../../architecture/rendering.md).
