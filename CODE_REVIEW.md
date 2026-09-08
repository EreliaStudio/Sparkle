# Sparkle whole-codebase review

Reviewed: 2026-09-08  
Companion checklist: [`CODE_REVIEW_ORDER.md`](CODE_REVIEW_ORDER.md)

## Executive assessment

Sparkle has a much stronger behavioral test suite than most projects at this stage: the existing test binary exposes 1,036 tests, including OpenGL integration and golden-image coverage, and all 1,036 passed. The code also shows deliberate work on error context, mutation-safe observer dispatch, stable model row IDs, GPU-resource recycling, and separation of platform/update/render runtimes.

It is not yet professionally releasable as the repository currently stands. A fresh Debug or Release build fails because a required header is absent. More importantly, the three-thread design publishes render commands that borrow mutable update-thread resources through raw pointers. This defeats the otherwise clean snapshot boundary and can produce use-after-free and data races. The next architectural pass should concentrate on explicit ownership, immutable render data, transaction-safe lifecycle changes, and a narrower platform boundary.

This report is a whole-repository architectural and static review, following the subsystem sequence in `CODE_REVIEW_ORDER.md`, then covering CMake, packaging, tests, resources, and tools. It intentionally reports consequential issues rather than every local formatting preference.

## Severity model

- **Blocker**: prevents a reproducible build or permits undefined behavior in ordinary supported use.
- **High**: can hang, terminate, leak native resources, or corrupt traversal/lifecycle state.
- **Medium**: creates fragile APIs, portability failures, hidden coupling, or substantial maintenance cost.
- **Low**: consistency and repository-quality work that should be resolved before a stable public release.

## Findings

### Blockers

#### B1. The checked-out repository cannot be built

`src/core/window_surface.cpp:15`, `tests/TU/TestSuite/srcs/window_surface_test.cpp:11`, and `tests/TU/TestSuite/srcs/application_test.cpp:17` include `core/platform/detail/window_surface_driver.hpp`, but that file does not exist in the repository or generated include tree. Both commands below fail at compilation for the same reason:

```powershell
cmake --workflow --preset testDebug
cmake --build --preset testRelease
```

Restore and track the driver abstraction (and add it to the review order), or remove the injection seam and its tests. Add a clean-clone configure/build/test job so a locally retained generated or ignored file cannot conceal this again.

#### B2. Render snapshots are not ownership-complete or immutable

The update thread builds snapshots and the render thread retains the last one (`src/core/application_update_runtime.cpp:332-358`, `src/core/application_render_runtime.cpp:148-164`). However, commands keep borrowed resource pointers: `DrawTextureMeshRenderCommand::_texture` (`include/rendering/command/draw_texture_mesh_render_command.hpp:19`), `DrawFontRenderCommand::_atlas` (`include/rendering/command/draw_font_render_command.hpp:35`), and `Sampler::_texture` (`include/graphics/opengl/sampler.hpp:44`). Widgets likewise commonly borrow externally owned fonts, textures, and sprite sheets.

Consequences:

- Destroying/replacing a widget resource after publishing a snapshot can leave the render thread dereferencing a dead object.
- `TextRenderCommand` keeps an atlas pointer and shared mutable `State`; atlas callbacks set `dirty`, while `execute()` reads it (`src/rendering/command/text_render_command.cpp:7-15`, `84-100`). Those operations can occur on different threads without synchronization.
- Snapshot construction calls `Font::Atlas::loadGlyphs()` (`src/rendering/command/text_render_command.cpp:27-30`), mutating the same atlas whose GPU state may be consumed by the render thread.
- `GPUResource::LifeTime` only queues reclamation after destruction; it does not keep the pointed-to CPU resource alive (`src/graphics/opengl/gpu_resource.cpp:15-51`). It therefore cannot make a raw pointer in a snapshot safe.

