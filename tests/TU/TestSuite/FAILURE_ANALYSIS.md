# Current test-pipeline failure analysis

Audited **2026-09-05** on Windows, against the working tree based on `38baf22`
(`Implementing the base fundation of the futur test suite`). The working tree
contains staged and untracked test additions; this is an analysis of that tree,
not of the committed revision alone. No implementation or test fixes were made
for this report.

## Evidence and scope

Both `cmake --build --preset testDebug --parallel 4` and the equivalent
`testRelease` build succeeded and reported no work to do. Tests were run through
the actual CTest presets, one process per discovered test, serially, with a
30-second per-test timeout. Debug and Release were run consecutively to avoid
introducing clipboard and desktop contention between the two runs.

| Preset | Discovered | Enabled | Passed | Failed | Disabled | Runtime skips |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `testDebug` | 1004 | 847 | 824 | 23 | 157 | 0 |
| `testRelease` | 1004 | 847 | 824 | 23 | 157 | 0 |

Both configurations fail the same 23 named tests and return CTest exit code 8.
Debug took 81.35 seconds; Release took approximately 76 seconds. No test timed
out. Release reproduces the functional failures; the input-bounds subprocesses
exit 1 (no exception) instead of Debug's standard-library assertion termination.

The fresh Debug failure names match the previous full Debug run recorded in
[TEST_BACKLOG_COVERAGE.md](TEST_BACKLOG_COVERAGE.md), although test numbers have
changed as coverage grew. Use names when reproducing failures.

Local evidence, relative to the repository root:

- `build/failure_analysis_debug.log` and `build/failure_analysis_release.log`:
  complete CTest console output.
- `build/testDebug/build/failure_analysis_debug.xml` and
  `build/testRelease/build/failure_analysis_release.xml`: machine-readable JUnit
  output. These paths reflect CTest resolving the supplied relative output path
  from the preset's binary directory.
- `build/<preset>/Testing/Temporary/LastTest.log` and `LastTestsFailed.log`:
  per-test output and failure inventory, overwritten by later CTest runs.

The build artifacts are local and ignored by Git; the observations below preserve
the useful evidence without requiring those artifacts. These were local preset
runs, not a separately retrieved hosted-CI run. Fresh configuration, other
operating systems, and other compilers were not audited. The configured compiler
is Clang with the Windows/MSVC standard library.

Disabled tests are coverage/backlog items, not failures in the enabled pipeline.
See [TEST_BACKLOG_DISABLED.md](TEST_BACKLOG_DISABLED.md) and the executable
disabled regressions documented in the coverage audit. They were not force-run
for this report.

## Failure inventory

Priority means proposed repair order: **P1** for termination, invalid memory
access, or broken lifetime invariants; **P2** for incorrect behavior or fixture
defects; **P3** for diagnostics/API-contract alignment. These are triage labels,
not externally assigned issue severities.

