# Disabled-test decision catalogue

Last audited: **2026-09-07**. The source tree contains **111** disabled tests.

The catalogue is grouped by Google Test suite. Every case records its intended behavior, current blocker, and a specific recommendation. Failure injection, counters, and thread barriers remain internal unless callers need them as product features.

## `ApplicationTest`

### `DISABLED_CloseWhileReadyAndReleasingIsIdempotentlyCoordinated`

- **Goal:** Verify that close while ready and releasing is idempotently coordinated.
- **Why disabled:** Requires a deterministic runtime barrier exposing the Ready/Releasing milestones without polling private runtime state.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for close while ready and releasing is idempotently coordinated; do not expose scheduler controls publicly.

### `DISABLED_DuplicateRuntimeObjectsAndSnapshotEndpointsThrowLogicError`

- **Goal:** Verify that duplicate runtime objects and snapshot endpoints throw logic error.
- **Why disabled:** Runtime registration and snapshot endpoint registration are private implementation details. This contract needs an internal test seam or friend test fixture.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for duplicate runtime objects and snapshot endpoints throw logic error; do not expose scheduler controls publicly.

### `DISABLED_EventRoutingTargetsOnlyTheMatchingWindow`

- **Goal:** Verify that event routing targets only the matching window.
- **Why disabled:** Event publication is private to PlatformRuntime in the supplied public snapshot. Enable when the test harness exposes deterministic platform-event injection.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for event routing targets only the matching window; do not expose scheduler controls publicly.

### `DISABLED_QuitWhileRunIsActiveStopsAndJoinsAllRuntimes`

- **Goal:** Verify that quit while run is active stops and joins all runtimes.
- **Why disabled:** Requires the shared runtime synchronization fixture so quit() can be issued after run() has deterministically entered its platform loop.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for quit while run is active stops and joins all runtimes; do not expose scheduler controls publicly.

### `DISABLED_ReleasedNativeDuringSurfaceCreationIsReported`

- **Goal:** Verify that released native during surface creation is reported.
- **Why disabled:** Native/Surface parts are intentionally hidden behind Application. A runtime test seam is required to release the native frame between platform creation and surface creation.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for released native during surface creation is reported; do not expose scheduler controls publicly.

### `DISABLED_RuntimeFailuresReceiveApplicationContext`

- **Goal:** Verify that runtime failures receive application context.
- **Why disabled:** The requested spk::Exception/std::runtime_error contextual wrapping cannot be forced deterministically through the supplied facade alone; pair with platform/update/render fault injection.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for runtime failures receive application context; do not expose scheduler controls publicly.

### `DISABLED_StandardReadyWindowInitializationUpdateRenderAndClose`

- **Goal:** Verify that standard ready window initialization update render and close.
- **Why disabled:** The public facade has no readiness/update/render synchronization hook. Enable with the suite's shared window-runtime fixture so the test can deterministically wait for Ready, one update and one rendered frame before closeWindow().
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for standard ready window initialization update render and close; do not expose scheduler controls publicly.

### `DISABLED_WorkerExceptionsCrossTheRunBoundary`

- **Goal:** Verify that worker exceptions cross the run boundary.
- **Why disabled:** No public deterministic fault-injection hook is present for UpdateRuntime/RenderRuntime. Enable when a throwing test workload can be injected into a worker.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for worker exceptions cross the run boundary; do not expose scheduler controls publicly.

## `BehaviourTest`

### `DISABLED_DirectDispatchShouldRejectInteractionWhenOwnerIsInactive`

- **Goal:** Verify that direct dispatch should reject interaction when owner is inactive.
- **Why disabled:** Behaviour::_isAcceptingInteraction currently checks only the behaviour activation state; it does not inspect the owner activation state.
- **Proposal:** Decide and document the product contract for direct dispatch should reject interaction when owner is inactive, then implement it through existing APIs.

## `BufferGPUResourceTest`

### `DISABLED_OpenGLCreationFailureNeedsDriverInjectionSeam`

- **Goal:** Verify that open gl creation failure needs driver injection seam.
- **Why disabled:** The public buffer API has no injectable glGenBuffers failure seam.
- **Proposal:** Add a shared internal platform/driver seam for open gl creation failure needs driver injection seam and assert diagnostics and cleanup.

## `DataModelTest`

### `DISABLED_RowIDOverflowRequiresDeterministicPublicTestSeam`

