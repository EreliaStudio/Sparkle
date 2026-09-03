# Sparkle unit-test suite plan

## Purpose and conventions

This document is the implementation checklist for the new GoogleTest unit-test suite. It is based on the public API and implementation guards of the current library, with the legacy suite under `legacy-sparkle/tests/TUs` used as the style reference.

The legacy conventions to preserve are:

- use behavior-oriented test names (`TypeAreaTest, ExpectedBehavior`), with one independently understandable scenario per test;
- make the first case in every section a realistic usage example that can double as documentation;
- verify observable state and callbacks, not private implementation details;
- cover const and mutable access, type/template variants, boundaries, state transitions, reuse after reset, and ownership/lifetime behavior;
- use deterministic synchronization for multithreaded tests and a shared OpenGL test context for GPU tests;
- use golden images only when pixels are the public result; keep geometry/state tests non-visual;
- for every `[throws]` case, assert the exact exception type and, when the text is part of the diagnostic contract, its message/context;
- use death tests only for documented process-termination contracts; never rely on undefined behavior as a test oracle.

Public data-only records and enums are tested with the class that consumes them. Public nested classes with behavior are named explicitly below. Compile-time concepts and deleted operations are checked with `static_assert`/type traits in the closest owning section.

## Foundation and containers

### `spk::Exception`

- **Standard usage:** construct an exception with a message and source location, add context while propagating it, and verify `message()`, `location()`, `cause()`, and the formatted `what()` chain.
- Construct with and without a nested `std::exception_ptr`; verify nested Sparkle and standard exceptions are represented without slicing.
- Add several contexts and verify ordering, indentation, source frames, and `what()` stability across repeated calls.
- Verify empty messages and an empty exception pointer remain safe and inspectable.

### `spk::readTextFile`

- **Standard usage:** read a temporary UTF-8/text file and preserve its exact contents, including newlines and embedded NUL bytes.
- Read an empty file and a large file.
- **[throws `std::runtime_error`]** Read a missing, inaccessible, or directory path; verify the path is present in the diagnostic.

### `spk::CachedData<T>`

- **Standard usage:** lazily generate once, reuse the cached value, invalidate, and regenerate on the next access.
- Cover `get`, conversions, dereference/arrow, `regenerate`, `set`, `emplace`, and `release`; verify destructor callbacks run exactly once for each discarded cached value.
- Cover empty and populated copy/move construction and assignment, self-assignment, move-only-compatible paths, and reuse of moved-from objects where supported.
- Verify `invalidate`/`release` on an empty cache are no-ops and `release` transfers the current value without invoking the configured destructor.
- **[throws `std::runtime_error`]** Access an empty cache with no generator (`CachedData: generator not set`).

### `spk::DataModel<T>`

- **Standard usage:** construct rows, append/insert/edit/enable/erase them, resolve stable row IDs, and observe inserted/changed/removed/reset notifications with exact ranges.
- Cover empty models, initializer-list/vector construction, insertion at front/middle/end, zero-count erase, full clear, repeated no-op enable/clear, and const/mutable accessors.
- Verify row IDs stay stable across insertions, move with rows, disappear after erasure, and are not reused after reset.
- **[throws `std::out_of_range`]** Access invalid rows; insert past `rowCount`; erase a range whose start/count exceeds the model.
- **[throws `std::overflow_error`]** Exhaust the row-ID counter through a focused test seam or equivalent deterministic fixture.

### `spk::TextModel`

- **Standard usage:** use the inherited string model API and connect it to `TextModel::View` with its default delegate.
- Verify UTF-8 strings, empty strings, duplicate strings, enabled state, stable IDs, and inherited notifications.

### `spk::JSON::Value`

- **Standard usage:** build a mixed object/array, serialize it, parse it back, and read every supported scalar and custom `toJSON`/`fromJSON` type.
- Cover null, booleans, signed/unsigned integer boundaries, finite floating-point values, strings/escapes/Unicode, empty and nested containers, reset/type predicates, lookup, append, resize, equality, compact and pretty formatting.
- Parse whitespace, BOM allowed/rejected, all JSON escapes and numeric forms; reject trailing tokens, malformed literals/numbers/escapes/UTF-8, duplicate keys when forbidden, and nesting beyond `maxDepth`.
- **[throws `std::runtime_error`]** Store an unsigned integer beyond `int64_t`, or NaN/infinity; request the wrong type or an out-of-range numeric conversion.
- **[throws `std::runtime_error`]** Use object operations on non-objects, array operations on non-arrays, miss an object key, exceed an array index, or request `size()` on a scalar.
- **[throws `std::runtime_error`]** Read/write missing or inaccessible files and serialize non-finite floating values; verify parse diagnostics include useful offsets/context.

### `spk::JSON::Error`

- **Standard usage:** construct an error and verify file, JSON path, raw message, and composed `what()` text.
- Cover root and nested/indexed paths, empty file/path/message, and stable access after copies/moves.

### `spk::JSON::Loader`

- **Standard usage:** parse a valid JSON file into a `Value`.
- **[throws `spk::JSON::Error`]** Parse a missing, unreadable, or invalid file; verify file, root path, and wrapped parser message.

### `spk::JSON::Reader`

- **Standard usage:** read required/optional scalars, vectors, fixed arrays, enums, child objects, and child arrays from a realistic document.
- Verify optional defaults apply only to absent fields, exact fixed-array arity, path extension through keys/indices, `contains`, `value`, `file`, `path`, and `forbidUnknown` with all keys allowed.
- **[throws `spk::JSON::Error`]** Read a missing required member, a wrong scalar/container/custom type, or a fixed array with wrong arity; verify the exact failing path.
- **[throws `spk::JSON::Error`]** Use an unknown enum value (listing known values), request a child with the wrong shape, encounter a non-object child-array element, call object operations on a scalar, or reject an unknown key.

### `spk::PolymorphicContainer<T>`

- **Standard usage:** through a small public test subclass, register several derived objects, query one/all by type and predicate, and unregister them while observing addition/removal contracts.
- Cover const and mutable queries, no match, multiple inheritance levels, regex-like predicates at wrappers, registration order, and unregistering a foreign/already removed element.
- Exercise positive and negative type caches before and after additions/removals so stale pointers or stale negative results cannot survive.
- Verify ownership/destruction timing and that callbacks see live elements during addition/removal.

### `spk::ProtectedData<T>` and `Reader`/`Writer`

- **Standard usage:** read the initial value, mutate it through a writer, then observe it through multiple concurrent readers.
- Verify a writer excludes readers/writers, readers coexist, locks release at scope exit, and non-copyable API properties hold at compile time.
- Stress readers and writers with deterministic final state; document rather than execute the explicitly undefined owner-outlives-guard misuse.

### `spk::ThreadSafeCollection<Key, Value>`

- **Standard usage:** emplace, `contains`, `get`, `tryGet`, erase, and reuse a key.
- Cover move-only values, construction arguments, missing erase, stable shared ownership after erase, and concurrent readers/writers on distinct and shared keys.
- **[throws `std::logic_error`]** Emplace a duplicate key.
- **[throws `std::out_of_range`]** `get` a missing key; verify `tryGet` returns null for the same case.

### `spk::ThreadSafeFIFO<T>` and `Producer`/`Consumer`

- **Standard usage:** create endpoints, publish/emplace values, wait, drain in FIFO order, and reuse the channel.
- Cover empty drain, move-only values, multiple producers, one consumer, batches arriving during drain, and endpoint lifetime after the originating wrapper is destroyed.
- Verify `wait` blocks until publication, returns false on stop request, handles an already requested stop, and does not lose notifications around the wait boundary.
- Verify the consumer is movable but not copyable and that its returned drain buffer is reused safely between calls.

