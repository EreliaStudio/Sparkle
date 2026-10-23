# spk::Client

**Header:** [`network/client.hpp`](../../../include/network/client.hpp)  
**Tests:** `tests/TU/Core/srcs/network/client_test.cpp`, `message_dispatcher_integration_test.cpp`

Client manages one connection attempt/session, outgoing messages, incoming message queue and dispatcher.

- `connect(address, port, behaviour)` begins an attempt and returns a `Connection` handle (normally initially Pending); it does not wait for the handshake to finish.
- `connection()` and `isConnected()` inspect its current state; `disconnect()` terminates the session.
- `send(message, priority)` submits an outgoing transfer when the session permits it.
- `messages()` exposes the incoming queue. Alternatively, subscribe through `messageDispatcher()` and call `treatMessages()` on the calling thread.
- Configuration methods control handshake, incoming reassembly, pages, timeouts and failures.

The default `ConnectBehaviour::Throw` rejects replacing an existing session unless `DisconnectPrevious` is requested. Sending while disconnected is invalid. See [Connection](Connection.md), [send network messages](../../guides/send-network-messages.md) and [message dispatch](../../guides/network-message-dispatch.md).
