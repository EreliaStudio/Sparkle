# Sparkle widget extension API plan

Status: remaining work after implementation audit (2026-08-25).

This document now contains only unfinished work. The implemented declarations and behavior for `CheckBox`, `RadioButton`, `RadioButtonGroup`, `ProgressBar`, `LabeledProgressBar`, `DataModel<T>::View`, `PopupWidget`, `ComboBox`, `Tooltip`, `ToggleSwitch`, render-pass inheritance, pointer observation, and `TextEdit` selection/clipboard support have been removed from the plan. The public headers are the source of truth for those completed APIs.

The remaining work is split into correctness work needed to finish the original design and optional follow-up features that were deliberately deferred from the first pass.

---

## 1. Root-hosted transient widget safety

Priority: high.

`PopupWidget` and `Tooltip` keep non-owning pointers to widgets that may be reparented, deactivated, moved to another root, or destroyed. Their current parent-edition handling hides a tooltip, but it does not fully protect either transient type from a stale target pointer.

### `PopupWidget`

- Observe the anchor's parent changes and lifetime in the same way popup content is observed.
- Close the popup and clear its anchor when the anchor is detached or destroyed.
- Resolve the hosting root from `anchorWidget()->root()`, not from the popup's current root. This matters after a popup has already been reparented to a previous root.
- Safely migrate a closed popup when its anchor moves to another root.
- Close an open popup when the anchor or any of its ancestors becomes inactive.
- Recompute anchored geometry while the popup is open if the anchor or root geometry changes.
- Close an open popup when its window loses focus.
- Keep outside-press consumption and the one-popup-per-root coordinator behavior unchanged.

### `Tooltip`

- Clear the target pointer when the target is detached or destroyed; hiding alone leaves a dangling pointer.
- When the target moves to another root, hide first and migrate the tooltip before it can update or render in the old root traversal.
- Reject or immediately hide `show()` when the target is not effectively active through its complete ancestor chain.
- Recompute `AboveTarget`, `BelowTarget`, and `Automatic` geometry while a shown target moves or resizes.
- Preserve the existing one-visible-tooltip-per-root rule and passive pointer observation.

### Acceptance criteria

- Destroying an anchor or target while its transient is open/shown never dereferences freed memory.
- Switching away from a tab containing an open combo closes its popup.
- Reparenting an anchor or target between roots leaves no transient attached to the old root.
- Moving or resizing an anchor keeps an open popup or shown tooltip correctly aligned and constrained.

---

## 2. `DataModel<T>::View` model updates, scrolling, and delegate invalidation

Priority: high for correctness; medium for performance and presentation.

### Correctness

- Clamp `_scrollOffset` whenever geometry, row count, delegate, or row extents change. Removing rows while scrolled near the end must not leave the viewport beyond the content.
- Define and validate the model notification range contract: `first` and `count` must describe the post-operation inserted/changed range or the pre-operation removed range.
- Preserve selection by stable `RowID` after insert, remove, change, and reset notifications, emitting exactly one empty-selection notification only when that identity disappears.
- Keep `Selection::widget` pinned, non-null, and bound to the selected row after every model or delegate update.

### Incremental updates and virtualization

The current notification handlers call `_rebuildItems()`, destroy the recycled widgets, and scan every model row. This does not yet satisfy the planned incremental-update behavior for large models.

- Apply inserted, removed, and changed ranges without discarding the entire delegate pool.
- Rebind only affected visible rows plus the pinned selection row.
- Cache row extents or prefix offsets so hit testing, scrolling, and visible-range calculation do not require a full `rowCount()` scan on every update.
- Keep the number of live delegate widgets bounded by the viewport plus the selected-row pin, including for a 100,000-row model.

### Delegate changes and default presentation

- Add an explicit `DataModel<T>::View::refreshDelegate()`/`invalidateDelegate()` operation, or a delegate edition contract, so changing row height, font, colors, or padding after `setModel()` immediately rebinds rows and recomputes metrics.
- Make `TextModel::Delegate` visibly distinguish selected and disabled rows. Its current `bindItem(..., bool selected)` ignores the selection argument.

### Acceptance criteria

- Scrolling remains within valid content bounds after every model mutation and resize.
- A model range change does not recreate unaffected delegate widgets.
- Changing default delegate styling after rows exist updates the visible rows immediately.
- Selected, unselected, and disabled text rows are visually distinguishable with configurable styling.

---

## 3. `ComboBox` delegate invalidation and popup sizing

