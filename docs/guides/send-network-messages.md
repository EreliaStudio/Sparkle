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

## Receive and reply

The client receives `spk::Message`; the server receives `spk::ReceivedMessage`, which includes the emitter connection ID. Subscribe through `messageDispatcher()` and call `treatMessages()` from the owning update loop. The fully explained callback example is in [network message dispatch](network-message-dispatch.md).

See [Client](../reference/network/Client.md), [Server](../reference/network/Server.md) and [Connection](../reference/network/Connection.md).
