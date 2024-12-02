#include "widget/component/spk_texture_renderer.hpp"

namespace spk::WidgetComponent
{
	std::string TextureRenderer::_pipelineCode = R"(## LAYOUTS DEFINITION ##
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

	spk::Pipeline TextureRenderer::_pipeline = spk::Pipeline(TextureRenderer::_pipelineCode);

	void TextureRenderer::_updateGPUData()
	{
		struct Vertex
		{
			spk::Vector2Int position;
			spk::Vector2 uvs;
		};

		std::vector<Vertex> vertices = {
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y},
				.uvs = {_section.anchor.x, _section.anchor.y}
			},
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y + _geometry.size.y},
				.uvs = {_section.anchor.x, _section.anchor.y + _section.size.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y},
				.uvs = {_section.anchor.x + _section.size.x, _section.anchor.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y + _geometry.size.y},
				.uvs = {_section.anchor.x + _section.size.x, _section.anchor.y + _section.size.y}
			}
		};

		std::vector<unsigned int> indexes = {
			0, 1, 2, 2, 1, 3
		};

		_object.layout().clear();
		_object.indexes().clear();

		_object.layout() << vertices;
		_object.indexes() << indexes;

		_object.layout().validate();
		_object.indexes().validate();
	}

	TextureRenderer::TextureRenderer() :
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

	const float& TextureRenderer::layer() const
	{
		return (_layer);
	}
	const float& TextureRenderer::alpha() const
	{
		return (_alpha);
	}
	const spk::Geometry2D& TextureRenderer::geometry() const
	{
		return (_geometry);
	}
	const spk::SafePointer<spk::Image>& TextureRenderer::texture() const
	{
		return (_texture);
	}
	const spk::Image::Section& TextureRenderer::section() const
	{
		return (_section);
	}

	void TextureRenderer::setLayer(const float& p_layer)
	{
		_layer = p_layer;

		_layerElement = _layer;

		_textureInformations.validate();
	}
	void TextureRenderer::setAlpha(const float& p_alpha)
	{
		_alpha = p_alpha;

		_alphaElement = _alpha;

		_textureInformations.validate();
	}
	void TextureRenderer::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
		_needUpdateGPU = true;
	}
	void TextureRenderer::setTexture(const spk::SafePointer<spk::Image>& p_texture)
	{
		_texture = p_texture;
		_textureSampler2D.bind(_texture);
	}
	void TextureRenderer::setSection(const spk::Image::Section& p_section)
	{
		_section = p_section;
		_needUpdateGPU = true;
	}

	void TextureRenderer::render()
	{
		if (_needUpdateGPU == true)
		{
			_updateGPUData();
			_needUpdateGPU = false;
		}

		_object.render();
	}
}