- **Goal:** Verify safe rejection at the boundary described by row id overflow requires deterministic public test seam.
- **Why disabled:** _nextRowID is private and the supplied API exposes no deterministic seam for forcing uint64_t exhaustion.
- **Proposal:** Test checked arithmetic through a narrow internal helper; remove the requirement if the state is unreachable by design.

## `DataModelViewTest`

### `DISABLED_DuplicateDelegateWidgetIsRejected`

- **Goal:** Verify that duplicate delegate widget is rejected.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Validate widget identity before ownership adoption and use a fixture that cannot double-delete.

### `DISABLED_InvalidDelegateReplacementPreservesExistingItems`

- **Goal:** Verify that invalid delegate replacement preserves existing items.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Make replacement transactional by validating new items before committing.

### `DISABLED_ReactiveModelShrinkAndResizeClampScrollOffset`

- **Goal:** Verify that reactive model shrink and resize clamp scroll offset.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Clamp scrolling after model/viewport changes, repopulate visible items, and enable the regression.

## `DebugOverlayRenderTest`

### `DISABLED_Grid`

- **Goal:** Verify that grid.
- **Why disabled:** The process terminates in the overlay setup/lifetime path before image comparison.
- **Proposal:** Fix the overlay fixture/lifetime crash, generate and review its golden, then enable.

### `DISABLED_SingleCell`

- **Goal:** Verify that single cell.
- **Why disabled:** The process terminates in the overlay setup/lifetime path before image comparison.
- **Proposal:** Fix the overlay fixture/lifetime crash, generate and review its golden, then enable.

## `DrawColorMeshRenderCommandTest`

### `DISABLED_CommandsReuseSharedProgramAndGPUResources`

- **Goal:** Verify that the render command commands reuse shared program and gpu resources.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement commands reuse shared program and gpu resources with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_EmptyMeshProducesNoVisiblePixels`

- **Goal:** Verify that the render command empty mesh produces no visible pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement empty mesh produces no visible pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_MultipleMeshesRenderIndependently`

- **Goal:** Verify that the render command multiple meshes render independently.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement multiple meshes render independently with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_SingleMeshRendersVertexColors`

- **Goal:** Verify that the render command single mesh renders vertex colors.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement single mesh renders vertex colors with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_VertexAlphaBlendsWithExistingColor`

- **Goal:** Verify that the render command vertex alpha blends with existing color.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement vertex alpha blends with existing color with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_VertexDepthParticipatesInDepthTesting`

- **Goal:** Verify that the render command vertex depth participates in depth testing.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement vertex depth participates in depth testing with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `DrawFontRenderCommandTest`

### `DISABLED_ExecutionBindsAtlasSampler`

- **Goal:** Verify that the render command execution binds atlas sampler.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement execution binds atlas sampler with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_ExecutionUploadsFontUniformData`

- **Goal:** Verify that the render command execution uploads font uniform data.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement execution uploads font uniform data with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_GlyphMeshDepthParticipatesInDepthTesting`

- **Goal:** Verify that the render command glyph mesh depth participates in depth testing.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement glyph mesh depth participates in depth testing with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_GlyphMeshSamplesAtlasAndRendersGlyphColor`

- **Goal:** Verify that the render command glyph mesh samples atlas and renders glyph color.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement glyph mesh samples atlas and renders glyph color with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_NullAtlasIsRejected`

- **Goal:** Verify that the render command null atlas is rejected.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement null atlas is rejected with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_OutlineColorAndThicknessAffectRenderedPixels`

- **Goal:** Verify that the render command outline color and thickness affect rendered pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement outline color and thickness affect rendered pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `DrawTextureMeshRenderCommandTest`

### `DISABLED_ExecutionBindsTextureSamplerAtReservedBindingPoint`

- **Goal:** Verify that the render command execution binds texture sampler at reserved binding point.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement execution binds texture sampler at reserved binding point with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_NullTextureIsRejected`

- **Goal:** Verify that the render command null texture is rejected.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement null texture is rejected with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_SourceTextureLifetimeIsExplicitlyExercised`

- **Goal:** Verify that the render command source texture lifetime is explicitly exercised.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement source texture lifetime is explicitly exercised with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_TextureAlphaBlendsWithExistingColor`

- **Goal:** Verify that the render command texture alpha blends with existing color.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement texture alpha blends with existing color with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_TextureMeshDepthParticipatesInDepthTesting`

