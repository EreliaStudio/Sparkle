# Generic network replication

C++23, header-only replication primitives in `spk::Network`, available through
`<network/network.hpp>` or individual headers and linked with `sparkle::core`.
The initial API deliberately keeps transport binding and application interest
policy explicit; it does not depend on an entity engine or a spatial grid.

## Organization

One top-level class/struct per header. Nested types remain with the class they
belong to. Enums and the codec concept also have their own small headers.
`types.hpp` contains only shared aliases. No `Detail` namespace or umbrella
`replication.hpp` remains.

| Header under `include/network/replication/` | Type / responsibility |
| --- | --- |
| `sequence.hpp` | Public monotonic `Sequence` with overflow checking |
| `publisher.hpp` | `Publisher<State>`: snapshots, followers and fair send queues |
| `receiver.hpp` | `Receiver<State>`: session, tracking and revision validation |
| `update.hpp` | Immutable `Update<State>` envelope |
| `request.hpp` | Correlated acquisition `Request` |
| `reply.hpp` | Acquisition `Reply` and nested result enum |
| `request_queue.hpp` | Client requests, timeouts and retry policy |
| `request_service.hpp` | Server decisions, stale completion rejection and Ready ordering |
| `protocol.hpp` | `Protocol<State, Codec>`: actual Sparkle Message encoding |
| `state_codec.hpp` | Public `StateCodec` concept |
| `edit.hpp` | Set, Forget and Destroy enum |
| `failure.hpp` | Transient and Permanent failure enum |
| `types.hpp` | UUID identity aliases and steady Clock |

Suggested reading order: `sequence`, `update`, `receiver`, `publisher`,
`request_queue`, `request_service`, then `protocol`.

The executable serialization example is in `examples/network_replication/`.
Its state and codec each have their own header. The Core unit suite compiles
and exercises the example along with the production primitives.

## Build and tests

```sh
cmake -S . -B build/core-test \
  -DSPARKLE_BUILD_GRAPHICS=OFF \
  -DSPARKLE_BUILD_TESTS=ON \
  -DCMAKE_PREFIX_PATH="/path/to/gtest-install"
cmake --build build/core-test --target SparkleCoreTestSuite
ctest --test-dir build/core-test -R '^Core.Network(Replication|Request|Sequence)' \
  --output-on-failure --no-tests=error
```

Tests reside in `tests/TU/Core/srcs/network/replication/` and use the existing
Core test discovery and Linux/Windows Debug/Release PR jobs. Installed packages
include these headers through the existing public include-directory installation.

## Use

The server publishes a state with `publish(id, state)` and chooses recipients
with `follow(peer, id)`. Repeated follow is idempotent. `forget(peer, id)` ends
one client's tracking; `destroy(id)` removes the authoritative published object.

For client-requested objects:

1. Client calls `RequestQueue::request`, then sends attempts from `due(now)`.
2. Server calls `RequestService::receive`. A true result requires a new
   application authorization/acquisition decision.
3. For a new provider result, call `fulfill(peer, request, state)`. It rejects
   an obsolete attempt before publishing its result. For an already published
   state, call `accept(peer, request)` instead.
4. Dispatch the publisher, then dispatch the request service on the same ordered
   transport. Ready waits for the initial snapshot to be accepted for sending.
5. Client decodes and applies the accepted update, then consumes the Ready reply.

Generation and authorization remain application responsibilities. Provider work
must be idempotent or deduplicated: a timeout does not prove the server has not
started an acquisition. Multiple business reasons to follow one object must be
combined before calling forget; the publisher does not reference-count reasons.

## Contracts and current scope

- Single owner thread. Sender callbacks must not reenter their publisher/service.
- Publisher outlives the RequestService that references it.
- `open(peer)` produces a session to communicate through a trusted handshake.
  Initialize the client receiver and request queue with that session. Ordinary
  updates cannot replace it. The application clears local replicas on transition.
- Close both the RequestService and Publisher for a disconnected peer. Clear
  obsolete transport queues on reconnect.
- Sender returns true only after acceptance by a reliable ordered transport.
  A failed send retains its update but other peers still get a turn.
- One FIFO per peer plus rotation between peers. Pending updates coalesce per
  object without changing their queue position.
- Publication interval limits change publication frequency; it does not repeatedly
  send unchanged snapshots. Coalescing describes final replicated state, not
  every intermediate gameplay event.
- State values must be genuinely immutable, without mutable aliases into game
  objects. Receivers decode State once and never construct application entities.
- Old sessions, tracking lifetimes and revisions are rejected. Termination
  history is bounded and retained until session reset; overflow fails explicitly.
- Ready is ordered after the snapshot accepted by transport, not an application
  acknowledgement from the remote client.
- Defaults: 15-second acquisition timeout; retry delays of 1, 2, 4, then 8 seconds;
  maximum eight attempts. Failed requests remain observable. Release and request
  again to manually retry a terminal failure.
- `RequestQueue::release` cancels only the local acquisition transaction.
  Remote interest release still needs an application control message translated
  to Publisher::forget. There is no generic Unsubscribe/provider cancellation yet.
- No automatic socket binding, handshake, heartbeat or lease implementation.
  Protocol::kind distinguishes Update, Request and Reply. Each typed channel
  needs a distinct application-selected message type.
- No Erelia grid, spatial indexing, entities, rendering or global-world reset.
- No delta compression, prediction, interpolation or cross-endian encoding.
- Limits bound object counts, peers, history, frame size and dispatch attempts;
  there is not yet a global byte budget for snapshots and transport queues.
- Codec validates semantic bounds and lengths before allocating. Output frame
  size is checked after encoding, not as an allocation cap inside the codec.
- No transactional rollback guarantee under allocation failure or application
  exceptions. Accepted state data is not an atomic application batch.

## Validation

The Core unit scenarios include Sequence advancement and exhaustion. Coverage includes 65 continuously dirty
objects, blocked/throwing senders, coalescing, tracking termination/reintroduction,
sessions, timing, retries, stale responses/provider results, decode-once behavior,
all frame truncation boundaries, and Ready ordering.

Local validation is recorded in the PR. Windows execution remains a CI check.
The unit fixtures use typed envelopes and real Message serialization; they do not
claim real-socket integration coverage.
