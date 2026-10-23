# Engine architecture

The headless Engine manages its root Entity and its Systems. Entities can contain other Entities through `InherenceTrait<Entity>` and own Components and Behaviours added through their typed creation methods. Entity context identifies the Engine; Registries can query registered objects within a context.

~~~mermaid
flowchart TD
    Engine["Engine"]
    Entity["Entity"]
    System["System"]
    Component["Component"]
    Behaviour["Behaviour"]
    Registry["Registry"]
    Engine -->|"root hierarchy"| Entity
    Engine -->|"owns added systems"| System
    Entity -->|"owns added components"| Component
    Entity -->|"owns added behaviours"| Behaviour
    Registry -.->|"tracks context objects"| Entity
    click Engine href "../reference/engine/Engine.md" "Engine reference"
    click Entity href "../reference/engine/Entity.md" "Entity reference"
    click System href "../reference/engine/System.md" "System reference"
    click Component href "../reference/engine/Entity.md" "Entity attachments"
    click Behaviour href "../reference/engine/Entity.md" "Entity attachments"
    click Registry href "../reference/engine/Entity.md" "Registry overview"
~~~

The Engine's `addEntity(Entity*)` operation is non-owning: caller lifetime still matters. In contrast, `addSystem<T>()`, `addComponent<T>()`, and `addBehaviour<T>()` create objects through `unique_ptr` and transfer ownership to their collections.

The update mechanism uses `UpdateContext` and `UpdatableTrait`; Systems and eligible Entities/Behaviours participate in an Engine update. Registries follow context changes, not merely a single container's membership.

See [Engine](../reference/engine/Engine.md), [Entity](../reference/engine/Entity.md), [System](../reference/engine/System.md), and [create an Entity](../guides/create-an-entity.md).