- **Goal:** Verify that the render command texture mesh depth participates in depth testing.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement texture mesh depth participates in depth testing with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_TexturedMeshSamplesExpectedUVs`

- **Goal:** Verify that the render command textured mesh samples expected u vs.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement textured mesh samples expected u vs with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `EngineFacadeTest`

### `DISABLED_ModifyingEntitiesDuringUpdateNeedsStableTraversalContract`

- **Goal:** Verify that modifying entities during update needs stable traversal contract.
- **Why disabled:** Entity child traversal is live and non-owning; mutation during callbacks cannot be tested without risking iterator invalidation.
- **Proposal:** Decide and document the product contract for modifying entities during update needs stable traversal contract, then implement it through existing APIs.

### `DISABLED_ModifyingSystemsDuringUpdateNeedsStableTraversalContract`

- **Goal:** Verify that modifying systems during update needs stable traversal contract.
- **Why disabled:** Engine iterates its live system vector; the API exposes no deferred-mutation or snapshot traversal contract.
- **Proposal:** Decide and document the product contract for modifying systems during update needs stable traversal contract, then implement it through existing APIs.

## `EntityAttachmentCollectionTest`

### `DISABLED_EntityAndEnginePublicWrappersExerciseTheSameCollectionSemantics`

- **Goal:** Verify that entity and engine public wrappers exercise the same collection semantics.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Replace the placeholder with an executable test for entity and engine public wrappers exercise the same collection semantics using current public behavior; add only a narrow internal seam if needed.

## `EntityTest`

### `DISABLED_AddOrRemoveDuringCallbacksNeedsStableTraversalContract`

- **Goal:** Verify that add or remove during callbacks needs stable traversal contract.
- **Why disabled:** Entity traverses live attachment/child vectors directly; add/remove during callbacks has no explicit snapshot or deferred-edit contract.
- **Proposal:** Decide and document the product contract for add or remove during callbacks needs stable traversal contract, then implement it through existing APIs.

### `DISABLED_BehavioursShouldAppearInEngineScopedRegistries`

- **Goal:** Verify that behaviours should appear in engine scoped registries.
- **Why disabled:** Behaviour does not currently inherit Registry<Engine *, Behaviour>::Object, so behaviour registry queries remain empty.
- **Proposal:** Decide and document the product contract for behaviours should appear in engine scoped registries, then implement it through existing APIs.

### `DISABLED_ContextChangesShouldPropagateThroughExistingChildren`

- **Goal:** Verify that context changes should propagate through existing children.
- **Why disabled:** Entity currently propagates geometry through existing children, but not context changes or registry membership.
- **Proposal:** Decide and document the product contract for context changes should propagate through existing children, then implement it through existing APIs.

### `DISABLED_RecursiveDestructionNeedsEntityOwnershipContract`

- **Goal:** Verify that recursive destruction needs entity ownership contract.
- **Why disabled:** Entity parent/child relationships are non-owning; the current API does not recursively destroy child entities.
- **Proposal:** Decide and document the product contract for recursive destruction needs entity ownership contract, then implement it through existing APIs.

### `DISABLED_ZInteractionOrderingNeedsExplicitEntityComparatorContract`

- **Goal:** Verify that z interaction ordering needs explicit entity comparator contract.
- **Why disabled:** Entity currently inherits the default pointer-order child comparator; no z/depth interaction ordering contract is exposed.
- **Proposal:** Decide and document the product contract for z interaction ordering needs explicit entity comparator contract, then implement it through existing APIs.

## `EventTest`

### `DISABLED_OutOfRangeFocusChannelHasDefinedBehavior`

- **Goal:** Verify that out of range focus channel has defined behavior.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Validate channels before indexing or make invalid values unrepresentable, then test safe rejection.

## `ImageRenderCommandTest`

### `DISABLED_ClippedDestinationOnlyAffectsVisiblePixels`

- **Goal:** Verify that the render command clipped destination only affects visible pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement clipped destination only affects visible pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_DifferentDestinationGeometriesMapTextureConsistently`

- **Goal:** Verify that the render command different destination geometries map texture consistently.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement different destination geometries map texture consistently with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_EmptyDestinationProducesNoVisiblePixels`

- **Goal:** Verify that the render command empty destination produces no visible pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement empty destination produces no visible pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_OverlappingImagesRespectDepth`

