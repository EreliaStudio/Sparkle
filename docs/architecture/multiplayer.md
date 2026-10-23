# Multiplayer: Publishable and EntityHierarchy

The `Publishable` abstraction encodes registered Parts. A Source tracks modified Parts and produces publications; a Replicant decodes received state. `PublishableEntity2D` and `PublishableEntity3D` combine regular Engine Entities with the publication model.

~~~mermaid
flowchart TD
    Source["Source PublishableEntity"]
    ServerHierarchy["ServerEntityHierarchy"]
    Server["Server"]
    Client["Client"]
    ClientHierarchy["ClientEntityHierarchy"]
    Replica["Replicant PublishableEntity"]
    Source --> ServerHierarchy
    ServerHierarchy -->|"publication"| Server
    Server -->|"network"| Client
    Client -->|"dispatch"| ClientHierarchy
    ClientHierarchy -->|"initialization / update"| Replica
    click Source href "../reference/multiplayer/Publishable.md" "Publishable contract"
    click ServerHierarchy href "../reference/multiplayer/EntityHierarchy.md" "Server hierarchy"
    click ClientHierarchy href "../reference/multiplayer/EntityHierarchy.md" "Client hierarchy"
    click Replica href "../reference/multiplayer/Publishable.md" "Replicant contract"
    click Server href "../reference/network/Server.md" "Server API"
    click Client href "../reference/network/Client.md" "Client API"
~~~

The server exposes `selectEntities(const Connection&)` as a pure virtual selection hook. It builds the selected hierarchy, including required ancestors, for each connection. The client tracks a published selection, requests initialization of missing objects and creates replicas through registered factories. Publications carry updates; initialization is handled separately on demand.

~~~mermaid
sequenceDiagram
    participant S as ServerEntityHierarchy
    participant C as ClientEntityHierarchy
    S->>C: Publication (selection and updates)
    C->>S: Initialization request (missing UUIDs)
    S->>C: Initialization response
    Note over C: Factory creates Entity and decodes Publishable state
    S->>C: Later publication (modified or all)
~~~

This sequence is a conceptual overview, not a wire-format specification. Read [Publishable reference](../reference/multiplayer/Publishable.md), [EntityHierarchy reference](../reference/multiplayer/EntityHierarchy.md) and [replication guide](../guides/replicate-entities.md). The integration test `tests/TU/Core/srcs/engine/entity_hierarchy_integration_test.cpp` provides a concrete verified scenario.