### `spk::ThreadSafeSlot<T>` and `Producer`/`Consumer`

- **Standard usage:** publish snapshots and acquire the latest immutable value from direct and split endpoints.
- Verify an untouched slot returns null, newer publications replace older ones, acquired snapshots remain alive and unchanged, and move-only values work.
- Stress concurrent publication/acquisition; readers must only see complete values and eventually observe the final publication.
- Verify copied producers/consumers share state and remain valid after the factory/wrapper is destroyed.

## Design-pattern traits

### `spk::ContractProvider<Args...>` and `Contract`

- **Standard usage:** subscribe several callbacks, trigger in registration order, resign one contract, trigger again and than verify RAII unsubscription.
- Cover empty providers, empty callbacks if supported, move construction/assignment of contracts, self move-assignment, provider destruction before contracts, explicit invalidation, and `empty`/validity state.
- Exercise subscribe, resign, invalidate, provider destruction, and nested trigger during dispatch; verify mutations are deferred, order is deterministic, and the latest queued nested arguments are delivered.
- Verify a throwing callback restores a usable provider, applies pending removals safely, and propagates the original exception.

### `spk::StatefullTrait<State>`

- **Standard usage:** subscribe per state, change states, and verify only the matching provider fires after the stored state changes.
- Cover initially unset state, same-state no-op, several enum/hashable states, contract resignation, and callback-driven state changes.

### `spk::ActivableTrait`

- **Standard usage:** activate, deactivate, toggle, and observe the matching callbacks and `isActive()` state.
- Cover default/explicit initial status, repeated no-op transitions, and callback-triggered transitions.

### `spk::InherenceTrait<T, Comparator>`

- **Standard usage:** build and reparent a small hierarchy, verify parent/children access, comparator ordering, hooks, and parent-edition callbacks.
- Cover detaching to null, reparenting between parents, repeated same-parent assignment, resort after a key change, destruction order, and removing an already detached child through safe APIs.
- **[throws `std::runtime_error`]** Add a null child.
- **[throws `std::invalid_argument`]** Remove a null child.
- **[throws `std::logic_error`]** Remove an object that is not a child.
- **[throws `std::logic_error`]** Create a circular hierarchy. <<<--- This one may not be implemented yet, but create the test anyway

### `spk::NameTrait`

- **Standard usage:** construct with ordinary, empty, Unicode, and duplicate names and verify exact preservation.

### `spk::ResizeableTrait`

- **Standard usage:** update the complete size hint and individual minimal/maximal/preferred fields and observe one notification per effective call.
- Cover zero, negative, infinite, and contradictory hints as currently accepted data; verify destructor invalidates outstanding contracts.

### `spk::VersionedTrait`

- **Standard usage:** invalidate repeatedly, verify monotonically increasing versions and callbacks, then move the object and keep its version/lifetime semantics coherent.
- Cover version wrap through a deterministic seam if wrap behavior is intended, subscriber lifetime, moved-from behavior, and destruction invalidation.

### `spk::ContextualizableTrait<Context>`

- **Standard usage:** change context and observe old/new values before storage changes.
- Cover default context, same-context no-op, pointer/null and value contexts, callback-driven changes, and contract resignation.

## Math and value types

### `spk::TVector2<T>`

- **Standard usage:** construct/convert vectors and perform component-wise arithmetic and compound assignment for float, signed, and unsigned aliases.
- Cover zero, negative, fractional, extrema, equality, indexed `data`, structured component aliases, and stream formatting.
- Record native C++ division/overflow preconditions; do not encode undefined integer division-by-zero or overflow as runtime behavior.

### `spk::TVector3<T>`

- **Standard usage:** construct/convert, calculate arithmetic, dot/cross products, length, and normalize a floating vector.
- Cover parallel, antiparallel, orthogonal, negative, very small/large values, handedness, aliases/data layout, equality, and streaming across aliases.
- **[throws `std::domain_error`]** Normalize a zero-length floating vector.

### `spk::TVector4<T>`

- **Standard usage:** construct from components/Vector2/Vector3, convert types, perform arithmetic/compound assignment, compare, and stream.
- Cover homogeneous `w` values, zero/negative/extreme values, aliases/data layout, and compile-time trivial-copy properties used by GPU data.

### `spk::Rect2D`

- **Standard usage:** create a rectangle, test containment, move to origin, shrink, intersect, compare, and stream it.
- Cover points on every boundary, empty rectangles, disjoint/touching/contained/identical overlaps, negative anchors, oversized shrink offsets, and unsigned-size arithmetic boundaries.

### `spk::Quaternion`

- **Standard usage:** create rotations from axis-angle/Euler, compose and invert them, rotate through matrices, convert back to Euler, and interpolate with `slerp`.
- Cover identity, conjugate, non-unit input normalization, opposite/near-identical quaternions, alpha 0/1/outside range as supported, gimbal-lock neighborhoods, `lookAt` with normal directions, and floating tolerances.
- **[throws `std::runtime_error`]** Invert the all-zero quaternion.
- **[throws `std::domain_error`]** Normalize the all-zero quaternion; cover degenerate `lookAt` vectors according to the resulting public contract.

### `spk::Matrix<X, Y>` and `Column`

- **Standard usage:** build identity, translation, scale, rotation, orthographic and perspective matrices; multiply matrices/vectors and verify a known transform pipeline.
- Cover zero initialization, const/mutable column access, non-square compile-time availability, multiplication order, and quaternion normalization.
- **[throws `std::out_of_range`]** Access a column or row outside its dimensions.
- **[throws `std::invalid_argument`]** Use equal orthographic bounds on each axis/plane, or invalid perspective FOV/aspect/near/far values (including boundary and non-finite inputs once their contract is fixed).

### `spk::UUID`

- **Standard usage:** generate, stringify, parse, compare/order, inspect bytes/version/RFC variant, and round-trip canonical text.
- Cover null, explicit storage, uniqueness sample, case policy, all-zero/all-maximum bytes, malformed length/hyphens/hex/whitespace, and `tryParse` no-throw behavior.
- **[throws]** `fromString` on every malformed category; assert the implementation's exact exception type once stabilized.

### Public enums and POD records

- Verify default values, equality, aggregate initialization, and meaningful numeric/bitmask mappings for `Alignment`, `ActivationStatus`, `Orientation`, `ReferenceFrame`, `FocusMode`, `ViewRegion`, `Color`, OpenGL `Padding<N>`, render/update contexts, and request/event record variants.
- Use `static_assert` for required trivial-copy/layout properties and variant membership; test bitmask composition for `ClearRenderCommand::Mask` and edge composition for `ScalableWidget::Edge`.

## Input, events, windowing, and application

### `spk::Keyboard`

- **Standard usage:** start with every key up, update keys/glyph, use mutable/const indexing, stringify/stream keys, and parse names back.
- Cover representative control, alphanumeric, numpad, function, OEM, left/right modifier, unknown, case/alias policy, and invalid text.
- **[throws `std::out_of_range`]** Index with a value outside the backing array if the public enum can represent it.

### `spk::Mouse`

- **Standard usage:** start with all buttons up and zero position/delta/wheel, then update and read every button through const/mutable indexing.
- Cover invalid button values according to the array bounds contract.

### `spk::InputState` conversions

