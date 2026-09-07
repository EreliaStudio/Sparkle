# Disabled-test analysis

Audited on **2026-09-05** against the current source tree and
`build/testDebug/tests/TU/TestSuite/SparkleTestSuite.exe`.

## Executive summary

The suite contains **157 tests whose names begin with `DISABLED_`**, plus **6 enabled
tests that can call `GTEST_SKIP()` conditionally**.

| Category | Count | Current result | Recommended action |
|---|---:|---|---|
| Implemented golden-image tests | 36 | 33 report a missing golden, 2 DebugOverlay cases terminate, 1 has an empty body | Review/generate baselines; fix the DebugOverlay setup; then enable. No product API is needed. |
| Implemented behavioral tests | 17 | 15 assertion failures, 1 unsafe invalid-enum case, 1 duplicate-ownership/death-test case | Treat as product defects or unresolved contracts. Fix behavior/contracts, not test visibility. |
| Empty deliberate-failure/API specifications | 3 | 2 deliberate failures and 1 empty body | Keep as backlog prose until a test seam or product requirement exists; an empty Google Test is misleading. |
| `GTEST_SKIP()` specification placeholders | 101 | Always skipped | Replace with real tests or delete/defer the declarations. Most need fixtures or internal test seams, not public APIs. |
| Enabled tests with conditional skips | 6 | All 6 passed in this audit | Keep enabled. Their skip guards protect the user's clipboard and non-Windows builds. |

Running the disabled set with `--gtest_also_run_disabled_tests` is therefore **not
expected to pass today**. More importantly, an always-skipped or empty test does not
provide coverage merely because it is compiled.

## Should public APIs be added?

**Generally, no.** Do not enlarge the supported public API solely so a unit test can
force private counters, OpenGL failures, Win32 failures, thread interleavings, or
internal lifecycle stages. Prefer, in order:

1. an existing public observable (pixels, state, callback, exception, or lifetime);
2. a test-library fixture around the real platform/OpenGL objects;
3. a narrow internal dependency-injection seam compiled only for tests;
4. a friend test/access shim when injection would overcomplicate production code.

A new public API is reasonable only when the capability is itself useful to callers
and its semantics can be supported long-term. The clearest candidates for a product
decision are:

- `DataModel<T>` mutable access: add it only if callers actually need in-place
  mutation and define how notifications/versioning work. Do not add it for coverage.
- `Window::Native::identifier()`: add a read-only getter only if native identity is a
  supported caller concept. Otherwise verify identity inside an internal fixture.
- render-target `Texture` construction: add it if off-screen rendering is a real
  library feature; otherwise keep it internal to the rendering test harness.
- ownership, traversal mutation, z-order, invalid focus channels, and missing-glyph
  behavior: these need documented contracts before tests are enabled. They do not
  inherently require more getters/setters.

## Implemented golden-image tests (36)

These are real test bodies, but they are not ready to enable as a batch. The 33
ordinary failures all rendered an actual image and then reported that the expected
PNG did not exist. Review each generated image before copying it into
`resources/expectedImages`; never bless output automatically in CI.

