#include "structure/graphics/renderer/spk_color_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
	void ColorRenderer::_initProgram()
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

			_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
		}
	}

	void ColorRenderer::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float}	// layer
		});

		_colorUbo = spk::OpenGL::UniformBufferObject(L"ColorData", 0, 16);
		_colorUbo.addElement(L"uColor", 0, 16);
	}

	ColorRenderer::ColorRenderer(const spk::Color &p_color)
	{
		_initProgram();
		_initBuffers();
		setColor(p_color);
	}

	void ColorRenderer::setColor(const spk::Color &p_color)
	{
		_color = p_color;

		_colorUbo[L"uColor"] = _color;
		_colorUbo.validate();
	}

	void ColorRenderer::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void ColorRenderer::prepareSquare(const spk::Geometry2D &geom, float layer)
	{
		size_t nbVertex = _bufferSet.layout().size() / sizeof(Vertex);

		spk::Vector3 topLeft = spk::Viewport::convertScreenToOpenGL({geom.anchor.x, geom.anchor.y}, layer);
		spk::Vector3 bottomRight = spk::Viewport::convertScreenToOpenGL({geom.anchor.x + static_cast<int32_t>(geom.size.x),
																		 geom.anchor.y + static_cast<int32_t>(geom.size.y)},
																		layer);

		_bufferSet.layout()	<< Vertex{{topLeft.x, bottomRight.y}, topLeft.z}
							<< Vertex{{bottomRight.x, bottomRight.y}, topLeft.z}
							<< Vertex{{topLeft.x, topLeft.y}, topLeft.z}
							<< Vertex{{bottomRight.x, topLeft.y}, topLeft.z};

		std::array<unsigned int, 6> indices = {0, 1, 2, 2, 1, 3};
		for (const auto &index : indices)
		{
			_bufferSet.indexes() << index + nbVertex;
		}
	}

	void ColorRenderer::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void ColorRenderer::render()
	{
		_program->activate();
		_bufferSet.activate();
		_colorUbo.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_colorUbo.deactivate();
		_bufferSet.deactivate();
		_program->deactivate();
	}
}