- Verify narrow/wide strings and streams for `Down` and `Up`.
- Cover an invalid cast and document whether it throws, produces an unknown token, or is an unreachable precondition.

### `spk::EventBase`, `Event<Record>`, and `DeviceEvent<Record, Device>`

- **Standard usage:** bind records/devices by reference, consume an event, request/release keyboard and mouse focus, and inspect each channel independently.
- Cover repeated identical focus requests, conflicting requests on one channel, null widget requests, out-of-range channels, and independence of copied record/device state.

### `spk::MouseMovedEvent`

- **Standard usage:** inspect record/device, request a cursor position, and consume the request.
- Cover no request, repeated requests (latest value), negative/large coordinates, and no-throw access.

### `spk::EventDispatcher`

- **Standard usage:** use a recording subclass/tree to dispatch every event type and verify acceptance, child propagation order, consumption short-circuiting, and passive observers.
- Cover inactive/non-accepting nodes, empty trees, nested dispatch, focus-change records, and exceptions from handlers without corrupting later dispatch.

### `spk::WinAPI::WakeEvent`

- **Standard usage:** create, notify, wait from another thread, and verify auto-reset behavior and handle validity.
- **[throws `std::system_error`]** Inject/force `CreateEventW` and `SetEvent` failures where feasible; verify Win32 error codes and operation names.

### `spk::WinAPI::MessageQueue`

- **Standard usage:** dispatch posted messages and wake a waiting queue with the supplied event.
- Cover no pending messages, quit messages, repeated wakeups, invalid wait handles, and thread affinity.
- **[throws `std::system_error`]** Force `MsgWaitForMultipleObjectsEx` failure.

### `spk::WinAPI::Window::Class`

- **Standard usage:** register a uniquely named class, create windows from it, and release it after windows are destroyed.
- Cover Unicode identifiers, duplicate registration behavior, and multiple independent class instances.
- **[throws `std::system_error`]** Surface class-registration/unregistration failures with the Win32 operation and code.

### `spk::WinAPI::Window`

- **Standard usage:** create a hidden window, inspect client geometry/handle, route a message through the callback, request/consume closure, destroy, and recreate.
- Cover visible/hidden, zero/minimum dimensions, negative position, Unicode title, callback exceptions captured and rethrown, idempotent destroy, and destruction messages.
- **[throws `std::logic_error`]** Create an already-created window; request geometry before creation.
- **[throws `std::system_error`]** Cover conversion, size adjustment, creation, geometry, or destruction API failures through narrow platform seams where practical.

### `spk::Window::Native`

- **Standard usage:** verify `Pending -> Ready -> Releasing -> Released`, access the native frame, and tolerate repeated lifecycle calls as specified.
- Cover identifier preservation indirectly, destruction in every lifecycle state, and const/mutable access.

### `spk::Window::State`

- **Standard usage:** manipulate the root, devices, background, focus per channel, dispatch roots, and complete lifecycle transitions.
- Cover replacing focus (release old/acquire new), release by wrong widget, null focus, clear with none, inactive focused widgets, and root fallback.

### `spk::Window::Surface`

- **Standard usage:** create against a native window, set geometry, make current, manage GPU resources, present, destroy, and follow lifecycle transitions.
- Cover repeated create/destroy, destruction from partial states, resize before/after creation, and resource reclamation.
- **[throws `std::logic_error`]** Create without a native window or from the wrong state; make current/present before initialization.
- **[throws `std::runtime_error`/`std::system_error`]** Cover unsupported WGL context creation and Win32/OpenGL setup failures using capability-aware tests.

### `spk::Window`

- **Standard usage:** compose shared Native/State/Surface parts and verify root, geometry, closing, and closed state across lifecycles.
- Cover mismatched/partial lifecycle timing and shared-part lifetime after facade destruction.

### `spk::Application`

- **Standard usage:** create a hidden window, retrieve it, run enough of the loop to initialize/update/render, close it, quit with an exit code, and join all runtimes.
- Cover multiple windows, close while pending/ready/releasing, quit before/while running, window lookup const/mutable, event routing, and worker exceptions crossing to `run()`.
- **[throws `std::logic_error`]** Create a duplicate window identifier; register duplicate runtime objects/snapshot endpoints.
- **[throws `std::out_of_range`]** Retrieve/close an unknown identifier if that is the facade's final contract.
- **[throws `spk::Exception`/`std::runtime_error`]** Verify contextual wrapping for platform/update/render failures and a released native frame during surface creation.

### `spk::Clipboard`

- **Standard usage:** save existing clipboard text, write/read Unicode text, query availability, and restore prior contents.
- Cover empty text, multiline/supplementary characters, non-text clipboard content, contention/open failure, and no-throw boolean/optional failure reporting.

## Engine and registries

### `spk::Registry<Context, Type>` and `Object`

- **Standard usage:** construct contextual objects, query by context, change context, and verify automatic add/remove on construction, context change, and destruction.
- Cover null/default contexts, multiple objects, same-context no-op, context destruction order, const behavior, and registry cleanup between tests.
- Verify addition/removal contracts fire once with the correct old/new context and live object pointer.

### `spk::Registry::Query` operations

- **Standard usage:** compose base registry selection with predicate, intersection, and union operations; execute repeatedly and observe invalidation after registry edits.
- Cover empty/all sets, duplicates, different contexts, operation order, cached execution, operation copy/move restrictions, and subscriptions disappearing with a query.
- Exercise `FromRegistry`, `IntersectWith`, `UnionWith`, `ContainParticipant`, and `ContainBehaviour` with type, regex, and predicate forms; additions/removals/name/context changes must invalidate only affected queries.

### `spk::EntityAttachment`

- **Standard usage:** attach/detach to entities and verify owner, engine context propagation, activation, and naming.
- Cover construction with owner/null, reattachment across entities/engines, owner destruction ordering, and same-owner no-op behavior.

### `spk::EntityAttachmentCollection<T>`

- **Standard usage:** through `Entity`, `Engine`, and exposed test subclasses, add typed elements, query first/all by type/predicate/regex, observe contracts, remove, and destroy exactly once.
- Cover no matches, duplicate types/names, const queries, cache invalidation after removal, foreign removal, and callbacks that query or remove during notification.

### `spk::BehaviourCollection`

- Apply the attachment-collection matrix specifically to behaviours, including type/predicate/regex lookup and addition/removal notifications.

### `spk::SystemParticipantCollection`

- Apply the attachment-collection matrix specifically to participants, including typed 2D/3D queries and cache invalidation.

### `spk::SystemCollection`

- Apply the polymorphic-collection matrix to systems, including const/mutable lookup, regex selection, notifications, and foreign removal.

### `spk::Behaviour`

- **Standard usage:** derive a recorder, attach it, propagate geometry, update with context, build render commands, and dispatch interactions while active.
- Cover null owner, deactivate/reactivate, repeated geometry, virtual hook ordering, and interaction rejection when inactive/owner inactive.

### `spk::System`

- **Standard usage:** derive a recorder, add it to an engine, attach/detach context, update active systems, and dispatch events.
- Cover null engine, reattachment, deactivation, repeated attach, and removal/destruction contracts.

### `spk::System::Participant`

- **Standard usage:** attach to an entity, receive geometry, contribute a render command, and appear in registry queries.
- Cover null owner, active/inactive state, reattachment, removal, and hook ordering.

### `spk::System::Participant2D`

- **Standard usage:** attach to an `Entity2D` and verify covariant owner access and typed registry membership.
- **[throws `std::invalid_argument`]** Attach to a plain/3D entity; verify a failed attach preserves prior ownership/context.