| ID | Failing test | Classification | Priority |
| --- | --- | --- | --- |
| F01 | `AnimationLabelTest.NullSheetAndOutOfSheetRangeAreRejected` | Missing range validation | P2 |
| F02 | `ClipboardTest.ClipboardOpenContentionUsesNoThrowFailureReporting` | Fixture and query-semantics mismatch | P2 |
| F03 | `CommandPanelTest.MissingOperationsReportRequestedName` | Removal contract mismatch | P3 |
| F04 | `EngineFacadeTest.DestroyingPopulatedEngineDestroysOwnedSystemsAndDetachesExternalEntities` | Missing engine teardown | P1 |
| F05 | `EventTest.RepeatedIdenticalFocusRequestKeepsFirstRecord` | Focus arbitration contract mismatch | P2 |
| F06 | `EventTest.FocusChannelsAreIndependent` | Same arbitration mismatch; weak independence assertion | P2 |
| F07 | `ExceptionTest.MultipleContextsRemainOrderedAndWhatIsStable` | Formatting-order contract mismatch | P3 |
| F08 | `ReadTextFileTest.StandardUsagePreservesExactContentsIncludingNulAndNewlines` | Windows text-mode translation | P2 |
| F09 | `InherenceTraitTest.CircularHierarchyThrowsLogicError` | Missing cycle rejection | P1 |
| F10 | `InherenceTraitTest.DestroyingChildRemovesItFromLivingParent` | Missing hierarchy teardown | P1 |
| F11 | `InherenceTraitTest.DestroyingParentDetachesLivingChildren` | Missing hierarchy teardown | P1 |
| F12 | `InherenceTraitTest.DestroyingMiddleNodeDetachesFromParentAndOrphansChildren` | Missing hierarchy teardown in both directions | P1 |
| F13 | `JSONValueTest.MalformedJsonInputsAreRejected` | Missing raw UTF-8 validation | P2 |
| F14 | `KeyboardTest.OutOfRangeIndexThrows` | Unchecked array access | P1 |
| F15 | `LinearLayoutTest.PixelRoundingUsesAllSpaceWithoutGapsOrOverlap` | Independent rounding exceeds allocation | P2 |
| F16 | `MouseTest.InvalidButtonThrows` | Unchecked array access | P1 |
| F17 | `ProgramTest.SupportedPrimitivesAcceptZeroRawIndexedAndInstancedDraws` | Missing primitive mappings | P2 |
| F18 | `ProgramTest.DrawCountFirstVertexAndIndexOffsetOverflowAreRejected` | Unchecked index-offset multiplication | P1 |
| F19 | `QuaternionTest.EulerRoundTripPreservesRotationNearGimbalLock` | Unhandled Euler singularity | P2 |
| F20 | `WindowTest.ZeroClientDimensionsAreSupported` | Native minimum-size policy mismatch | P2 |
| F21 | `WindowTest.MinimumNonZeroClientDimensionsAreSupported` | Same native minimum-size mismatch | P2 |
| F22 | `WindowTest.CallbackExceptionIsCapturedAndRethrown` | Throw crosses internal `noexcept` boundary | P1 |
| F23 | `WindowTest.DestructionMessagesReachCallback` | Final destruction callback bypassed | P2 |

## Individual analyses

### F01 - Animation range accepts an invalid last frame

**Evidence:** [animation_label_test.cpp](srcs/animation_label_test.cpp), line 124:
`setAnimationRange(0, count)` does not throw `std::invalid_argument`. The null-sheet
assertion is not the failing part.

**Cause and origin:** In
[animation_label.cpp](../../../src/ui/widget/animation_label.cpp),
`setAnimationRange` only rejects `end < start`. The range is inclusive, so `end`
must also be smaller than `_spriteCount()`. `_advanceFrame` falls back to whole-sheet
cycling for an invalid range and snapshot construction falls back to frame zero;
these defensive fallbacks hide the invalid configuration. The setter's validation
dates to `aacb094` (2026-08-24).

**Proposed fix:** Validate both endpoints against the current sheet before
changing the range, frame, or elapsed time. Reject an empty sheet's range, or
explicitly document a deferred-configuration policy if that is desired. Keep
`std::invalid_argument` consistent with the existing reversed-range error.

**Validation:** Exercise `count - 1`, `count`, a start beyond the sheet, reversed
ranges, single-frame ranges, and state preservation after rejection. Re-run
`AnimationLabelTest.*` and animation rendering tests.

### F02 - Clipboard contention is not established by the fixture

**Evidence:** [clipboard_test.cpp](srcs/clipboard_test.cpp), lines 165-167:
`hasText()`, `writeText(U"contended")`, and `readText().has_value()` all return
true where the test expects false.

**Cause and origin:** The holder thread and
[clipboard.cpp](../../../src/core/platform/clipboard.cpp) both call
`OpenClipboard(nullptr)` within the same process. On this machine, that setup
does not force the library's open attempt to fail. The promises establish timing,
but do not prove exclusion. Also, `hasText()` calls only
`IsClipboardFormatAvailable`: it asks whether a text format exists, not whether
the clipboard can currently be opened. Microsoft documents those separately in
[OpenClipboard](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-openclipboard)
and [IsClipboardFormatAvailable](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-isclipboardformatavailable).
The clipboard wrapper dates to `699b9ef` (2026-08-26); this contention test is a
working-tree addition. The exact same-process native behavior is an observation,
not a portability guarantee.

