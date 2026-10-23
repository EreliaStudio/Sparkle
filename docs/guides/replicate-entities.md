# Replicate selected Entities

Replication combines the headless Engine Entity hierarchy, `PublishableEntity` Parts, a server-side selection hook and a client-side factory.

## Server responsibilities

1. Derive concrete entities from `PublishableEntity2D` or `PublishableEntity3D` and construct Sources with `Mode::Source`.
2. Derive from `ServerEntityHierarchy`; override `selectEntities(const Connection&)` to return the Source objects visible to that client.
3. Register concrete types with `registerType<TEntity>(id)` and attach Source entities beneath `serverHierarchy.root()`.
4. Bind to a `Server`; call `publish(server.connections(), Publishable::UpdateSelection::All)` for a full update, then use `Modified` as appropriate.

The selection hook is pure virtual: Sparkle does not implicitly select all entities. Required ancestors are included by the hierarchy implementation.

## Client responsibilities

1. Construct `ClientEntityHierarchy` with the same three message IDs.
2. `bind(client)` and `registerType(typeID, factory)`.
3. The factory returns `std::unique_ptr<Entity>` to a concrete Entity that also derives from `PublishableEntity`, initialized in `Mode::Replicant` with the authoritative UUID.
4. Pump `server.treatMessages()` and `client.treatMessages()` on their owning loops so initialization requests and replies are dispatched.
5. Use `clientHierarchy.find(uuid)` to access a created replica.

## Message flow

~~~mermaid
sequenceDiagram
    participant S as Server hierarchy
    participant C as Client hierarchy
    S->>C: Full publication (selected IDs and updates)
    C->>S: Request missing UUIDs
    S->>C: Initialization response
    Note over C: Create via registered factory
    S->>C: Later modified publication
~~~

The end-to-end integration test at `tests/TU/Core/srcs/engine/entity_hierarchy_integration_test.cpp` is the source-backed example for construction, type registration, per-client selection, ancestor inclusion, updates, and removal. See [EntityHierarchy](../reference/multiplayer/EntityHierarchy.md) and [multiplayer architecture](../architecture/multiplayer.md).