Make a render snapshot a genuinely self-contained immutable value. A practical design is a stable shared `RenderResource` state/handle whose CPU payload has immutable generations; commands own handles, not object pointers. Atlas growth should create/publish a new generation or be serialized onto the render thread. No field reachable from a published snapshot should be mutated by the update thread.

#### B3. The public vector/rectangle representation relies on non-standard and unsafe aliasing

`TVector2`, `TVector3`, `TVector4`, and `Rect2D` overlay named fields with `std::array` through anonymous structs/unions (`include/math/vector2.hpp:15-24`, `vector3.hpp:17-24`, `vector4.hpp:14-21`, `rect2d.hpp:14-32`). The anonymous structs are compiler extensions; `clang++ -std=c++23 -pedantic-errors` rejects the public headers even though the target declares `CXX_EXTENSIONS OFF`. Tests explicitly rely on reading the inactive union member (`*Vector*Test.DataArrayAliasesNamedComponents`), which is not a portable C++ object model.

Choose one standard representation. Prefer `std::array<T, N>` plus `x()/y()/z()/w()` accessors, or named fields plus `operator[]` implemented with a switch. Do not expose union type-punning as a supported API.

### High severity

#### H1. `quit()` can leave `run()` asleep forever

`Application::Impl::quit()` only stores an exit code (`src/core/application_impl.cpp:235-238`). The platform loop may be blocked indefinitely in `MessageQueue::waitForActivity()` (`application_impl.cpp:171-186`). Unlike `PlatformRequestProducer::publish`, `quit()` does not notify `_platformWakeEvent`. A quiet application quit from another thread can therefore hang until an unrelated window message arrives.

Store the exit code and notify the wake event as one operation. Add a deterministic test that first proves the platform loop is idle, then calls `quit()` from another thread under a bounded timeout.

#### H2. The stop callback invokes a throwing function across a non-throwing boundary

The stop callback at `src/core/application_impl.cpp:174-176` calls `WakeEvent::notify()`, which throws `std::system_error` when `SetEvent` fails (`include/core/platform/wake_event.hpp:51-56`). Standard stop-callback invocation is non-throwing; an exception escaping it terminates the process. Clang-Tidy also identifies this path.

Provide a `notifyNoThrow()`/error-code form for shutdown, or catch and record the failure inside the callback. Reserve the throwing overload for ordinary request publication.

#### H3. Widget traversal is invalidated by legal callbacks

Engine traversal deliberately snapshots collections before callbacks (`src/engine/entity.cpp:45-57`, `94-109`, `143-181`). Widget traversal does not: event propagation uses live reverse iterators (`src/ui/widget.cpp:133-142`), and geometry invalidation, resize, update, and snapshot construction use live range loops over `children()` (`widget.cpp:164-213`, `317-363`). Widget callbacks can reparent or destroy widgets, invalidating the underlying `std::vector` iterators and causing undefined behavior.

Adopt one mutation policy across `EventDispatcher`, `Entity`, and `Widget`: snapshot `(pointer, generation)` entries and verify membership before each call, or queue hierarchy mutations until traversal ends. Add tests where the current child removes itself, removes a sibling, and inserts a child during event/update/render traversal.

#### H4. Registry callbacks can observe partially constructed or destroyed objects

`Registry::Object` registers from its base constructor and unregisters from its base destructor (`include/engine/registry.hpp:23-54`). Addition callbacks can receive a `TType*` before the most-derived constructor has completed; removal callbacks can receive it after the derived portion has already been destroyed. Calling derived behavior from either callback is unsafe.

Move registration to an explicit lifecycle owned by `Engine`/entity collections, after construction and before destruction starts. A token returned by `registerObject()` can make the lifetime explicit without CRTP constructor side effects.

#### H5. Layout elements retain untracked borrowed objects

