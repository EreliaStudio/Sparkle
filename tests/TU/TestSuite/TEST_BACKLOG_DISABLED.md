# Sparkle disabled unit-test backlog


Last audited: **2026-09-07** against `docs/unit_test_plan.md` and all sources compiled by `SparkleTestSuite`.

Current inventory: **111 disabled behaviors**, matching the 111 disabled test instances reported by Google Test.

This file lists behavior that does not have complete TU coverage. **Missing** means no test exists; **Disabled** means a `DISABLED_` specification exists but does not implement the behavior; **Partial** means a test implements only part of the behavior. Test validation and enabled/disabled status do not affect whether complete coverage is removed from this inventory.

Remove an item when its behavior is fully implemented by a test. If only part is implemented, retain the uncovered part.

## `spk::DataModel<T>`

1) **RowIDOverflowThrows** — Disabled  
Force row-ID exhaustion through a deterministic seam and verify `std::overflow_error` without model mutation.

## `spk::DataModelView<T>`

1) **ReactiveModelShrinkAndResizeClampScrollOffset** — Disabled

Shrink the model and resize the viewport, then verify the scroll offset and visible delegates are clamped and refreshed consistently.

2) **DuplicateDelegateWidgetIsRejected** — Disabled

Reject duplicate widget ownership from a delegate without double destruction or corrupting the existing view.

3) **InvalidDelegateReplacementPreservesExistingItems** — Disabled

Make delegate replacement transactional so a failing replacement leaves the existing items and delegate intact.

## `spk::JSON::Value`

1) **ParserDiagnosticsExposeExactOffsetAndContext** — Disabled  
Verify stable byte offsets, nearby source context, and wording for representative malformed documents.

## `spk::VersionedTrait`

1) **VersionWrapBehavior** — Disabled  
Place the counter near `UINT64_MAX` and verify the documented overflow/wrap behavior and callbacks.

## `spk::WinAPI::WakeEvent`

1) **CreateEventFailureReportsCodeAndOperation** — Disabled  
Inject `CreateEventW` failure and verify `std::system_error`, Win32 code, and operation name.

## `spk::WinAPI::Window::Class`

1) **UnregistrationFailureReportsCodeAndOperation** — Disabled  
Inject `UnregisterClassW` failure and verify diagnostics and safe cleanup.

## `spk::Window::State`

1) **InactiveFocusedWidgetDispatchBehavior** — Disabled  
Deactivate a focused widget and verify event routing and focus-release policy.

2) **BackgroundColorAppearsInSnapshot** — Disabled  
Build a snapshot after mutation and verify its clear command uses the configured color.

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

3) **BehavioursAppearInEngineScopedRegistries** — Disabled  
Attach/detach behaviours and verify engine registries never retain stale entries.

4) **CallbackMutationHasStableTraversal** — Disabled  
Add/remove during callbacks and verify deterministic visitation and coherent final state.

5) **ZInteractionOrderingUsesDocumentedComparator** — Disabled  
Verify overlapping unequal/equal-z entities dispatch in the documented order.

## `spk::Engine`

1) **SystemsMayMutateDuringUpdateSafely** — Disabled  
Add/remove systems during update and verify current/next-frame traversal rules.

2) **EntitiesMayMutateDuringUpdateSafely** — Disabled  
Add/remove entities during traversal and verify iteration, ownership, and registry state.

## `spk::Event`

1) **OutOfRangeFocusChannelHasDefinedBehavior** — Disabled

Pass an invalid focus-channel value and verify it is rejected before any channel-indexed access.

## `spk::Texture`

1) **OpenGLAndPNGFailuresAreReported** — Disabled
Inject texture/OpenGL/PNG failures and verify diagnostics and cleanup.

## `spk::Image`

1) **OversizedEncodedSpanIsRejected** — Disabled  
Use a synthetic address-space seam and verify rejection before decoder access.

2) **DualChannelAndRGBAAssetsDecodeDeterministically** — Disabled  
Add fixed encoded assets and verify dimensions, conversion, pixels, and orientation.

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

