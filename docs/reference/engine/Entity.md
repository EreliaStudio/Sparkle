# spk::Entity

**Header:** [`engine/entity.hpp`](../../../include/engine/entity.hpp)  
**Tests:** `tests/TU/Core/srcs/engine/engine_test.cpp`

An Entity is an Engine-context object with a parent/child hierarchy, activation state, geometry, name, Components and Behaviours. `Entity2D` and `Entity3D` add corresponding transforms.

## Ownership and relationships

An Entity is not automatically owned by its parent. Its `InherenceTrait<Entity>` tracks structural parent/child relationships. The Entity **does** own Components and Behaviours created by `addComponent<T>()` and `addBehaviour<T>()`. Their returned references remain meaningful while the corresponding attachments exist.

Activation affects eligibility to update, with effective activation depending on the hierarchy. `setParent(...)` reparents the Entity through its inheritance trait.

## Main operations

| Operation | Meaning |
| --- | --- |
| `Entity(name, parent)` | Construct an Entity with an optional parent |
| `addComponent<T>(...)` | Construct and own a Component |
| `addBehaviour<T>(...)` | Construct and own a Behaviour |
| `removeComponent(...)`, `removeBehaviour(...)` | Remove owned attachments |
| `isEffectivelyActive()` | Query effective activation |

See [Engine](Engine.md), [create an Entity](../../guides/create-an-entity.md), and [Engine architecture](../../architecture/engine.md).
