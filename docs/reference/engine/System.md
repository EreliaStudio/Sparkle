# spk::System

**Header:** [`engine/system.hpp`](../../../include/engine/system.hpp)  
**Tests:** `tests/TU/Core/srcs/engine/engine_test.cpp`

A System is an activable, named, updatable object contextualized to an Engine. It is typically created and owned by `Engine::addSystem<T>(...)`, which attaches it to the Engine.

To implement update behavior, derive from `System` and override the protected `_updateState(UpdateContext&)` hook provided by `UpdatableTrait`. See [create an Entity](../../guides/create-an-entity.md) for an example of combining Entities and Systems.

The relationship with Engine and other classes is shown in [Engine architecture](../../architecture/engine.md).
