# Send network messages

The public networking API separates message construction, asynchronous connection state, outgoing Transfer results and incoming dispatch.

## Construct a message

~~~cpp
#include "network/message.hpp"

#include <cstdint>

spk::Message::Writer writer(100);
writer << std::uint32_t{42};
spk::Message message = std::move(writer).build();
~~~

`Message::Writer` is the explicit payload builder. The receiver decodes with `message.reader()`, for example `reader.get<std::uint32_t>()`. Message types are application-assigned identifiers.

## Connect and send

~~~cpp
spk::Client client;
spk::Connection session = client.connect("127.0.0.1", 40000);
// Connect starts an attempt. Only send after the session reaches Connected.
if (session.status() == spk::Connection::Status::Connected)
{
    spk::Transfer transfer = client.send(message);
}
~~~

Use an application update loop or a status-change contract to wait for connection completion; do not assume that the immediate status is Connected. A running Server can use `start(port)`, `sendTo(connection, message)`, or `sendToAll(message)`. Sending to a disconnected client is an invalid operation.

## Subscribe to incoming messages

The client receives `spk::Message`; the server receives `spk::ReceivedMessage`, which also identifies the originating connection.

~~~cpp
auto subscription = client.messageDispatcher().subscribeTo(
    100,
    [](const spk::Message &received) {
        const auto value = received.reader().get<std::uint32_t>();
        // Handle value in the application.
    });

// In the application's update loop, on the calling thread:
client.treatMessages();
~~~

Keep the returned subscription contract alive for as long as the handler is needed; its destruction unsubscribes it. For a Server handler, use `const spk::ReceivedMessage &` and its `emitter` when routing a response. Dispatcher callbacks run synchronously during `treatMessages()`, not on the network worker thread. Choose **either** the manual `messages()` drain **or** `treatMessages()` for a given receive queue; do not mix consumers.

See [Client](../reference/network/Client.md), [Server](../reference/network/Server.md) and [Connection](../reference/network/Connection.md).