**Proposed fix:** Hold the clipboard using a distinct real HWND, preferably in a
helper process, with explicit acquired/release barriers. Verify that an independent
open attempt actually fails before asserting the wrapper's error results.
Expect `writeText()` to return false and `readText()` to return `nullopt` under
verified contention. Test format availability separately using seeded content;
do not require `hasText()` to become false merely because a lock is held. Preserve
clipboard restoration and guarantee holder release on all assertion paths.

**Validation:** Repeat the corrected fixture, then test successful access after
release and re-run the text-edit clipboard tests. A production change is not
justified by the current fixture alone.

### F03 - Removing an absent command is intentionally a no-op in the current code

**Evidence:** [command_panel_test.cpp](srcs/command_panel_test.cpp), line 139:
`removeButton("absent")` returns normally. Missing lookup and subscription already
throw as expected.

**Cause and origin:**
[command_panel.cpp](../../../src/ui/widget/command_panel.cpp), `removeButton`,
explicitly returns when the map lookup fails. This branch dates to `9b7ecfc`
(2026-08-24). The public header does not state that missing removal must throw;
the test assumes the same policy as lookup.

**Proposed fix:** Choose and document the removal contract. The least disruptive
repair is to preserve idempotent removal and change this assertion to verify a
no-op, while retaining named diagnostics for lookup/subscription. If strict
removal is intended, throw `std::out_of_range` containing the requested name
before mutating layout or ordering, and audit repeated-removal callers.

**Validation:** Check missing removal, repeated removal, successful removal,
unchanged button order/count on a no-op or exception, and named diagnostics.

### F04 - Engine destruction leaves external entities attached to dead state

**Evidence:** [engine_facade_test.cpp](srcs/engine_facade_test.cpp), lines 222-223:
an external entity retains non-null `parent()` and `context()` after the engine's
scope ends. The owned-system destruction count does not fail.

**Cause and origin:**
[engine.hpp](../../../include/engine/engine.hpp) has no explicit teardown;
[engine.cpp](../../../src/engine/engine.cpp) clears these links only in
`removeEntity`. The root's inherited hierarchy destructor also does nothing
(F10-F12). Engine/context attachment dates to `1674331` (2026-09-01), layered on
the older hierarchy behavior. This can leave both a dead root pointer and a dead
engine context in a still-live entity.

**Proposed fix:** Detach external entities in the engine destructor body while
the root, registries, and systems are still alive. Use mutation-safe traversal
and the normal removal/context cleanup semantics. Define cleanup for descendants
and attachments that reference the same engine. Do not delete external entities.
Fixing hierarchy links alone does not clear the separate context pointer.

**Validation:** Preserve the owned-system assertion; check parent, context, and
registry membership after teardown, nested external hierarchies, explicit removal
before teardown, and both engine-first and entity-first destruction orders.

### F05 - A duplicate focus take is accepted

**Evidence:** [event_test.cpp](srcs/event_test.cpp), line 88: the second identical
keyboard `takeFocus` returns true instead of false. The recorded widget remains
correct because the replacement is identical.

**Cause and origin:**
[event.cpp](../../../src/core/event/event.cpp), `_setFocusChange`, permits any
new `Take` to replace an existing record. `git show 37e41d5 --
src/core/event/event.cpp` confirms a real policy change on 2026-08-25: an
unconditional rejection of a second request became rejection only for a new
`Release`. This test asserts the earlier policy. That history establishes the
behavior change, not that the change was accidental.

**Proposed fix:** Define whether the boolean means accepted or changed. Preserve
the current replacement policy if needed by nested widgets/popups, and align the
test with it; alternatively return false for an identical record as an explicit
idempotency rule. Restoring first-request-wins for all requests is a broader
behavior change and requires checking focus routing consumers.

**Validation:** Cover take/take for the same and different widgets, take/release,
release/take, release/release, and null widgets. Assert both return values and
final records, then run popup, menu, text-edit, and window-state interactions.

### F06 - The independence test actually fails on replacement policy

**Evidence:** [event_test.cpp](srcs/event_test.cpp), line 130: a second keyboard
take for `mouseWidget` returns true instead of false.

**Cause and origin:** Same branch and `37e41d5` change as F05. Keyboard and mouse
still index separate `_focusRecords` slots. This failure is not evidence that
the channels overwrite each other: the test reuses the keyboard channel and
expects its previous request to block replacement. It does not inspect final
records after these operations.

