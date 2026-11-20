# Sparkle v0.1.5 — spk::LifeCycle and Pool::Lease integration

> **Design decision (documentation only).** This is a Core-level prerequisite for observing chunk Entity destruction. It does not authorize C++ implementation or a merge.

## Purpose and public contract

`spk::LifeCycle` centralizes three **distinct** subscriptions:
- `subscribeToAcquisition(...)`
- `subscribeToRelease(...)`
- `subscribeToDestruction(...)`

There is **no** generic state-change subscription.

An object starts in the `Acquired` state, including ordinary `Widget` and `Entity` objects not managed by a pool. Constructing an object establishes the state but does **not** emit an Acquisition callback, because there can be no meaningful external subscription during construction.

`acquire()` and `release()` are **idempotent**, state-changing transitions. Each successful transition invokes its respective subscribers immediately; invoking the operation in the same state does not emit twice. These operations are not exposed for arbitrary users through `Pool<T>::Lease`: the Pool invokes them on leased objects. Prefer protected transitions with explicit access for the pool and relevant derived owners, while the subscription methods stay public.

On destruction, emit `Release` first **if and only if** the object is still acquired, followed by `Destruction` exactly once. Do not allow exceptions to escape destructors. A protected explicit finalization hook is needed at the start of `Widget` and `Entity` destruction: callbacks invoked from the base `LifeCycle` destructor alone would run after derived members are torn down. The base destructor can guard against missing finalization, without guaranteeing derived state still exists.

`Widget` uses `LifeCycle::subscribeToDestruction(...)` directly, with a no-argument callback. Remove the previous Widget-specific destruction provider, pointer-taking callback, and compatibility adapter. Existing Widget observers must migrate to the shared lifecycle subscription. `Entity` (and therefore `Entity3D`) also gains the lifecycle notification.

## Integration with the existing generic `spk::Pool<T>`

**Keep the current ownership logic.** On the `V0.1.5` branch, `Pool<T>::Lease` owns an `Element*` plus a `std::weak_ptr<State>`. At lease release it returns the element to the available pool if the state still exists, or deletes the element if the pool has been destroyed. The test `LeaseCanOutliveOriginatingPool` already exercises this case. The pool's state does *not* stay alive solely because of outstanding weak leases.

When `T` derives from `LifeCycle`, insert lifecycle transitions **around** the already-existing pool operations:
1. A newly factory-constructed object begins `Acquired`. Normalize it to `Released` while it enters pool management, before exposing it through a Lease.
2. On each obtain (including first obtain, reuse and copy-obtain), transition to `Acquired` before handing the object to a caller or invoking caller-provided initialization.
3. When a Lease returns an element, transition to `Released` before recycling it; if the weak pool state is expired, still release and then delete.
4. Available elements are already `Released` when the pool destroys them. Their destructors emit `Destruction` without a second `Release`.
5. If obtaining or initializing throws, delete safely and respect the ordering `Release` (if acquired) then `Destruction`.

The pool remains usable for ordinary types (such as `int`, `std::vector`) with **no required inheritance**, via compile-time conditional integration. Lease copy/move and the current `onObtain` callback semantics remain intact. Do not mistake a lifecycle `release()` transition for ownership return: only the Lease/Pool machinery controls recycling.

Avoid invoking arbitrary lifecycle subscribers while holding the pool's internal state mutex; guard against reentrancy without changing its established ownership semantics.

## World rendering integration

The renderer subscribes to each chunk `Entity3D` destruction, retaining the associated coordinate separately. The callback removes external graphics allocations and enqueues the six neighbor coordinates for a **single** synchronization on the next render. Owned components free their resources through RAII. The callback must not dereference a partially destroyed entity. Unbinding the renderer releases all associated subscriptions; rebinding always requires an explicit call after Engine context changes.

`ChunkCollection` keeps one **immediate** chunk-modification notification for add/replace/explicit edit. Rendering deduplicates coordinates until the render pass; it does not require a separate generic chunk-removal event.

## Required tests

- LifeCycle: initial acquired state without constructor event, distinct subscriptions, acquire/release idempotence, release-before-destruction order, exception-safe finalization.
- Pool: fresh/reused/copy-obtained Leases, move/replace, release and reacquire counts, ordinary non-LifeCycle elements unaffected, callbacks outside state lock.
- Pool destruction with an outstanding Lease: leased object survives; at Lease destruction it receives Release then Destruction without insertion into a dead pool.
- Pool clear/destruction of idle objects: destruction without duplicate release.
- Entity/Widget: direct destruction notifications through LifeCycle, with Widget subscribers migrated to no-argument callbacks and no legacy adapter.
- Rendering: removal enqueues neighbors and cleans external resources; no use-after-free, duplicate rebuild or auto-rebind.

## Implementation slice

This prerequisite is now **PR00**: [LifeCycle, Entity/Widget and Pool integration](implementation/PR-00-lifecycle.md). PR02 depends on it because chunk Entity3D removal needs destruction callbacks; PR08 also consumes them. The existing Pool/Lease lifetime mechanics remain unchanged.

## Delivery

Document this as a small, separate Core integration prerequisite before Graphics PR08 and validate against the actual C++ APIs before implementing. No implementation is included in the docs PR.