| Source / tests | Count | Audit result and recommendation |
|---|---:|---|
| `animation_label_test.cpp`: `FirstFrame`, `AlternateFrame`, `Resized` | 3 | Missing goldens. Review the three outputs, add baselines, remove `DISABLED_`. |
| `check_box_test.cpp`: `Unchecked`, `Checked`, `CustomSpacingAndIndicatorSize` | 3 | Missing goldens; no API change. |
| `checkable_icon_button_test.cpp`: `Unchecked`, `Checked`, `AlternateSprites` | 3 | Missing goldens; no API change. |
| `combo_box_test.cpp`: `Placeholder`, `SelectedItem`, `OpenPopup` | 3 | Missing goldens; no API change. |
| `command_panel_test.cpp`: `Empty`, `ThreeButtons`, `PaddedButtons` | 3 | Missing goldens; no API change. |
| `container_widget_test.cpp`: `TextContent`, `OffsetContent` | 2 | Missing goldens; no API change. |
| `debug_overlay_test.cpp`: `SingleCell`, `Grid` | 2 | The process terminates before the missing-golden assertion. Fix the test setup/lifetime or DebugOverlay crash first, then create reviewed goldens. |
| `dynamic_text_label_test.cpp`: `InitialValue`, `RefreshedValue` | 2 | Missing goldens; no API change. |
| `engine_widget_test.cpp`: `NoEngineSmallViewport`, `NoEngineLargeViewport` | 2 | Missing goldens; no API change. |
| `icon_button_test.cpp`: `DefaultSprite`, `AlternateSprite`, `LargeGeometry` | 3 | Missing goldens; no API change. |
| `image_label_test.cpp`: `WholeTexture`, `SpriteSection`, `WideGeometry` | 3 | Missing goldens; no API change. |
| `interface_window_template_test.cpp`: `Normal`, `Minimized`, `CustomPaddingAndMenu` | 3 | Missing goldens; no API change. |
| `menu_bar_menu_test.cpp`: `StandaloneWithSeparator` | 1 | Missing golden; no API change. |
| `menu_bar_test.cpp`: `ClosedMenus`, `CustomDimensions` | 2 | Missing goldens; no API change. |
| `menu_bar_menu_break_test.cpp`: `ThreePartSeparator` | 1 | **Empty body**, so its apparent pass is not coverage. Implement the render assertion and add a reviewed golden. |

The names above omit the common suite prefix and `DISABLED_` only for readability.

## Implemented behavior tests that expose unresolved behavior (17)

| Fully qualified test | Observed result | Recommendation / API decision |
|---|---|---|
| `DataModelViewTest.DISABLED_ReactiveModelShrinkAndResizeClampScrollOffset` | Assertion failure | Fix scroll clamping/repopulation on model shrink and viewport growth. Existing observables are sufficient; no API needed. |
| `DataModelViewTest.DISABLED_DuplicateDelegateWidgetIsRejected` | Death-test child exits with failure | Define delegate ownership validation and reject duplicate `unique_ptr` ownership before destruction. No new public API is needed. The test may need a safer malicious-delegate fixture. |
| `DataModelViewTest.DISABLED_InvalidDelegateReplacementPreservesExistingItems` | Assertion failure | Make delegate replacement transactional, or document destructive failure and rewrite the test. No API needed. |
| `MatrixTest.DISABLED_PerspectiveRejectsNonFiniteParameters` | Assertion failure | Add `std::isfinite` validation to `perspective`. No API needed. |
| `ProgressBarTest.DISABLED_NaNRatioIsRejectedWithoutMutation` | Assertion failure | Reject non-finite ratios before mutation/geometry calculation. No API needed. |
| `SpinBoxTest.DISABLED_ArithmeticAtUnsignedExtremaSaturatesBeforeApplyingLimits` | Assertion failure | Use checked/saturating arithmetic before applying limits. No API needed. |
| `SpinBoxTest.DISABLED_SignedExtremaSaturateBeforeNarrowing` | Assertion failure | Avoid narrowing/overflow and saturate at the configured bounds. No API needed. |
| `ScalableWidgetTest.DISABLED_ParentBoundsConstrainAllResizeDirections` | Assertion failure | Decide whether parent clipping is a contract. If yes, clamp resize geometry; if no, delete/rewrite this requirement. No API needed. |
| `ScalableWidgetTest.DISABLED_MouseFocusReleaseAndDeactivationCancelResizing` | Assertion failure | Clear resize/capture state on focus loss and deactivation. No API needed. |
| `PushButtonInteractionTest.DISABLED_WindowFocusLossCancelsPendingClick` | Assertion failure | Cancel pressed/captured state on focus loss. No API needed. |
| `PushButtonInteractionTest.DISABLED_DeactivationCancelsPendingClick` | Assertion failure | Cancel pressed/captured state on deactivation. No API needed. |
| `ToggleSwitchInteractionTest.DISABLED_FocusLossCancelsPendingToggle` | Assertion failure | Cancel the pending toggle on focus loss. No API needed. |
| `TooltipInteractionTest.DISABLED_DestroyedTargetIsForgotten` | Assertion failure | Replace/guard the raw target lifetime (destruction subscription, observer token, or equivalent). This is an ownership fix, not a getter request. |
| `TooltipInteractionTest.DISABLED_MaximumWidthClampsUnbreakableTextIncludingZero` | Assertion failure | Define zero-width and long-word behavior, then implement wrapping/clipping. No API needed. |
| `WidgetRenderIntegrationTest.DISABLED_ExistingTextSnapshotSurvivesAtlasGrowthAndFontMove` | Pixel assertion failure | Repair atlas-growth/move invalidation or explicitly document snapshots as invalidated. Existing capture APIs suffice. |
| `WorkspaceTest.DISABLED_ApplyStylePropagatesToContentAndMenus` | Assertion failure | Implement style propagation or narrow the documented `applyStyle` contract. No API needed. |
| `EventTest.DISABLED_OutOfRangeFocusChannelHasDefinedBehavior` | Process failure/unsafe oracle | Validate the enum before array indexing, or make the channel representation impossible to forge. Do not enable until UB is removed. No extra public testing API is needed. |