### `spk::System::Participant3D`

- **Standard usage:** attach to an `Entity3D` and verify covariant owner access and typed registry membership.
- **[throws `std::invalid_argument`]** Attach to a plain/2D entity; verify a failed attach preserves prior ownership/context.

### `spk::Entity`

- **Standard usage:** build a hierarchy, add behaviours/participants, attach it to an engine, propagate geometry/update/render/events, then remove it cleanly.
- Cover inactive branches, parent/context changes, add/remove during callbacks, duplicate names/types, root vs detached entity, z/interaction ordering inherited from dispatch, and recursive destruction.

### `spk::Entity2D`

- **Standard usage:** construct with/without a 2D parent and verify the automatically owned transform, typed registry membership, and parent-transform relationship.
- Cover reparenting through plain/2D/3D entities, context changes, transform lifetime, and const/mutable access.

### `spk::Entity3D`

- **Standard usage:** construct with/without a 3D parent and verify the automatically owned transform, typed registry membership, and parent-transform relationship.
- Cover reparenting through plain/2D/3D entities, context changes, transform lifetime, and const/mutable access.

### `spk::Transform2D`

- **Standard usage:** place/move/rescale/rotate/depth-change a child hierarchy; read local/world values and validate local/world model and inverse matrices.
- Cover identity, parent composition, reparenting, cache reuse/invalidation through descendants, edition callbacks, negative scale/rotation/depth, and round-trip point transforms.
- **[throws `std::runtime_error`]** Request any accessor with an invalid `ReferenceFrame` value.
- **[throws `std::runtime_error`]** Request an inverse matrix with a zero-scaled axis; verify other cached values remain usable.

### `spk::Transform3D`

- **Standard usage:** compose position/scale/quaternion rotations in a parent hierarchy and validate local/world values plus model/inverse matrices.
- Cover Euler/quaternion overloads, axis-angle rotation, identity, non-unit inputs, negative scale, reparenting, recursive cache invalidation, edition callbacks, and point round trips.
- **[throws `std::runtime_error`]** Request an accessor with an invalid `ReferenceFrame` value.
- **[throws `std::runtime_error`]** Request an inverse matrix with any zero-scaled axis.

### `spk::Engine`

- **Standard usage:** add entities/systems, apply geometry, update, build the pre-scene/scene render passes, dispatch events, and remove everything.
- Cover detached/null entity calls according to contract, duplicate add/remove, inactive entities/systems, modifications during update, root behavior, pass ordering, and destruction with populated registries.

## Graphics resources and geometry

### `spk::Texture`

- **Standard usage:** use a test subclass to upload color pixels, inspect format/size/source/mipmap, edit a subregion, synchronize, read back, and export PNG.
- Cover all format descriptors/classifiers, 1D/2D/3D/cube targets where supported, zero sizes, resize preservation/reset policy, exact-boundary writes, mipmap changes, move/lifetime, and render targets.
- **[throws `std::invalid_argument`/`std::overflow_error`]** Invalid/depth CPU formats, mismatched spans, null pixel source, zero/invalid render target, and byte-count multiplication overflow.
- **[throws `std::out_of_range`]** Subregion write outside texture bounds.
- **[throws `std::logic_error`]** Unsupported target/source operations, resize/write a render target, pixels from render target, synchronize empty/invalid/incompatible textures, or export non-CPU/non-color/empty textures.
- **[throws `std::runtime_error`]** OpenGL texture creation or PNG writing failure.

### `spk::Image`

- **Standard usage:** load the same known image from file and encoded bytes and verify dimensions, RGBA conversion, pixels, and PNG round trip.
- Cover grayscale, dual-channel, RGB, RGBA, transparency, tiny/large images, moved instances, and input lifetime.
- **[throws `std::runtime_error`]** Missing file, undecodable/truncated data, or unsupported channel count.
- **[throws `std::invalid_argument`/`std::overflow_error`]** Empty encoded data or data larger than stb's accepted range.

### `spk::SpriteSheet`

- **Standard usage:** load a grid, inspect count/unit/sections, map coordinates to IDs, and retrieve sprites both ways.
- Cover 1x1, non-square grids, image dimensions not divisible by count, first/last cells, and section precision.
- **[throws `std::invalid_argument`]** Either sprite-count dimension is zero.
- **[throws `std::out_of_range`]** Coordinates or linear IDs at/past each boundary.

### `spk::Font`

- **Standard usage:** load a known TrueType font, decode UTF-8, obtain/cache an atlas, load glyphs, measure text/baselines, choose an optimal size, and observe atlas edits.
- Cover ASCII, multibyte/supplementary/invalid UTF-8 policy, empty strings, whitespace/newlines, missing glyph fallback, duplicate loads, outline sizes, glyph rescaling, atlas growth/quadrants, and size ordering/cache identity.
- Verify measurements against glyph metrics and a golden atlas for a small deterministic set; check all-renderable loading separately as a slower test.
- **[throws `std::runtime_error`]** Missing font file, empty raw data, or invalid TrueType data.
- **[throws `std::logic_error`]** Request an atlas/operation without loaded font data or force impossible negative atlas placement through a seam.

### `spk::Font::Size`

- Verify constructors, equality/inequality, strict ordering by glyph then outline, zero sizes, and use as stable atlas-map keys.

### `spk::Font::Glyph`

- Verify position/UV/step/baseline/size data and rescaling, including zero, fractional, non-uniform, and repeated scale ratios.

### `spk::Font::Atlas`

- Verify subscription, lazy glyph loading/fallback, UTF-8 overloads, measurements, atlas growth, texture pixels, and subscriber notification exactly once per effective edition.

### `spk::GPUResource`

- **Standard usage:** use a fake resource plus a real OpenGL context to validate, activate, synchronize only when generations differ, bind, move, release, reclaim, pool, and reuse instances by kind/score.
- Cover several surfaces/collections, destruction on another thread, resource before/after surface destruction, identifier uniqueness, explicit clear, and creation returning null.
- **[throws `std::invalid_argument`]** Activate with a null target surface.
- **[throws `std::logic_error`]** Subscribe/activate a moved-from resource or accept a null created instance; verify contextual wrapping of synchronize/bind failures.

### `spk::GPUResourceCollection`

- Verify per-surface entry creation, generation tracking, cross-thread release queue, explicit reclamation/clear, pooling by kind and recycling score, and safe teardown before/after resources.
- **[throws `std::logic_error`]** Reject a resource whose creation hook returns a null instance.

### `spk::BufferGPUResource` and `Storage`

- **Standard usage:** through concrete buffers, resize/reserve/append/write/clear, select usage, upload, and retrieve exact bytes.
- Cover zero size, capacity growth, offsets at end, partial overwrite, CPU/GPU versioning, reuse, and move-only resource behavior.
- **[throws `std::overflow_error`]** Byte-size arithmetic exceeds `size_t`.
- **[throws `std::out_of_range`]** A write exceeds the logical buffer size; **[throws `std::runtime_error`]** OpenGL buffer creation failure.

### `spk::VertexBuffer`

- **Standard usage:** configure a mixed vertex layout, push/insert/access typed vertices, activate it, and verify resolved offsets/stride/count and GPU data.
- Cover every attribute type/interpretation, normalization, padding, duplicate locations, clear/reconfigure while empty, zero vertices, reserve/resize, and GL attribute configuration.
- **[throws `std::invalid_argument`]** Component count outside 1..4, incompatible type/interpretation/normalization, duplicate location, or unknown attribute type.
- **[throws `std::logic_error`]** Typed data without layout, wrong vertex stride, or layout edits while vertices exist.
- **[throws `std::overflow_error`]** Attribute/padding/stride/buffer size or GL stride overflow.

