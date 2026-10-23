# spk::Server

**Header:** [`network/server.hpp`](../../../include/network/server.hpp)  
**Tests:** `tests/TU/Core/srcs/network/server_test.cpp`

Server manages a listening endpoint and multiple `Connection` handles.

| Operation | Purpose |
| --- | --- |
| `start(port)`, `stop()` | Start/stop accepting clients |
| `connections()`, `connection(id)` | Inspect connected sessions |
| `sendTo(connection, message)` | Send to one recipient |
| `sendTo(connections, message)` | Send to a chosen set |
| `sendToAll(message)` | Send to the current audience |
| `messages()`, `messageDispatcher()`, `treatMessages()` | Receive manually or by synchronous dispatch |

`start(0)` requests an ephemeral port and `port()` returns the chosen port after startup. `sendToAll` with no connections returns an empty results map. Queue limits, budgets, failure limits and policy enums are documented by the named public APIs in the header.

See [Client](Client.md), [Connection](Connection.md), and [send network messages](../../guides/send-network-messages.md).
