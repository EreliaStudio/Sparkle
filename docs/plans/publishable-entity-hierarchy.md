# Publishable simplification and EntityHierarchy multiplayer plan

> **Status:** Documentation-only design plan, updated 21 October 2026. This PR does not implement the design.
>
> **Target:** `Version-0.1.3`. Draft [PR #31](https://github.com/EreliaStudio/Sparkle/pull/31) contains an earlier generic `Publishable` proposal; revise that API in its own implementation work before treating it as the foundation of this plan.
>
> **Development rules:** Follow `docs/ai-development-guidelines.md`: small coherent responsibilities, explicit behavior and ownership, C++23, `spk::Exception` for invalid operations, and no explanatory comments in newly modified public headers. Do not make unrelated Engine changes.

## 1. Intended Erelia usage and scope

Erelia creates and uses ordinary Engine Entities and Components. Sparkle observes the server-side Engine subtree, selects which publishable Entities each client needs, and synchronizes their state.

```cpp
serverHierarchy.registerType<Monster>(42);
auto monster = std::make_unique<Monster>(
    "Orc Warrior",
    &world,
    PublishableEntity::Mode::Source);
monster->transform().setPosition({10, 5, 0});

auto result = serverHierarchy.publish(UpdateSelection::Modified);
// Erelia periodically invokes publish(UpdateSelection::All).
```

The client instead registers a factory for the same numeric type ID. It constructs detached Replicants from initialization responses, then attaches them to an Erelia-owned Engine root after successful decoding.

Key constraints:

- **Erelia owns server Entities**. `ServerEntityHierarchy` observes them; it does not construct, retain ownership of, or delete them.
- **`ClientEntityHierarchy` owns its Replicants**. Local client Entities remain Erelia-owned and are never removed by network reconciliation.
- Ordinary Systems need not know about networking. Erelia chooses when to publish and how to determine Interest.
- No per-Entity-type network publisher Systems, special Interest class tree, partial/structural Entity types, network-owned gameplay systems, or implicit networking worker threads.
- The design is Entity-specific. Do not introduce `NetworkReplicationHierarchy<T>` without an actual non-Entity use case.

## 2. Simplify generic `spk::Publishable` (revise PR #31)

### 2.1 Unified Part model

`Publishable` is a non-template reusable object containing non-owning registrations of abstract `Publishable::Part` instances.

Each Part implements only:

```cpp
virtual void _encode(ByteStream::Writer&) const = 0;
virtual void _decode(const ByteStream::Reader&) = 0;
```

All serializable data belongs to Parts. Remove separate Edition transport, `encodeEdited()`, distinct Edition/Update encoders and caches, and initialization encoder/decoder callbacks.

- Every registered Part is included in initialization.
- A single outgoing `Publication::Update` flag enables ordinary updates. Without that flag, the Part is initialization-only.
- An initialization-only Part may still be invalidated; its new value appears in later initializations, but not in periodic updates.
- `encodeUpdate(UpdateSelection::Modified)` includes modified update-enabled Parts and consumes only modifications captured successfully.
- `encodeUpdate(UpdateSelection::All)` includes every update-enabled Part and **does not** consume pending modifications.
- `encodeInitialization()` includes every Part and **does not** consume pending modifications.
- Empty selections encode as empty streams. All modes use the same Part wire representation.

Each Part maintains one lazily baked, immutable `ByteStream` snapshot and **one `uint64_t` generation**. Snapshots bind their generation and bytes atomically with respect to game-thread mutation. `invalidate()` normally increments the Part generation, invalidates its cache, and marks update-enabled registration as modified. Multiple invalidations may be coalesced; intermediate states need not be published.

Each framed Part section contains its Part ID, generation, length and bytes. Initialization accepts generation zero on the first decode. After that, only a strictly newer received generation may decode; advance the stored generation **only after successful decoding**. Partial failures may leave already decoded Parts applied; there is no cross-Part transactional rollback. A game-defined decoder must preserve its own invariants if it throws. Malformed framing, unknown Part IDs in strict network mode, and unread/invalid section data throw.

**One Part may be registered in only one Publishable at a time.** Unregistration and destruction must be safe for non-owning references and existing registration lifetimes.

### 2.2 Source/Replicant generation policy

The generic `Publishable::Part` must not depend on Entity or network role types. Its owning publication/registration context can supply generic permission for local invalidation.

- In a **Source**, a local `invalidate()` increments its generation and queues publication.
- In a **Replicant**, a local `invalidate()` **does not increment the generation and does not queue outgoing work**. Local gameplay setters may still modify state.
- Incoming authoritative decoding sets that same generation to the received newer value after successful decode.
- No extra local-vs-remote generation counters and no timestamps.
- Replicants do not publish authoritative state; a repeated snapshot with an equal generation is not reapplied even if local prediction diverged. Erelia owns prediction reconciliation.
- Generic `Publishable` remains usable independently of Entity; role enforcement is configured by `PublishableEntity`, not baked into the Part type.

Game-thread-only mutation, invalidation, snapshot encoding and decode are assumed for this Engine integration. The network transports already captured immutable bytes asynchronously. Do not add gameplay data locks or silent background synchronization.

### 2.3 Illustrative generic API (not final header)

```cpp
class Publishable
{
public:
    using PartID = std::uint32_t;
    enum class Publication : std::uint8_t { Update = 1 << 0 };
    enum class UpdateSelection { Modified, All };

    class Part
    {
    protected:
        void invalidate();
        virtual void _encode(ByteStream::Writer&) const = 0;
        virtual void _decode(const ByteStream::Reader&) = 0;
    };

    ByteStream encodeInitialization() const;
    ByteStream encodeUpdate(
        UpdateSelection selection = UpdateSelection::Modified) const;
    void decode(const ByteStream::Reader& reader);

protected:
    void add(PartID id, Part* part, Flags<Publication> flags = {});
};
```

Preserve useful framing, lifetime and explicit unknown-Part safety from PR #31; do not copy the earlier Edition-based API.

## 3. Entity publication capabilities and Parts

### 3.1 Inheritance without an Entity diamond

`PublishableEntity` is a capability inheriting `Publishable`, **not `spk::Entity`**.

```cpp
class PublishableEntity : public Publishable
{
public:
    enum class Mode { Source, Replicant };
    // Immutable UUID, stored EntityTypeID after first resolution.
protected:
    void addReservedPart(PartID internalID, Part& part);
    void addPart(PartID publicID, Part& part, Flags<Publication> flags = {});
};

class PublishableEntity2D : public Entity2D, public PublishableEntity
{
    // Activation Part + PublishableTransform2D adapter
};
class PublishableEntity3D : public Entity3D, public PublishableEntity
{
    // Activation Part + PublishableTransform3D adapter
};
```

A non-spatial Entity can combine `Entity` and the publication capability and register an activation-only built-in Part. The hierarchy requires both polymorphic contracts and never assumes all publishable Entities are 2D or 3D.

`PublishableEntity::Mode` is an explicit immutable constructor argument. Source construction generates a stable process-lifetime-unique UUID; Replicant construction takes the authoritative server UUID. An Entity UUID is never reassigned or reused for a different server Entity lifetime. A temporarily untracked Source preserves its UUID.

`PublishableEntity` manages its identity, resolved numeric network type, reserved/public Part registration and local invalidation permissions. It knows neither spatial types nor Engine parenting nor connections.

### 3.2 Reserved Part IDs and adapters

Reserve internal Part IDs **0–15** for Sparkle:

| Internal ID | Purpose |
| --- | --- |
| 0 | Entity activation |
| 1 | Transform2D or Transform3D (only for spatial Entities) |
| 2–15 | Reserved for future Sparkle Parts |
| 16 onward | Erelia Parts: `internalID = publicID + 16` |

Public Erelia Part IDs remain zero-based; reject overflow above `UINT32_MAX - 16`. Generic `Publishable` keeps normal zero-based IDs without translation. Reserved and user IDs cannot collide; reject duplicate registrations.

The concrete 2D/3D wrappers own their built-in adapters as members, construct them, then call `addReservedPart(id, part)` in the constructor body. `PublishableEntity` registers non-owning Part references; it does not allocate adapters or take an `Entity&` in its base constructor. This avoids accessing not-yet-constructed derived members.

- `PublishableTransform2D` / `PublishableTransform3D` are **Part adapters**, not Engine Components.
- They hold non-owning references to existing `Transform2D` / `Transform3D` Components and subscribe to their edition events via RAII.
- The activation adapter likewise observes the existing `ActivableTrait` state.
- Both adapters may observe editions on either side; the publication role suppresses local generation advancement in Replicants. Decode must retain normal Engine notifications without generating outbound changes.
- Activation and Transform are **separate Parts**, with independent generations and cached snapshots.
- Synchronize local Transform position/rotation/scale (and 2D depth where applicable), not visual `Rect2D` geometry. The server may have no visual state.
- Ordinary Engine `Entity`, `Entity2D`, `Entity3D`, Transform, geometry, and parenting semantics must not be modified to embed network behavior.

`PublishableComponent` and `PublishableBehaviour` each combine the corresponding Engine attachment with `Publishable::Part`; activation is automatically part of each wrapper's serialized state. Game-defined Parts have fixed registration and schema for the Entity lifetime. No network-driven dynamic Part factories, removal tombstones or missing-Part requests.

Server and client concrete C++ classes may differ for a shared numeric type ID, but Erelia is responsible for compatible reserved/game Part layouts. Sparkle verifies framing/IDs/generations, not application schema versions.

## 4. Structural observation and type discovery

### 4.1 Enhance `InherenceTrait`

Add a direct-child subscription based on existing `ContractProvider` RAII contracts:

```cpp
enum class RelationEdition { Added, Removed };

subscribeToChildEdition(callback);   // (child pointer, relation edition)
subscribeToParentEdition(callback);  // (parent pointer, relation edition)
```

Notifications identify the affected pointer and whether it was Added or Removed. A reparent generates the corresponding removal and addition events. Child subscriptions report **direct children only**, not every descendant. Preserve existing Engine parent-context propagation and 2D/3D world-cache invalidation when adapting current parent subscription consumers. Define safe notifications during detach and destruction.

`ServerEntityHierarchy` subscribes to the supplied replication root and recursively subscribes to discovered children. When an already populated subtree is attached, traverse **that subtree once** to install subscriptions. An initial scan installs subscriptions for an existing root tree; do not rescan the entire Engine world on every publication.

**Construction-order constraint:** `Entity` can call `setParent()` from its base constructor before `PublishableEntity` and the concrete game class exist. On Child Added, record/observe the structural change **without dynamic-casting or reading UUID/type**. Resolve pending identity/type only at safe game-thread boundaries (`publish()` and initialization-request handling), when construction has completed. On removal/destruction, withdraw pending/tracked references without inspecting destroyed derived state. Construction/destruction must not overlap synchronization processing.

### 4.2 Source type registration

- Erelia registers exact concrete C++ types to explicit numeric `EntityTypeID` values, e.g. `registerType<Monster>(42)`.
- Resolve first discovery with `std::type_index(typeid(*entity))`. No implicit registration of derived types through a registered base.
- Keep RTTI local; transmit only numeric type IDs. Store the resolved type ID on the publishable Entity so removing/re-adding it to a root does not change its identity or type.
- Missing exact type registration is a configuration error: throw `spk::Exception` on safe discovery.
- Client registers the same numeric type ID with an explicit construction callback receiving `const UUID&` and returning a detached, owned Replicant `Entity`.
- Missing client-side numeric type registration throws `spk::Exception`; no placeholder or delayed-registration cache.
- Type registrations/unregistrations are explicit (not RAII). Reject conflicting numeric registrations and unsafe rebinding while affected instances exist.

## 5. Hierarchy organization, ownership and boundaries

`EntityHierarchy` itself is **not an Entity and is not templated**. It tracks non-owning references, UUID lookup, authoritative parent UUID relationships, pending-parent relationships, registered types and RAII structural subscriptions.

- `ServerEntityHierarchy` observes **Erelia-owned Source Entities** below an Erelia-owned replication root. It never destroys them.
- `ClientEntityHierarchy` owns its constructed Replicants in addition to using the common base's non-owning tracking. It destroys only those replicas when they leave Interest or on explicit clear.
- Client-local child Entities remain Erelia-owned and follow existing Engine detach/destruction behavior when a replicated parent disappears. The hierarchy does not implicitly reparent or delete them.
- Each hierarchy receives **one explicit Erelia-owned Engine root**. The logical UUID graph is separate from this physical attachment root.
- On the server, the root is the replication boundary: the path from each publishable descendant to the boundary must contain only publishable ancestors, excluding the boundary itself. Non-publishable children are allowed if they do not lead to publishable descendants. Invalid intermediate ancestry throws.
- A Source moved outside the boundary stops being tracked automatically; reattaching it rediscovers the same Entity and UUID. A move inside the boundary is reparenting, not destruction/recreation.
- On the client, replicas whose authoritative parent is null or not yet initialized are temporarily attached to the Erelia-owned attachment root. Keep their desired parent UUID and reparent when the parent arrives.
- Erelia ensures server/client root coordinate systems are compatible. Sparkle does not serialize the root's Transform or perform world-space conversion.

A `Server` or `Client` supports **at most one bound EntityHierarchy**. A second binding to the same endpoint throws. Do not add hierarchy IDs to the wire protocol. Network bindings use RAII dispatcher contracts; the hierarchy never disconnects a Client/Server on its own.

## 6. Server Interest and publication

### 6.1 Interest is Erelia's selection callback

For each connected recipient, Erelia supplies the directly interesting Source Entities. It may use Registry/Query, proximity, unlocked/revealed state, and other game rules.

Sparkle computes the **upward ancestor closure**:

- Interesting child implies all parents up to the replication boundary are selected.
- Interesting parent does **not** imply selection of its children.
- Unselected Entities and their UUIDs are not exposed through initialization child lists (those lists have been removed).
- All selected ancestors are ordinary, fully publishable Entities: no partial Entity representation.
- A returned Entity must be a tracked Source inside this hierarchy's boundary; otherwise throw `spk::Exception` rather than filtering it silently.
- Sparkle does not implement gameplay authorization or conditional per-recipient Part encoding. **Each Part snapshot is recipient-independent and cached once**. Erelia models per-player visibility by selecting different Entities via Interest or using its own targeted messages. Never put secret per-player data in shared Parts of an otherwise selected Entity.

### 6.2 One publication counter and per-Part generations

- One global monotonically increasing `uint64_t` publication counter **per server hierarchy**, incremented on each `publish()`.
- Each Part independently has its own monotonically increasing generation on the Source. No timestamps, reset epochs, or shared counter between Parts.
- `clear(ClearMode::Runtime)` preserves the publication counter; client runtime clear preserves its latest accepted publication counter for the same bound session.
- Each publication contains the **complete selected UUID set** and, for each selected Entity, its authoritative **parent UUID** (or explicit null/root representation) and included Part snapshots.
- Interest selection and parent relationships are ordered by the publication counter; Part payloads are ordered by their **own Part generations**.
- On receiving an old publication, ignore its selection and parent relationships, but still apply newer-generation Part snapshots for Entities remaining in the latest accepted selection. Never resurrect unselected Entities from stale publications.

### 6.3 Publication execution and transport

`ServerEntityHierarchy::publish(UpdateSelection)` is called explicitly by Erelia:

1. Get recipients from the new read-only `Server::connections()` snapshot, returning **only Connected** connections.
2. Resolve pending structural events, validate Source identity/type and calculate each recipient's Interest closure. Abort on invalid Interest/configuration before consuming pending modified state.
3. Form the union of selected Entities across recipients.
4. Capture each required Entity/Part immutable snapshot **once** for the cycle; a Modified capture consumes successful pending invalidations only after capture.
5. Build each personalized publication from the shared captured bytes and its complete selection/parent metadata.
6. Submit personalized messages independently at `Message::Priority::Low`. An admission failure for one client does not roll back accepted messages for others.

`publish()` returns a `PublicationResult` containing per-recipient accepted `Transfer` handles and per-recipient **expected admission failures** (e.g. queue capacity); it attempts every eligible recipient. Invalid configuration, framing/serialization failures and unexpected exceptions still throw. Transport acknowledgment means network delivery, not successful application by the remote game.

A failed or lost Modified-only publication **does not** retain per-client Part history. Erelia periodically invokes `publish(UpdateSelection::All)` to repair missed updates. `All` does not consume pending Modified state, and the client skips redundant/equal-generation snapshots.

## 7. Initialization protocol and client reconciliation

### 7.1 Three explicit message types

Erelia supplies **three distinct** `Message::Type` IDs for:

1. Publication.
2. Initialization request.
3. Initialization response.

Use Sparkle's existing `MessageDispatcher` and `Message::RequestID` header field. Do not duplicate the RequestID in every payload or introduce another message router. Initialization requests and responses use `Message::Priority::High`; publications use `Low`.

### 7.2 Client discovery and requests

The client remembers its latest accepted **complete** selected-UUID set and each selected Entity's authoritative parent UUID.

- On a newer publication, validate **all framing and structural metadata first**; accept its generation; reconcile the complete selected set **before applying Part data**, immediately destroying out-of-Interest Replicants.
- Then update parent relationships and apply newer-generation Part snapshots. A game-defined decoder failure propagates without rolling back accepted Interest or previously applied Parts.
- Unknown selected UUIDs are requested in **batches**, with one unique RequestID for each request batch.
- The client tracks pending UUIDs, request IDs and configurable timeouts. It retries failed or timed-out requests **when subsequent server publications arrive**, not from a hidden background timer or a periodic client update method.
- Explicit server rejection suppresses automatic retries while that UUID remains continuously in Interest. Once it leaves and later reappears, it can be requested anew. Obsolete rejections from earlier RequestIDs must not suppress newer attempts.
- A failed transport Transfer makes a request eligible for retry. Transport ACK alone does not establish application initialization success.
- For still-unknown Entities, retain at most the **highest-generation snapshot per Part ID**, not an unbounded event log. Drop no-longer-applicable buffers on selection change.
- Receiving an older publication never restores its obsolete membership or parenting. Its Part snapshots may still contribute valid state to Entities selected by the newest publication.

### 7.3 Server initialization authorization and response

- Authorization is checked **at request-processing time** against the current Interest-selected ancestor closure for that client; do not grant an initialization simply because the requester knows an UUID.
- The server returns **one response per requested UUID**, carrying the batch RequestID and that UUID; each has an independent success or rejection result.
- Nonexistent, outside-Interest and otherwise inaccessible UUIDs use **one generic `Rejected` outcome**, without disclosing existence or private Part data. Sparkle implements selection checks, not Erelia's gameplay-specific permission system.
- A successful response contains **only Entity UUID, numeric EntityTypeID and all registered Part snapshots with generations**. It contains **no parent UUID and no child UUID list**: authoritative relationships come solely from publications.
- Source snapshots are independent of the recipient; initialization always contains all registered Parts of a selected Entity.

### 7.4 Client construction and parenting

- Resolve the numeric EntityTypeID using Erelia's registered construction callback receiving the authoritative UUID; construct a **detached** `Mode::Replicant` object.
- Validate its expected network mode, type contract and Part registration.
- Decode its full initialization and apply any buffered strictly newer Part snapshots **before attaching it to the Engine**. On initialization failure, destroy the detached temporary Entity.
- If still applicable, retain ownership and attach under its initialized authoritative parent, otherwise temporarily under Erelia's attachment root. When the desired parent initializes, attach waiting children using their stored parent UUIDs.
- When a newer publication changes an Entity's parent to an uninitialized selected Entity, move it immediately to the attachment root and request that parent. Late initialization never overrides newer publication parenting.
- A successful initialization that arrives after the Entity has left Interest follows the previously chosen **process-then-reconcile** rule: it may be constructed and immediately discarded; it is not retained in an unbounded cache.
- The client relies on existing Engine `Registry` and `Query` mechanisms for Entity lifecycle visibility, not new dedicated hierarchy add/remove notifications.

**Parent/child UUID lists are not duplicated in initialization**. Children can attach when their parent appears because the logical hierarchy already stores each child's authoritative parent UUID from the latest publication.

## 8. Runtime clearing and network lifecycle

Provide explicit `clear(EntityHierarchy::ClearMode mode)` with no implicit default:

- **Runtime:** clear tracked runtime data/subscriptions and rebuild observation of the existing server root; on the client, remove owned replicas and clear selection, pending initialization, buffers and rejected UUID state. Preserve type registration, current network binding, and **publication ordering baselines/counters**.
- **Full:** additionally remove registrations and release message/endpoint bindings, **without** disconnecting the Client or stopping the Server.

The hierarchy does not auto-clear on disconnection; Erelia decides when to call `clear()`. A new synchronization session requires Erelia to establish a new baseline; do not infer cross-session ordering from the old counter. No protocol epoch or timestamp is currently planned.

## 9. Ownership and non-goals summary

| Concern | Owner |
| --- | --- |
| Cached immutable Part snapshots, registration, generation validation | `Publishable` |
| Source/Replicant role, immutable UUID, reserved Parts, public ID translation | `PublishableEntity` and concrete wrappers |
| Engine parenting and ordinary Component/Behaviour/Transform behavior | Existing Engine |
| UUID/type lookup, structural observation, parent UUID graph | `EntityHierarchy` |
| Source observation, Interest closure, fan-out, initialization authorization | `ServerEntityHierarchy` |
| Replica lifetime, initialization/retries, selection reconciliation | `ClientEntityHierarchy` |
| Game entity construction and server lifetime, numeric schemas, Interest decisions, reveal/unlock rules, update cadence, predictions, local visual Entities | Erelia |
| Delivery, message priority, connection snapshot, Transfer outcomes | Existing Server/Client/Transport (plus `Server::connections()`) |

Not in scope: per-recipient Part serialization, gameplay-specific security/permissions, per-client server publication history, implicit retries for missed modified Part updates, auto-disconnect/auto-clear behavior, automatic recursive child initialization, dynamic network Part removal, hidden scheduling, replacing generic Engine hierarchy or visual geometry code.

## 10. Implementation work to perform in subsequent PRs

1. **Revise generic Publishable (PR #31 scope).** Remove Edition API and init callbacks; implement one cached immutable snapshot per Part, one generation, `Modified/All` policy, correct decode semantics, strict validation, single-owner Part registration, and a generic owning-registration permission for Replicant no-op invalidation.
2. **Minimal Engine observation change.** Add direct-child `subscribeToChildEdition` and Added/Removed parent/child event semantics to `InherenceTrait`, with RAII and safe constructor/destructor notifications. Adapt existing `Entity` context propagation and `Entity2D/Entity3D` parent-cache subscription consumers without changing their intended behavior.
3. **Engine publication adapters.** Implement independent `PublishableEntity` capability, Source/Replicant mode, UUID/type identity, reserved ID 0–15/public +16 mapping, activation Part and Transform2D/3D Part adapters, plus `PublishableComponent` / `PublishableBehaviour` activation and normal data serialization.
4. **Shared EntityHierarchy.** Implement non-owning UUID/type lookup, one root, structural subscriptions, deferred safe discovery, precise type registration via `typeid`, parent UUID graph, binding uniqueness, and explicit Runtime/Full clear policy.
5. **Server infrastructure and hierarchy.** Add `Server::connections()` returning Connected-only snapshot; add three explicit message ID bindings; Interest callback and ancestor closure; Source validation, global publication counter, captured update union and personalized messages, low priority, `PublicationResult` for per-recipient results.
6. **Client hierarchy.** Add factory-based Replicant construction, strict unknown-type rejection, batched RequestIDs and independent per-Entity responses, timeout/rejection state, buffered latest-per-Part data, selection-first reconciliation, fallback attachment root, pending parent resolution and RAII subscriptions.
7. **Validate with unit, integration, stress and sanitizer tests** on Linux and Windows; document Erelia-facing examples and verify actual CI runs in the implementation PRs.

This PR **only documents the plan** and must not claim implemented code or passing CI.

## 11. Required test cases

- Generic Part laziness, immutable cached bytes and captured generation agreement; repeated reads; Modified/All semantics; initialization-only Parts; pending invalidation preserved if capture fails.
- Part single-owner registration, unregistration/destruction safety, Source invalidation increments, Replicant invalidation does not increment, successful remote decode advances same counter, duplicate/older generation ignored, first-generation-zero snapshot accepted.
- Reserved IDs 0 activation, 1 Transform, 2–15 reserved, game IDs offset +16, duplicates/overflow rejected; two separate built-in generations; Transform2D/3D local spatial state without `Rect2D`.
- Existing `InherenceTrait`/Engine behavior preserved under two-event parent changes, child addition/removal and destruction; initial and newly attached subtree subscriptions; base-constructor attachment does not trigger unsafe dynamic cast; subtree move within vs outside replication root.
- Source concrete type lookup, exact RTTI mapping, missing registrations, duplicate IDs, immutable UUID across root detach/re-attach; client differing C++ types with matching Part schemas; no Entity diamond.
- Client initialization off-Engine, callback returns detached Replicant with correct UUID/mode; initialization failure destroys temporary instance; local Entities remain owned by Erelia.
- Two or more Connected recipients with distinct/overlapping Interest; upward ancestor closure, no sibling selection, invalid callback selection throws before Part drain, no disclosure of unselected child UUIDs.
- Publication capture once over recipient union, generation ordering and independent Part generations; old selection cannot resurrect Entities while a newer Part from an older publication can still be applied; immediate out-of-Interest removal even when an unrelated decoder throws.
- Parent arrives after child, reparent to unknown parent moves child to attachment root, pending relationships resolve without child UUID lists, older parent relationships cannot override newer.
- Request batches, unique RequestID from Message header, one success/rejection per UUID, generic rejection, stale rejection vs newer retry, timeout checked on new publications, late success process-then-reconcile, no auto-disconnect.
- Admission failure for one recipient does not prevent others being attempted; return accepted Transfers and failures; no per-client resend; `All` repairs loss without consuming Modified.
- Runtime clear retains network binding and ordering while cleaning client/observer runtime; Full releases bindings/registrations without disconnecting; single hierarchy per endpoint enforced; Connected-only Server snapshot.
- Baseline performance tests for changed Part caching and recipient fan-out; ASAN/UBSAN and cross-platform CI coverage as applicable.

## 12. Remaining implementation checks (do not silently choose new policy)

- Precisely define how structural event buffering avoids dangling pointers if an Entity is attached and destroyed before the next safe discovery boundary; never dereference an unfinished or destructed polymorphic object.
- Ensure the Part's single generation and cache remain consistent when invalidation is suppressed for Replicants, and when decode calls ordinary Engine setters that emit edition events. Confirm actual Engine activation subscription APIs.
- Confirm exception-safety across staged per-client publication building and `PublicationResult` collection; invalid selections must not consume pending modified state.
- Define `uint64_t` counter overflow behavior and wire bounds/limits, with strict decoding and protocol tests.
- Keep source/client endpoint session changes explicit: runtime clearing is **not** an automatic new session or epoch. No additional session-reset protocol unless future concrete requirements demand it.