### `spk::IndexBuffer`

- **Standard usage:** configure each unsigned index type, push/insert/access indices, activate, and verify stride/count/GPU data.
- Cover empty configuration reset, reserve/resize, maximum values, and clearing data before reconfiguration.
- **[throws `std::logic_error`]** Data access without type, wrong C++ type, type/configuration change while populated.
- **[throws `std::overflow_error`]** Index-buffer byte-size arithmetic overflow.

### `spk::UniformBuffer`

- **Standard usage:** allocate at a binding point, set/get typed data, synchronize, retrieve, and verify binding.
- Cover zero-sized/default buffers, byte-vector overload, mutable access followed by validation, and reuse.
- **[throws `std::invalid_argument`]** Raw data size differs from allocation.
- **[throws `std::logic_error`]** Typed set/get/retrieve size differs from the requested type.

### `spk::ShaderStorageBuffer`

- **Standard usage:** configure fixed plus dynamic parts, set typed data, resize, edit through CPU views, retrieve a GPU view, and verify binding/alignment.
- Cover dynamic-only form, zero elements, const/mutable views, multiple trivially copyable types, buffer reuse, and exact layout bytes.
- **[throws `std::invalid_argument`/`std::overflow_error`]** Zero dynamic stride, invalid fixed/dynamic sizes or counts, wrong typed setters, and total-size overflow.
- **[throws `std::logic_error`]** Wrong/no fixed part, wrong requested types, or insufficient alignment for fixed/dynamic views.

### `spk::VertexArray`

- **Standard usage:** connect configured vertex/index buffers, validate, activate, render, then modify generations and verify VAO reconfiguration.
- Cover buffer replacement, attribute disabling, resource lifetime/recycling, and default empty layout.
- **[throws `std::logic_error`]** Activate a vertex array without both buffers.
- **[throws `std::overflow_error`]** Configured vertex stride exceeds `GLsizei`; **[throws `std::runtime_error`]** VAO creation failure.

### `spk::LayoutBuffer`

- **Standard usage:** populate its vertex/index buffers, validate the linked vertex array, activate it, and verify const access plus synchronized generations.
- Cover empty/default layout, repeated validation, clear-and-reconfigure, and non-copyable/non-movable compile-time guarantees.

### `spk::Sampler`

- **Standard usage:** bind a texture at a unit with each filtering/wrap/mipmap mode and verify OpenGL sampler state.
- Cover property changes/versioning, texture replacement, high valid binding points, and resource reuse.
- **[throws `std::logic_error`]** Activate without a texture; **[throws `std::runtime_error`]** sampler creation failure.

### `spk::Program`

- **Standard usage:** compile/link known shaders, bind uniform/storage blocks and samplers, then issue raw/indexed/instanced draws with every supported primitive.
- Cover source replacement, generation reuse, zero draw counts, first/count boundaries, absent/optimized-out blocks, maximum binding points, and GL state verification.
- **[throws `std::runtime_error`]** Shader/program creation, compilation, linking, or named block lookup failure; preserve driver logs.
- **[throws `std::invalid_argument`/`std::out_of_range`]** Empty block names or binding points beyond GL limits.
- **[throws `std::overflow_error`]** Draw counts/first vertex exceed GL integer ranges; **[throws `std::logic_error`]** invalid program synchronization or unsupported primitive cast.

### `spk::Mesh<Vertex>` and `Builder<Mesh>`

- **Standard usage:** build a quad from vertices/triangles, reserve capacity, consume the builder, inspect layout/count/type, and render it.
- Cover empty built meshes, manual index order, shared mesh-content lifetime, and each concrete color/texture builder's attribute layout.
- **[throws `std::out_of_range`]** Add an index that references a missing vertex.
- **[throws `std::logic_error`]** Read layout from a default-empty mesh or reuse a consumed builder.
- **[throws `std::overflow_error`]** Vertex index exceeds the public index type through a deterministic seam.

### `spk::ColorMesh2D`

- **Standard usage:** build colored 2D geometry and verify positions, depth, color, indices, attributes, stride, counts, and empty state.
- Cover one vertex, one triangle, quad shape, multiple shapes, depth values, and moved/shared meshes.

### `spk::TextureMesh2D`

- **Standard usage:** build textured 2D geometry and verify position/depth/UV payload and attribute layout.
- Cover UV boundaries/outside values, one triangle, quads, multiple shapes, and shared content lifetime.

### `spk::TextureMesh3D`

- **Standard usage:** build 3D geometry and verify position/normal/UV payload and attribute layout.
- Cover zero/non-unit normals, winding, UV boundaries, several triangles, and shared content lifetime.

## Rendering

### `spk::RenderPass`

- **Standard usage:** append/emplace recording commands and execute them in insertion order.
- Cover empty passes, move construction/assignment, command lifetime, and a command that throws.
- **[throws `spk::Exception`]** Verify command failures are wrapped with render-pass context while retaining their cause.

### `spk::RenderSnapshot` and `Builder`

- **Standard usage:** request passes out of order, append commands, build, and execute passes ordered by key while preserving per-pass insertion order.
- Cover repeated requests for the same key, empty builder/snapshot, move/single-use behavior, equal orders with distinct names, and immutable execution.
- **[throws `InvalidRenderPassKeyError`]** Reuse a pass name with a different order or an order with a different name.
- **[throws `spk::Exception`]** Verify execution identifies the failing pass and preserves the nested command error.

### `spk::RenderCommand`

- Verify polymorphic destruction and execution through `const RenderCommand` ownership in a pass; recorder commands establish ordering and failure behavior.

### `spk::ClearRenderCommand`

- **Standard usage:** clear selected color/depth/stencil attachments and verify every mask and mask combination, clear color, and unaffected attachments.

### `spk::ViewportRenderCommand`

- **Standard usage:** apply normal, offset, zero-sized, and clipped viewports and verify OpenGL viewport state.

### `spk::ViewportUniformRenderCommand`

- **Standard usage:** build and bind the projection matrix for a non-empty offset viewport; verify matrix/binding and repeated execution.
- **[throws `std::invalid_argument`]** Reject any viewport with an empty dimension.

### `spk::ScissorRenderCommand`

- **Standard usage:** apply offset, empty, partial, and full scissors and verify OpenGL scissor state/pixels.

### `spk::DrawColorMeshRenderCommand`

- **Standard usage:** draw empty/single/multiple color meshes and verify color, alpha, depth, and shared program/resource reuse.

### `spk::DrawTextureMeshRenderCommand`

- **Standard usage:** draw a textured mesh and verify UV sampling, alpha/depth, sampler binding, and source lifetime.
- **[throws `std::invalid_argument`]** Reject a null texture.

### `spk::DrawFontRenderCommand`

- **Standard usage:** draw a glyph mesh with glyph/outline colors and thickness; verify uniform data, atlas sampling, and depth.
- **[throws `std::invalid_argument`]** Reject a null atlas.

### `spk::ImageRenderCommand`

- **Standard usage:** render whole/partial texture sections into normal, empty, clipped, and overlapping destinations at different depths.

### `spk::SpriteRenderCommand`

- **Standard usage:** render first/last sprites by coordinates with geometry/depth variants.
- **[throws `std::invalid_argument`]** Reject a null sheet; propagate out-of-range sprite coordinates.