**Proposed fix:** Resolve arbitration with F05. Strengthen this test to read both
records after each request and prove the untouched channel remains identical,
regardless of the chosen within-channel policy.

**Validation:** Mutate keyboard with a mouse record present and vice versa;
assert channel isolation independently of acceptance/replacement results.

### F07 - Exception contexts print newest first

**Evidence:** [exception_test.cpp](srcs/exception_test.cpp), lines 124-125:
context positions are 305, 196, and 90, whereas the assertions require increasing
positions. Repeated `what()` stability is not the failed assertion.

**Cause and origin:**
[exception.cpp](../../../src/exception.cpp), `_append`, walks `_contexts.rbegin()`
to `rend()`. `addContext` appends at the end, producing newest/outermost context
first and the original error deeper in the trace. This dates to `8856017`
(2026-08-22). The test plan requires ordering but does not specify oldest-first.

**Proposed fix:** Prefer documenting the existing outermost-to-cause order and
updating the test accordingly if this reflects the intended diagnostic format.
If chronological insertion order is wanted instead, change traversal deliberately
and update nested-exception/indentation expectations together.

**Validation:** Assert the full relative context order, original-message placement,
nested causes, and stable repeated calls. Do not remove ordering assertions.

### F08 - Text-mode reads normalize CRLF on Windows

**Evidence:** [file_utils_test.cpp](srcs/file_utils_test.cpp), lines 17-18:
19 bytes become 18; `"alpha\nbe\0ta\r\ngamma\n"` becomes
`"alpha\nbe\0ta\ngamma\n"`. The embedded NUL survives; the missing byte is CR.

**Cause and origin:**
[file_utils.cpp](../../../src/file_utils.cpp) opens the stream with `std::ios::in`
only. The Windows runtime translates line endings in text mode. This is the
implementation introduced by `ca29c10` (2026-09-02). The
[test plan](../../../docs/unit_test_plan.md) explicitly asks for exact contents.

**Proposed fix:** Open with `std::ios::in | std::ios::binary` and retain the
existing error reporting. Returning bytes in a `std::string` does not require
text-mode newline conversion.

**Validation:** Mixed LF/CRLF, lone CR, embedded NUL, empty input, Unicode bytes,
and byte `0x1A`, alongside existing missing/unreadable-file cases.

### F09 - A descendant can become the root's parent

**Evidence:** [inherence_trait_test.cpp](srcs/inherence_trait_test.cpp),
lines 239-241: `grandChild.addChild(&root)` does not throw and `root.hasParent()`
becomes true.

**Cause and origin:**
[inherence_trait.hpp](../../../include/design_pattern/trait/inherence_trait.hpp),
`setParent`, removes the old link and inserts the new one without checking whether
the proposed parent is the node itself or one of its descendants. This is present
at the earliest available commit, `da04f9d` (2026-08-20). Recursive traversal can
then loop indefinitely or overflow the stack.

**Proposed fix:** Before any mutation, walk the proposed parent's ancestor chain
and reject encountering `self` with `std::logic_error`. Cover direct self-parenting
as well as longer cycles. Invalid operations must leave both sides and
notifications unchanged.

**Validation:** Self-parent, two-node and longer cycles, valid reparenting,
unchanged original hierarchy on failure, and no callbacks for rejected edits.

### F10 - Destroyed children remain in the living parent's collection

**Evidence:** [inherence_trait_test.cpp](srcs/inherence_trait_test.cpp),
lines 257-258: the parent's child collection is nonempty and its removal count is
zero instead of one after the child dies.

**Cause and origin:**
[inherence_trait.hpp](../../../include/design_pattern/trait/inherence_trait.hpp)
has `virtual ~InherenceTrait() = default`, present since `da04f9d`. Container
destruction does not unlink a non-owning pointer held by another object.

**Proposed fix:** Add a lifetime-safe unlink path, ensuring a living parent drops
the child exactly once. Coordinate it with existing derived-class teardown.
Do not simply call arbitrary derived callbacks from a base destructor: the
child's derived state may already be gone. Separate structural cleanup from
notifications requiring live derived objects, and define a non-throwing teardown
policy. The test's parent callback count must be preserved or deliberately
revised with the public lifecycle contract.