`Layout::Element` stores raw `Widget*`, `Layout*`, and `ResizeableTrait*` (`include/ui/layout/layout.hpp:46-60`). Its size subscription expires when the target dies, but the pointers are not cleared. Later `sizeHint()` or `setGeometry()` dereferences them (`src/ui/layout/layout.cpp:201-205`, `269-284`). A layout can therefore outlive an externally owned element and use freed memory. Nested layouts can also form indirect cycles because only `layout == this` is rejected (`layout.cpp:296-306`).

For widgets, subscribe to destruction and erase/clear the element. For layouts, add equivalent lifetime observation or require ownership. Validate the full layout ancestry before insertion, not only direct self-insertion.

#### H6. Public native resources are not RAII types

`WinAPI::Window::~Window()` is defaulted (`include/core/platform/window.hpp:75-79`), `Window::Native::~Native()` is defaulted (`src/core/window_native.cpp:21-25`), and `Window::Surface::~Surface()` is defaulted (`src/core/window_surface.cpp:209-213`). Destruction does not guarantee `DestroyWindow`, OpenGL context deletion, `ReleaseDC`, or GPU collection cleanup. The application runtime normally performs explicit release, but these types and constructors are public and exceptions can bypass the happy path.

Make destructors perform idempotent, non-throwing cleanup. Keep explicit `destroy()` for checked error reporting, then use best-effort cleanup in destructors. If these are not valid public standalone types, hide construction behind `Application` and document thread affinity.

#### H7. Observer-driven destructors can terminate

`Widget::~Widget()` triggers arbitrary subscriber code and reparents children (`src/ui/widget.cpp:104-112`). `InherenceTrait::~InherenceTrait()` also triggers child parent-edition callbacks (`include/design_pattern/trait/inherence_trait.hpp:86-105`). Destructors are implicitly `noexcept`; a callback exception terminates. Clang-Tidy reports the `Widget` path.

Do not execute fallible user callbacks from destructors. Introduce an explicit `close()`/`detach()` phase that can report failure, while destructor cleanup only invalidates tokens and clears links without callbacks. At minimum, define and enforce a no-throw callback policy at subscription boundaries.

### Medium severity

#### M1. Context changes publish before committing state

`ContextualizableTrait::changeContext()` triggers observers before assigning `_context` (`include/engine/contextualizable_trait.hpp:35-44`); the test suite even codifies this ordering. Registry observers remove/add during that notification, so an addition callback for the new registry can query the object and still see its old context. A later callback exception also leaves some observer side effects committed while the object's context remains unchanged.

Treat a context change as a transaction: validate first, store the new value, then emit an immutable `{old, current}` event. If callbacks may throw, specify whether notification is best-effort, aggregated, or fail-fast; do not imply rollback when rollback is not implemented.

#### M2. Base construction invokes an overridable geometry hook

When an `Entity` is constructed with a parent, its constructor calls `handleGeometryChange()`, which invokes `_onGeometryChange()` (`src/engine/entity.cpp:11-22`, `73-89`). Virtual dispatch during base construction reaches only `Entity`, so a derived entity can silently miss its initial geometry notification. Clang-Tidy flags this call.

Remove virtual work from constructors. Attach and initialize newly created entities through an explicit post-construction path/factory, or make geometry initialization non-virtual and invoke the derived hook only after construction.

#### M3. Rectangle arithmetic is not closed over its public value range

`Rect2D` combines signed anchors with unsigned sizes, then converts sizes to `int32_t` and adds them (`src/math/rect2d.cpp:15-52`). Values above `INT32_MAX`, or a large positive anchor plus width, can produce implementation-defined conversion or signed overflow. `shrink()` accepts a signed offset but has unclear behavior for negative values.

Use checked `int64_t` intermediates and define whether coordinates saturate, throw, or use a wider public coordinate type. Reject negative shrink margins or rename/document the operation as inset/outset.

#### M4. The static registry is global, unbounded, and not thread-safe

