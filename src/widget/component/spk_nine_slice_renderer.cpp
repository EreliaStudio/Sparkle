#include "widget/component/spk_nine_slice_renderer.hpp"

namespace spk::WidgetComponent
{
	std::string NineSliceRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
0 Vector2Int modelPosition
1 Vector2 modelUVs


## FRAMEBUFFER DEFINITION ##
0 Color pixelColor


## CONSTANTS DEFINITION ##
spk_ScreenConstants_Type spk::ScreenConstants 64 64 {
	canvasMVP 0 64 0 64 1 0 {}
}
spk_WidgetConstants_Type spk::WidgetConstants 4 4 {
	layer 0 4 0 4 1 0 {}
}


## ATTRIBUTES DEFINITION ##
textureInformations_Type textureInformations 8 8 {
	layer 0 4 0 4 1 0 {}
	alpha 4 4 4 4 1 0 {}
}


## TEXTURES DEFINITION ##
texture Texture_texture


## VERTEX SHADER CODE ##
#version 450

layout (location = 0) in ivec2 modelPosition;
layout (location = 1) in vec2 modelUVs;
layout (location = 0) out flat int out_instanceID;
layout (location = 1) out vec2 fragmentUVs;

layout(constants) uniform spk_ScreenConstants_Type
{
	mat4 canvasMVP;
} spk_ScreenConstants;

layout(constants) uniform spk_WidgetConstants_Type
{
	float layer;
} spk_WidgetConstants;

layout(attributes) uniform textureInformations_Type
{
	float layer;
	float alpha;
} textureInformations;

uniform sampler2D Texture_texture;

void main()
{
	(gl_Position) = ((spk_ScreenConstants.canvasMVP) * (vec4(modelPosition, (spk_WidgetConstants.layer) + (textureInformations.layer), 1.0f)));
	(fragmentUVs) = (modelUVs);
	out_instanceID = gl_InstanceID;
}

## FRAGMENT SHADER CODE ##
#version 450

layout (location = 0) in flat int instanceID;
layout (location = 1) in vec2 fragmentUVs;
layout (location = 0) out vec4 pixelColor;

layout(attributes) uniform textureInformations_Type
{
	float layer;
	float alpha;
} textureInformations;

uniform sampler2D Texture_texture;

void main()
{
	if ((textureInformations.alpha) == (0))
	{
		discard;
	}
	vec4 baseColor = texture(Texture_texture, fragmentUVs);
	if ((baseColor.a) == (0))
	{
		discard;
	}
	(pixelColor) = (baseColor);
	(pixelColor.a) *= (textureInformations.alpha);
})";

	spk::Pipeline NineSliceRenderer::_pipeline = spk::Pipeline(NineSliceRenderer::_pipelineCode);

	void NineSliceRenderer::_updateGPUData()
	{
		struct Vertex
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;
		};

		int pointsX[4] = {
			static_cast<int>(_geometry.anchor.x),
			static_cast<int>(_geometry.anchor.x + _cornerSize.x),
			static_cast<int>(_geometry.anchor.x + _geometry.size.x - _cornerSize.x),
			static_cast<int>(_geometry.anchor.x + _geometry.size.x)
		};

		int pointsY[4] = {
			static_cast<int>(_geometry.anchor.y),
			static_cast<int>(_geometry.anchor.y + _cornerSize.y),
			static_cast<int>(_geometry.anchor.y + _geometry.size.y - _cornerSize.y),
			static_cast<int>(_geometry.anchor.y + _geometry.size.y)
		};

		spk::Vector2 uvs[4] = {
			spk::Vector2(0.0f, 0.0f),
			spk::Vector2(0.0f, 1.0f),
			spk::Vector2(1.0f, 0.0f),
			spk::Vector2(1.0f, 1.0f)
		};

		unsigned int quadIndices[6] = { 0, 1, 2, 2, 1, 3 };

		_object.layout().clear();
		_object.indexes().clear();

		for (size_t x = 0; x < 3; x++)
		{
			for (size_t y = 0; y < 3; y++)
			{
				unsigned int baseVertexIndex = static_cast<unsigned int>(_object.layout().size() / sizeof(Vertex));

				for (size_t i = 0; i < 2; i++)
				{
					for (size_t j = 0; j < 2; j++)
					{
						Vertex newVertex;
						newVertex.position = spk::Vector2Int(pointsX[x + i], pointsY[y + j]);

						spk::SpriteSheet::Sprite sprite = _texture->sprite(spk::Vector2UInt(x, y));
						size_t uvIndex = j + i * 2;

						newVertex.uvs = sprite.anchor + sprite.size * uvs[uvIndex];

						_object.layout() << (newVertex);
					}
				}

				for (size_t i = 0; i < 6; i++)
				{
					_object.indexes() << (baseVertexIndex + quadIndices[i]);
				}
			}
		}

		_object.layout().validate();
		_object.indexes().validate();
	}

	NineSliceRenderer::NineSliceRenderer() :
		_object(_pipeline.createObject()),
		_textureInformations(_object.attribute(L"textureInformations")),
		_layerElement(_textureInformations[L"layer"]),
		_alphaElement(_textureInformations[L"alpha"]),
		_textureSampler2D(_object.sampler2D(L"texture"))
	{
		_layerElement = _layer;
		_alphaElement = _alpha;

		_textureInformations.validate();
	}

	const float& NineSliceRenderer::layer() const
	{
		return _layer;
	}

	const float& NineSliceRenderer::alpha() const
	{
		return _alpha;
	}

	const spk::Geometry2D& NineSliceRenderer::geometry() const
	{
		return _geometry;
	}

	const spk::SafePointer<spk::SpriteSheet>& NineSliceRenderer::texture() const
	{
		return _texture;
	}

	const spk::Vector2UInt& NineSliceRenderer::cornerSize() const
	{
		return _cornerSize;
	}

	void NineSliceRenderer::setLayer(const float& p_layer)
	{
		_layer = p_layer;
		_layerElement = _layer;
		_textureInformations.validate();
	}

	void NineSliceRenderer::setAlpha(const float& p_alpha)
	{
		_alpha = p_alpha;
		_alphaElement = _alpha;
		_textureInformations.validate();
	}

	void NineSliceRenderer::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
		_needUpdateGPU = true;
	}

	void NineSliceRenderer::setTexture(const spk::SafePointer<spk::SpriteSheet>& p_texture)
	{
		_texture = p_texture;
		_textureSampler2D.bind(_texture);
	}

	void NineSliceRenderer::setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		_needUpdateGPU = true;
	}

	void NineSliceRenderer::render()
	{
		if (_needUpdateGPU)
		{
			_updateGPUData();
			_needUpdateGPU = false;
		}

		_object.render();
	}
}