**Validation:** Child-first destruction, explicit detach before destruction,
multiple siblings, exactly-once parent notification, and no use of dead derived
state in callbacks. Run widget/entity lifetime tests after the shared-trait fix.

### F11 - Surviving children retain a dead parent pointer

**Evidence:** [inherence_trait_test.cpp](srcs/inherence_trait_test.cpp),
lines 271-272: `hasParent()` remains true and `parent()` remains non-null.

**Cause and origin:** The same default destructor as F10 destroys only the
parent's own container; it never clears back-pointers in surviving children.
This behavior is already present at `da04f9d`.

**Proposed fix:** Orphan all non-owned children during safe teardown. Use a
snapshot or a draining loop so unlinking cannot invalidate an active iterator.
Do not delete externally owned children, and do not dispatch callbacks into a
parent whose derived state has been destroyed.

**Validation:** One/many surviving children, already detached children, safe
reparenting after the former parent dies, and no duplicate detach notifications.

### F12 - Destroying a middle node leaves dangling links on both sides

**Evidence:** [inherence_trait_test.cpp](srcs/inherence_trait_test.cpp),
lines 289-291: the root retains the middle node and the leaf retains that node
as its parent.

**Cause and origin:** This combines F10 and F11: neither direction is maintained
by the default destructor inherited from `da04f9d`. It is one shared lifetime
defect exposed by three separate tests, not three independent algorithms.

**Proposed fix:** Implement parent unlinking and child orphaning as one coherent,
idempotent teardown operation. Review callback mutation and ownership boundaries
before sharing the operation across widgets and entities.

**Validation:** Keep this middle-node test in addition to both endpoint tests.
Include deeper trees, arbitrary destruction order, and explicit partial detach
before destruction. F04 still needs engine-context cleanup after this is fixed.

### F13 - Invalid raw UTF-8 passes the JSON string parser

**Evidence:** [json_value_test.cpp](srcs/json_value_test.cpp), line 201: a quoted
string containing bytes `C3 28` is accepted. The trace can display this as a
garbled character followed by `(`; byte values identify the actual case.

**Cause and origin:**
[json_object.cpp](../../../src/container/json/json_object.cpp),
`Parser::parseString`, validates escapes and control characters but appends other
bytes directly. Unicode escape handling does not validate unescaped multibyte
input. This parser path dates to `02b4cd0` (2026-08-21).

**Proposed fix:** Validate UTF-8 sequences in raw string content, for object keys
as well as values. Reject missing/invalid continuation bytes, overlong encodings,
surrogates, and values above `U+10FFFF` through the parser's normal error path.
Use unsigned-byte comparisons and preserve valid encoded bytes.

**Validation:** Identify each malformed input with a byte-oriented trace; retain
valid two-, three-, and four-byte strings, escaped Unicode/surrogate-pair tests,
and the existing malformed-document matrix. This run does not show a general
failure to reject malformed JSON syntax.

### F14 - Invalid keyboard indexing aborts instead of throwing

**Evidence:** [keyboard_test.cpp](srcs/keyboard_test.cpp), lines 116 and 136:
both const and mutable probes terminate with Debug exit status `-1073740791`
and `array subscript out of range`, instead of catching `std::out_of_range` and
exiting zero. In Release, both subprocesses exit 1 because the expected exception
is not thrown.

**Cause and origin:**
[keyboard.cpp](../../../src/input/keyboard.cpp), both `operator[]` overloads,
use unchecked `keys[static_cast<std::size_t>(key)]`. The invalid enum converts to
`NbKey`, exactly one past the array. This implementation is present at `da04f9d`.
Debug's standard-library assertion is a symptom; an unchecked Release access has
undefined behavior, not a defined fallback value.

**Proposed fix:** Use `keys.at(...)` or an explicit range check throwing
`std::out_of_range` in both overloads. Validate all cast enum values before access.

**Validation:** Keep subprocess isolation until the guard exists, then verify
the sentinel, larger casts, relevant negative casts, first/last valid keys, and
const/mutable behavior in both configurations.

### F15 - Rounding each cell independently overshoots the layout

**Evidence:** [linear_layout_test.cpp](srcs/linear_layout_test.cpp), line 75:
three child widths sum to 102 rather than the available 101 pixels.

