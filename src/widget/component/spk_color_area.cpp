#include "widget/component/spk_color_area.hpp"

namespace spk::WidgetComponent
{
	ColorArea::ColorArea(const spk::Color& p_color)
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
		// _colorInformation = _color;
		// _informations.validate();
	}

	void ColorArea::setLayer(const float& p_layer)
	{
		_layer = p_layer;

		// _layerInformation = _layer;
		// _informations.validate();
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

		// _object.layout().clear();
		// _object.indexes().clear();

		// _object.layout() << vertices;
		// _object.indexes() << indexes;

		// _object.layout().validate();
		// _object.indexes().validate();
	}

	void ColorArea::render()
	{
		if (_needUpdateGPUData == true)
		{
			_updateGPUData();
			_needUpdateGPUData = false;
		}

		// _object.render();
	}
}