#include "widget/components/spk_nine_sliced.hpp"

namespace spk::WidgetComponent
{
	std::string NineSlicedBox::_renderingPipelineCode = R"(#version 450
		
	#include <widgetAttribute>
	#include <screenConstants>

	Input -> Geometry : ivec2 screenPosition;
	Input -> Geometry : vec2 UVs;
	Geometry -> Render : vec2 fragmentUVs;

	Texture spriteSheet;

	void geometryPass()
	{
		pixelPosition = screenConstants.canvasMVP * vec4(screenPosition, widgetAttribute.layer, 1.0);

		fragmentUVs = UVs;
	}

	void renderPass()
	{
		pixelColor = texture(spriteSheet, fragmentUVs);
		if (pixelColor.a == 0)
			discard;
	})";

	spk::Pipeline NineSlicedBox::_renderingPipeline = spk::Pipeline(_renderingPipelineCode);

	NineSlicedBox::ShaderInput::ShaderInput() :
		position(0),
		uvs(0)
	{
	}

	NineSlicedBox::ShaderInput::ShaderInput(const spk::Vector2Int& p_position, const spk::Vector2& p_uvs) :
		position(p_position),
		uvs(p_uvs)
	{
	}

	void NineSlicedBox::_updateVertices()
	{
		static std::vector<ShaderInput> _bufferShaderInput;
		static std::vector<unsigned int> _bufferRenderingPipelineIndexes;

		_bufferShaderInput.clear();
		_bufferRenderingPipelineIndexes.clear();

		int pointsX[4] = {0, _cornerSize.x, static_cast<int>(_size.x) - _cornerSize.x, static_cast<int>(_size.x)};
		int pointsY[4] = {0, _cornerSize.y, static_cast<int>(_size.y) - _cornerSize.y, static_cast<int>(_size.y)};

		spk::Vector2 uvs[4] = {
			spk::Vector2(0, 0),
			spk::Vector2(0, 1),
			spk::Vector2(1, 0),
			spk::Vector2(1, 1)};

		unsigned int indexes[6] = {0, 1, 2, 2, 1, 3};

		for (size_t x = 0; x < 3; x++)
		{
			for (size_t y = 0; y < 3; y++)
			{
				unsigned int baseIndexes = static_cast<unsigned int>(_bufferShaderInput.size());
				for (size_t i = 0; i < 2; i++)
				{
					for (size_t j = 0; j < 2; j++)
					{
						ShaderInput newVertex;

						newVertex.position = _anchor + spk::Vector2Int(pointsX[x + i], pointsY[y + j]);
						newVertex.uvs = _spriteSheet->sprite(spk::Vector2UInt(x, y)) + _spriteSheet->unit() * uvs[j + i * 2];

						_bufferShaderInput.push_back(newVertex);
					}
				}

				for (size_t i = 0; i < 6; i++)
				{
					_bufferRenderingPipelineIndexes.push_back(baseIndexes + indexes[i]);
				}
			}
		}

		_renderingObject.setVertices(_bufferShaderInput);
		_renderingObject.setIndexes(_bufferRenderingPipelineIndexes);
	}

	NineSlicedBox::NineSlicedBox() :
		_spriteSheet(nullptr),
		_renderingObject(_renderingPipeline.createObject()),
		_renderingObjectAttribute(_renderingObject.attribute("widgetAttribute")),
		_layerAttributeElement(_renderingObjectAttribute["layer"]),
		_renderingPipelineTexture(_renderingPipeline.texture("spriteSheet")),
		_needGPUUpdate(false),
		_cornerSize(16)
	{
	}

	void NineSlicedBox::setSpriteSheet(const spk::SpriteSheet* p_spriteSheet)
	{
		_spriteSheet = p_spriteSheet;
		_renderingPipelineTexture.attach(p_spriteSheet);
	}

	void NineSlicedBox::setGeometry(const spk::Vector2Int& p_anchor, const spk::Vector2UInt& p_size)
	{
		_anchor = p_anchor;
		_size = p_size;

		_needGPUUpdate = true;
	}

	void NineSlicedBox::setLayer(const float& p_layer)
	{
		_layerAttributeElement = p_layer;
		_renderingObjectAttribute.update();
	}

	void NineSlicedBox::setCornerSize(const spk::Vector2Int& p_cornerSize)
	{
		_cornerSize = p_cornerSize;

		_needGPUUpdate = true;
	}

	void NineSlicedBox::render()
	{
		if (_spriteSheet == nullptr)
			return;

		if (_needGPUUpdate == true)
		{
			_updateVertices();
			_needGPUUpdate = false;
		}

		_renderingPipelineTexture.attach(_spriteSheet);
		_renderingObject.render();
	}

	const spk::Vector2Int& NineSlicedBox::cornerSize() const
	{
		return (_cornerSize);
	}
}