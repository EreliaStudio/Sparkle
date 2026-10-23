# Networking architecture

Sparkle exposes Client and Server as the public transport endpoints. A `Connection` represents a connection attempt/session with `Disconnected`, `Pending`, or `Connected` status. `Message` carries a type, optional request identifier and a ByteStream payload; `Transfer` reports an outgoing transfer outcome.

~~~mermaid
flowchart LR
    Client["Client"] -->|"connection attempt"| Connection["Connection"]
    Client -->|"Message"| Transport["Paged transport"]
    Transport -->|"Message"| Server["Server"]
    Client --> ClientDispatcher["MessageDispatcher"]
    Server --> ServerDispatcher["MessageDispatcher"]
    click Client href "../reference/network/Client.md" "Client API"
    click Server href "../reference/network/Server.md" "Server API"
    click Connection href "../reference/network/Connection.md" "Connection API"
    click Transport href "../guides/send-network-messages.md" "Messages and transfers"
    click ClientDispatcher href "../guides/network-message-dispatch.md" "Message subscriptions"
    click ServerDispatcher href "../guides/network-message-dispatch.md" "Message subscriptions"
~~~

Connections are established asynchronously: `Client::connect` returns a handle rather than guaranteeing immediate success. Workers deliver received messages into queues; `treatMessages()` calls subscribers synchronously on the caller's thread. Choose manual queue consumption **or** dispatcher treatment for a given queue.

Implementation includes framing, handshake, pagination, acknowledgment, admission and transport budgets. Application guidance belongs in [send network messages](../guides/send-network-messages.md) and [network message dispatch](../guides/network-message-dispatch.md); detailed limits and policy parameters should be read from current public headers.

[Client](../reference/network/Client.md) · [Server](../reference/network/Server.md) · [Connection](../reference/network/Connection.md).