## Empty or deliberately failing specifications (3)

| Fully qualified test | Current body | Recommendation |
|---|---|---|
| `IconButtonTest.DISABLED_CoordinateResolutionWithoutIconsetThrows` | Empty; therefore reports pass | Do not add iconset removal just for this test. If a no-iconset state is supported, construct it and assert the exception; otherwise delete the test requirement. |
| `WakeEventTest.DISABLED_CreateEventFailureReportsCodeAndOperation` | Unconditional `FAIL()` | Add an internal Win32-call injection seam and assert error code/context. Do not expose raw Win32 injection publicly. |
| `WindowClassTest.DISABLED_UnregistrationFailureReportsCodeAndOperation` | Unconditional `FAIL()` | First define destructor/error semantics. Use an internal platform shim; avoid a throwing destructor and avoid a public failure-injection API. |

## Always-skipped specification placeholders (101)

Every item in this section has both a `DISABLED_` name and `GTEST_SKIP()` in its
body. Once implemented, remove both mechanisms. The lists use the exact test name
after `DISABLED_`.

### Application and platform lifecycle (15)

- `application_test.cpp` (8):
  `StandardReadyWindowInitializationUpdateRenderAndClose`,
  `CloseWhileReadyAndReleasingIsIdempotentlyCoordinated`,
  `QuitWhileRunIsActiveStopsAndJoinsAllRuntimes`,
  `EventRoutingTargetsOnlyTheMatchingWindow`,
  `DuplicateRuntimeObjectsAndSnapshotEndpointsThrowLogicError`,
  `WorkerExceptionsCrossTheRunBoundary`,
  `RuntimeFailuresReceiveApplicationContext`, and
  `ReleasedNativeDuringSurfaceCreationIsReported`.
  Build a deterministic internal runtime fixture with barriers, event injection, and
  worker fault injection. **Do not add these controls to the public facade.**

- `window_surface_test.cpp` (4):
  `StandardNativeCreationMakeCurrentPresentAndDestroy`,
  `RepeatedSuccessfulCreateIsRejected`,
  `DestroyFromPartiallyInitializedOpenGLStateCleansUp`, and
  `UnsupportedWGLAndWin32SetupFailuresPropagatePrecisely`.
  Add a hidden-window fixture and internal WGL/Win32 shims. No public API is needed.