### `spk::NineSliceRenderCommand`

- **Standard usage:** render a 3x3 sheet unstretched and stretched while preserving corners and filling edges/center.
- **[throws `std::invalid_argument`]** Reject null/non-3x3 sheets and corner sizes exceeding half the destination.

### `spk::TextRenderCommand`

- **Standard usage:** render UTF-8/UTF-32 text in every alignment with glyph/outline/depth variants and compare compact golden images.
- Cover empty text, missing glyphs, atlas growth, repeated commands, and font lifetime.
- **[throws `std::invalid_argument`]** Reject a null font.

### Render-command integration matrix

- **Standard usage:** execute `Clear`, viewport, viewport-uniform, scissor, image, sprite, nine-slice, text, color-mesh, texture-mesh, and font commands in a controlled framebuffer and compare state/pixels to compact golden references.
- Verify empty/clipped/offscreen geometry, repeated/overlapping draws, alpha/depth ordering, non-zero viewport origins, Unicode text/alignment/outline, texture sections, and resource lifetime after command construction.
- **[throws `std::invalid_argument`]** Null font/atlas/texture/sprite sheet; non-3x3 nine-slice; corners exceeding half the destination; empty uniform viewport.
- Verify null/empty text creates a safe no-op where specified, sprite coordinate errors propagate, and command execution does not leak OpenGL state beyond its contract.

## UI models and layouts

### `spk::DataModel<T>::View`

- **Standard usage:** bind a model/delegate, lay out visible rows, select one, scroll it into view, and observe a stable-ID selection while the model changes.
- Cover null model/delegate, empty/disabled/variable-height rows, viewport virtualization/reuse, geometry zero, scrolling boundaries, wheel/click consumption, selection deletion/movement/reset, and const/mutable selected widget access.
- **[throws `std::invalid_argument`]** Delegate returns null or a widget that is not a child of the view.
- **[throws `std::out_of_range`]** Select beyond the model or scroll without a valid model/delegate/row.

### `spk::DataModel<T>::View::Delegate`

- Use valid and deliberately invalid recorder delegates to verify creation/binding/row-extent call order, visible-row virtualization, selected flags, and ownership preconditions.

### `spk::TextModel::Delegate`

- **Standard usage:** display string rows with the default delegate, customize font/size/colors/padding/height, select a row, and verify label binding.
- Cover empty strings, selected/unselected rebinds, model replacement, and style changes after items exist.
- **[throws `std::invalid_argument`]** Set a null font or bind an item that is not a `TextLabel`.

### `spk::TextModel::View`

- Verify construction installs the default delegate, typed model assignment, delegate customization, selection, virtualization, and inherited exception behavior.

### `spk::Layout` and `Layout::Element`

- **Standard usage:** add widget/layout elements, set size policies/alignment/padding, compute hints, resolve geometry, and react to child hint changes.
- Cover fixed/minimum/extend combinations, insufficient/excess/exact space, zero elements, contradictory/infinite hints, rounding distribution, nested layouts, clear/remove/foreign element, and every alignment.
- **[throws `std::invalid_argument`]** Add a null widget/layout or make a layout contain itself.

### `spk::LinearLayout<Orientation>`

- **Standard usage:** lay out mixed-policy widgets horizontally and vertically with padding and alignment.
- Cover empty/single/many elements, remove by element/widget/layout (present and absent), nested layouts, dynamic size-hint edits, constrained space, and pixel rounding without gaps/overlap.

### `spk::GridLayout`

- **Standard usage:** grow a grid, place widgets/layouts, clear cells, remove rows/columns, and verify hints/geometries/padding.
- Cover sparse grids, overwrite behavior, zero rows/columns, first/last/middle removal, child hint changes, and all fixed-column/fixed-row/fixed-grid clear/growth rules.
- **[throws `std::out_of_range`]** Remove invalid rows/columns, clear invalid cells, or address outside a fixed dimension.

### `spk::GridLayoutFixedColumns<N>`

- Verify fixed column count, automatic row growth, valid/invalid column placement, clear restoring zero rows, and deleted column-resize operations at compile time.

### `spk::GridLayoutFixedRows<N>`

- Verify fixed row count, automatic column growth, valid/invalid row placement, clear restoring zero columns, and deleted row-resize operations at compile time.

### `spk::GridLayoutFixedGrid<Columns, Rows>`

- Verify immutable dimensions, in-place clear, every boundary cell, invalid cells, and all grid-resize operations deleted at compile time.

### `spk::FormLayout`

- **Standard usage:** add label/field rows with independent policies, inspect returned elements, lay out, and remove rows.
- Cover null label/field independently, duplicate widget use as currently permitted/forbidden, empty form, and removing a stale/foreign row.
- **[throws `std::invalid_argument`]** Add any null widget.

## UI widget foundation

### `spk::Widget`

- **Standard usage:** build a named hierarchy, activate it, apply style, set geometry/z-order/render pass, update, dispatch events, build a snapshot, and inspect root/view region.
- Cover construction with null/real parent, reparent/removal/destruction, comparator ordering by absolute z, relative/absolute geometry ratios, resize vs set-geometry behavior, clipping/scissor intersection, focus notifications, inactive branches, and child size-hint propagation.
- Verify cached view regions/z-orders invalidate recursively after geometry, hierarchy, or z changes; render-pass inheritance/override follows reparenting.
- Verify default style resources load and move safely; all style-dependent widgets respond to edited values/resources.
- **[throws `spk::Exception`]** Verify update/render hook failures gain widget name/context and retain their original cause.

### `spk::Widget::Style`

- **Standard usage:** construct the default resources, move the style, edit every scalar/resource field, and apply it across representative widget types.
- Verify moved-from destruction, deleted copying, embedded font/image/sheets validity, and reactive visual/size changes.

### `spk::Screen`

- **Standard usage:** activate screens in sequence and verify exactly one global active screen, automatic deactivation, and cleanup on destruction.
- Cover no active screen, repeated activation, inactive destruction, nested/independent screens, and activation callbacks.

### `spk::ScalableWidget`

- **Standard usage:** hover and drag each edge/corner, enforce size hints, and inspect active/hovered edges and final geometry.
- Cover no edge, grab offset zero/large, minimum/maximum constraints, parent bounds policy, focus/mouse loss cancelling resize, double-click path, and tiny/zero geometry.

### `spk::SpacerWidget`

- **Standard usage:** use as fixed/minimum/extend space in both linear layouts and verify it contributes only its size hint and geometry.

### `spk::ContainerWidget`

- **Standard usage:** assign a child as content, set anchor/size, resize the container, and verify content geometry.
- Cover null content (clear), content reparented away, replacement, zero/oversized content, negative anchors, and destruction order.
- **[throws `std::invalid_argument`]** Set non-null content that is not already a child.

## UI display and input widgets

### `spk::Panel`

- **Standard usage:** apply a 3x3 sprite sheet, set corners/depth/geometry, and verify size hint plus nine-slice rendering.
- Cover explicit vs style-derived corner size, zero corners, geometry smaller than corners, style reapplication, and accessors.
- **[throws `std::invalid_argument`]** Null/wrong-grid sprite sheet or negative corner component.

### `spk::ImageLabel`

- **Standard usage:** display a whole texture then a section at a depth and verify render geometry.
- Cover style/no texture, empty/edge/out-of-range sections according to texture-section contract, texture lifetime, and geometry changes.
- **[throws `std::invalid_argument`]** Set a null texture.

### `spk::TextLabel`

