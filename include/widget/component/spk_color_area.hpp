#pragma once

#include <string>

#include "structure/graphics/spk_geometry_2D.hpp"
#include "structure/graphics/spk_color.hpp"

namespace spk::WidgetComponent
{
	class ColorArea
	{
	private:
		bool _needUpdateGPUData = false;
		spk::Geometry2D _geometry;
		spk::Color _color;
		float _layer;

		void _updateGPUData();

	public:
		ColorArea(const spk::Color& p_color = spk::Color::white);

		void setGeometry(const spk::Geometry2D& p_geometry);

		const spk::Color& color() const;
		const float& layer() const;
		const spk::Geometry2D& geometry() const;

		void setColor(const spk::Color& p_color);
		void setLayer(const float& p_layer);

		void render();
	};
}