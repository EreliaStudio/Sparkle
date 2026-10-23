# UI and widgets

Widgets form a hierarchy using `InherenceTrait<Widget>`, participate in events and state updates, and contribute rendering commands through the rendering snapshot mechanism. Layouts organize widgets; higher-level controls include labels, buttons, text editors, scroll areas, and model views.

~~~mermaid
flowchart TD
    Widget["Widget"]
    Layout["Layout"]
    Control["Concrete widgets"]
    Events["EventDispatcher"]
    Snapshot["RenderSnapshot"]
    Widget --> Layout
    Widget --> Control
    Widget --> Events
    Widget --> Snapshot
    click Widget href "../reference/ui/Widget.md" "Widget reference"
    click Layout href "../reference/ui/Widget.md" "UI reference"
    click Snapshot href "../reference/rendering/RenderSnapshot.md" "Rendering reference"
~~~

The diagram groups roles rather than implying every edge is an inheritance relation. Start with [Widget reference](../reference/ui/Widget.md) and [rendering architecture](rendering.md). Widget-specific recipes should follow once individually verified against the graphics test suite.
