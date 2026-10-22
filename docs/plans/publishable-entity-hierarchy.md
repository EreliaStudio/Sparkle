# Publishable simplification and EntityHierarchy multiplayer plan

> **Status:** Documentation-only design plan, updated 22 October 2026. This PR does not implement the design.
>
> **Target:** `Version-0.1.3`. Draft [PR #31](https://github.com/EreliaStudio/Sparkle/pull/31) implements an **older**, Edition-based `Publishable` API. Its implementation must be revised separately before this plan can be treated as implemented.
>
> **Development rules:** Follow `docs/ai-development-guidelines.md`: explicit API contracts, coherent responsibilities, C++23, `spk::Exception` for invalid operations, and no explanatory comments in new/modified public headers. Avoid unrelated Engine redesigns.

## 1. Intended Erelia usage and responsibilities

Erelia owns its server Entities and creates Components/Behaviours through the existing Engine API. Sparkle observes an Engine subtree, selects interesting Source Entities, and publishes their Parts to replicas owned by the client hierarchy.

```cpp
class Monster : public spk::PublishableEntity3D
{
public:
    Monster()
    {
        auto& health = addComponent<HealthComponent>();
        auto& inventory = addComponent<InventoryComponent>();

        setPartRegistry({
            {0, health, Publication::Update},
            {1, inventory, Publication::Update}
        });
    }
};

// Erelia-owned Source Entity:
serverHierarchy.registerType<Monster>(42);
auto monster = std::make_unique<Monster>(/* Erelia constructor arguments */);
auto result = serverHierarchy.publish(UpdateSelection::Modified);
// Erelia also periodically invokes publish(UpdateSelection::All).
```

The registry example is illustrative: constructor signatures, flags qualification, and API naming must be finalized during implementation. The **semantics** of replacing the registry, rather than individual registration operations, are agreed.

- **Erelia owns server Entities**, their Component/Behaviour composition, and gameplay rules. The server hierarchy only observes.
- **`ClientEntityHierarchy` owns Replicants**, which Erelia constructs through a registered factory. Ordinary client-local Entities are not network-owned.
- Erelia decides when to publish, Interest selection, client/server registry compatibility, and when registered Parts may be destroyed.
- No implicit multiplayer publisher Systems, auto-update threads, generic gameplay replication factories, per-recipient codecs, or implicit registry synchronization.
- The hierarchy is Entity-specific; do not generalize it to `NetworkReplicationHierarchy<T>` without a concrete requirement.

## 2. Generic `spk::Publishable`: single-Part publication

### 2.1 One Part codec, one cache, one generation

`Publishable` is a non-template capability holding **non-owning** registrations of `Publishable::Part` objects. Each Part implements:

```cpp
virtual void _encode(ByteStream::Writer&) const = 0;
virtual void _decode(const ByteStream::Reader&) = 0;
```

All serializable data belongs to Parts. Remove Edition-specific APIs/codecs/caches, `encodeEdited()` and initialization encoder/decoder callbacks from the prior PR #31 design.

- **Initialization** contains every Part currently registered.
- A single outgoing `Publication::Update` flag makes a Part eligible for later updates. A Part without it is initialization-only.
- `encodeUpdate(UpdateSelection::Modified)` captures modified update-enabled Parts; pending changes are consumed **only after successful capture**.
- `encodeUpdate(UpdateSelection::All)` captures all update-enabled Parts without consuming pending Modified changes.
- `encodeInitialization()` includes all registered Parts without consuming pending Modified changes.
- An initialization-only Part may be invalidated and rebaked for future initializations, but does not appear in periodic updates.
- An empty selection produces an empty ByteStream. All publication modes share one Part wire framing.

Each Part has **one lazily baked immutable ByteStream** and **one monotonically increasing `uint64_t` generation**. Capture the generation and its bytes consistently. A Source invalidation increments its generation, invalidates its cached snapshot, and marks applicable registrations modified. Multiple invalidations may be coalesced; intermediate versions need not be transmitted.

Every serialized Part section carries its numeric ID, generation, payload length, and payload bytes. Initial generation **0** is valid for a previously undecoded Part. Later reception applies only strictly newer generations, updating the receiving generation **after** successful decoding. Failed decoding does not advance the generation: the same generation can be retried by a later `All` snapshot. Unknown IDs in strict network mode, invalid framing/lengths, unread payload bytes, and malformed sections throw. Independent sections should still be processed where possible; aggregate decoder errors as one contextual `spk::Exception` without introducing an aggregate exception class.

A Part may belong to **at most one Publishable at a time**. Removing its registration does not destroy the Part. Part cache/generation ownership must be distinguished from registration-specific pending and receiving state.

### 2.2 Replace the registry explicitly: `setPartRegistry()`

**This section supersedes the earlier `addPart()` and first-use registration-freeze decisions.**

The generic capability exposes a protected **whole-registry replacement** operation, rather than incremental game-facing `add()`/`addPart()` and `remove()` calls:

```cpp
class Publishable
{
public:
    using PartID = std::uint32_t;
    enum class Publication : std::uint8_t { Update = 1 << 0 };
    enum class UpdateSelection { Modified, All };

    struct PartRegistration
    {
        PartID id;
        Part& part;
        Flags<Publication> flags;
    };

    using PartRegistry = std::vector<PartRegistration>;

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
    void setPartRegistry(PartRegistry registrations);
};
```

This is an **illustrative**, not final, header. A non-owning descriptor container or equivalent value type is acceptable. No constructor-only builder or `finalizeParts()` is required.

Rules for **every** call to `setPartRegistry()`:

1. Validate **the entire new registry** first: duplicate IDs, repeated Part references, invalid flags, incompatible concurrent ownership, and other existing registration invariants.
2. Install the validated replacement **atomically**, or leave the previous registry intact on failure. The call does not send network messages.
3. A registration is **unchanged only when its Part ID, Part object identity, and publication flags are all identical**. Preserve binding-specific pending/receiving state for unchanged registrations.
4. **Any** change to ID, referenced Part, or flags constitutes a new binding. Discard the old binding's pending and receiving-generation tracking; initialize the new binding's tracking.
5. Newly registered or changed **update-enabled** bindings start **modified**, even without a prior explicit `invalidate()`. Removed registrations lose their pending changes.
6. The **Part object's own cache and generation survive registry changes** when that object continues to exist. Resetting binding-specific receiving tracking does not reset or rewrite intrinsic Part data.
7. Part IDs **may be reused/reassigned freely** for another object; there is no permanent per-ID history, no registry compatibility identifier, and no registry version on the wire.
8. Each encode/decode operation interprets its Parts against **one coherent registry snapshot**. Erelia ensures that referenced Part objects remain alive during use, including after any registry replacement; Sparkle does **not** add a gameplay-driven registry-change scheduling policy.

Decoding an incoming ID always consults the locally selected registry snapshot. Sparkle validates the ID/framing/generation, **not** whether that ID still means the same application data type on the sender. Stale in-flight packets can be misinterpreted after a reassignment: Erelia is responsible for safe schema coordination and compatibility. Resetting receiver tracking is **not** a substitute for a wire version or epoch. No automatic Part manifest, addition/removal record, synchronization of registry layout, or network Component factory is introduced.

Source/Replicant Part registration permissions are generic; no Entity-specific role types belong in `Publishable::Part`. Its registration can change at runtime, while ordinary Engine Component lifetimes remain Erelia's responsibility. Erelia must stop referencing a Component in the registry **before destroying it**.

### 2.3 Source versus Replicant generation

- In a **Source**, `invalidate()` increments generation, invalidates the cache and marks update-enabled bindings modified.
- In a **Replicant**, local `invalidate()` is a **complete no-op**: it does not increment, invalidate the cache, or queue updates.
- Successful authoritative decoding updates the receiving Part generation and invalidates any relevant cached representation. Equal/older snapshots are not applied.
- No separate prediction clock or local-vs-remote generation counters. Erelia is responsible for gameplay prediction reconciliation.
- Game-thread-only mutation, registry configuration, encode, and decode are assumed for this Engine integration. Transport operates on captured immutable bytes; do not add unrelated gameplay locks or hidden synchronization.

Counter overflow is an explicit `spk::Exception` **before** increment, including Part generations and the global publication sequence.

## 3. Publishable Entity and Engine attachment integration

### 3.1 Composition without an Entity diamond

`PublishableEntity : Publishable` is a capability, **not** a subclass of Engine `Entity`. Spatial wrappers combine them explicitly:

```cpp
class PublishableEntity2D : public Entity2D, public PublishableEntity {};
class PublishableEntity3D : public Entity3D, public PublishableEntity {};
```

A nonspatial wrapper may combine `Entity` and `PublishableEntity` similarly.

- `PublishableEntity::Mode { Source, Replicant }` is explicit and immutable. Sources obtain a stable UUID; Replicants receive the exact authoritative UUID.
- Concrete server and client C++ Entity types may differ for the same numeric `EntityTypeID`, provided their Part codecs and registries are compatible.
- Erelia still constructs Components and Behaviours through the normal `addComponent<T>()` and `addBehaviour<T>()` APIs. There is **no** dedicated `addPublishableComponent()` factory.
- `PublishableEntity` exposes the whole-registry setter for **game-defined** Parts, with public ID translation. Replacement **must not remove Sparkle's reserved internal Parts**.
- Reserved adapters are installed internally only after their owning wrapper members exist; do not access unconstructed derived members in a base constructor.

### 3.2 Reserved Parts, activation and Transform

Internal IDs **0–15** belong to Sparkle:

| Internal ID | Purpose |
| --- | --- |
| 0 | Entity activation |
| 1 | Transform2D/Transform3D for spatial Entities |
| 2–15 | Reserved for future internal Parts |
| 16+ | Erelia game-defined Parts; `internalID = publicID + 16` |

Erelia's public Part IDs start at zero. Reject overflow above `UINT32_MAX - 16`, duplicate IDs, and collisions with reserved IDs. Generic `Publishable` IDs remain direct, zero-based numeric IDs.

The concrete 2D/3D wrappers own activation and Transform **Part adapters**. They observe existing Engine activation/Transform edition callbacks with RAII subscriptions and invalidate their Source Parts automatically. Transform Components themselves remain networking-free. Synchronize local position, rotation, scale and any applicable 2D depth; **not** visual `Rect2D` geometry. Activation and Transform have separate snapshots/generations.

**Decoding Entity activation Part ID 0 occurs after other Entity Parts** in an initialization or update, regardless of wire order. No special deferral based on Engine attachment: a detached Replicant decodes data and activation immediately and fires ordinary Engine notifications. Detaching an Entity does not artificially deactivate it.

`PublishableComponent` / `PublishableBehaviour` combine normal Engine attachments with `Publishable::Part`:

- They implement **`_encode()` and `_decode()` as final wrappers**. The activation boolean is serialized automatically for **every** such Part; Erelia cannot omit or override it.
- They provide **optional** game-data hooks (illustratively `_encodeData()` / `_decodeData()`), defaulting to empty for activation-only Parts. Game code encodes only its gameplay data.
- On decode, apply the optional game data **before** applying the serialized activation through normal Engine APIs.
- Activation/deactivation automatically invalidates the Source Part. Game-defined data changes invalidate explicitly. Replicant invalidation remains a no-op.
- `Publication::Update` applies to **both activation and gameplay data** together; without it, both are initialization-only.
- Runtime registry replacement may remove or replace an attachment's registration, but Sparkle **does not delete or construct that Component/Behaviour**, emit removal tombstones, or reconcile client composition automatically. Erelia must maintain referenced-object lifetimes.

## 4. Structural observation, safe construction and type discovery

### 4.1 `InherenceTrait` direct-child notifications

Extend `InherenceTrait` with direct-child `subscribeToChildEdition()` and explicit `RelationEdition { Added, Removed }` events for both child and parent relation subscriptions. Reparenting reports removal/addition. Existing Engine context and Transform world-cache subscriptions retain their intended behavior.

The structural parent/child graph must be **committed consistently before edition notifications**; a callback error does **not** roll back the relationship. For explicit operations, notify **all** structural subscribers even when one throws, then surface aggregated contextual diagnostics via ordinary `spk::Exception`. During destruction/detachment from a destructor, notify best-effort, **never propagate** a callback exception, and log failures via existing `SPK_LOG(Error)`. Do not redesign unrelated generic `ContractProvider` semantics just to implement this policy.

`ServerEntityHierarchy` observes the Erelia-owned root recursively and subscribes to new direct children/subtrees. Perform one initial scan and one traversal of newly attached subtrees, not whole-world scans every publication.

**Base-construction safety:** Engine `Entity` may attach itself to a parent **before** the derived `PublishableEntity`/game type exists. Structural notifications record pending child pointers without unsafe downcasts or UUID accesses. Resolve network identity/type only at a later safe game-thread publication or initialization-request boundary. Removal/destruction must immediately withdraw pending potentially dead pointers **without dereferencing them**. Erelia does not reenter synchronization processing during construction/destruction. Objects created and destroyed between cycles never publish.

### 4.2 Explicit types

- Register exact concrete Source C++ Entity types under Erelia-selected numeric `EntityTypeID` values, e.g. `registerType<Monster>(42)`.
- Resolve safe discovery with `std::type_index(typeid(*entity))`; do not implicitly map a derived class via a registered base.
- UUID and resolved type persist across leaving/reentering the server replication root.
- Missing exact Source type, conflicting IDs, and unsafe registration rebinding are `spk::Exception` errors.
- Client registers a callback per numeric type ID that constructs a **detached**, owned `Mode::Replicant` Entity from the authoritative UUID.
- Never transmit RTTI names. Server/client game classes may differ while honoring the same schema.

## 5. EntityHierarchy ownership, boundaries and parenting

`EntityHierarchy` is non-template and not an Engine Entity. It owns type registries, non-owning tracking, UUID lookup, parent UUID relationships and RAII structural subscriptions.

- `ServerEntityHierarchy` observes **Erelia-owned** Sources within one Erelia-owned replication root. It never owns/destroys Source Entities.
- `ClientEntityHierarchy` owns its Replicants as `unique_ptr`, while local client Entities remain Erelia-owned.
- The non-publishable server boundary root is permitted. Paths to replicated descendants must consist of publishable ancestors below that root; no non-publishable intermediate ancestor.
- On server root detach/re-attachment, Sources retain UUID/type. A move within the root is reparenting.
- Client root attachment applies only to **top-level Replicants** whose authoritative parent is null/root. **Do not temporarily attach an Entity to the root if its authoritative parent is missing.** Keep it detached, initialized, and accepting Part/activation updates until the parent appears.
- When an already attached parent becomes detached, its existing child subtree stays attached to that parent; do not flatten the hierarchy.
- If a client parenting operation throws, retain the initialized Replicant and retry the relationship in a later publication. Compare authoritative desired parent with the actual Engine parent; do not reinitialize the Entity for a parenting failure.
- If a parent never appears, keep selected children detached and updated until Interest removes them.
- Erelia owns the root coordinate-system compatibility; Sparkle does not serialize the Engine root or convert world-space positions.

Only **one hierarchy may bind to a given Server/Client endpoint**. Use normal RAII dispatcher subscriptions and explicit release; do not add wire hierarchy IDs or implicit endpoint disconnection.

## 6. Server Interest, publication and transport

### 6.1 Per-recipient selection

Erelia supplies an Interest callback selecting directly interesting tracked Source Entities per connected recipient. Sparkle includes the **transitive parent closure** up to the replication boundary: child implies ancestors; parent does not imply children.

- Invalid, out-of-bound or non-Source selections throw instead of silently filtering.
- All selected entities are ordinary fully publishable Entities. No partial Entity classes.
- Each personalized publication includes the recipient's **complete selected UUID set** and each selected Entity's authoritative **parent UUID**.
- Part serialization and caching are **recipient-independent**. Erelia controls gameplay-specific secrecy/unlock through Entity selection or other targeted messages; never put per-client secrets in shared Part snapshots.

### 6.2 Counters and accepted state

- One globally monotonic `uint64_t` publication sequence **per server hierarchy** orders membership and parenting.
- Each Part's independent generation orders its data; generation zero is valid only for first reception.
- An older publication cannot restore obsolete membership or parenting; its newer Part snapshots may still apply to Entities remaining in the current selected set.
- A newer publication validates framing/structural metadata first, reconciles membership **immediately**, then applies eligible Parts independently. Decoder failures do not undo already accepted membership/parent information or previously applied Part data.
- Counter overflow throws `spk::Exception` instead of wrapping.

### 6.3 Publication cycle

`ServerEntityHierarchy::publish(UpdateSelection)` is **explicitly** called by Erelia:

1. Enumerate recipients using new read-only `Server::connections()` returning a snapshot of **Connected-only** connections.
2. Resolve safe structural discovery, validate types and per-recipient Interest, and calculate parent closure.
3. Build the **union of selected Entities**. Unselected Entities' pending Modified bits remain pending.
4. Capture snapshots for selected Parts **once per cycle** with recipient-independent cached bytes.
5. **Transactional preparation:** invalid selection, framing or unexpected serialization failure during payload preparation aborts the cycle **without consuming Modified invalidations**. After all payloads are successfully prepared, commit captured Modified consumption.
6. Submit each valid personalized message at `Message::Priority::Low` independently. Per-recipient admission/transport failures go into `PublicationResult.failures`; valid recipients continue. A personalized message exceeding the transport limit is also a **per-recipient** failure, not a global serializer failure.

`PublicationResult` holds accepted per-recipient `Transfer` handles and failures; an accepted Transfer/ACK confirms transport, not successful gameplay decoding. Do not maintain per-client resend history. Failed/lost modified-only delivery is repaired by Erelia's periodic `publish(UpdateSelection::All)`, which does not consume Modified changes. Persistent per-recipient oversize payloads require Erelia to alter selection/state; `All` cannot repair those.

## 7. Client publication and initialization protocol

### 7.1 Message IDs and batching

Erelia supplies **three explicit, distinct** `Message::Type` IDs: publication (Low), initialization request (High), initialization response (High). Use the existing `MessageDispatcher` and existing `Message::RequestID` header; do not introduce another dispatcher or duplicate RequestID in the payload.

The client holds the newest complete selected-UUID set and desired parent UUID for each selected Entity. On a newer publication:

1. Strictly validate framing and structural metadata before acceptance.
2. Reconcile selection and remove out-of-Interest Replicants immediately, even if later Part decoding fails.
3. Update authoritative parenting, retaining detached children whose parent is missing.
4. Decode independent, newer-generation Parts; continue after one Part fails and aggregate exceptions as **one regular contextual `spk::Exception`**.
5. Batch initialization requests for unknown selected UUIDs. Buffer at most the latest received snapshot per Part until initialization succeeds. Never resurrect Entities from stale membership snapshots.

If `MessageDispatcher::treatMessages()` throws from handling a message, the failing message is consumed and processing of the remaining queue resumes on the **next** invocation, consistent with the existing dispatcher behavior.

### 7.2 Server authorization and responses

- Authorize each UUID at **request processing time** against that connection's currently selected ancestor closure.
- Respond **independently once per UUID**, echoing the batch RequestID. There is no unsolicited initialization push.
- Invalid/nonexistent/out-of-Interest UUIDs use a single generic `Rejected` outcome, without leaking existence.
- Authorized success contains only **UUID, numeric EntityTypeID and all currently registered Part snapshots with generations**. No parent UUID, child UUID list or registry manifest.
- A response too large for the transport returns `TooLarge` for that UUID, suppressing retry until it leaves/re-enters Interest.
- An unexpected per-UUID Source encoding failure returns `InternalError` (without exposing server diagnostics), continues processing other UUIDs, and aggregates server-side diagnostic exceptions for Erelia. Client suppresses retry until Interest changes.

### 7.3 Client construction, reception and failure accounting

- Use Erelia's registered callback `(const UUID&) -> unique_ptr<Entity>` to create a **detached Replicant** with exactly that UUID; server/client concrete C++ classes may differ.
- Decode full initialization and any buffered newer Parts while detached. **Never defer Part decode or activation just because Engine parenting is unresolved.** Decode Entity activation after other Entity Parts; wrapper Component/Behaviour activation after their own gameplay data.
- On successful initialization, the client owns the replica and attaches it to its known initialized parent, or to the Engine root **only for a true top-level Entity**; missing-parent descendants remain detached.
- If construction or initial decoding fails, discard the temporary Entity. Allow at most **four attempts in total (one initial + three retries)** for these application-level failures, across a shared counter per selected UUID. After four failures, suppress until the UUID leaves and reenters Interest.
- Network Transfer failures/timeouts are a **separate retry category** and are checked only on later server publications; no implicit background retry loop or client update timer.
- Explicit `Rejected`, `TooLarge` and `InternalError` suppress retries until selection changes. Obsolete responses from previous RequestIDs cannot suppress newer attempts.
- Once a UUID is successfully initialized, **ignore every subsequent initialization response for that UUID**, even if the response contains newer generations. Later authoritative updates come only from publications.
- If an ordinary Part update fails decoding after successful initialization, keep the Entity and previously decoded Parts; do **not** count it toward initialization retries or force reinitialization. The failed Part's generation does not advance; future snapshots of that generation may retry.
- Client parenting failures do not destroy a correctly initialized Entity, and are retried on later publications. No permanent "failed parent" state or activation deferral.
- Late successful initialization for an Entity that is no longer selected follows the agreed process-then-reconcile behavior: it may be built and immediately discarded, never retained indefinitely.

## 8. Explicit clear and session boundaries

Provide explicit `clear(EntityHierarchy::ClearMode mode)`:

- **Runtime:** release runtime tracking and reobserve the existing server root; client removes owned Replicants, membership, pending requests, buffers and rejection states. Preserve network binding, type registrations and the existing publication ordering baseline/counter for that session.
- **Full:** additionally remove type registrations and network bindings, **without** disconnecting the Client or stopping the Server.
- Do not automatically clear on disconnect or invent registry synchronization epochs/session-reset messages.
- Erelia coordinates new synchronization-session transitions and excludes stale queued messages. It must not interpret the prior session's counter baseline as a new session.

## 9. Ownership and explicit non-goals

| Concern | Responsible owner |
| --- | --- |
| Immutable Part snapshots, generations, validation, atomic local registry replacement | `Publishable` |
| Source/Replicant mode, UUID and ID mapping, built-in Parts/adapters | `PublishableEntity` and concrete Engine wrappers |
| Component/Behaviour/Transform ownership and dynamic composition | Existing Engine and Erelia |
| Registry compatibility across network peers, live Part object lifetime, ID reuse safety | **Erelia** |
| UUID/type mapping, structural observation, logical parent graph | `EntityHierarchy` |
| Interest closure, Source observation and publication fan-out | `ServerEntityHierarchy` |
| Replica ownership, initiation/retries, selection and parenting reconciliation | `ClientEntityHierarchy` |
| Paged transport, priorities, connections and Transfer outcomes | Server/Client/transport, plus `Server::connections()` |

**Not in scope:** implicit network Part-registry synchronization; automatic creation/removal of Component objects based on wire IDs; Part-manifest or tombstone messages; gameplay-driven registry version/compatibility checks; client-specific cached Part encodings; server per-client Part history; automatic update/retry loops; partial Entity types; visual geometry serialization; generic Engine redesign.

## 10. Implementation sequence in future PRs

1. **Revise generic `Publishable` (PR #31 scope):** remove Edition API/init callbacks; implement single Part cache/generation, Modified/All, strict framing, Source/Replicant invalidation behavior, and **atomic, repeatable `setPartRegistry()`**. Test changed binding identity using `(ID, Part object, flags)`; initially-modified new update-enabled bindings, arbitrary ID reassignment and complete validation rollback. Preserve in-flight operation registry snapshots and non-owning Part lifetime contracts.
2. **Engine observation:** extend `InherenceTrait` with direct-child/parent Added/Removed subscriptions, consistent graph-before-notification order, all-subscriber diagnostics for explicit operations, and best-effort nonthrowing destructor notifications. Preserve existing Engine use.
3. **Entity wrappers and adapters:** implement Source/Replicant identity, reserved IDs, Transform/activation adapters, normal `addComponent`/`addBehaviour` usage, and internal fixed Sparkle adapters alongside **replaceable game-defined Part registry**. Add final activation codec wrappers and optional gameplay hooks for publishable attachments.
4. **Shared EntityHierarchy:** UUID/type lookup, root discovery, safe deferred construction observation, RAII contracts, detached parent resolution and explicit clear modes.
5. **Server hierarchy:** Connected-only recipients; Erelia Interest callback and ancestor closure; global publication counter; transactional union capture; independent per-recipient admission/oversize results; authorization and init replies.
6. **Client hierarchy:** Replicant factories; selection-first reconciliation; batched RequestIDs, response outcomes, four-attempt constructor/decode ceiling; detached missing-parent processing, no activation deferral, robust per-Part error aggregation.
7. **Unit/integration/stress/sanitizer testing** across Linux and Windows. Confirm actual CI results in implementation PRs; this design PR has no implementation CI claim.

## 11. Required test coverage

- One snapshot/generation per Part; initialization includes all currently registered Parts; Modified/All, empty payloads, generation zero, strict malformed-frame/unknown-ID/unread payload checks; failed decode may retry same generation.
- `setPartRegistry()` repeated replacement, duplicate IDs/Part objects/flags rejection, rollback on validation failure, non-owning Parts, unchanged binding state retention, changing flags counted as replacement, added bindings initially Modified, removed bindings cleared, **Part ID reuse for different objects**, receiver-generation reset on changed binding, Part intrinsic snapshot/generation preserved, no automatic wire changes.
- Concurrent role ownership rejection; Source invalidation versus complete Replicant no-op; immutable snapshots captured consistently. Explicit `uint64_t` overflow errors.
- 0 activation, 1 Transform, 2–15 reserved, game +16 offset/overflow, fixed reserved adapters unaffected by game registry replacement, Transform events, no `Rect2D`; attachment activation serialized automatically and decoded after optional data.
- Game-created attachments through normal Engine methods; optional empty data hooks; activation-only Parts; `Publication::Update` gates both activation and gameplay changes; removal from registry precedes Erelia destruction of referenced attachments.
- `InherenceTrait` direct-child notifications, final consistent parenting before callbacks, callback error collection without rollback, destructor logging without throwing; constructor-time defer/no unsafe downcasts, rapid add/remove before cycle.
- Source exact type mapping/UUID persistence, client different C++ classes, ancestor Interest union and no sibling leakage.
- Publication preparation failure preserves all Modified state; union captures selected Parts once, unselected invalidations remain pending; personalized transport capacity/oversize failure doesn't block other recipients; `All` recovery.
- Stale structural publication cannot restore old membership, but newer Part generations may apply for still-selected Entities; selection reconciled before Part failures; errors aggregated without undoing independent decoded Parts.
- Initialization request batching, response per UUID, RequestID header reuse, request-time permission, generic `Rejected`, `TooLarge`, `InternalError`, no parent lists or Part manifests; later retry on new publication only.
- Four combined game factory/initial-decode attempts; unrelated network retries do not consume that count; failed ordinary Part update doesn't discard initialized replica; extra initialization responses ignored after success.
- Missing parent => initialized **detached** Replicant; decode activation and other updates detached; attach when parent available; later reparent failure retried without reinitialization; existing detached subtrees preserved; top-level attaches root.
- Runtime/Full clear boundaries, same-endpoint unique binding, queued old-session messages handled by Erelia; no automatic disconnect.
- Performance/stress/sanitizer and cross-platform coverage as appropriate.

## 12. Implementation boundaries and remaining technical checks

- Precisely ensure that unconstructed/destroyed Engine Entities are never dereferenced by deferred hierarchy discovery.
- Implement atomic registry installation and coherent per-operation reads without silently extending `Publishable` into a game-driven registry scheduler. **No prohibition on Erelia changing its registry from a callback has been agreed.** Erelia guarantees lifetime of referenced objects during active reads.
- Ensure replacing a Part binding correctly resets *binding-specific* receive tracking without changing the independent Part's intrinsic Source cache/generation; recognize that stale packets and schema drift remain Erelia's problem.
- Ensure decode/activation notifications cannot accidentally advance Replicant generations; Replicant invalidation remains a complete no-op.
- Implement transactional publication preparation with no Modified consumption on a whole-cycle serialization failure, while preserving independent recipient transport failures.
- Keep network wire bounds, counter overflow and structural validation explicit and tested. Do not introduce registry versioning, synchronization handshakes, factory registries or reentrancy controls absent a concrete requirement.
