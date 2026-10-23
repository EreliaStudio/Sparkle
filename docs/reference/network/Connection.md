# spk::Connection

**Header:** [`network/connection.hpp`](../../../include/network/connection.hpp)

A copyable handle to shared connection state, with a process-wide connection ID. It is **not** a socket or an owning Server/Client.

- `status()`: `Disconnected`, `Pending`, `Connected`
- `error()`: latest connection outcome reason, e.g. refusal or timeout
- `id()`, `isConnected()`: identity and convenience status
- `subscribeToStatusChange(callback)`: a lifetime-bound contract
- `maxQueuedBytes()` and `setMaxQueuedBytes(...)`: outgoing admission configuration
- `ConnectionSet`: unordered set with stable hashing and equality

Use the status rather than assuming `Client::connect()` has finished. See [Client](Client.md) and [networking architecture](../../architecture/networking.md).