**Cause and origin:**
[linear_layout.hpp](../../../include/ui/layout/linear_layout.hpp),
`_applyGeometry`, converts each resolved float size independently through
`_dimension`. [layout.cpp](../../../src/ui/layout/layout.cpp), `_toDimension`,
uses `std::lround`: three shares of approximately `101 / 3` become `34 + 34 + 34`.
Cursor advancement preserves adjacency but pushes the final edge outside the
allocation. The allocation loop dates to `1b2afa3` (2026-08-21).

**Proposed fix:** Quantize the allocation as a whole. For example, round cumulative
boundaries and derive widths from consecutive boundaries, or floor shares and
distribute remaining pixels deterministically among eligible elements. Preserve
minimum/maximum constraints, intentional unused space, and padding; do not force
all layouts to fill when their sizing policies prohibit it.

**Validation:** Both axes; 100/101/102-pixel budgets; mixed policies, padding,
nonzero anchors, constrained children, and too-small budgets. Assert adjacency,
total allocated width/height, final boundary, and repeatability.

### F16 - Invalid mouse indexing has the same bounds defect

**Evidence:** [mouse_test.cpp](srcs/mouse_test.cpp), lines 79 and 99:
both const and mutable probes hit the Debug array assertion and exit
`-1073740791`, instead of catching `std::out_of_range`. In Release, both exit 1
without the expected exception.

**Cause and origin:**
[mouse.cpp](../../../src/input/mouse.cpp) indexes `buttons[...]` with a cast enum
without checking `NbButton`. Like F14, this is present at `da04f9d`.

**Proposed fix:** Use `buttons.at(...)` or the equivalent explicit guard in both
overloads. Apply the same invalid-input policy as the keyboard API.

**Validation:** Sentinel/larger/negative casts where representable, valid first
and last buttons, both overloads and configurations. A Release run that merely
returns from the access does not establish safety.

### F17 - Two public primitive values lack OpenGL mappings

**Evidence:** [program_test.cpp](srcs/program_test.cpp), lines 158-160:
six unexpected `std::logic_error("Unsupported primitive")` exceptions, covering
three draw entry points for two primitive values.

**Cause and origin:**
[program.hpp](../../../include/graphics/opengl/program.hpp) exposes `LineLoop`
and `TriangleFan`, but `_openGLPrimitive` in
[program.cpp](../../../src/graphics/opengl/program.cpp) maps only Points, Lines,
LineStrip, Triangles, and TriangleStrip. The incomplete mapping is already in the
initial `da04f9d` implementation; later formatting/path changes are not evidence
of a new regression.

**Proposed fix:** Add `LineLoop -> GL_LINE_LOOP` and
`TriangleFan -> GL_TRIANGLE_FAN`, keeping rejection of unsupported enum casts.
These values should be mapped even for zero-count calls.

**Validation:** Re-run the primitive matrix and invalid-cast test. Add primitive
names to traces. For meaningful drawing validation, bind a valid VAO/index buffer
and check GL errors or rendered pixels; `EXPECT_NO_THROW` alone only establishes
C++ dispatch acceptance.

### F18 - Index-to-byte conversion wraps before the driver call

**Evidence:** [program_test.cpp](srcs/program_test.cpp), line 172:
`render(Triangles, UnsignedInt, SIZE_MAX, 0)` does not throw
`std::overflow_error`. Other count/first-vertex assertions pass.

**Cause and origin:**
[program.cpp](../../../src/graphics/opengl/program.cpp), `render` and
`renderInstanced`, compute `firstIndex * stride` before converting to
`std::uintptr_t`. The unsigned multiplication can wrap; converting the wrapped
value afterward cannot detect it. This code is present at `da04f9d`. The current
test exercises only the non-instanced offset failure, but both methods share it.

**Proposed fix:** Resolve/validate the index type, check that `firstIndex` is no
larger than the representable byte-offset limit divided by stride, and only then
multiply. Check both `size_t` arithmetic and `uintptr_t` representability where
their widths differ. Share the helper between both indexed methods and validate
before any OpenGL call, including zero-count draws.

**Validation:** Boundary and boundary-plus-one for one-, two-, and four-byte
indices in both methods. Retain count and instance-count checks. The failing test
does not create a GL context, so the invalid-input path must reject before
touching the driver; use a context for accepted-boundary draw probes.

