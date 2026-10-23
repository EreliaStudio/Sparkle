# Rendering and graphics

Sparkle's optional graphics target provides GPU resources, mesh types, rendering commands, passes, snapshots and graphical application infrastructure.

~~~mermaid
flowchart LR
    Contributor["Snapshot contributors"] --> Builder["RenderSnapshot::Builder"]
    Builder --> Snapshot["RenderSnapshot"]
    Snapshot --> Pass["RenderPass"]
    Pass --> Commands["Render commands"]
    Commands --> OpenGL["Graphics resources / OpenGL"]
    click Snapshot href "../reference/rendering/RenderSnapshot.md" "RenderSnapshot reference"
    click Builder href "../reference/rendering/RenderSnapshot.md" "Snapshot builder"
~~~

`RenderSnapshot::Builder` groups commands into passes and produces a snapshot that can be executed with a `RenderContext`. Specialized rendering Engine objects live under `include/rendering/engine/`.

Graphics resources include images, sprite sheets, fonts, textures and OpenGL buffers. Resolution-family behavior is covered by [texture resolution families](../guides/texture-resolutions.md). For the design see `include/rendering/`, `include/graphics/`, and [RenderSnapshot reference](../reference/rendering/RenderSnapshot.md).

The exact threading, lifetime, and GPU ownership rules of custom rendering commands must be verified against implementation and tests; this overview deliberately avoids asserting that borrowed mutable objects are safe to capture in snapshots.