Every template specialization owns a process-global `_entries` map (`include/engine/registry.hpp:71`). Empty contexts are retained after their last element/subscription disappears, query objects retain per-context caches/subscriptions, and no operation is synchronized. Pointer contexts also make address reuse a meaningful identity hazard.

Make registries instance members of their owning `Engine`. This naturally defines lifetime, thread affinity, cleanup, and testing boundaries, and avoids hidden global state between applications/tests.

#### M5. The runtime loops have no pacing or blocking strategy

Update and render workers continuously call `executeOnce()` (`src/core/application_impl.cpp:55-67`). The renderer spins even when no new snapshot exists; the updater also spins at unrestricted speed. This consumes a core per worker and makes timing/load behavior machine-dependent.

Give each runtime an explicit policy: event/condition-variable blocking for render, and fixed-step, variable-step with a cap, or user-selected pacing for update. Wake the renderer when a snapshot or lifecycle request arrives.

#### M6. `Widget::Style` is a global god object

`Widget::Style` owns every widget's scalar theme values and thirteen heavyweight resources in one move-only aggregate (`include/ui/widget.hpp:39-99`), exposed through public mutable global `Widget::defaultStyle`. This couples basic `Widget` to fonts/images/sprite sheets and makes adding a widget modify a central type. Lazy global mutation is also not thread-safe.

Split styles by component (`TextStyle`, `ButtonStyle`, `WindowStyle`, etc.) and aggregate them in a `Theme`. Pass/shared-inject a `const Theme` or theme service at the UI root. Store resources in shared immutable handles so applying a theme does not create raw lifetime dependencies.

#### M7. CMake does not clearly model supported platforms or host tools

The build compiles Win32-only sources unconditionally, while only the final `user32` link is guarded by `WIN32` (`CMakeLists.txt:82-84`). Either declare Windows as the supported platform at configure time or split a platform target/backend. `sparkleResourceBuilder` is always built as a target subdirectory (`CMakeLists.txt:24`); when cross-compiling, that creates a target executable which cannot necessarily run on the host.

Create target-oriented components such as `sparkle-core`, `sparkle-render-opengl`, and `sparkle-platform-win32`, or at least isolate platform source selection. Model the resource compiler as a host tool (or support a pre-generated resource source) for cross builds.

#### M8. Recursive source globbing hides the library composition

Production, headers, resources, and test sources are all discovered with `GLOB_RECURSE` (`CMakeLists.txt:27-48`, `tests/TU/TestSuite/CMakeLists.txt:6-13`). `CONFIGURE_DEPENDS` reduces stale regeneration but still makes target boundaries implicit and can silently compile scratch `.cpp` files.

List sources explicitly per component/target. That makes the CMake decomposition itself document the intended architecture and ensures additions require a conscious ownership decision.

#### M9. Consumer-wide preprocessor state leaks from the library target

`UNICODE`, `_UNICODE`, and `NOMINMAX` are `PUBLIC` compile definitions (`CMakeLists.txt:70-74`). They alter every downstream translation unit. This currently compensates for exposing `Windows.h` in public headers; compiling `sparkle.hpp` directly without `NOMINMAX` demonstrates `min`/`max` macro collisions in unrelated public templates.

Keep Win32 headers behind PImpl or a narrowly named native interop header, use explicit `W` APIs, and make these definitions private. Public headers should be robust under either include order and should not require consumers to inherit global Windows policy.

#### M10. Error semantics are fragmented

The code alternates among `spk::Exception`, `std::runtime_error`, `logic_error`, `invalid_argument`, `out_of_range`, `system_error`, `optional`, and `bool`. Runtime boundaries enrich arbitrary exceptions, which is good, but lower layers provide no consistent rule for programmer errors versus operational errors. Some shutdown errors are swallowed completely (`src/core/application_impl.cpp:190-199`, `src/graphics/opengl/gpu_resource.cpp:36-44`).