- `window_state_test.cpp` (2):
  `InactiveFocusedWidgetHasDocumentedDispatchBehavior` and
  `BackgroundColorMutationIsObservableInProducedSnapshot`.
  Test through the complete Widget/snapshot fixture. A public background-color getter
  is optional product ergonomics, not required for this test.

- `window_native_test.cpp` (1):
  `IdentifierPreservationNeedsObservableAccessor`.
  Add `identifier()` only if identity is a supported caller-facing concept; otherwise
  use friend/internal inspection.

### Engine and entity contracts (11)

- `engine_facade_test.cpp` (2):
  `ModifyingSystemsDuringUpdateNeedsStableTraversalContract` and
  `ModifyingEntitiesDuringUpdateNeedsStableTraversalContract`.
- `engine_test.cpp` (6):
  `DirectDispatchShouldRejectInteractionWhenOwnerIsInactive`,
  `RecursiveDestructionNeedsEntityOwnershipContract`,
  `ContextChangesShouldPropagateThroughExistingChildren`,
  `BehavioursShouldAppearInEngineScopedRegistries`,
  `AddOrRemoveDuringCallbacksNeedsStableTraversalContract`, and
  `ZInteractionOrderingNeedsExplicitEntityComparatorContract`.
- `entity_attachment_collection_test.cpp` (1):
  `EntityAndEnginePublicWrappersExerciseTheSameCollectionSemantics`.
- `registry_query_test.cpp` (2 of its 4 placeholders):
  `ContainParticipantSupportsTypeRegexPredicateAndReactiveEdits` and
  `ContainBehaviourSupportsTypeRegexPredicateAndReactiveEdits`.

These primarily require product contract decisions and implementation: snapshot vs.
deferred traversal, owning vs. non-owning children, propagation rules, registry
membership, and interaction ordering. The public Entity/Engine wrappers already give
the right testing surface; add APIs only when the chosen contract itself calls for
them.

### Rendering placeholders (52)

- `draw_color_mesh_render_command_test.cpp` (6):
  `EmptyMeshProducesNoVisiblePixels`, `SingleMeshRendersVertexColors`,
  `MultipleMeshesRenderIndependently`, `VertexAlphaBlendsWithExistingColor`,
  `VertexDepthParticipatesInDepthTesting`,
  `CommandsReuseSharedProgramAndGPUResources`.
- `draw_texture_mesh_render_command_test.cpp` (6):
  `TexturedMeshSamplesExpectedUVs`, `TextureAlphaBlendsWithExistingColor`,
  `TextureMeshDepthParticipatesInDepthTesting`,
  `ExecutionBindsTextureSamplerAtReservedBindingPoint`,
  `SourceTextureLifetimeIsExplicitlyExercised`, `NullTextureIsRejected`.
- `draw_font_render_command_test.cpp` (6):
  `GlyphMeshSamplesAtlasAndRendersGlyphColor`,
  `OutlineColorAndThicknessAffectRenderedPixels`,
  `ExecutionUploadsFontUniformData`, `ExecutionBindsAtlasSampler`,
  `GlyphMeshDepthParticipatesInDepthTesting`, `NullAtlasIsRejected`.
- `image_render_command_test.cpp` (6):
  `WholeTextureSectionFillsDestination`,
  `PartialTextureSectionUsesOnlyRequestedUVRegion`,
  `EmptyDestinationProducesNoVisiblePixels`,
  `ClippedDestinationOnlyAffectsVisiblePixels`, `OverlappingImagesRespectDepth`,
  `DifferentDestinationGeometriesMapTextureConsistently`.
- `nine_slice_render_command_test.cpp` (9):
  `UnstretchedThreeByThreeSheetPreservesAllNineRegions`,
  `StretchedDestinationPreservesCornerSizes`,
  `StretchedDestinationFillsHorizontalAndVerticalEdges`,
  `StretchedDestinationFillsCenterRegion`,
  `NonThreeByThreeSpriteSheetIsRejected`,
  `CornerWidthGreaterThanHalfDestinationIsRejected`,
  `CornerHeightGreaterThanHalfDestinationIsRejected`,
  `WideDestinationKeepsCornersAndFillsCenter`,
  `TallDestinationKeepsCornersAndFillsCenter`.
