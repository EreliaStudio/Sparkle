# Remaining-backlog coverage audit

Audited on **2026-09-05**. The former remaining backlog contained 110 entries.
Its rule was to remove an entry once a test implements its behavior, independently
of enabled/disabled status. This audit follows that rule: implemented regression
specifications are distinguished from passing tests below.

## Validation

- 89 new enabled test cases pass in both `testDebug` and `testRelease`.
- Both build presets compile the new tests, including disabled specifications.
- Two checked-in goldens were rendered and visually inspected: the complete
  render-command scene and the alternate unchecked icon button.
- 15 safely executable regression specifications were explicitly run in Debug
  and fail as expected: 14 new specifications plus the expanded matrix test.
- The new NaN progress-bar specification and expanded invalid focus-channel
  specification remain unexecuted because current implementations can reach
  invalid numeric conversion or out-of-bounds access.
- A full Debug CTest run reported 23 failures in pre-existing test cases (823
  enabled cases at that point). This is not a claim that the entire suite passes.

## Coverage locations

Each row includes the new tests and, where relevant, the existing tests that
already implemented part of the requirement.

| Former inventory area | Coverage |
| --- | --- |
| Matrix non-finite perspective parameters | [matrix_test.cpp](srcs/matrix_test.cpp): NaN and both infinities in every parameter |
| Event focus-channel bounds | [event_test.cpp](srcs/event_test.cpp): exact boundary and values beyond it |
| Font, Glyph, Atlas | [font_test.cpp](srcs/font_test.cpp), [font_size_test.cpp](srcs/font_size_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): bundled Arial metrics, UTF-8/UTF-32, missing glyphs, malformed/empty/missing/locked files, caching, packing, growth, pixels, notifications, moves and GPU lifetime |
| GPUResource and GPUResourceCollection | [gpu_resource_test.cpp](srcs/gpu_resource_test.cpp): lazy creation, independent identities/contexts, retry after each failure stage, clear/recreation, both destruction orders, reentrant release during reclamation |
| Render-command integration | [render_command_integration_test.cpp](srcs/render_command_integration_test.cpp): every command family, golden scene, mixed depth, program rebinding, viewport/scissor/blend/depth/VAO state, repeatability and a second native context |
| DataModel View and Delegate | [data_model_view_test.cpp](srcs/data_model_view_test.cpp): row binding, stable IDs, selection, recycling, model edits, scrolling, zero/partial/exact pages, 1,000 rows, disabled rows, replacement/null models, invalid delegate products |
| TextModel Delegate and View | [data_model_view_test.cpp](srcs/data_model_view_test.cpp): default delegate, presentation, Unicode/empty/duplicate rows, row identities and reactive changes |
| Widget | [widget_test.cpp](srcs/widget_test.cpp): hierarchy, caches, reparenting/destruction, z-order, active/passive traversal, consumption, clipping, render passes, hint propagation and exception causes |
| Widget Style | [widget_style_reaction_test.cpp](srcs/widget_style_reaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): scalar fields, resource replacement, composed consumers, size hints and visible changes |
| ScalableWidget | [scalable_widget_test.cpp](srcs/scalable_widget_test.cpp): all edges/corners, grab offset, min/max, parent bounds, capture/cancellation, double-click and degenerate geometry |
| ContainerWidget | [container_widget_test.cpp](srcs/container_widget_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): oversized offset content and exact clipped pixel area |
| Panel | [panel_test.cpp](srcs/panel_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): nine-slice configuration, depth/corners/hints, invalid resources, restyling and pixels |
| ImageLabel | [image_label_test.cpp](srcs/image_label_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): wide clipping and overlapping depths in either insertion order |
| TextLabel | [text_label_test.cpp](srcs/text_label_test.cpp), [widget_style_reaction_test.cpp](srcs/widget_style_reaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): measurement, colors, padding, nine alignments, outline modes, Unicode/missing/empty text and existing snapshots after atlas growth |
| TextArea | [text_area_test.cpp](srcs/text_area_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): wrapped multiline text, explicit newlines, long words, width boundaries, line padding, colors and all alignments |
| PushButton | [push_button_test.cpp](srcs/push_button_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): click/state/hover, outside release, other buttons, double-click, cancellation, text/icon/flat/alignment/geometry matrix |
| CheckableIconButton | [checkable_icon_button_test.cpp](srcs/checkable_icon_button_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): alternate unchecked golden and both alternate states |
| CheckBox and DebugOverlay | [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): oversized content crossing a constrained visible region with nonempty rendering and no pixels outside the clip |
| RadioButton and RadioButtonGroup | [radio_button_test.cpp](srcs/radio_button_test.cpp), [radio_button_group_test.cpp](srcs/radio_button_group_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): sparse grid, object/cell/click selection, exclusivity, callbacks, invalid operations, geometry/style and visual states |
| ToggleSwitch | [toggle_switch_test.cpp](srcs/toggle_switch_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): click/notification, orientations, exact/interrupted timing, cancellation, padding/thumb boundaries, raw/nine-slice backgrounds and restyling |
| TextEdit | [text_edit_test.cpp](srcs/text_edit_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): keyboard/mouse editing and selection, clipboard round trips/contention, validation/callback failure, Unicode scrolling, blink boundaries, focus, obscuring/read-only, invalid resources and rendered states |
| SpinBox and NumericSpinBox | [numeric_controls_test.cpp](srcs/numeric_controls_test.cpp): signed/unsigned/floating types, controls, clamping/limits/steps, no-ops, overflow specifications, numeric grammar/extrema/intermediate/invalid forms, notifications and layout/style |
| SliderBar and ScrollBar | [slider_bar_test.cpp](srcs/slider_bar_test.cpp), [scroll_bar_test.cpp](srcs/scroll_bar_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp): both orientations, arrows/dragging, forwarding, endpoints, range/scale, tiny geometry, cancellation, mid-drag changes and invalid configurations |
| IScrollArea and ScrollArea | [scroll_area_test.cpp](srcs/scroll_area_test.cpp), [scroll_area_template_test.cpp](srcs/scroll_area_template_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp): content ownership, both axes, vertical wheel, visibility dependencies, replacement/hints/resize, offsets and iconset policy |
| ProgressBar | [progress_bar_test.cpp](srcs/progress_bar_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): all fill directions, exact pixel areas, clamping, non-finite policy and zero/tiny geometry |
| PromptPanel | [prompt_panel_test.cpp](srcs/prompt_panel_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): message/actions/policies/accessors/layout, errors and background rendering |
| PopupWidget | [popup_widget_test.cpp](srcs/popup_widget_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp): all alignments, offsets, explicit positions, root clipping, flags, inside/outside/Escape, repeated lifecycle, inactive dispatch, external content and coordinator cleanup |
| IInterfaceWindow and MenuBar | [interface_window_test.cpp](srcs/interface_window_test.cpp), [interface_window_menu_bar_test.cpp](srcs/interface_window_menu_bar_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_style_reaction_test.cpp](srcs/widget_style_reaction_test.cpp): title/resize drag, focus loss, restoration, root resize, content hints/padding, all eight enable combinations, callbacks and narrow/large layout |
| MessageBox and RequestMessageBox | [message_box_test.cpp](srcs/message_box_test.cpp), [request_message_box_test.cpp](srcs/request_message_box_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp): sizing/dynamic edits, information/request actions, replaced/empty callbacks, title-close policy, ordering, repeated close, contract lifetime and named diagnostics |
| Tooltip | [tooltip_test.cpp](srcs/tooltip_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp): exact delays, cursor following, leave/click/focus, all placements/root edges, maximum width, target lifecycle, repeated/manual operations and clipping/render pass |
| EngineWidget | [engine_widget_test.cpp](srcs/engine_widget_test.cpp), [engine_widget_integration_test.cpp](srcs/engine_widget_integration_test.cpp): all 14 active and four passive events, inactive/null/replaced engines, render-pass order, exception context and non-owning lifetime |
| Workspace | [workspace_test.cpp](srcs/workspace_test.cpp), [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp), [widget_style_reaction_test.cpp](srcs/widget_style_reaction_test.cpp): typed content, empty/populated menus, layout, z-order, hints and style propagation |

