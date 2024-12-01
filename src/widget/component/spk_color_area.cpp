#include "widget/component/spk_color_area.hpp"

namespace spk::WidgetComponent
{
	std::string ColorArea::_pipelineCode = R"(
	## LAYOUTS DEFINITION ##
	0 Vector2Int screenPosition


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
	Informations_Type Informations 20 32 {
		layer 0 4 0 4 1 0 {}
		color 4 16 16 16 1 0 {
			r 0 4 0 4 1 0 {}
			g 4 4 4 4 1 0 {}
			b 8 4 8 4 1 0 {}
			a 12 4 12 4 1 0 {}
		}
	}


	## TEXTURES DEFINITION ##


	## VERTEX SHADER CODE ##
	#version 450

	layout (location = 0) in ivec2 screenPosition;
	layout (location = 0) out flat int out_instanceID;

	layout(constants) uniform spk_ScreenConstants_Type
	{
		mat4 canvasMVP;
	} spk_ScreenConstants;

	layout(constants) uniform spk_WidgetConstants_Type
	{
		float layer;
	} spk_WidgetConstants;

	layout(attributes) uniform Informations_Type
	{
		float layer;
		vec4 color;
	} Informations;

	void main()
	{
		(gl_Position) = ((spk_ScreenConstants.canvasMVP) * (vec4(screenPosition, (spk_WidgetConstants.layer) + (Informations.layer), 1)));
		out_instanceID = gl_InstanceID;
	}

	## FRAGMENT SHADER CODE ##
	#version 450

	layout (location = 0) in flat int instanceID;
	layout (location = 0) out vec4 pixelColor;

	layout(attributes) uniform Informations_Type
	{
		float layer;
		vec4 color;
	} Informations;

	void main()
	{
		(pixelColor) = (Informations.color);
	}
	)";
	spk::Pipeline ColorArea::_pipeline = spk::Pipeline(_pipelineCode);

	ColorArea::ColorArea(const spk::Color& p_color) :
		_object(_pipeline.createObject()),
		_informations(_object.attribute(L"Informations")),
		_colorInformation(_informations[L"color"]),
		_layerInformation(_informations[L"layer"])
	{
		setColor(p_color);
	}

	void ColorArea::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_geometry = p_geometry;
		_needUpdateGPUData = true;
	}

	const spk::Color& ColorArea::color() const
	{
		return (_color);
	}

	const float& ColorArea::layer() const
	{
		return (_layer);
	}

	const spk::Geometry2D& ColorArea::geometry() const
	{
		return (_geometry);
	}

	void ColorArea::setColor(const spk::Color& p_color)
	{
		_color = p_color;
		_colorInformation = _color;
		_informations.validate();
	}

	void ColorArea::setLayer(const float& p_layer)
	{
		_layer = p_layer;

		_layerInformation = _layer;
		_informations.validate();
	}

	void ColorArea::_updateGPUData()
	{
		struct Vertex
		{
			spk::Vector2Int position;
		};

		std::vector<Vertex> vertices = {
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y}
			},
			{
				.position = {_geometry.anchor.x, _geometry.anchor.y + _geometry.size.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y}
			},
			{
				.position = {_geometry.anchor.x + _geometry.size.x, _geometry.anchor.y + _geometry.size.y}
			},
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

	void ColorArea::render()
	{
		if (_needUpdateGPUData == true)
		{
			_updateGPUData();
			_needUpdateGPUData = false;
		}
		_object.render();
	}
}