- `sprite_render_command_test.cpp` (5):
  `FirstSpriteCoordinatesSelectFirstSection`,
  `LastSpriteCoordinatesSelectLastSection`,
  `GeometryVariantsPreserveSelectedSprite`,
  `DepthVariantsParticipateInDepthTesting`,
  `OutOfRangeCoordinatesPropagateSpriteSheetFailure`.
- `text_render_command_test.cpp` (12):
  `Utf8TextRendersExpectedGlyphs`, `Utf32TextRendersExpectedGlyphs`,
  `AllHorizontalAndVerticalAlignmentsPlaceTextCorrectly`, `GlyphColorIsApplied`,
  `OutlineColorAndSizeAreApplied`, `DepthParticipatesInOverlapOrdering`,
  `EmptyTextProducesNoVisiblePixels`, `MissingGlyphUsesFontFallbackBehavior`,
  `AtlasGrowthKeepsExistingTextCommandValid`,
  `RepeatedExecutionProducesStablePixels`,
  `MultipleCommandsSharingFontRemainIndependent`,
  `FontLifetimeContractIsExercised`.
- `viewport_uniform_render_command_test.cpp` (2):
  `OffsetViewportBuildsAndBindsProjectionMatrix` and
  `RepeatedExecutionKeepsProjectionAndBindingStable`.

The placeholder messages saying the render APIs are “not included” are stale for this
repository: `ColorMesh2D`, `TextureMesh2D`, `Texture`, `Font`, `Program`, and
`OpenGLTestContext::capture()` exist, and other enabled tests already perform
off-screen capture. Implement pixel-visible behavior with the existing test library.
Use a test-only OpenGL spy/introspection seam only for exact binding points, uploaded
uniforms, and private resource-reuse assertions. **No general public render API should
be added for these 52 tests.**

### Graphics resources and failure injection (12)

- `buffer_resources_test.cpp` (1):
  `OpenGLCreationFailureNeedsDriverInjectionSeam`.
- `program_test.cpp` (1):
  `ShaderAndProgramCreationFailureNeedDriverInjectionSeam`.
- `texture_sampler_vertex_array_test.cpp` (6):
  `RenderTargetContractNeedsPublicConstructionSeam`,
  `ByteCountOverflowIsUnreachableWithPublic32BitDimensions`,
  `OpenGLAndPngFailureInjectionNeedDedicatedSeams`,
  `CreationFailureNeedsOpenGLInjectionSeam` (Sampler),
  `OversizedStrideNeedsDeterministicConfigurationSeam`, and
  `CreationFailureNeedsOpenGLInjectionSeam` (VertexArray).
- `mesh_test.cpp` (2):
  `VertexIndexOverflowNeedsDeterministicCapacitySeam` and
  `RenderingRequiresProgramAndFramebufferIntegrationFixture`.
- `image_test.cpp` (2):
  `OversizedEncodedSpanNeedsSyntheticAddressSpaceSeam` and
  `DualChannelAndRGBAFixturesNeedDeterministicEncodedAssets`.

Use internal injectable GL/PNG call tables for forced driver/decoder failures. Add
small deterministic image fixtures for channel conversion. Use the existing
framebuffer harness for mesh rendering. Requirements that need impossible multi-GB
objects or counters near their maximum should use a narrow internal arithmetic helper
or be removed as unreachable; do not publish counter setters. Public render-target
texture construction is justified only if it is a real supported feature.

### Data, JSON, diagnostics, and container contracts (11)

