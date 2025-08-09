# Hello Triangle

This tutorial builds a minimal Sparkle program step by step. We start with an
empty application window, add a custom widget, compile a shader, describe the
geometry in pixel coordinates, and finally introduce a uniform buffer object
for colour.

## 1. Create the application window

Begin with a basic `main` function that creates a window and runs the event
loop.

```cpp
int main()
{
    spk::GraphicalApplication app;
    auto window = app.createWindow(L"Triangle", {{0,0},{800,600}});

    return app.run();
}
```

## 2. Define an empty widget

Declare a widget subclass. It will later hold the shader program and buffers,
but for now the callbacks remain empty.

```cpp
#include <sparkle.hpp>

class TriangleWidget : public spk::Widget
{
    spk::OpenGL::Program _program;
    spk::OpenGL::BufferSet _buffers;

public:
    TriangleWidget(const std::wstring& name, spk::SafePointer<spk::Widget> parent)
        : spk::Widget(name, parent)
    {}

protected:
    void _onGeometryChange() override {}
    void _onPaintEvent(spk::PaintEvent&) override {}
};
```

## 3. Compile the shader and describe attributes

Create a vertex and fragment shader that pass through a colour attribute. Then
configure the buffer layout for position, layer, and colour.

```cpp
const char* vertexSrc = R"(
    #version 450
    layout(location=0) in vec2 inPosition;
    layout(location=1) in float inLayer;
    layout(location=2) in vec4 inColor;
    out vec4 vColor;
    void main() {
        gl_Position = vec4(inPosition, inLayer, 1.0);
        vColor = inColor;
    }
)";

const char* fragmentSrc = R"(
    #version 450
    in vec4 vColor;
    layout(location=0) out vec4 outColor;
    void main() { outColor = vColor; }
)";

_program = spk::OpenGL::Program(vertexSrc, fragmentSrc);

_buffers = spk::OpenGL::BufferSet({
    {0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
    {1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},   // layer
    {2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector4}  // colour
});
```

## 4. Fill buffers on geometry change and paint

Populate the buffers inside `_onGeometryChange` so resizing the widget rebuilds
the geometry. The positions are supplied in pixel coordinates and converted to
OpenGL space via `spk::Viewport`.

```cpp
void TriangleWidget::_onGeometryChange()
{
    struct Vertex { spk::Vector2 pos; float layer; spk::Color color; };

    auto geom = geometry();
    spk::Vector2Int center = {geom.x + static_cast<int>(geom.width) / 2,
                              geom.y + static_cast<int>(geom.height) / 2};
    int size = 100; // triangle half-size in pixels

    spk::Vector3 top   = spk::Viewport::convertScreenToOpenGL({center.x, center.y - size}, 0.0f);
    spk::Vector3 left  = spk::Viewport::convertScreenToOpenGL({center.x - size, center.y + size}, 0.0f);
    spk::Vector3 right = spk::Viewport::convertScreenToOpenGL({center.x + size, center.y + size}, 0.0f);

    _buffers.layout().clear();
    _buffers.indexes().clear();

    _buffers.layout() << Vertex{top.xy(),   top.z,  spk::Color::green}
                      << Vertex{left.xy(),  left.z, spk::Color::green}
                      << Vertex{right.xy(), right.z, spk::Color::green};
    _buffers.indexes() << 0 << 1 << 2;

    _buffers.layout().validate();
    _buffers.indexes().validate();
}

void TriangleWidget::_onPaintEvent(spk::PaintEvent&)
{
    _program.activate();
    _buffers.activate();
    _program.render(_buffers.indexes().nbIndexes(), 1);
    _buffers.deactivate();
    _program.deactivate();
}
```

Instantiate the widget after creating the window:

```cpp
int main()
{
    spk::GraphicalApplication app;
    auto window = app.createWindow(L"Triangle", {{0,0},{800,600}});

    TriangleWidget tri(L"TriangleWidget", window->widget());
    tri.setGeometry(window->geometry());
    tri.activate();

    return app.run();
}
```

Running this displays a green triangle centred in the window.

## 5. Use a uniform buffer for colour

To demonstrate uniform buffers, remove the colour attribute and supply a colour
through a `spk::OpenGL::UniformBufferObject`.

Update the shaders:

```cpp
const char* vertexSrc = R"(
    #version 450
    layout(location=0) in vec2 inPosition;
    layout(location=1) in float inLayer;
    void main() { gl_Position = vec4(inPosition, inLayer, 1.0); }
)";

const char* fragmentSrc = R"(
    #version 450
    layout(location=0) out vec4 outColor;
    layout(std140, binding = 0) uniform ColorUBO { vec4 color; };
    void main() { outColor = color; }
)";

_program = spk::OpenGL::Program(vertexSrc, fragmentSrc);
```

Reconfigure the buffer layout without the colour attribute and update the
geometry building code accordingly:

```cpp
_buffers = spk::OpenGL::BufferSet({
    {0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2},
    {1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float}
});

void TriangleWidget::_onGeometryChange()
{
    struct Vertex { spk::Vector2 pos; float layer; };

    auto geom = geometry();
    spk::Vector2Int center = {geom.x + static_cast<int>(geom.width) / 2,
                              geom.y + static_cast<int>(geom.height) / 2};
    int size = 100;

    spk::Vector3 top   = spk::Viewport::convertScreenToOpenGL({center.x, center.y - size}, 0.0f);
    spk::Vector3 left  = spk::Viewport::convertScreenToOpenGL({center.x - size, center.y + size}, 0.0f);
    spk::Vector3 right = spk::Viewport::convertScreenToOpenGL({center.x + size, center.y + size}, 0.0f);

    _buffers.layout().clear();
    _buffers.indexes().clear();

    _buffers.layout() << Vertex{top.xy(),   top.z}
                      << Vertex{left.xy(),  left.z}
                      << Vertex{right.xy(), right.z};
    _buffers.indexes() << 0 << 1 << 2;

    _buffers.layout().validate();
    _buffers.indexes().validate();
}
```

Create the uniform buffer object and use it during rendering:

```cpp
spk::OpenGL::UniformBufferObject _colorUbo;

_colorUbo = spk::OpenGL::UniformBufferObject(L"ColorUBO", 0, 16);
_colorUbo.addElement(L"color", 0, 16);
_colorUbo[L"color"] = spk::Color::green;
_colorUbo.validate();

void TriangleWidget::_onPaintEvent(spk::PaintEvent&)
{
    _program.activate();
    _buffers.activate();
    _colorUbo.activate();
    _program.render(_buffers.indexes().nbIndexes(), 1);
    _colorUbo.deactivate();
    _buffers.deactivate();
    _program.deactivate();
}
```

The triangle now renders using the colour stored in the uniform buffer.