### F19 - Euler extraction loses the coupled angle at gimbal lock

**Evidence:** [quaternion_test.cpp](srcs/quaternion_test.cpp), helper line 18 and
test starting at line 153: absolute normalized quaternion dots are approximately
`0.9396927` and `0.9961948`, outside the allowed distance `0.002` from one. The
input matrix uses pitches `89.9`, `90`, `90.1`, `-89.9`, `-90`, and `-90.1` degrees
with roll 15 and yaw -25. The current trace does not label which pitch produced
each failed assertion.

**Cause and origin:**
[quaternion.cpp](../../../src/math/quaternion.cpp), `toEuler`, always extracts
roll and yaw through independent `atan2` expressions. Near pitch singularities,
their inputs approach zero and float cancellation can lose the coupled rotation.
Clamping the pitch argument protects `asin` but does not preserve that information.
The conversion dates to `bb16a19` (2026-08-21). Source inspection strongly supports
this singularity diagnosis; individual failing pitches should be traced during
repair rather than inferred from log order.

**Proposed fix:** Use a singularity-aware extraction consistent with `fromEuler`'s
rotation convention: choose one angle deterministically at the singularity and
derive the remaining coupled angle from stable quaternion/matrix components.
Use higher-precision intermediates where helpful. Increasing precision alone
does not define the singular case, and widening tolerance would hide rotation
loss.

**Validation:** Add per-pitch traces, both exact singularities and nearby values,
multiple coupled roll/yaw inputs, quaternion sign equivalence, and rotated basis
vectors. Keep equivalence of rotations as the oracle; Euler triples are not unique.

### F20 - A requested zero-width client is widened by native window policy

**Evidence:** [winapi_window_test.cpp](srcs/winapi_window_test.cpp), line 112:
requested width zero yields client width 120 on this machine. The height-zero
assertion passes.

**Cause and origin:**
[window.hpp](../../../include/core/platform/window.hpp) fixes the style to
`WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS`.
[window.cpp](../../../src/core/platform/window.cpp) adjusts the outer rectangle
but leaves default minimum-size handling to Windows. The native path exists
since `da04f9d`. The observed widening is consistent with decorated-window minimum
sizing; 120 is an environment-specific measurement, not a portable constant.
Microsoft describes overriding default size constraints through
[WM_GETMINMAXINFO](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-getminmaxinfo).

**Proposed fix:** Decide whether this wrapper promises exact zero client dimensions
for a decorated native window. If yes, explicitly implement the necessary native
size policy and verify resulting `GetClientRect`, accounting for nonclient borders
and DPI. If native clamping is intended, document it and revise the test to verify
successful creation and truthful actual geometry. Do not return a fabricated
requested size from `geometry()`.

**Validation:** Inspect `WM_GETMINMAXINFO`/outer rectangles during creation,
test width-only and height-only zero cases, normal sizes, and multiple DPI settings.

### F21 - A one-pixel client width meets the same native minimum

**Evidence:** [winapi_window_test.cpp](srcs/winapi_window_test.cpp), line 126:
requested width one also yields 120; the height-one assertion passes.

**Cause and origin:** Same fixed style and default sizing path as F20, present
at `da04f9d`. This second failure shows that changing a special case for zero
would leave the underlying minimum-size mismatch unresolved.

**Proposed fix:** Resolve this with F20 through one documented native sizing
policy. If exact tiny dimensions are required only for rendering fixtures,
consider a distinct borderless/child-window fixture rather than changing the
normal application-window contract implicitly.

**Validation:** One-pixel width/height independently and together, values on both
sides of the native minimum, hidden/visible creation, and reported actual geometry.

### F22 - Callback exceptions terminate before the capture handler

**Evidence:** [winapi_window_test.cpp](srcs/winapi_window_test.cpp), line 193:
the subprocess exits with status 3 rather than zero when a callback throws
`std::runtime_error("callback exploded")`.

**Cause and origin:**
[window.cpp](../../../src/core/platform/window.cpp), `_procedure`, has a
`try/catch` intended to store `_pendingException`, but it calls `_process`, which
is itself `noexcept` and invokes the user callback. A callback throw therefore
terminates at `_process` before it can reach the outer catch. Both declarations
in [window.hpp](../../../include/core/platform/window.hpp) and definitions matter.
This boundary mismatch is present in the original `da04f9d` native-window path.

