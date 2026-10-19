# Publishable simplification and EntityHierarchy multiplayer plan

> **Status:** Design plan, not an implementation PR. Agreed decisions through 19 October 2026; details explicitly marked *Open* are not settled.
>
> **Target:** `Version-0.1.3`. Existing draft [PR #31](https://github.com/EreliaStudio/Sparkle/pull/31) implements an earlier, more complex generic `Publishable`; its proposed API should be revised before it is considered the foundation of this work. This plan is separate so PR #31's current code/test claims are not misrepresented.
>
> **Guideline:** Follow `docs/ai-development-guidelines.md`: coherent responsibilities, explicit policies, no speculative mechanisms, exceptions for invalid operations, and no explanatory comments in new/modified headers.

## 1. Goal and usage model

Allow Erelia to build its ordinary Sparkle Entity/Component/Behaviour object model, with a lightweight, reusable serialization mechanism and a central network-facing hierarchy.

- Server game code constructs, simulates, and modifies Entities normally.
- Game code defines concrete Entity types, their fixed publishable Parts, Part IDs and wire layouts.
- Game code provides an Interest-selection callback for each connected client; Sparkle adds required ancestors.
- Server game code explicitly invokes `ServerEntityHierarchy::publish()` at its desired interval.
- Client code receives publications, initializes unknown Entities on demand, applies updates, and reconciles the hierarchy.
- Ordinary Engine Systems are not required to know about publication, sessions, or networking.

**Do not** reintroduce per-Entity-type publishing Systems, a special Interest inheritance tree, separate "partial Entity" types, per-client historical publication inventories on the server, or game-specific message semantics into generic `Publishable`.

## 2. Simplify generic `spk::Publishable`

### 2.1 One Part serialization contract

A `Publishable` is a non-template object with nested abstract `Publishable::Part`. It registers non-owned Parts by explicit zero-based `PartID` (`uint32_t`). The only required Part encoding hooks are:

```cpp
virtual void _encode(ByteStream::Writer &writer) const = 0;
virtual void _decode(const ByteStream::Reader &reader) = 0;
```

Remove the old distinct Edition and Update encoding/decoding hooks, `encodeEdited()`, and separate Edition/Update caches. **Everything that must be serialized is a Part.** Remove dedicated initialization encoder/decoder callbacks.

### 2.2 Flags describe publication, not Part validity

Keep a single outgoing `Publication::Update` flag. Every registered Part is always included in initialization. A Part without `Update` is initialization-only.

| Operation | Parts included | Pending modified state |
| --- | --- | --- |
| `encodeInitialization()` | All Parts | Leave untouched |
| `encodeUpdate(UpdateSelection::Modified)` | Invalidated Parts with `Update` | Consume only the successfully captured modifications |
| `encodeUpdate(UpdateSelection::All)` | All Parts with `Update` | Leave untouched |

Use an explicit `UpdateSelection { Modified, All }` enum; `Modified` is the proposed default. Both update modes use exactly the same Part bytes and format. When nothing is selected, return an empty stream.

An initialization-only Part **may** be invalidated. Its next initialization uses the new data, but ordinary updates never transmit it to already initialized clients. A separate explicit resend mechanism could be considered later; **do not implement one now**.

### 2.3 Lazy, immutable cached snapshots and per-Part generations

Each Part owns **one** cached serialization and one authoritative generation counter (e.g. `uint64_t`):

- First encoding request builds and caches its `ByteStream` lazily.
- Repeated encoding returns/reuses that baked snapshot, without invoking `_encode()` again.
- `invalidate()` increments the generation, invalidates the cache, and marks relevant `Update` registrations as modified. It does not serialize or send.
- A snapshot binds **bytes and generation from the same state**, and remains immutable after capture. Later invalidation must not relabel an earlier snapshot.
- Serializing a Part after multiple invalidations may skip intermediate generations. That's acceptable: a generation identifies **state freshness**, not an event that must be replayed.
- All outgoing Part sections (including initialization) carry Part ID, generation, and length-framed bytes.
- The first received snapshot must initialize a Part even if its generation is zero. Once initialized, snapshots with `generation <= lastAppliedGeneration` are ignored. Decoding received data must not increment the server-authoritative generation.
- The caller/implementation must coordinate mutation with snapshot capture; avoid a data race or pairing old serialized bytes with a newer generation.

`encodeUpdate(Modified)` drains pending modifications only after successful capture. An invalidation that occurs during capture **remains pending** for the next call; do not lose it while clearing the previous batch.

Publishable decoding preserves strict framing, validation, bounded Part readers, and an explicit unknown-Part policy (networked Entities should use the strict/throw behavior). Existing reusable registration/lifetime protections remain relevant.

### 2.4 Illustrative public shape (not final header)

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
        virtual void _encode(ByteStream::Writer &) const = 0;
        virtual void _decode(const ByteStream::Reader &) = 0;
    public:
        std::uint64_t generation() const;
    };

    ByteStream encodeInitialization() const;
    ByteStream encodeUpdate(UpdateSelection selection = UpdateSelection::Modified) const;
    void decode(const ByteStream::Reader &reader);

