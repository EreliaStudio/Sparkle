#include "structure/graphics/painter/spk_color_painter.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include <array>

namespace spk
{
	void ColorPainter::_initProgram()
	{
		if (_program == nullptr)
		{
			const char *vertexShaderSrc = R"(#version 450
			
				layout(location = 0) in vec2 inPosition;
				layout(location = 1) in float inLayer;

				layout(std140, binding = 0) uniform ColorData {
					vec4 uColor;
				};

				void main()
				{
					// We can expand this if needed for transformations
					gl_Position = vec4(inPosition, inLayer, 1.0);
				}
				)";

			const char *fragmentShaderSrc = R"(#version 450
				layout(std140, binding = 0) uniform ColorData {
					vec4 uColor;
				};

				layout(location = 0) out vec4 outputColor;

				void main()
				{
					outputColor = uColor;
				}
				)";

			_program = std::make_unique<spk::OpenGL::Program>(vertexShaderSrc, fragmentShaderSrc);
		}
	}

	void ColorPainter::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float}	// layer
		});

		_colorUniformBufferObject = spk::OpenGL::UniformBufferObject(L"ColorData", 0, 16);
		_colorUniformBufferObject.addElement(L"uColor", 0, 16);
	}

	ColorPainter::ColorPainter(const spk::Color &p_color)
	{
		_initProgram();
		_initBuffers();
		setColor(p_color);
	}

	void ColorPainter::setColor(const spk::Color &p_color)
	{
		_color = p_color;

		_colorUniformBufferObject[L"uColor"] = _color;
		_colorUniformBufferObject.validate();
	}

	void ColorPainter::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void ColorPainter::prepareSquare(const spk::Geometry2D &p_geometry, float p_layer)
	{
		size_t numberOfVertices = _bufferSet.layout().size() / sizeof(Vertex);

		spk::Vector3 topLeft = spk::Viewport::convertScreenToOpenGL({p_geometry.anchor.x, p_geometry.anchor.y}, p_layer);
		spk::Vector3 bottomRight = spk::Viewport::convertScreenToOpenGL(
			{p_geometry.anchor.x + static_cast<int32_t>(p_geometry.size.x), p_geometry.anchor.y + static_cast<int32_t>(p_geometry.size.y)}, p_layer);

		_bufferSet.layout() << Vertex{{topLeft.x, bottomRight.y}, topLeft.z} << Vertex{{bottomRight.x, bottomRight.y}, topLeft.z}
							<< Vertex{{topLeft.x, topLeft.y}, topLeft.z} << Vertex{{bottomRight.x, topLeft.y}, topLeft.z};

		std::array<unsigned int, 6> indices = {0, 1, 2, 2, 1, 3};
		for (const auto &index : indices)
		{
			_bufferSet.indexes() << index + numberOfVertices;
		}
	}

	void ColorPainter::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void ColorPainter::render()
	{
		_program->activate();
		_bufferSet.activate();
		_colorUniformBufferObject.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_colorUniformBufferObject.deactivate();
		_bufferSet.deactivate();
		_program->deactivate();
	}
}