- `data_model_test.cpp` (2):
  `RowIDOverflowRequiresDeterministicPublicTestSeam` and
  `MutableDataAccessorIsNotPresentInSuppliedPublicAPI`.
- `exception_test.cpp` (1):
  `ExactFormattingContractRequiresImplementationSnapshot`.
- `json_error_test.cpp` (1):
  `ExactComposedWhatFormattingRequiresImplementationSnapshot`.
- `json_value_test.cpp` (2):
  `ExactParserOffsetContextContractRequiresParserImplementationSnapshot` and
  `SerializingInjectedNonFiniteValueHasNoPublicConstructionPath`.
- `polymorphic_container_test.cpp` (1):
  `UnregisteringAlreadyRemovedOwnedElementCannotBeCalledSafely`.
- `registry_query_test.cpp` (remaining 2):
  `CopyMoveRestrictionsNeedExplicitPublicContract` and
  `AttachmentNameChangesInvalidateRegexQueries`.
- `text_model_test.cpp` (1):
  `ViewAndDefaultDelegateRequireTheirDefinitions`.
- `versioned_trait_test.cpp` (1):
  `VersionWrapRequiresDeterministicPublicSeam`.

Exact formatting and compile-time copy/move restrictions can be tested against the
current implementation without a new API once their stability is declared. The
TextModel placeholder is stale: `defaultDelegate()` exists and is already exercised
in `data_model_view_test.cpp`; remove it if coverage is sufficient or replace it with
a non-duplicated test. Use internal seams for row-ID/version overflow. Do not provide a
mutable `DataModel` accessor or non-finite JSON injection unless callers need those
features. Repeated removal through a dangling owned pointer is not a safe test oracle;
prefer a stable handle/token API only if repeated removal is a supported operation.

## Conditional runtime skips (6)

These tests are **not disabled**. They remain discoverable and ran successfully on
this Windows audit:

- `ClipboardTest.StandardUnicodeRoundTripPreservesExistingText`
- `ClipboardTest.EmptyTextRoundTripsSafely`
- `ClipboardTest.MultilineAndSupplementaryCharactersRoundTrip`
- `ClipboardTest.NonTextClipboardContentReportsNoText`
- `ClipboardTest.ClipboardOpenContentionUsesNoThrowFailureReporting`
- `TextEditTest.ClipboardCopyCutPasteAndContentionPreserveTextAndSelection`

Keep their conditional skips. Five skip on non-Windows because Clipboard is Win32
backed; all six may skip rather than destroy pre-existing non-text clipboard content
that the helper cannot restore losslessly. That is responsible integration-test
behavior, not missing coverage caused by an API gap.

## Recommended order of work

1. Remove misleading declarations: replace the 101 always-skipped tests and the 3
   empty/deliberate-failure cases with tracked backlog items until they have real
   bodies. `TEST_BACKLOG_DISABLED.md` already serves this role.
2. Review and add the 33 immediately generated golden images. Diagnose the 2
   DebugOverlay terminations and implement the MenuBarBreak assertion.
3. Fix the 17 concrete behavior/contract cases; enable each test with its fix.
4. Implement the 52 render-command placeholders on the existing OpenGL test context.
5. Add shared **internal** injection fixtures for GL, Win32/WGL, runtime threads, and
   overflow arithmetic; do not create one-off public testing APIs.
6. Add a CI check that fails when `DISABLED_` is introduced without a linked issue and
   separately reports runtime `GTEST_SKIP()` counts.

## Reproduction commands

```powershell
# Inventory source-level disabled declarations
rg -n "TEST.*DISABLED_" tests/TU/TestSuite/srcs

# Run them (currently expected to fail/skip as described above)
build/testDebug/tests/TU/TestSuite/SparkleTestSuite.exe `
  --gtest_also_run_disabled_tests `
  --gtest_filter="*DISABLED_*"
```

The audit ran implemented cases individually as well, because unsafe/death-test cases
can terminate a combined run before Google Test prints its final summary.
