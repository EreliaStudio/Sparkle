#include "structure/graphics/renderer/spk_texture_renderer.hpp"

#include "structure/graphics/spk_viewport.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	void TextureRenderer::_initProgram()
	{
		if (_program == nullptr)
		{
			const char *vertexShaderSrc = R"(#version 450
				layout(location = 0) in vec2 inPosition;
				layout(location = 1) in float inLayer;
				layout(location = 2) in vec2 inUV;

				layout(location = 0) out vec2 fragUV;

				void main()
				{
					gl_Position = vec4(inPosition, inLayer, 1.0);
					fragUV = inUV;
				}
				)";

			const char *fragmentShaderSrc = R"(#version 450
				layout(location = 0) in vec2 fragUV;
				layout(location = 0) out vec4 outputColor;

				uniform sampler2D diffuseTexture;

				void main()
				{
					outputColor = texture(diffuseTexture, fragUV);
					if (outputColor.a == 0)
						discard;
				}
				)";

			_program = new spk::OpenGL::Program(vertexShaderSrc, fragmentShaderSrc);
		}
	}

	void TextureRenderer::_initBuffers()
	{
		_bufferSet = spk::OpenGL::BufferSet({
			{0, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}, // position
			{1, spk::OpenGL::LayoutBufferObject::Attribute::Type::Float},	// layer
			{2, spk::OpenGL::LayoutBufferObject::Attribute::Type::Vector2}	// uv
		});

		_samplerObject = spk::OpenGL::SamplerObject("diffuseTexture", spk::OpenGL::SamplerObject::Type::Texture2D, 0);
	}

	TextureRenderer::TextureRenderer()
	{
		_initProgram();
		_initBuffers();
	}

	void TextureRenderer::setTexture(spk::SafePointer<spk::OpenGL::TextureObject> p_image)
	{
		_samplerObject.bind(p_image);
	}

	spk::SafePointer<spk::OpenGL::TextureObject> TextureRenderer::texture()
	{
		return (_samplerObject.texture());
	}

	void TextureRenderer::clear()
	{
		_bufferSet.layout().clear();
		_bufferSet.indexes().clear();
	}

	void TextureRenderer::prepare(const spk::Geometry2D &geom, const spk::Image::Section &section, float layer)
	{
		size_t nbVertex = _bufferSet.layout().size() / sizeof(Vertex);

		spk::Vector3 topLeft = spk::Viewport::convertScreenToOpenGL({geom.anchor.x, geom.anchor.y}, layer);
		spk::Vector3 bottomRight = spk::Viewport::convertScreenToOpenGL({geom.anchor.x + static_cast<int32_t>(geom.size.x),
																		 geom.anchor.y + static_cast<int32_t>(geom.size.y)},
																		layer);

		float u1 = section.anchor.x;
		float v1 = section.anchor.y + section.size.y;
		float u2 = section.anchor.x + section.size.x;
		float v2 = section.anchor.y;

		_bufferSet.layout()	<< Vertex{{topLeft.x, bottomRight.y}, topLeft.z, {u1, v1}}
							<< Vertex{{bottomRight.x, bottomRight.y}, topLeft.z, {u2, v1}}
							<< Vertex{{topLeft.x, topLeft.y}, topLeft.z, {u1, v2}}
							<< Vertex{{bottomRight.x, topLeft.y}, topLeft.z, {u2, v2}};

		std::array<unsigned int, 6> indices = {0, 1, 2, 2, 1, 3};
		for (const auto &index : indices)
		{
			_bufferSet.indexes() << index + nbVertex;
		}
	}

	void TextureRenderer::validate()
	{
		_bufferSet.layout().validate();
		_bufferSet.indexes().validate();
	}

	void TextureRenderer::render()
	{
		if (_samplerObject.texture() == nullptr)
			return;

		_program->activate();
		_bufferSet.activate();
		_samplerObject.activate();

		_program->render(_bufferSet.indexes().nbIndexes(), 1);

		_samplerObject.deactivate();
		_bufferSet.deactivate();
		_program->deactivate();
	}
}