- **Standard usage:** set font/text/size/colors/alignment/padding/depth and verify measured hint, anchored geometry, and rendered glyphs.
- Cover empty/Unicode/missing glyph text, every alignment, outline/no outline, zero/large padding, style changes, and font atlas growth after snapshot construction.
- **[throws `std::invalid_argument`]** Set a null font.

### `spk::TextArea`

- **Standard usage:** render wrapped multiline text with configured width, line padding, alignment, colors, and depth.
- Cover empty text, explicit newlines, long words, Unicode, zero/narrow width, minimal width changes, vertical/horizontal alignments, and size-hint updates.
- **[throws `std::invalid_argument`]** Set a null font.

### `spk::DynamicTextLabel`

- **Standard usage:** install a producer, refresh immediately, then update at the configured interval.
- Cover empty/null producer, zero/negative/very large duration according to current accepted contract, multiple elapsed intervals, producer replacement, and producer exceptions with widget context.

### `spk::AnimationLabel`

- **Standard usage:** set a sheet/range/loop speed, advance frames over updates, wrap, and render the selected sprite.
- Cover single/full/sub ranges, exact/multiple/partial time steps, zero/negative duration policy, sheet replacement and range reset, depth, and inactive updates.
- **[throws `std::invalid_argument`]** Null sheet or range end before start; out-of-sheet ranges must either reject immediately or have an explicitly documented clamp policy.

### `spk::PushButton`

- **Standard usage:** style text/icon, hover, press/release inside, receive one click, and observe released/pressed visuals and accessors.
- Cover press/release outside, mouse leave, double-click, non-left buttons, deactivation mid-press, flat mode, text-only/icon-only/both/empty, all alignments, explicit/reset padding and icon size, and focus/consumption.
- **[throws `std::invalid_argument`]** Set an icon from a null sprite sheet; verify invalid sprite IDs/coordinates propagate.

### `spk::IconButton`

- **Standard usage:** set an icon sheet/ID, change by ID/coordinates, click, and verify inherited button behavior.
- Cover sheet replacement, style-provided sheet, first/last sprite, and ID set before sheet.
- **[throws `std::invalid_argument`]** Set a null iconset; **[throws `std::logic_error`]** Resolve coordinates without an iconset; propagate sprite-range errors.

### `spk::CheckableIconButton`

- **Standard usage:** configure two sprites, click/toggle, observe state once, and verify only the matching child button is active.
- Cover programmatic same-state no-op, repeated toggles, style/sheet replacement, invalid sprite IDs, geometry/hints, and deactivation.

### `spk::CheckBox`

- **Standard usage:** configure label/indicator, click label or indicator, toggle, and observe state once.
- Cover programmatic changes, press/release outside, spacing/indicator size/style/font changes, Unicode/empty label, and disabled/inactive behavior.

### `spk::RadioButton`

- **Standard usage:** configure label/indicator, attach through a group, click/select, and observe local state while preserving group exclusivity.
- Cover detached button behavior, programmatic checking, styling, geometry, spacing, and indicator sizing.

### `spk::RadioButtonGroup`

- **Standard usage:** insert a grid of buttons, select by object/cell/click, move selection, observe old/new states and one group selection event.
- Cover mandatory vs optional selection, first insertion, selected erasure, clear, sparse cells, row/column counts, insertion order access, style/font/icon/sprite/spacing changes, and destruction detaching group pointers.
- **[throws `std::invalid_argument`]** Insert into an occupied cell, select/erase a foreign button, select an empty cell, or set null font/iconset.
- **[throws `std::out_of_range`]** `buttonAt` beyond insertion order; verify nullable `button` remains no-throw for empty cells.

### `spk::ToggleSwitch`

- **Standard usage:** click to toggle, animate between states, observe one state event, and render configured backgrounds/outline/thumb.
- Cover both orientations, zero/exact/multi-step animation, programmatic state during animation, padding/thumb sizes, press/release outside, focus loss, raw sections vs nine-slice backgrounds, and style reapplication.
- **[throws `std::invalid_argument`]** Negative duration, null checked/unchecked textures, or non-3x3 checked/unchecked nine-slice sheets.

### `spk::TextEdit`

- **Standard usage:** focus by click, type/edit/select/copy/cut/paste text, validate changes, and render text/placeholder/caret/selection.
- Cover cursor navigation (home/end/arrows), backspace/delete, shift selection both directions, mouse drag/double-click, replacement, select-all, UTF-32 glyphs, horizontal scrolling, blink timing, focus/mouse loss, read-only mode, obscured copy policy, clipboard failure, and event consumption.
- Verify validation states: valid commits and notifies once, undefined remains editable without commit where designed, invalid replacements are rejected atomically; callback changes/exceptions leave coherent text/selection.
- Cover selection normalization/clamping, empty text, placeholder, style/geometry/font atlas changes, and subscriber resignation.
- **[throws `std::invalid_argument`]** Null or non-3x3 sheet, negative corner component, or null font.

### `spk::SpinBox<T>`

- **Standard usage:** set bounds/value/step, click increase/decrease, clamp, observe changes, and inspect child controls for signed, unsigned, and floating types.
- Cover equal/inverted limits as currently accepted, removing limits, zero/negative/fractional step, exact bounds, same-value no-op, arithmetic boundary/overflow policy, icon/text modes, sprite changes, geometry too narrow, and style updates.

### `spk::NumericSpinBox<T>`

- **Standard usage:** type a valid number, receive a typed edition, then use raise/lower buttons and verify text synchronization for integer/unsigned/floating aliases.
- Cover signs, decimal/exponent forms supported by `from_chars`, empty/sign/dot undefined states, trailing junk, whitespace, non-ASCII, overflow/underflow, unsigned negative input, same parsed value, programmatic synchronization recursion, steps and arithmetic boundaries.
- Verify invalid/undefined input does not change the value or notify; icon/style/geometry child behavior matches `SpinBox`.

### `spk::SliderBar`

- **Standard usage:** configure orientation/scale/range/value, drag to a new ratio, and observe clamped value and one edition per effective change.
- Cover horizontal/vertical, scale 1 and near zero, ratio 0/1/outside clamp, negative/cross-zero ranges, exact endpoints, tiny/zero geometry, drag outside, focus loss, programmatic update during drag, and same-value no-op.
- **[throws `std::invalid_argument`]** Scale outside `(0,1]`, non-finite/non-ascending range, non-finite ratio, or non-finite value.

### `spk::ScrollBar`

- **Standard usage:** configure orientation/range/scale/step/value, click arrows and drag slider, and observe forwarding of editions.
- Cover endpoint clamping, orientation/icon updates, custom sprite IDs, tiny geometry, step 1/near zero, scale interactions, and same-value no-op.
- **[throws `std::invalid_argument`]** Step outside `(0,1]` or null iconset; propagate slider range/value/scale and sprite errors.

### `spk::IScrollArea`

- **Standard usage:** install oversized child content, show the required bars, scroll both axes/wheel, resize the viewport, and access typed content.
- Cover content smaller/equal/larger on each axis, one bar causing the other, zero geometry/content, dynamic content hint changes, content replacement/reparenting, scroll preservation/clamping, custom bar width/iconset, and hidden-bar state.
- Propagate the container's **[throws `std::invalid_argument`]** case when content is not already parented to its container.

### `spk::ScrollArea<Content>`

- Verify typed content construction/parenting/access for several widget content classes and inherited scrolling behavior with/without an iconset.

### `spk::ProgressBar`

