# Architecture

Sparkle's main architectural domains are documented independently. Start with the [overview](overview.md), then choose a subsystem.

~~~mermaid
flowchart TD
    Foundation["Foundations and traits"]
    Engine["Engine"]
    Network["Networking"]
    Multiplayer["Multiplayer publication"]
    Rendering["Rendering and graphics"]
    UI["UI"]
    Foundation --> Engine
    Foundation --> Network
    Engine --> Multiplayer
    Network --> Multiplayer
    Engine --> Rendering
    Rendering --> UI
    click Foundation href "overview.md" "Foundations"
    click Engine href "engine.md" "Engine architecture"
    click Network href "networking.md" "Networking architecture"
    click Multiplayer href "multiplayer.md" "Entity publication"
    click Rendering href "rendering.md" "Rendering architecture"
    click UI href "ui.md" "UI architecture"
~~~

- [Overall architecture](overview.md)
- [Engine and registries](engine.md)
- [Networking](networking.md)
- [Multiplayer / Publishable / EntityHierarchy](multiplayer.md)
- [Rendering and graphics](rendering.md)
- [UI and widgets](ui.md)

Arrows represent conceptual dependencies, not guaranteed C++ include dependencies. Diagram click support depends on the Markdown renderer; the links above are the canonical navigation.