- **Goal:** Verify that the render command overlapping images respect depth.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement overlapping images respect depth with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_PartialTextureSectionUsesOnlyRequestedUVRegion`

- **Goal:** Verify that the render command partial texture section uses only requested uv region.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement partial texture section uses only requested uv region with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_WholeTextureSectionFillsDestination`

- **Goal:** Verify that the render command whole texture section fills destination.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement whole texture section fills destination with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `ImageTest`

### `DISABLED_DualChannelAndRGBAFixturesNeedDeterministicEncodedAssets`

- **Goal:** Verify that dual channel and rgba fixtures need deterministic encoded assets.
- **Why disabled:** The repository currently provides no deterministic two-channel or transparent encoded image fixture.
- **Proposal:** Replace the placeholder with an executable test for dual channel and rgba fixtures need deterministic encoded assets using current public behavior; add only a narrow internal seam if needed.

### `DISABLED_OversizedEncodedSpanNeedsSyntheticAddressSpaceSeam`

- **Goal:** Verify safe rejection at the boundary described by oversized encoded span needs synthetic address space seam.
- **Why disabled:** A span larger than INT_MAX requires a genuinely addressable multi-gigabyte range; Image exposes no decoder-size seam.
- **Proposal:** Test checked arithmetic through a narrow internal helper; remove the requirement if the state is unreachable by design.

## `JSONValueTest`

### `DISABLED_ExactParserOffsetContextContractRequiresParserImplementationSnapshot`

- **Goal:** Verify that exact parser offset context contract requires parser implementation snapshot.
- **Why disabled:** The backlog requires useful parse offsets/context, but the supplied Value header does not define the parser diagnostic format. The enabled parse-error test still verifies a non-empty diagnostic.
- **Proposal:** Replace the placeholder with an executable test for exact parser offset context contract requires parser implementation snapshot using current public behavior; add only a narrow internal seam if needed.

## `MenuBarBreakRenderTest`

### `DISABLED_ThreePartSeparator`

- **Goal:** Verify that three part separator.
- **Why disabled:** The body is empty and currently provides no coverage.
- **Proposal:** Implement capture/comparison, generate and review the separator golden, then enable.

## `MeshTest`

### `DISABLED_RenderingRequiresProgramAndFramebufferIntegrationFixture`

- **Goal:** Verify that rendering requires program and framebuffer integration fixture.
- **Why disabled:** Mesh owns buffer layout only; rendering requires Program and render-command fixtures covered by later integration targets.
- **Proposal:** Replace the placeholder with an executable test for rendering requires program and framebuffer integration fixture using current public behavior; add only a narrow internal seam if needed.

### `DISABLED_VertexIndexOverflowNeedsDeterministicCapacitySeam`

- **Goal:** Verify safe rejection at the boundary described by vertex index overflow needs deterministic capacity seam.
- **Why disabled:** Reaching more than uint32_t vertices would require impractical memory; Builder exposes no injectable count seam.
- **Proposal:** Test checked arithmetic through a narrow internal helper; remove the requirement if the state is unreachable by design.

## `NineSliceRenderCommandTest`

### `DISABLED_CornerHeightGreaterThanHalfDestinationIsRejected`

- **Goal:** Verify that the render command corner height greater than half destination is rejected.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement corner height greater than half destination is rejected with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_CornerWidthGreaterThanHalfDestinationIsRejected`

- **Goal:** Verify that the render command corner width greater than half destination is rejected.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement corner width greater than half destination is rejected with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_NonThreeByThreeSpriteSheetIsRejected`

- **Goal:** Verify that the render command non three by three sprite sheet is rejected.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement non three by three sprite sheet is rejected with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_StretchedDestinationFillsCenterRegion`

- **Goal:** Verify that the render command stretched destination fills center region.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement stretched destination fills center region with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_StretchedDestinationFillsHorizontalAndVerticalEdges`

- **Goal:** Verify that the render command stretched destination fills horizontal and vertical edges.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement stretched destination fills horizontal and vertical edges with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_StretchedDestinationPreservesCornerSizes`

- **Goal:** Verify that the render command stretched destination preserves corner sizes.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement stretched destination preserves corner sizes with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_TallDestinationKeepsCornersAndFillsCenter`

- **Goal:** Verify that the render command tall destination keeps corners and fills center.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement tall destination keeps corners and fills center with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_UnstretchedThreeByThreeSheetPreservesAllNineRegions`

- **Goal:** Verify that the render command unstretched three by three sheet preserves all nine regions.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement unstretched three by three sheet preserves all nine regions with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_WideDestinationKeepsCornersAndFillsCenter`