protected:
    void add(PartID id, Part *part, Flags<Publication> flags = {});
};
```

This sketch omits lifetime/unknown-ID policy declarations already implemented in PR #31. Preserve those when they are useful; do not make API choices solely from this sketch.

## 3. Publishable Engine objects

- `PublishableEntity` is an Engine `Entity` and a `Publishable`. It has a stable `spk::UUID` and a stored numeric `EntityTypeID`.
- `PublishableComponent` / `PublishableBehaviour` are their corresponding Engine types and `Publishable::Part` implementations.
- Concrete game Entity constructors create/register their fixed Parts; no dynamic network-driven creation/removal of individual Parts, no Part tombstones, and no separate requests for missing Parts.
- Part IDs are local to each Entity; Parts do not need individual UUIDs.
- Client and server may use different C++ concrete types with the same game-defined numeric type IDs, Part IDs and compatible wire formats. Erelia owns that compatibility contract.
- Component/Behaviour activation is synchronized automatically by the Sparkle wrapper, including activation changes. An inactive Part may still carry its normal state; Interest does not decide Part activation.
- Structural geometry/transform data is synchronized as part of the Entity contract. Use existing Engine capabilities rather than a new spatial inheritance hierarchy.

## 4. EntityHierarchy responsibilities

Use one shared base `EntityHierarchy` for Entity ownership, UUID lookup, type construction/registration, and hierarchy relationships. Keep network direction-specific responsibilities in:

- **`ServerEntityHierarchy`**: server binding, Interest expansion, publication/snapshot fan-out, initialization request authorization/responses.
- **`ClientEntityHierarchy`**: client binding, selected-UUID reconciliation, batched initialization requests, buffered unknown-Entity Part snapshots, and client-side reparenting/removal.

The split avoids linking server publication/Interest logic into the client.

Use the existing Sparkle Server/Client messaging and dispatcher facilities. Binding should have clear RAII subscription lifetimes; normal network dispatch occurs through the existing explicit `treatMessages()` flow. Do not add hidden networking threads/timers to these classes.

### 4.1 Type registration and construction

- Erelia explicitly registers and unregisters each game-defined `EntityTypeID` and its construction callback on the relevant hierarchy.
- An internal C++ `std::type_index -> EntityTypeID` lookup can provide convenient typed registration/lookup, while `EntityTypeID -> constructor` supports receiving network initialization.
- Never transmit RTTI hash codes as protocol identifiers.
- The hierarchy assigns the numeric type ID **once at construction**, and the Entity stores it privately thereafter. Existing Entities retain their type ID after a type registration is removed.
- Reject construction when a type is not registered. Do not silently rebind an incompatible numeric type ID while existing Entities with that ID still exist.
- Erelia may organize registrations inside game Systems' `bind(EntityHierarchy &)`, but Sparkle's generic `System` has no such requirement.
- Registration/unregistration is explicit; **no RAII type-registration contract** is planned.

## 5. Server selection and publication

### 5.1 Interest is a game callback

Erelia provides a callback yielding the complete collection of directly interesting Entities for a client/connection. It can use Registry, Query, range checks, game rules, or spatial structures.

Sparkle takes the closure of that collection over **all ancestors**:

- Interesting child -> include all parents transitively.
- Interesting parent -> **do not** automatically include its other children.
- All selected ancestors are normal, fully publishable Entities, not special structural/partial copies.
- A client's range/Interest settings are changed by **ordinary game-defined messages**, validated/stored by Erelia; Sparkle need not define an Interest type hierarchy.

### 5.2 One publication cycle and global ordering

`ServerEntityHierarchy::publish()` is explicitly invoked by game code. On each call:

1. Increment **one global `uint64_t` publication counter** for the cycle (not per client).
2. Calculate each client's selected Entity UUID set plus ancestor closure.
3. Form the union of selected Entities across clients.
4. Capture each union Entity's required cached Part update **once for this cycle**; `Modified` encoding consumes pending changes.
5. Fan out/reuse the captured immutable bytes while building the per-client publication containing its **complete selected UUID list** and relevant Entity data.

Do not let encoding for client A drain changes before client B can receive them. The server does not retain each client's previously published Entity set.

Per-Entity publication entries contain the Entity UUID and **current parent UUID in every update**, plus the encoded Part state where selected. A null/root parent has an explicit representation.

The global publication generation orders complete Interest selections. Individual Part generations independently order serialized Part data.

### 5.3 Initialization request authorization

- Clients batch requests for unknown Entity UUIDs; no per-Part initialization requests.
- On the server, a request is authorized against the requesting client's **current** selected closure including ancestors.
- Requests for nonexistent or unauthorized UUIDs receive an explicit expected-failure response; don't leak Part state.
- Authorization is **not** satisfied just because the client knows a UUID from a parent's child list.

An Entity initialization response contains at least its UUID, game-defined `EntityTypeID`, current parent UUID, **all direct child UUIDs**, and **all registered Part snapshots** with generations.

The full child UUID list includes uninteresting children. That intentionally exposes their IDs, **not** their initialization data.

## 6. Client synchronization and hierarchy reconstruction

### 6.1 Complete selected-Entity reconciliation

The client remembers only the latest accepted authoritative selected-Entity set. On receiving a newer publication cycle:

- Compare the complete set with currently managed remote Entities.
- Request initialization of unknown selected UUIDs, batching all unknowns from the publication and suppressing duplicate outstanding requests.
- Remove/release remote Entities absent from the latest set.
- Apply current Part snapshots by generation; stale Part data cannot overwrite newer Part data.
- Ignore old/duplicate **selection sets** using the global publication counter.

**Important implementation issue:** discarding every Part payload from an older *publication* can permanently lose a disjoint `Modified`-only Part change if a newer publication updated other Parts. Determine how to reject stale **selection** without losing safe, newer-per-Part snapshots for Entities still selected. Do not accidentally resurrect unselected Entities.

### 6.2 Unknown Entities and pending Part snapshots

For an unknown Entity receiving updates, queue/request **Entity** initialization only; retain at most the **latest snapshot per Part ID** (highest generation), not an unbounded replay log. Once initialization arrives:

- Construct the Entity and decode its full initial Part state.
- Apply buffered snapshots only if their Part generation is newer.
- Discard the buffer when completed or when no longer applicable.

This relies on each Part snapshot representing its **complete replacement state**, not an incremental command or patch.

### 6.3 Parents and children

- Every ongoing Entity publication includes current parent UUID, enabling reparenting without reconstruction.
- A newly constructed Entity whose parent is unknown attaches to a **local-only common construction root**.
- When a parent initializes, its full direct child UUID list is used to attach **already existing** children to that parent, unless a newer known parent relationship supersedes it.
- Unknown child UUIDs are **ignored**, not queued or recursively initialized.
- Missing needed parents are requested through the same batched Entity initialization path.
- Use existing Engine Entity parenting APIs; do not introduce network-specific partial Entity classes.
- The current hierarchy must not be undone by a delayed parent/child initialization response.

### 6.4 Late initialization response policy — **Option B**

If an initialization was requested while an Entity was interesting but arrives after a newer selection removed it, **process initialization normally, then run reconciliation**. The newly created Entity may immediately be removed. Do not retain it as an indefinite cache and do not cancel the response solely because Interest changed.

## 7. Responsibility and scope boundaries

`Publishable` knows: Part IDs, registration, one Part encoder/decoder, cache invalidation, per-Part generations, selection of modified/all update sections, and safe serialization/decoding.

`EntityHierarchy` knows: owned Entity identity/type/hierarchy, remote construction, Interest-selected Entity collections, publication-cycle version, initialization requests, recipient selection, and hierarchy reconciliation.

Erelia knows: concrete Entity/Part layouts, meaningful numeric type IDs, Interest rules, view range settings, ordinary gameplay requests and their validation, simulation/scheduling, and client presentation.

Not planned: separate Edition transport, callbacks for initialization-only data, special partial Entities, automatic recursive loading of all children, per-Part dynamic network factories, per-client server-side publication histories, and background networking jobs in the hierarchy.

## 8. Implementation sequence

1. **Revise PR #31's generic Publishable proposal:** reduce to Initialization + Update, unify Part snapshot serialization, add generations and Update-only flag, eliminate callbacks and Edition, retain safety and reuse.
2. **Implement Engine wrappers:** fixed registration of Publishable Components/Behaviours, Entity UUID/type ID, activation and structural synchronization.
3. **Implement EntityHierarchy base and type registration:** ownership, lookup, construction, relationship operations and construction root.
4. **Implement server/client direction-specific hierarchies:** Interest callbacks, publication capture and fan-out, initialization requests, client reconciliation.
5. **Add focused unit/integration/stress coverage on Linux and Windows**, document examples and verify real CI on the implementing PR heads.

The documentation/planning PR itself does **not** claim these tasks are implemented.

## 9. Required test scenarios

- Lazy cache reuse; no extra `_encode()` calls until invalidation; generation increments per invalidation; old captured snapshots remain unchanged.
- Modified vs All inclusion; initialization-only Parts; pending invalidations survive failed encoding and concurrent capture; shared snapshots across multiple recipients.
- Generation 0 first decode; duplicate/older generation skipped; initialization older than buffered updates; different Parts at different generations; malformed frames and unknown IDs rejected.
- Two clients with disjoint and overlapping Interest; ancestor closure; one encoded snapshot per Entity per publication; complete selection counter ordering.
- Child arrives before parent; construction root; complete child UUID list; unknown children ignored; reparent without recreation; stale relationships do not override newer ones.
- Batched unknown-Entity requests; unauthorized init requests rejected; late authorized init response constructed and then removed when no longer selected.
- Out-of-order publication snapshots where stale selection must be ignored **without losing non-overlapping modified Part data**.
- Subscription and object lifetimes, type unregister/re-register invariants, inactive Component/Behaviour state, server/client differing C++ types with compatible Part IDs.

## 10. Open design/implementation checks (do not silently choose)

1. Resolve the precise rule for applying late Part payloads from a publication whose **selection** is stale; see section 6.1.
2. Specify correct concurrency and version/bytes capture when mutable Part state changes during `_encode()`, including what thread may call `invalidate()`.
3. Verify activation changes and structural transforms are reliably invalidated/serialized by the chosen Engine wrappers, without creating hidden extra message types.
4. Define authoritative ownership boundaries between remote Entities in `ClientEntityHierarchy` and game-local Entities so reconciliation never removes locally owned objects.
5. Confirm wire-format/version compatibility policy and handling of Part-generation overflow or replicated Entity incarnation changes (if UUIDs can ever be reused).

These are follow-up design checks, not justification for introducing extra abstractions prematurely.