Priority: high.

- Recalculate popup height and width when delegate metrics change after the delegate has already created rows.
- Measure every delegated row's minimum width, not only the currently selected display item, so the popup can grow beyond the collapsed combo width when required.
- Separate user row activation from programmatic `setSelectedRow()`. Selecting through the popup should close it; a programmatic selection should update state and notification without implicitly closing an already open popup unless that behavior is explicitly documented.
- Keep selection identity centralized in the contained `DataModel<std::string>::View`; do not add a second ComboBox selection state.

### Acceptance criteria

- Delegate metric changes update popup geometry without reopening it.
- Long delegated rows are not clipped merely because the collapsed field is narrower.
- Programmatic and user-driven selection have documented, deterministic close behavior.

---

## 4. `TextEdit` editing atomicity and clipboard portability

Priority: high for atomic cut; optional for additional platforms.

### Atomic cut

`cutSelection()` currently writes to the clipboard before `_replaceSelection()` validates deletion. If validation rejects the candidate, the text remains unchanged but the clipboard has already changed.

- Build and validate the deletion candidate before writing to the clipboard.
- If validation fails, leave text, selection, caret, edition notifications, and clipboard unchanged.
- If clipboard writing fails, leave the edit unchanged.
- On success, write once, edit once, emit one selection change if needed, and emit one edition notification.

### Clipboard portability

The Windows `CF_UNICODETEXT` implementation is complete for the initial platform target. Non-Windows functions are currently stubs.

- Add native Unicode clipboard backends when Linux or macOS becomes a supported runtime target.
- Keep UTF-32 conversion, obscured-text protection, validation, and failure-without-throwing semantics consistent across backends.

### Acceptance criteria

- A validation-rejected cut has no observable side effect.
- Successful copy, cut, and paste preserve supplementary Unicode codepoints.
- Clipboard ownership failures do not throw during normal editing.

---

## 5. Keyboard interaction and activation

Priority: medium. This was explicitly deferred from the first implementation pass.

### `DataModel<T>::View`

- Acquire keyboard focus on interaction without holding mouse focus after an ordinary click.
- Support Up/Down, Home/End, Page Up/Page Down, and scrolling the active row into view.
- Skip rows for which `DataModel<T>::isEnabled(row)` is false.
- Add row activation for double-click and Enter with a contract carrying the current `Selection`.
- Render a configurable keyboard-focus/active-row indication.

A possible API addition is:

```cpp
using ActivationProvider = ContractProvider<Selection>;
using ActivationCallback = ActivationProvider::callback_type;
using ActivationContract = ActivationProvider::Contract;

void activateRow(std::size_t row);
[[nodiscard]] ActivationContract subscribeToActivation(ActivationCallback callback);
```

### `ComboBox`

- Open with Enter, Space, Alt+Down, or F4.
- While open, forward navigation to the contained list and commit with Enter.
- Close with Escape without changing the prior committed selection.
- Define whether arrow keys change the committed selection while the popup is closed.

---

## 6. Optional widget follow-ups

Priority: low. These are extensions, not blockers for the implemented first-pass APIs.

- Add a shared enabled/disabled widget trait, then apply it consistently to `CheckBox`, `RadioButton`, `ToggleSwitch`, `DataModel<T>::View`, and `ComboBox`.
- Add an indeterminate state and rendering contract to `CheckBox`.
- Add drag-to-switch interaction to `ToggleSwitch`, retaining mouse-channel focus only for the drag.
- Add triple-click select-all to `TextEdit`.
- Consider extracting shared labelled-checkable layout code from `CheckBox` and `RadioButton` without merging their different state semantics.
- Consider a shared non-visual model-selection controller if more model-backed selectors are introduced; do not make `ComboBox` inherit from `DataModel<std::string>::View`.

---

## 7. Verification checklist for the remaining work

- Sparkle builds in Debug and Release configurations.
- The relevant Playground showcase tabs demonstrate any newly completed behavior.
- Popup and tooltip scenarios cover anchor deactivation, destruction, reparenting, cross-root migration, movement, and window focus loss.
- List scenarios cover empty models, disabled rows, variable row extents, selection removal, reset with stable IDs, and large virtualized models.
- Combo scenarios cover delegate metric changes, long custom rows, and programmatic versus user selection.
- TextEdit scenarios cover forward/reverse selection, drag outside bounds, double-click selection, obscured copy/cut policy, supplementary Unicode, validation rejection, and clipboard failure.