- **Goal:** Verify that the render command wide destination keeps corners and fills center.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement wide destination keeps corners and fills center with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `ProgramTest`

### `DISABLED_ShaderAndProgramCreationFailureNeedDriverInjectionSeam`

- **Goal:** Verify that shader and program creation failure need driver injection seam.
- **Why disabled:** Compilation diagnostics are covered, but forcing glCreateShader/glCreateProgram to return zero needs an injectable GL seam.
- **Proposal:** Add a shared internal platform/driver seam for shader and program creation failure need driver injection seam and assert diagnostics and cleanup.

## `PushButtonInteractionTest`

### `DISABLED_DeactivationCancelsPendingClick`

- **Goal:** Verify that deactivation cancels pending click.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Clear pending interaction/capture state on focus loss and deactivation, then enable.

### `DISABLED_WindowFocusLossCancelsPendingClick`

- **Goal:** Verify that window focus loss cancels pending click.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Clear pending interaction/capture state on focus loss and deactivation, then enable.

## `RegistryQueryTest`

### `DISABLED_AttachmentNameChangesInvalidateRegexQueries`

- **Goal:** Verify that attachment name changes invalidate regex queries.
- **Why disabled:** The supplied ContainParticipant/ContainBehaviour implementation subscribes only to add/remove events; no name-edition subscription is visible. Keep this disabled until regex queries invalidate on attachment rename as required by the backlog.
- **Proposal:** Implement attachment name changes invalidate regex queries through current query/entity APIs, including cache invalidation.

### `DISABLED_ContainBehaviourSupportsTypeRegexPredicateAndReactiveEdits`

- **Goal:** Verify that contain behaviour supports type regex predicate and reactive edits.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement contain behaviour supports type regex predicate and reactive edits through current query/entity APIs, including cache invalidation.

### `DISABLED_ContainParticipantSupportsTypeRegexPredicateAndReactiveEdits`

- **Goal:** Verify that contain participant supports type regex predicate and reactive edits.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement contain participant supports type regex predicate and reactive edits through current query/entity APIs, including cache invalidation.

## `SamplerTest`

### `DISABLED_CreationFailureNeedsOpenGLInjectionSeam`

- **Goal:** Verify that creation failure needs open gl injection seam.
- **Why disabled:** The public API cannot force glGenSamplers to return zero.
- **Proposal:** Add a shared internal platform/driver seam for creation failure needs open gl injection seam and assert diagnostics and cleanup.

## `ScalableWidgetTest`

### `DISABLED_MouseFocusReleaseAndDeactivationCancelResizing`

- **Goal:** Verify that mouse focus release and deactivation cancel resizing.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Clear pending interaction/capture state on focus loss and deactivation, then enable.

### `DISABLED_ParentBoundsConstrainAllResizeDirections`

- **Goal:** Verify that parent bounds constrain all resize directions.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Confirm the resize contract, implement bounds/cancellation behavior, and enable.

## `SpinBoxTest`

### `DISABLED_ArithmeticAtUnsignedExtremaSaturatesBeforeApplyingLimits`

- **Goal:** Verify that arithmetic at unsigned extrema saturates before applying limits.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Use checked saturating arithmetic before narrowing/applying limits, then enable.

### `DISABLED_SignedExtremaSaturateBeforeNarrowing`

- **Goal:** Verify that signed extrema saturate before narrowing.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Use checked saturating arithmetic before narrowing/applying limits, then enable.

## `SpriteRenderCommandTest`

### `DISABLED_DepthVariantsParticipateInDepthTesting`

- **Goal:** Verify that the render command depth variants participate in depth testing.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement depth variants participate in depth testing with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_FirstSpriteCoordinatesSelectFirstSection`

- **Goal:** Verify that the render command first sprite coordinates select first section.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement first sprite coordinates select first section with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_GeometryVariantsPreserveSelectedSprite`

- **Goal:** Verify that the render command geometry variants preserve selected sprite.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement geometry variants preserve selected sprite with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_LastSpriteCoordinatesSelectLastSection`

- **Goal:** Verify that the render command last sprite coordinates select last section.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement last sprite coordinates select last section with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_OutOfRangeCoordinatesPropagateSpriteSheetFailure`

- **Goal:** Verify that the render command out of range coordinates propagate sprite sheet failure.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement out of range coordinates propagate sprite sheet failure with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `TextRenderCommandTest`

