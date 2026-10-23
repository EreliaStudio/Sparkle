# Network message subscriptions

`Client`, `Server`, and `RemoteNode::Endpoint` own a message dispatcher exposed by
`messageDispatcher()`. Keep the `Contract` returned by `subscribeTo(type, callback)`
alive for as long as the subscription is needed. Destroying or resigning the contract
unsubscribes it. Destroying the owner invalidates its contracts.

```cpp
auto subscription = client.messageDispatcher().subscribeTo(
    responseType,
    [](const spk::Message &message) {
        auto reader = message.reader();
        // Decode application state here.
    });

// Call from the application's update loop.
client.treatMessages();
```

The nested `Client::MessageDispatcher`, `Server::MessageDispatcher`, and
`RemoteNode::Endpoint::MessageDispatcher` types expose `Provider`, `Callback`, and
`Contract` aliases. They specialize the shared `spk::MessageDispatcher<TIncoming>`
implementation, available in `<network/message_dispatcher.hpp>`.

| Owner | Callback argument | Reply path |
| --- | --- | --- |
| `Client` | `const spk::Message &` | `client.send(message)` |
| `Server` | `const spk::ReceivedMessage &` | `server.sendTo(received.emitter, response)` |
| `RemoteNode::Endpoint` | `const spk::RemoteNode::Endpoint::Request &` | `endpoint.reply(request, response)` |

Server callbacks retain the sender. Routed endpoint callbacks retain both the proxy
connection and the originating connection; do not reply to the proxy as if it were
the originating client. For delayed replies, copy the request envelope or necessary
routing information before returning from the callback. Callback references must
not be retained. Copying a `Message` shares its immutable payload storage.

## Pumping and ownership

`Client::treatMessages()` and `Server::treatMessages()` drain their receive queues
and synchronously notify matching subscribers on the calling thread. Socket workers
only enqueue received messages; subscriptions do not introduce a new worker.

For a routed endpoint, keep the existing unwrapping step:

```cpp
endpoint.dispatch();       // Unwrap incoming routed requests into requests().
endpoint.treatMessages();  // Notify subscribers of those unwrapped requests.
```

`dispatch()` continues to support existing manual consumers of `requests()` and does
not implicitly invoke subscribers. The existing client/server `messages()` APIs
also remain unchanged. Choose one consumer for each receive queue: manual draining
or dispatcher treatment. A dispatcher retains any unfinished batch after a callback
exception, so do not switch consumption modes while that batch remains pending.

## Delivery behavior

- Messages are delivered in receive-queue order. Each matching subscription receives
  the same immutable envelope; subscriptions run in registration order.
- Unknown message types are consumed without notification.
- Messages queued during a callback wait until a later treatment call. Treatment
  processes a finite drained batch, rather than chasing new arrivals indefinitely.
- Contracts may be resigned and subscriptions added during callbacks. A newly added
  subscription does not receive the message currently being dispatched. A resigned
  subscription is skipped even if its turn in that message has not arrived yet.
- Empty callbacks are rejected with `spk::Exception`.
- A callback exception propagates to the caller. Remaining subscribers for that
  message are not called and that message is not replayed. Later messages in the
  drained batch are retained for the next call; new queued messages are drained on
  a subsequent call after that retained batch completes.
- Recursive or concurrent treatment of the same dispatcher throws `spk::Exception`.
  Subscriptions may be registered or resigned from other threads, using the existing
  `ContractProvider` synchronization. Do not destroy the dispatcher/owner during a
  callback or concurrently with treatment, registration, or queue use.
- Dispatchers are not copied or moved. Connections stopping or restarting do not
  automatically discard subscriptions; applications own their contract lifetimes.