**Proposed fix:** Keep the actual Win32 callback `_procedure` non-throwing. Remove
`noexcept` from the internal dispatch helper so the existing catch can capture
the exception, or catch directly around callback invocation inside that helper.
Preserve the first pending exception and the consume-on-rethrow behavior.
Ensure exceptions during creation and destruction use safe native return values
and do not bypass mandatory cleanup.

**Validation:** The current subprocess must catch the original message and exit
zero. Also check no exception pending, repeated rethrow, multiple throwing
callbacks, callback failure during native lifecycle messages, and continued
ability to destroy the window.

### F23 - WM_NCDESTROY skips the application callback

**Evidence:** [winapi_window_test.cpp](srcs/winapi_window_test.cpp), line 225:
`WM_DESTROY` appears in the callback's messages but `WM_NCDESTROY` does not.

**Cause and origin:**
[window.cpp](../../../src/core/platform/window.cpp), `_process`, intercepts
`WM_NCDESTROY` and calls `_processDestruction`, which directly invokes
`DefWindowProcW`, clears `GWLP_USERDATA`, and resets `_handle`. `_messageHandler`
is never invoked on that path. This behavior is present at `da04f9d`. Windows
sends the final message after `WM_DESTROY`; see
[WM_NCDESTROY](https://learn.microsoft.com/en-us/windows/win32/winmsg/wm-ncdestroy).

**Proposed fix:** Deliver the final notification while the window binding is
still usable, with an explicit rule for who calls `DefWindowProcW`. Always clear
the native binding and `_handle`, even if user code throws. Combine this work
with F22 so a newly forwarded destruction callback cannot terminate the process
or skip cleanup. Avoid double default processing and reentrant double destruction.

**Validation:** Both messages arrive exactly once and in order, handle cleanup
still occurs, `destroy()` remains idempotent, and throwing/reentrant destruction
handlers do not leave stale native state.

## Repair order and completion checks

1. Fix F22's exception boundary; address F23's cleanup-safe dispatch alongside it.
2. Fix bounds/overflow validation in F14, F16, and F18.
3. Fix hierarchy invariants in F09-F12, then engine-specific context teardown in
   F04. Audit existing derived destructors before changing the shared trait.
4. Fix deterministic behavior defects F01, F08, F13, F15, F17, and F19.
5. Correct F02's fixture; resolve and document the contracts behind F03, F05-F07,
   and F20-F21. A green test obtained by silently changing the asserted contract
   is not sufficient evidence of a repaired API.

For each repair, run the affected tests and neighboring consumers first, then
both full presets. Keep disabled-regression work separate and explicitly tracked.
Mark a failure resolved only after its named test passes under the agreed
contract and both configurations have been checked.

Reproduction from the repository root, using absolute JUnit destinations to
avoid the nested output-directory behavior of the audited commands:

```powershell
cmake --build --preset testDebug --parallel 4
$debugReport = Join-Path (Get-Location) 'build/failure_analysis_debug.xml'
ctest --preset testDebug --parallel 1 --timeout 30 --output-junit $debugReport

cmake --build --preset testRelease --parallel 4
$releaseReport = Join-Path (Get-Location) 'build/failure_analysis_release.xml'
ctest --preset testRelease --parallel 1 --timeout 30 --output-junit $releaseReport

# One failure, preserving CTest's process isolation:
ctest --preset testDebug -R '^WindowTest\.CallbackExceptionIsCapturedAndRethrown$' --output-on-failure

# A shared cause and its consumers:
ctest --preset testDebug -R '^(InherenceTraitTest|EngineFacadeTest)\.' --output-on-failure
```

For a fresh build tree, configure with `cmake --preset testDebug` / `testRelease`
first, or use the workflows documented in [the TU README](../README.md).

Git origins above are based on inspected lines and relevant commit diffs, not a
historical passing/failing bisect. `da04f9d` is the initial history boundary:
"present since" does not identify development that happened before that import.
All 23 failing test source files are additions relative to `HEAD` in this working
tree, so there is no committed passing baseline for these particular test cases.