Define an error policy by layer. A reasonable split is standard exceptions for argument/programmer errors, `std::system_error` for OS/GL-adjacent operational failures, and `spk::Exception` only as a boundary wrapper with nested cause/context. Destructors should be no-throw and report cleanup failures through diagnostics.

#### M11. Large units obscure responsibilities

Notable examples are `json_object.cpp` (1,152 lines), `text_edit.cpp` (1,069), `interface_window.cpp` (650), `registry_query.hpp` (625), and `radio_button.cpp` (530). Size alone is not a defect, but several mix state machines, layout, rendering, input, conversion, and public API behavior.

Split by responsibility, not arbitrary line count: JSON parser/writer/value; text-edit model/selection/layout/interaction/rendering; registry operations in separate headers; composite widget subcomponents. This will make the review order and unit ownership much clearer.

### Low severity and release hygiene

#### L1. Public naming contains permanent-looking spelling errors

`StatefullTrait`, `ResizeableTrait`, `InherenceTrait`, `LifeTime`, and `NbKind` are public vocabulary. Prefer `StatefulTrait`, `ResizableTrait`, a domain name such as `HierarchyNode`, `Lifetime`, and `KindCount`. Make these changes before API stability; afterward aliases and deprecation would be required.

Parameter naming also alternates between `p_value`, plain `value`, and single-letter names, and implementation placement is inconsistent (`src/graphics/mesh/*` implements `include/geometry/*`; `json_object.cpp` implements `object.hpp`). Pick one convention and align physical module names.

#### L2. The public umbrella has an unexplained duplicate

Both `include/sparkle.hpp` and extensionless `include/sparkle` are installed; the latter only includes the former and is omitted from `CODE_REVIEW_ORDER.md`. Keep one documented umbrella (normally `sparkle/sparkle.hpp` for an installable library), or explicitly document and test both spellings.

#### L3. The format tool can rewrite generated/dependency trees

`tools/format.ps1` recursively formats every matching file below an arbitrary mandatory folder. Running it on the repository root includes `build/`, `install/`, and `vcpkg_installed/` when present. It also does not verify `clang-format`, stop on formatter failure, or default to tracked project files.

Drive formatting from `git ls-files` or explicit source roots, exclude generated/vendor directories, enable strict error handling, and add a non-mutating CI check.

#### L4. Repository-level professional metadata and automation are missing

There is no root README, license, CI configuration, `.editorconfig`, warning policy, static-analysis preset, sanitizer configuration, or install/consumer smoke test. `.gitignore` contains only four entries. Add these before presenting Sparkle as a reusable library. In particular, CI should build Debug and Release from a clean clone, run tests, install, and compile a tiny `find_package(sparkle CONFIG REQUIRED)` consumer.

#### L5. CTest process granularity is expensive

`gtest_discover_tests()` creates 1,036 individual CTest processes; the existing Debug suite took 139.6 seconds despite most tests individually taking about 0.08-0.10 seconds. Keep per-test discovery where diagnosis/sharding benefits from it, but consider grouped suites or fixtures for local and CI fast paths, especially for tests sharing an OpenGL context.

## Review-order notes

These notes follow the eleven sections of `CODE_REVIEW_ORDER.md` and capture the principal composition decision for each area.