### `DISABLED_AllHorizontalAndVerticalAlignmentsPlaceTextCorrectly`

- **Goal:** Verify that the render command all horizontal and vertical alignments place text correctly.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement all horizontal and vertical alignments place text correctly with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_AtlasGrowthKeepsExistingTextCommandValid`

- **Goal:** Verify that the render command atlas growth keeps existing text command valid.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement atlas growth keeps existing text command valid with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_DepthParticipatesInOverlapOrdering`

- **Goal:** Verify that the render command depth participates in overlap ordering.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement depth participates in overlap ordering with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_EmptyTextProducesNoVisiblePixels`

- **Goal:** Verify that the render command empty text produces no visible pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement empty text produces no visible pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_FontLifetimeContractIsExercised`

- **Goal:** Verify that the render command font lifetime contract is exercised.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement font lifetime contract is exercised with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_GlyphColorIsApplied`

- **Goal:** Verify that the render command glyph color is applied.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement glyph color is applied with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_MissingGlyphUsesFontFallbackBehavior`

- **Goal:** Verify that the render command missing glyph uses font fallback behavior.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement missing glyph uses font fallback behavior with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_MultipleCommandsSharingFontRemainIndependent`

- **Goal:** Verify that the render command multiple commands sharing font remain independent.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement multiple commands sharing font remain independent with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_OutlineColorAndSizeAreApplied`

- **Goal:** Verify that the render command outline color and size are applied.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement outline color and size are applied with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_RepeatedExecutionProducesStablePixels`

- **Goal:** Verify that the render command repeated execution produces stable pixels.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement repeated execution produces stable pixels with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_Utf32TextRendersExpectedGlyphs`

