# Architecture overview

Sparkle is organized around a headless Core and an optional application/rendering target. Source directories describe responsibilities, not necessarily CMake target boundaries: notably, `src/core/` is compiled into the optional graphical `sparkle` target.

| Domain | Representative public types | Source |
| --- | --- | --- |
| Containers and data | `ByteStream`, `Flags`, `ThreadSafeFIFO`, JSON | `include/container/` |
| Traits and lifecycle | `ContractProvider`, `ActivableTrait`, `InherenceTrait`, `Publishable` | `include/design_pattern/` |
| Engine | `Engine`, `Entity`, `Component`, `Behaviour`, `System`, `Registry` | `include/engine/` |
| Networking | `Client`, `Server`, `Connection`, `Message`, `Transfer` | `include/network/` |
| Multiplayer | `PublishableEntity`, `ServerEntityHierarchy`, `ClientEntityHierarchy` | `include/engine/` |
| Rendering | `RenderSnapshot`, `RenderPass`, render commands | `include/rendering/` |
| UI | `Widget`, layouts, `ModelView` | `include/ui/` |

Follow each domain through the [architecture index](README.md). For implementation rules see [architecture guidelines](../contributing/architecture-guidelines.md).