1. **Language, errors, and values:** keep the value layer dependency-light and standard-layout; remove union aliases, harden `Rect2D` arithmetic, and settle the error taxonomy. JSON is comparatively cohesive, but parser and writer deserve separate implementation units.
2. **Containers and models:** `ProtectedData`, FIFO, and slot have understandable RAII endpoints. Document thread-safety on every container. `CachedData` should require a non-throwing destructor callback or catch/report failures. Model/view raw dependencies need lifetime tracking.
3. **Patterns and traits:** `ContractProvider` handles same-thread reentrancy thoughtfully, but is not thread-safe and should say so. Hierarchy, context, registry, activation, and naming are currently composed through many inheritance bases; explicit owned collaborators will scale better than additional traits.
4. **Input, events, and contexts:** generated overloads or a tagged dispatcher could replace the repeated event boilerplate. Validate channel values at the public boundary. State whether event mutation and hierarchy mutation during dispatch are supported.
5. **Geometry and graphics:** CPU-side resources need stable shared ownership across snapshots. Preserve the per-surface GPU instance cache, but separate CPU asset identity, immutable generation data, and context-local GL objects.
6. **Rendering:** this should be the immutable boundary. Commands should contain values and resource handles only. Also make GL state ownership explicit: commands currently rely on ordered side effects (viewport, UBO, scissor, program, sampler) rather than a declared pass state.
7. **Engine:** replace global CRTP registration with engine-owned registration tokens. Define whether entities are externally owned or engine owned; today `Engine` owns systems but only parents non-owning entities, which makes lifecycle semantics asymmetric.
8. **UI foundations:** layout membership and widget hierarchy are two separate non-owning graphs. Add tracked handles and cycle/lifetime rules. Split `Theme` from `Widget`, and choose a consistent mutation-during-traversal policy.
9. **Basic widgets:** composition by child members is generally clear. Extract repeated pressed/hovered/focus interaction state and repeated panel/text/icon style plumbing into focused collaborators rather than more inheritance.
10. **Composite widgets:** large composites should own private subcomponents and expose narrow façade APIs. Avoid returning mutable internals unless external mutation is an explicit invariant the parent observes.
11. **Platform/application:** retain PImpl, but hide Win32 from the cross-platform public surface, make native types RAII, wake every state-changing control path, block idle workers, and make shutdown wholly non-throwing.

## Recommended remediation sequence

1. Restore `window_surface_driver.hpp`; prove a clean Debug/Release build and rerun the current suite.
2. Redesign render-resource ownership and make snapshots immutable/self-contained. Add ThreadSanitizer coverage on a supported toolchain and targeted lifetime stress tests.
3. Fix shutdown wakeup/exception behavior and add bounded idle-quit tests.
4. Make widget traversal mutation-safe and remove fallible callbacks from destructors.
5. Replace CRTP global registries with engine-owned explicit lifecycle, then make context transitions commit-before-notify.
6. Give layouts tracked element lifetimes and full cycle detection.
7. Convert native window/surface wrappers to RAII.
8. Replace non-standard vector/rectangle storage and checked coordinate arithmetic before stabilizing the ABI.
9. Split theme/style and the largest mixed-responsibility units.
10. Decompose CMake targets, remove public Windows macro policy, and add clean-clone/install-consumer CI plus repository metadata.

## Verification performed

- Fresh `testDebug` workflow: **failed to build** at missing `core/platform/detail/window_surface_driver.hpp`.
- Fresh `testRelease` build: **failed to build** at the same missing header.
- Existing prebuilt Debug suite (built before the current source state): **1,036/1,036 passed**, 139.58 seconds. This is useful behavioral evidence but does not replace a fresh build.
- Targeted Clang-Tidy pass over application, widget, engine, graphics, rendering, and JSON units. Material diagnostics incorporated above include the throwing stop callback, exception escape from `Widget::~Widget`, and virtual dispatch during `Entity` construction.
- Pedantic C++23 public-header compile: **failed**, confirming anonymous struct/union extensions; umbrella compilation without target-supplied Windows macros also exposed `min`/`max` collisions.

## Definition of “professionally acceptable” for the next gate

The next gate should require: a clean clone builds in both configurations; all tests pass from that build; installed-package consumption works; idle quit and failure shutdown are bounded; published render snapshots own every dependency and are race-free; object registration/destruction never exposes partial objects; hierarchy/layout mutation cannot invalidate traversal; native resources are RAII; public headers compile standalone without extensions or leaked macro requirements; and CI enforces all of the above.
