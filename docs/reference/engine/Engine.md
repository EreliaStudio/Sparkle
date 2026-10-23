# spk::Engine

**Header:** [`engine/engine.hpp`](../../../include/engine/engine.hpp)  
**Tests:** `tests/TU/Core/srcs/engine/engine_test.cpp`

The Engine owns its root Entity and manages a collection of Systems. It attaches external Entities to its hierarchy, propagates geometry, and updates eligible objects using an `UpdateContext`.

## Important operations

| Operation | Meaning |
| --- | --- |
| `addEntity(Entity*)`, `removeEntity(Entity*)` | Attach/detach an existing Entity; do not transfer ownership |
| `addSystem<T>(...)`, `removeSystem(System&)` | Create/own and remove a System |
| `root()` | Access the root Entity |
| `handleGeometryChange(...)` | Update Engine geometry |
| `updateState(UpdateContext&)` | Run the Engine update |

The Entity must outlive its attachment to the Engine. Use [first Core application](../../getting-started/first-core-application.md) for a runnable-shaped example, [Entity](Entity.md) for child attachments, and [Engine architecture](../../architecture/engine.md) for relationships.
