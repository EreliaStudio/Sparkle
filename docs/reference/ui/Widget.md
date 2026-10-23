# spk::Widget

**Header:** [`ui/widget.hpp`](../../../include/ui/widget.hpp)  
**Tests:** `tests/TU/Graphics/srcs/ui/widget/`

Widget is the UI hierarchy base class. It composes naming, parent/child relationships, activation, event dispatch, sizing, geometry, updating and snapshot contribution through Sparkle traits.

Public operations include `applyStyle`, `resize`, `setZOrder`, `setTargetRenderPass`, `inheritTargetRenderPass` and `subscribeToDestruction`. Widgets can inherit or override a target rendering pass. The `Widget::Style` type holds shared UI styling resources and widget appearance defaults.

For relationships and pointers to controls and layouts, see [UI architecture](../../architecture/ui.md). These overview docs do not assert that all Widget callbacks can safely destroy their owning hierarchy.