2) **RenderingUsesProgramAndFramebufferCorrectly** — Disabled  
Render deterministic meshes and verify pixels, indices, and repeatability.

## `spk::ViewportUniformRenderCommand`

1) **OffsetViewportBuildsAndBindsProjectionMatrix** — Disabled  
Execute an offset viewport and inspect its projection matrix and reserved UBO binding.

2) **RepeatedExecutionKeepsProjectionAndBindingStable** — Disabled  
Verify repeated execution uploads identical data without stale GL state.

## `spk::DebugOverlay`

1) **SingleCellRenderGolden** — Disabled

Render one populated cell and compare it with the reviewed golden image without terminating during fixture teardown.

2) **GridRenderGolden** — Disabled

Render a deterministic multi-cell grid and compare it with the reviewed golden image without lifetime failures.

## `spk::DrawColorMeshRenderCommand`

1) **EmptyMeshProducesNoVisiblePixels** — Disabled  
Verify no framebuffer change or GL error.

2) **SingleMeshRendersVertexColors** — Disabled  
Render deterministic geometry and compare sampled colors.

3) **MultipleMeshesRenderIndependently** — Disabled  
Verify every command region appears without cross-command corruption.

4) **VertexAlphaBlendsWithExistingColor** — Disabled  
Verify translucent geometry blends over a known clear color.

5) **VertexDepthParticipatesInDepthTesting** — Disabled  
Overlap depths and verify the library convention.

6) **CommandsReuseSharedProgramAndGPUResources** — Disabled  
Verify realization is reused within one render context.

## `spk::DrawTextureMeshRenderCommand`

1) **TexturedMeshSamplesExpectedUVs** — Disabled  
Render a patterned texture and verify UV sampling and orientation.

2) **TextureAlphaBlendsWithExistingColor** — Disabled  
Verify transparent/translucent texels blend correctly.

3) **TextureMeshDepthParticipatesInDepthTesting** — Disabled  
Verify depth ordering between overlapping textured meshes.

4) **ExecutionBindsReservedTextureSampler** — Disabled  
Verify texture/sampler binding at the reserved unit.

5) **SourceTextureLifetimeIsExplicit** — Disabled  
Exercise the documented ownership boundary without use-after-free.

6) **NullTextureIsRejected** — Disabled  
Verify the exact exception without issuing GL calls.

## `spk::DrawFontRenderCommand`

1) **GlyphMeshSamplesAtlasAndRendersColor** — Disabled  
Render a known atlas glyph and verify coverage and color.

2) **OutlineColorAndThicknessAffectPixels** — Disabled  
Verify outline coverage/color independently from glyph fill.

3) **ExecutionUploadsFontUniformData** — Disabled  
Inspect all uploaded font parameters.

4) **ExecutionBindsAtlasSampler** — Disabled  
Verify atlas/sampler binding at the reserved unit.

5) **GlyphMeshDepthParticipatesInDepthTesting** — Disabled  
Verify overlapping glyph depth behavior.

6) **NullAtlasIsRejected** — Disabled  
Verify `std::invalid_argument` without GL access.

## `spk::ImageRenderCommand`

1) **WholeTextureFillsDestination** — Disabled  
Verify patterned texture corners, center, and orientation.

2) **PartialTextureSectionUsesRequestedUVRegion** — Disabled  
Verify only the requested texels map to the destination.

3) **EmptyDestinationProducesNoPixels** — Disabled  
Verify zero-width/height destinations leave the target unchanged.

4) **ClippedDestinationOnlyAffectsVisiblePixels** — Disabled  
Verify pixels outside the scissor intersection remain untouched.

5) **OverlappingImagesRespectDepth** — Disabled  
Verify image depth ordering.

6) **DestinationGeometryMapsTextureConsistently** — Disabled  
Verify translated, scaled, wide, and tall destination mapping.

## `spk::SpriteRenderCommand`

1) **FirstSpriteCoordinatesSelectFirstSection** — Disabled  
Verify `{0,0}` renders the first section.