- **Standard usage:** set ratios in every fill direction, observe effective changes, and render the corresponding fill geometry.
- Cover ratio below/above bounds, 0/1/NaN/infinity policy, zero/tiny geometry, texture/corners/depth/style changes, and same-value no-op.

### `spk::LabeledProgressBar`

- **Standard usage:** format the underlying progress ratio through a predicate, update automatically, and expose the configured label/progress properties.
- Cover null/throwing predicates, manual refresh, predicate replacement, style/geometry, and forwarding of edition subscriptions.

## UI composition widgets

### `spk::CommandPanel`

- **Standard usage:** add named buttons, subscribe/click, inspect by name, remove, and lay them out with spacer, policy, and padding.
- Cover empty/single/many order, remove then re-add, style/geometry changes, and const/mutable lookup.
- **[throws `std::invalid_argument`]** Add a duplicate name.
- **[throws `std::out_of_range`]** Lookup, remove, or subscribe to a missing name; verify the name appears in the message.

### `spk::PromptPanel`

- **Standard usage:** set a message, add actions, configure policies/padding, lay out, and access all child components.
- Cover empty message/actions, dynamic text size, add/remove/re-add buttons, and background/style geometry.
- Propagate all `CommandPanel` duplicate/missing-name exceptions.

### `spk::PopupWidget`

- **Standard usage:** set child content and anchor, open with placement/offset, constrain to root, close from outside click/Escape, and observe close once.
- Cover every alignment, root edges, oversized popup, `openAt`, repeated open/close, feature flags, anchor/content reparent/destruction, inside click, inactive popup, and coordinator cleanup.
- **[throws `std::logic_error`]** Call `open()` without an anchor.
- **[throws `std::invalid_argument`]** Set content that is not already a child.

### `spk::ComboBox`

- **Standard usage:** bind a text model, open the popup, select an enabled row, update the display, close, and observe selection.
- Cover placeholder/no selection, model/delegate replacement, row insert/remove/change/reset with stable IDs, disabled rows, maximum visible rows 0/1/many, open-button width, empty model, outside/Escape close, and custom display delegates.
- **[throws `std::invalid_argument`]** Delegate creates a null display item or one not parented to the combo box.
- Propagate model-view selection/scroll/delegate exceptions and keep popup/display state coherent after failure.

### `spk::TabWidget`

- **Standard usage:** add pages, select by tab click/index, observe selection, remove pages, and verify active content and tab geometry.
- Cover first automatic selection, removing before/at/after selection, duplicate page, empty widget, tab sizes/padding, hidden/inactive content, and same-selection no-op.
- **[throws `std::invalid_argument`]** Add null, non-child, or duplicate content.
- **[throws `std::out_of_range`]** Select an invalid page index.

### `spk::DebugOverlay`

- **Standard usage:** place existing child widgets in a sparse grid, set padding, resize, clear cells/all, and follow child reparenting.
- Cover overwrite/clear, destruction order, same cell updates, dynamic size hints, and repeated clear.
- **[throws `std::invalid_argument`]** Null widget, non-child widget, or the same widget in multiple cells.

### `spk::MenuBar`

- **Standard usage:** add menus/items/separators, click to open one menu, invoke an item action, close menus, and verify layout/style.
- Cover empty/single/many menus, menu switching, outside selection, clear/rebuild, spacing/inset/height, action lifetime, separator height/depth/style, and 3x1 rendering.
- **[throws `std::invalid_argument`]** Duplicate menu name, null separator sheet, or non-3x1 separator sheet.
- **[throws `std::out_of_range`]** Lookup/remove missing menu/button entries; verify the requested name.

### `spk::MenuBar::Menu`

- Verify item/separator insertion order, item actions, clear/reuse, spacing, background geometry, outside-click closing, and style propagation.

### `spk::MenuBar::Menu::Item`

- Verify caption/click callback, automatic owner-menu closing, active state, and callback lifetime/resignation.

### `spk::MenuBar::Menu::Break`

- Verify 3x1 sheet rendering, height/depth, size hints, style changes, and the documented null/wrong-grid exceptions.

### `spk::IInterfaceWindow`

- **Standard usage:** construct typed content, set title/padding/minimum size, move, resize, minimize/maximize/restore/close, and observe resize/close contracts.
- Cover every menu-button enable state, maximize-to-root and root resize, restored geometry, title-bar double click, drag cancellation/focus loss, zero/large padding/menu height, content hint changes, style/icon IDs, and repeated state commands.
- **[throws `std::logic_error`]** Use an invalid menu-button enum value.
- **[throws `std::invalid_argument`]** Set content that is not a child of the normal background.

### `spk::IInterfaceWindow::MenuBar`

- Verify title layout, minimize/maximize/close buttons, enable states, margin, iconset, callbacks, and invalid button-enum handling.

### `spk::InterfaceWindow<Content>`

- Verify typed content construction, automatic valid parenting, const/mutable access, and all inherited move/resize/minimize/maximize behavior.

### `spk::MessageBox`

- **Standard usage:** set title/message/buttons, show and size from content, invoke information close and request first/second actions, and handle title close.
- Cover minimum width, empty/long text, dynamic button edits, resize updates, action replacement/empty actions, second-action-on-title-close policy, repeated close, and callback ordering/lifetime.
- Propagate `CommandPanel` duplicate/missing button errors.

### `spk::InformationMessageBox`

- Verify the default close button, its caption/action, title-close behavior, size updates, and repeated closure.

### `spk::RequestMessageBox`

- Verify first/second button configuration and actions, title-close default/override/reset policy, action replacement, empty actions, and callback ordering.

### `spk::Tooltip`

- **Standard usage:** attach to a target, hover for the open delay, place automatically, move with cursor, leave for close delay, and hide on click/focus loss.
- Cover every explicit placement, root edges, oversized text/maximum width, zero delays, target reparent/destruction/replacement, manual show/hide, repeated events, parent clipping/render pass, and no target.
- **[throws `std::invalid_argument`]** Negative open or close delay.

### `spk::EngineWidget`

- **Standard usage:** attach an engine, propagate geometry/update/render and every active/passive event, then detach safely.
- Cover null engine, engine replacement, inactive widget/engine entities, exception propagation, render-pass ordering, and destruction order.

### `spk::Workspace<Content>`

- **Standard usage:** access typed content/menu bar, add menus, resize, and verify menu-over-content geometry, z-order, and size hints.
- Cover zero-height/small workspace, menu height beyond geometry, content hint changes, empty menu bar, and style propagation through composed widgets.

## Cross-cutting suites and implementation order

1. Add `gtest` to `vcpkg.json`; create `tests/TU/CMakeLists.txt`, a GoogleTest executable, discovery, labels, resource paths, and deterministic test utilities.
2. Implement foundation, value types, containers, traits, JSON, and engine registries first; these require no native window or GPU and give fast feedback.
3. Add event/layout/widget state tests using recorder subclasses and synthetic events.
4. Add one shared hidden Win32/OpenGL fixture, then GPU resources, render commands, and pixel-golden helpers.
5. Add application/runtime integration tests last, with timeouts and guaranteed cleanup.
6. Run Debug and Release suites through CTest; add sanitizer/coverage configurations where the toolchain supports them and label `unit`, `thread`, `win32`, `opengl`, `visual`, `integration`, and `slow` tests separately.

Before implementing a section, reconcile any case marked "according to contract" or "once stabilized." Those phrases identify behavior the current API accepts or leaves implicit (for example non-finite geometry and arithmetic overflow); the test must follow the chosen documented contract rather than accidentally freezing undefined behavior.