## Current API interpretations

- Font does not expose standalone line-height or pair-kerning methods. Its
  measurement uses glyph advances; the tests check this and the bundled font's
  fixed glyph metrics. Atlas lookup is lazy and non-const. Atlas objects are
  non-copyable/non-movable; moving Font preserves their addresses.
- GPUResourceCollection has no public typed registration/lookup or traversal
  callback API. Activation performs identity-based registration. The tests use
  that public path and exercise callback mutation through instance destruction
  during `reclaimReleased()`.
- `Widget::Style::lightNineSlice` currently has no consumer. Resource-reaction
  coverage concerns fields actually used by `applyStyle`; there is no reactive
  widget behavior to assert for that unused resource.
- Data-model rows may be selected programmatically while disabled; mouse
  selection rejects them. Text delegates still display disabled rows. Model
  edits rebuild items, while scrolling recycles them; row identity, not widget
  address, is stable across rebuilding/recycling.
- ScrollArea rejects an explicit null iconset; it has no supported icon-free
  state. Its wheel handler is vertical; horizontal scrolling is through its bar.
- SpinBox currently accepts inverted limits and applies maximum after minimum.
  NumericSpinBox accepts the non-finite tokens supported by `from_chars`.
- EngineWidget does not own its engine. Engine-first destruction requires
  explicit detachment, as documented in its public header.