- **Goal:** Verify that the render command utf32 text renders expected glyphs.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement utf32 text renders expected glyphs with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_Utf8TextRendersExpectedGlyphs`

- **Goal:** Verify that the render command utf8 text renders expected glyphs.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement utf8 text renders expected glyphs with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `TextureTest`

### `DISABLED_OpenGLAndPngFailureInjectionNeedDedicatedSeams`

- **Goal:** Verify that open gl and png failure injection need dedicated seams.
- **Why disabled:** The API cannot inject glGenTextures or stb_image_write failures deterministically.
- **Proposal:** Add a shared internal platform/driver seam for open gl and png failure injection need dedicated seams and assert diagnostics and cleanup.

## `ToggleSwitchInteractionTest`

### `DISABLED_FocusLossCancelsPendingToggle`

- **Goal:** Verify that focus loss cancels pending toggle.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Clear pending interaction/capture state on focus loss and deactivation, then enable.

## `TooltipInteractionTest`

### `DISABLED_DestroyedTargetIsForgotten`

- **Goal:** Verify that destroyed target is forgotten.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Track target destruction with an observer token instead of an unchecked raw lifetime.

### `DISABLED_MaximumWidthClampsUnbreakableTextIncludingZero`

- **Goal:** Verify that maximum width clamps unbreakable text including zero.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Define zero-width/long-word wrapping, implement it, and enable the assertions.

## `VersionedTraitTest`

### `DISABLED_VersionWrapRequiresDeterministicPublicSeam`

- **Goal:** Verify that version wrap requires deterministic public seam.
- **Why disabled:** `VersionedTrait` exposes no public seam for placing its version near `UINT64_MAX`.
- **Proposal:** Replace the placeholder with an executable test for version wrap requires deterministic public seam using current public behavior; add only a narrow internal seam if needed.

## `VertexArrayTest`

### `DISABLED_CreationFailureNeedsOpenGLInjectionSeam`

- **Goal:** Verify that creation failure needs open gl injection seam.
- **Why disabled:** The public API cannot force glGenVertexArrays to return zero.
- **Proposal:** Add a shared internal platform/driver seam for creation failure needs open gl injection seam and assert diagnostics and cleanup.

### `DISABLED_OversizedStrideNeedsDeterministicConfigurationSeam`

- **Goal:** Verify safe rejection at the boundary described by oversized stride needs deterministic configuration seam.
- **Why disabled:** Constructing a stride above GLsizei max through padding would require a multi-gigabyte logical layout; no synthetic seam exists.
- **Proposal:** Add a shared internal platform/driver seam for oversized stride needs deterministic configuration seam and assert diagnostics and cleanup.

## `ViewportUniformRenderCommandTest`

### `DISABLED_OffsetViewportBuildsAndBindsProjectionMatrix`

- **Goal:** Verify that the render command offset viewport builds and binds projection matrix.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement offset viewport builds and binds projection matrix with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

### `DISABLED_RepeatedExecutionKeepsProjectionAndBindingStable`

- **Goal:** Verify that the render command repeated execution keeps projection and binding stable.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Implement repeated execution keeps projection and binding stable with the existing OpenGL capture fixture; use internal GL introspection only for exact bindings/reuse.

## `WakeEventTest`

### `DISABLED_CreateEventFailureReportsCodeAndOperation`

- **Goal:** Verify that create event failure reports code and operation.
- **Why disabled:** The body deliberately fails because the required Win32 failure cannot be forced deterministically.
- **Proposal:** Add a shared internal platform/driver seam for create event failure reports code and operation and assert diagnostics and cleanup.

## `WidgetRenderIntegrationTest`

### `DISABLED_ExistingTextSnapshotSurvivesAtlasGrowthAndFontMove`

- **Goal:** Verify that existing text snapshot survives atlas growth and font move.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Repair atlas/font invalidation, or document snapshot invalidation and rewrite the contract.

## `WindowClassTest`

### `DISABLED_UnregistrationFailureReportsCodeAndOperation`

- **Goal:** Verify that unregistration failure reports code and operation.
- **Why disabled:** The body deliberately fails because the required Win32 failure cannot be forced deterministically.
- **Proposal:** Add a shared internal platform/driver seam for unregistration failure reports code and operation and assert diagnostics and cleanup.

## `WindowStateTest`

### `DISABLED_BackgroundColorMutationIsObservableInProducedSnapshot`

- **Goal:** Verify that background color mutation is observable in produced snapshot.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Replace the placeholder with an executable test for background color mutation is observable in produced snapshot using current public behavior; add only a narrow internal seam if needed.

### `DISABLED_InactiveFocusedWidgetHasDocumentedDispatchBehavior`

- **Goal:** Verify that inactive focused widget has documented dispatch behavior.
- **Why disabled:** The backlog requires inactive-focused-widget coverage, but the supplied window.hpp only forward-declares Widget. Enable this case beside the Widget tests once the complete Widget API is part of this handoff.
- **Proposal:** Replace the placeholder with an executable test for inactive focused widget has documented dispatch behavior using current public behavior; add only a narrow internal seam if needed.

## `WindowSurfaceTest`

### `DISABLED_DestroyFromPartiallyInitializedOpenGLStateCleansUp`

- **Goal:** Verify that destroy from partially initialized open gl state cleans up.
- **Why disabled:** The public API cannot intentionally stop WGL setup between device-context/bootstrap/render-context stages. Enable with a fault-injection WinAPI/OpenGL fixture.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for destroy from partially initialized open gl state cleans up; do not expose scheduler controls publicly.

### `DISABLED_RepeatedSuccessfulCreateIsRejected`

- **Goal:** Verify that repeated successful create is rejected.
- **Why disabled:** Requires the shared hidden native-window fixture. After one successful create(), a second create() must throw std::logic_error.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for repeated successful create is rejected; do not expose scheduler controls publicly.

### `DISABLED_StandardNativeCreationMakeCurrentPresentAndDestroy`

- **Goal:** Verify that standard native creation make current present and destroy.
- **Why disabled:** The placeholder came from a partial source handoff; required repository types now exist, but the executable fixture was never implemented.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for standard native creation make current present and destroy; do not expose scheduler controls publicly.

### `DISABLED_UnsupportedWGLAndWin32SetupFailuresPropagatePrecisely`

- **Goal:** Verify that unsupported wgl and win32 setup failures propagate precisely.
- **Why disabled:** Capability-aware WGL failure injection is not exposed by the supplied public snapshots. Cover std::runtime_error/std::system_error when the platform test fixture can substitute WGL/Win32 capabilities.
- **Proposal:** Use an internal runtime/Win32 barrier or fault fixture for unsupported wgl and win32 setup failures propagate precisely; do not expose scheduler controls publicly.

## `WorkspaceTest`

### `DISABLED_ApplyStylePropagatesToContentAndMenus`

- **Goal:** Verify that apply style propagates to content and menus.
- **Why disabled:** The test is implemented, but current product behavior does not satisfy its assertions safely.
- **Proposal:** Propagate styles to existing children, or narrow the `applyStyle` contract and rewrite the test.
