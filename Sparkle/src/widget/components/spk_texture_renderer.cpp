#include "widget/components/spk_texture_renderer.hpp"

namespace spk::WidgetComponent
{
	const std::string TextureRenderer::_renderingPipelineCode = R"(#version 450

		#include <screenConstants>
		
		Input -> Geometry : ivec2 modelPosition;
		Input -> Geometry : vec2 modelUVs;

		Geometry -> Render : vec2 fragmentUVs;

		AttributeBlock self
		{
			float layer;
		};

		Texture _texture;

		void geometryPass()
		{
			pixelPosition = screenConstants.canvasMVP * vec4(modelPosition, self.layer, 1.0f);

			fragmentUVs = modelUVs;
		}
		
		void renderPass()
		{
			vec4 textureColor = texture(_texture, fragmentUVs).rgba;

			if (textureColor.a == 0)	
				discard;

			pixelColor = textureColor;
		})";
	spk::Pipeline TextureRenderer::_renderingPipeline = spk::Pipeline(_renderingPipelineCode);

	TextureRenderer::ShaderInput::ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs) :
		position(p_position),
		uvs(p_uvs)
	{
	}

	void TextureRenderer::_updateGPUData()
	{
		std::vector<ShaderInput> data;
		std::vector<unsigned int> indexes = {0, 1, 2, 2, 1, 3};

		spk::Vector2 deltas[4] = {
			spk::Vector2Int(0, 0),
			spk::Vector2Int(0, 1),
			spk::Vector2Int(1, 0),
			spk::Vector2Int(1, 1),
		};

		for (size_t i = 0; i < 4; i++)
		{
			data.push_back(ShaderInput(anchor() + size() * deltas[i], uvAnchor() + uvSize() * deltas[i]));
		}

		_renderingObject.setVertices(data);
		_renderingObject.setIndexes(indexes);
	}

	TextureRenderer::TextureRenderer() :
		_renderingObject(_renderingPipeline.createObject()),
		_selfAttribute(_renderingObject.attribute("self")),
		_selfLayerElement(_selfAttribute["layer"]),
		_renderingPipelineTexture(_renderingPipeline.texture("_texture")),
		_texture(nullptr)
	{

	}

	void TextureRenderer::render()
	{
		if (_texture == nullptr)
			return ;

		if (_needGPUInputUpdate == true)
		{
			_updateGPUData();
			_needGPUInputUpdate = false;
		}

		_renderingPipelineTexture.attach(_texture);
		_renderingObject.render();
	}

	
	void TextureRenderer::setTexture(const spk::Texture* p_texture)
	{
		_texture = p_texture;
	}
	
	void TextureRenderer::setTextureGeometry(const spk::Vector2& p_uvAnchor, const spk::Vector2& p_uvSize)
	{
		_uvAnchor = p_uvAnchor;
		_uvSize = p_uvSize;
		_needGPUInputUpdate = true;
	}

	const Vector2& TextureRenderer::anchor() const
	{
		return _anchor;
	}

	const Vector2& TextureRenderer::size() const
	{
		return _size;
	}

	const Vector2& TextureRenderer::uvAnchor() const
	{   
		return (_uvAnchor);
	}
	
	const Vector2& TextureRenderer::uvSize() const
	{
		return (_uvSize);
	}

	void TextureRenderer::setLayer(const float& p_layer)
	{
		_selfLayerElement = p_layer;
		_selfAttribute.update();
	}

	void TextureRenderer::setGeometry(const Vector2& p_anchor, const Vector2& p_newSize)
	{
		_anchor = p_anchor;
		_size = p_newSize;
		_needGPUInputUpdate = true;
	}
}