- Render commands rebind the state they need; they do not promise to restore
  every GL binding to its value before command execution.

## Implemented disabled regression specifications

These contain executable assertions, rather than empty placeholders. Production
behavior was not changed as part of this test-completion task.

| Source | Remaining implementation defect captured by tests |
| --- | --- |
| [data_model_view_test.cpp](srcs/data_model_view_test.cpp) | Scroll offset after model shrink/viewport growth; atomic rejection of invalid replacement delegates; duplicate products (isolated in a subprocess to avoid unsafe double destruction) |
| [numeric_controls_test.cpp](srcs/numeric_controls_test.cpp) | Saturation at unsigned and signed extrema; signed tests use promoted `int8_t` arithmetic to avoid signed-overflow UB |
| [scalable_widget_test.cpp](srcs/scalable_widget_test.cpp) | Parent bounds and mouse-focus/deactivation cancellation |
| [widget_interaction_test.cpp](srcs/widget_interaction_test.cpp) | Push-button focus/deactivation cancellation, toggle focus cancellation, destroyed tooltip target, maximum tooltip width for unbreakable text |
| [widget_render_integration_test.cpp](srcs/widget_render_integration_test.cpp) | Existing snapshot UVs after atlas growth |
| [widget_style_reaction_test.cpp](srcs/widget_style_reaction_test.cpp) | Workspace style propagation |
| [progress_bar_test.cpp](srcs/progress_bar_test.cpp) | NaN ratio rejection before numeric conversion |
| [matrix_test.cpp](srcs/matrix_test.cpp), [event_test.cpp](srcs/event_test.cpp) | Expanded existing specifications for non-finite perspective arguments and invalid focus channels |

## Pre-existing full-suite failures

The full Debug run failed these existing tests:

```text
AnimationLabelTest.NullSheetAndOutOfSheetRangeAreRejected
ClipboardTest.ClipboardOpenContentionUsesNoThrowFailureReporting
CommandPanelTest.MissingOperationsReportRequestedName
EngineFacadeTest.DestroyingPopulatedEngineDestroysOwnedSystemsAndDetachesExternalEntities
EventTest.RepeatedIdenticalFocusRequestKeepsFirstRecord
EventTest.FocusChannelsAreIndependent
ExceptionTest.MultipleContextsRemainOrderedAndWhatIsStable
ReadTextFileTest.StandardUsagePreservesExactContentsIncludingNulAndNewlines
InherenceTraitTest.CircularHierarchyThrowsLogicError
InherenceTraitTest.DestroyingChildRemovesItFromLivingParent
InherenceTraitTest.DestroyingParentDetachesLivingChildren
InherenceTraitTest.DestroyingMiddleNodeDetachesFromParentAndOrphansChildren
JSONValueTest.MalformedJsonInputsAreRejected
KeyboardTest.OutOfRangeIndexThrows
LinearLayoutTest.PixelRoundingUsesAllSpaceWithoutGapsOrOverlap
MouseTest.InvalidButtonThrows
ProgramTest.SupportedPrimitivesAcceptZeroRawIndexedAndInstancedDraws
ProgramTest.DrawCountFirstVertexAndIndexOffsetOverflowAreRejected
QuaternionTest.EulerRoundTripPreservesRotationNearGimbalLock
WindowTest.ZeroClientDimensionsAreSupported
WindowTest.MinimumNonZeroClientDimensionsAreSupported
WindowTest.CallbackExceptionIsCapturedAndRethrown
WindowTest.DestructionMessagesReachCallback
```
