# Sparkle disabled unit-test backlog


Last audited: **2026-09-07** against `docs/unit_test_plan.md` and all sources compiled by `SparkleTestSuite`.

Current inventory: **41 disabled behaviors**, matching the 41 disabled test instances in the test sources.

Disposition review: all **41** remaining entries are **Implement**. This means the behavior is useful and testable, although it may first need a product fix, a documented contract, or an internal test fixture.

This file lists behavior that does not have complete TU coverage. **Missing** means no test exists; **Disabled** means a `DISABLED_` specification exists but does not implement the behavior; **Partial** means a test implements only part of the behavior. Test validation and enabled/disabled status do not affect whether complete coverage is removed from this inventory.

Remove an item when its behavior is fully implemented by a test. If only part is implemented, retain the uncovered part.

## `spk::DataModel<T>`

1) **RowIDOverflowThrows** — Disabled  
Force row-ID exhaustion through a deterministic seam and verify `std::overflow_error` without model mutation.

## `spk::WinAPI::WakeEvent`

1) **CreateEventFailureReportsCodeAndOperation** — Disabled  
Inject `CreateEventW` failure and verify `std::system_error`, Win32 code, and operation name.

## `spk::Window::State`

1) **InactiveFocusedWidgetDispatchBehavior** — Disabled  
Deactivate a focused widget and verify event routing and focus-release policy.

## `spk::Window::Surface`

1) **StandardNativeCreationMakeCurrentPresentAndDestroy** — Disabled  
Use a hidden native window and verify creation, readiness, current context, presentation, cleanup, and release.

2) **RepeatedSuccessfulCreateIsRejected** — Disabled  
Verify a second `create()` throws `std::logic_error` without damaging the surface.

3) **PartialOpenGLInitializationIsCleanedUp** — Disabled  
Fail at each WGL setup stage and verify every acquired resource is released.

4) **WGLAndWin32FailuresPropagatePrecisely** — Disabled  
Inject capability/setup failures and verify exception types, error codes, and context.

## `spk::Application`

1) **ReadyWindowInitializationUpdateRenderAndClose** — Disabled  
Use deterministic barriers to verify initialization, one update, one frame, and clean close.

2) **CloseDuringReadyAndReleasingIsIdempotent** — Disabled  
Repeat close requests at lifecycle milestones and verify one-time coordinated teardown.

3) **QuitStopsAndJoinsAllRuntimes** — Disabled  
Quit after the loop starts and verify all workers stop and join without hanging.

4) **EventsRouteOnlyToMatchingWindow** — Disabled  
Inject multi-window events and verify each reaches only its target runtime.

5) **DuplicateRuntimeAndSnapshotEndpointsThrow** — Disabled  
Inject duplicate registrations and verify `std::logic_error` with useful context.

6) **WorkerExceptionsCrossRunBoundary** — Disabled  
Inject update/render failures and verify the original exception crosses the public run boundary after cleanup.

7) **RuntimeFailuresReceiveApplicationContext** — Disabled  
Verify Sparkle and standard exceptions gain application/window/runtime context without losing their cause.

8) **ReleasedNativeDuringSurfaceCreationIsReported** — Disabled  
Release native state between platform and surface creation and verify deterministic reporting and cleanup.

## `spk::Registry::Query`

1) **ContainParticipantSupportsTypeRegexPredicateAndReactiveEdits** — Disabled
Query participants by type/name/predicate and verify additions, removals, and renames update results.

2) **ContainBehaviourSupportsTypeRegexPredicateAndReactiveEdits** — Disabled
Exercise the same matrix for behaviours.

3) **AttachmentNameChangesInvalidateRegexQueries** — Disabled
Rename after positive/negative caching and verify name/regex results invalidate.

## `spk::EntityAttachmentCollection<T>`

1) **EntityAndEngineWrappersPreserveCollectionSemantics** — Disabled  
Verify add/remove, ownership, callbacks, duplicates, and typed lookup through public wrappers.

## `spk::Behaviour`

1) **InactiveOwnerRejectsDirectInteractionDispatch** — Disabled  
Verify direct interaction neither handles nor consumes when the owner is inactive.

## `spk::Entity`

1) **RecursiveDestructionFollowsOwnershipContract** — Disabled  
Destroy a hierarchy and verify which children/attachments are destroyed, detached, or preserved.

2) **ContextChangesPropagateToExistingChildren** — Disabled  
Change context after population and verify exactly-once propagation.

3) **CallbackMutationHasStableTraversal** — Disabled

Add/remove during callbacks and verify deterministic visitation and coherent final state.

## `spk::Engine`

1) **SystemsMayMutateDuringUpdateSafely** — Disabled  
Add/remove systems during update and verify current/next-frame traversal rules.

2) **EntitiesMayMutateDuringUpdateSafely** — Disabled  
Add/remove entities during traversal and verify iteration, ownership, and registry state.

## `spk::Texture`

1) **OpenGLAndPNGFailuresAreReported** — Disabled
Inject texture/OpenGL/PNG failures and verify diagnostics and cleanup.

## `spk::Image`

1) **OversizedEncodedSpanIsRejected** — Disabled  
Use a synthetic address-space seam and verify rejection before decoder access.

## `spk::BufferGPUResource` and `Storage`

1) **OpenGLCreationFailure** — Disabled  
Inject `glGenBuffers` returning zero and verify exception, state, retry, and cleanup.

## `spk::VertexArray`

1) **OversizedStrideIsRejected** — Disabled  
Use a synthetic layout seam to exceed `GLsizei` and reject before OpenGL calls.

2) **OpenGLCreationFailure** — Disabled  
Inject `glGenVertexArrays` returning zero and verify diagnostics and cleanup.

## `spk::Sampler`

1) **OpenGLCreationFailure** — Disabled  
Inject `glGenSamplers` returning zero and verify reporting, cleanup, and retry.

## `spk::Program`

1) **ShaderAndProgramCreationFailures** — Disabled  
Inject create/compile/link/validate failures and verify logs, context, and handle cleanup.

## `spk::Mesh<Vertex>` and `Builder<Mesh>`

1) **VertexIndexOverflowIsRejected** — Disabled  
Use a capacity seam to exhaust the index type and verify atomic rejection before wraparound.

## `spk::ScalableWidget`

1) **ParentBoundsConstrainAllResizeDirections** — Disabled

Resize from every edge and corner and verify the resulting geometry remains within the parent bounds.

2) **MouseFocusReleaseAndDeactivationCancelResizing** — Disabled

Lose mouse focus or deactivate during a resize and verify capture and pending resize state are cleared.

## Widget interaction behavior

1) **PushButtonWindowFocusLossCancelsPendingClick** — Disabled

Press a push button, remove window focus, and verify release cannot complete the pending click.

2) **PushButtonDeactivationCancelsPendingClick** — Disabled

Deactivate a pressed push button and verify its pending click and capture state are cancelled.

3) **ToggleSwitchFocusLossCancelsPendingToggle** — Disabled

Remove focus during a pending toggle and verify release does not change the value.

4) **TooltipDestroyedTargetIsForgotten** — Disabled

Destroy the tooltip target and verify later updates do not retain or dereference the expired target.

5) **TooltipMaximumWidthClampsUnbreakableTextIncludingZero** — Disabled

Verify long unbreakable text and a zero maximum width follow the defined wrapping and clamping behavior.

## `spk::Workspace`

1) **ApplyStylePropagatesToContentAndMenus** — Disabled

Apply a new style after construction and verify existing content and menus receive it consistently.