2) **LastSpriteCoordinatesSelectLastSection** — Disabled  
Verify maximum valid coordinates render the last section.

3) **GeometryVariantsPreserveSelectedSprite** — Disabled  
Verify translation/scaling preserve sprite selection.

4) **DepthVariantsParticipateInDepthTesting** — Disabled  
Verify overlapping sprite depth behavior.

5) **OutOfRangeCoordinatesPropagateFailure** — Disabled  
Verify the sprite-sheet bounds exception propagates unchanged.

## `spk::NineSliceRenderCommand`

1) **UnstretchedSheetPreservesAllNineRegions** — Disabled  
Render a deterministic 3x3 sheet and verify every region.

2) **StretchedDestinationPreservesCornerSizes** — Disabled  
Verify corners retain configured pixel sizes.

3) **StretchedDestinationFillsEdges** — Disabled  
Verify edge spans contain no seams or overlap.

4) **StretchedDestinationFillsCenter** — Disabled  
Verify the center fills the remaining interior.

5) **NonThreeByThreeSheetIsRejected** — Disabled  
Verify `std::invalid_argument` for incompatible sheets.

6) **OversizedCornerWidthIsRejected** — Disabled  
Reject corner width greater than half the destination.

7) **OversizedCornerHeightIsRejected** — Disabled  
Reject corner height greater than half the destination.

8) **WideDestinationKeepsCornersAndFillsCenter** — Disabled  
Verify wide-target regions and continuity.

9) **TallDestinationKeepsCornersAndFillsCenter** — Disabled  
Verify tall-target regions and continuity.

## `spk::TextRenderCommand`

1) **UTF8TextRendersExpectedGlyphs** — Disabled  
Render deterministic UTF-8 text and compare its golden image.

2) **UTF32TextRendersExpectedGlyphs** — Disabled  
Verify equivalent UTF-8/UTF-32 inputs produce identical glyphs.

3) **AllAlignmentsPlaceTextCorrectly** — Disabled  
Cover every horizontal/vertical alignment.

4) **GlyphColorIsApplied** — Disabled  
Verify visible glyph pixels use the requested color.

5) **OutlineColorAndSizeAreApplied** — Disabled  
Verify requested outline coverage and color.

6) **DepthParticipatesInOverlapOrdering** — Disabled  
Verify overlapping text depth behavior.

7) **EmptyTextProducesNoVisiblePixels** — Disabled  
Verify empty UTF-8/UTF-32 leave the target unchanged.

8) **MissingGlyphUsesFallbackBehavior** — Disabled  
Verify documented missing-glyph fallback or omission.

9) **AtlasGrowthKeepsExistingCommandValid** — Disabled  
Grow the atlas and verify an existing command renders identically.

10) **RepeatedExecutionProducesStablePixels** — Disabled  
Verify repeatability without leaked GPU state.

11) **CommandsSharingFontRemainIndependent** — Disabled  
Verify shared-font commands retain independent text, colors, and anchors.

12) **FontLifetimeContractIsExercised** — Disabled  
Verify ownership across command construction, execution, and destruction.

## `spk::SpinBox<T>`

1) **ArithmeticAtUnsignedExtremaSaturatesBeforeApplyingLimits** — Disabled

Exercise increments and decrements at unsigned extrema and verify checked arithmetic clamps before applying configured limits.

2) **SignedExtremaSaturateBeforeNarrowing** — Disabled

Exercise signed extrema and verify arithmetic saturates before narrowing or applying limits.

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

## `spk::Widget` rendering

1) **ExistingTextSnapshotSurvivesAtlasGrowthAndFontMove** — Disabled

Grow and move the font atlas after snapshot creation and verify the existing text snapshot remains valid or follows the documented invalidation contract.

## `spk::Workspace`

1) **ApplyStylePropagatesToContentAndMenus** — Disabled

Apply a new style after construction and verify existing content and menus receive it consistently.

## `spk::MenuBar::Menu::Break`

1) **ThreePartSeparatorRenderGolden** — Disabled  
Verify 3x1 regions, height, depth, size hint, and style changes.
