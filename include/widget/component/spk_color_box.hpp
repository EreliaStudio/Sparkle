#pragma once

#include "widget/component/spk_color_area.hpp"

namespace spk::WidgetComponent
{
	class ColorBox
	{
	private:
		WidgetComponent::ColorArea _frontgroundColorArea;
		WidgetComponent::ColorArea _backgroundColorArea;

		spk::Vector2UInt _cornerSize;

		void updateFrontgroundGeometry();

	public:
		ColorBox(const spk::Color& p_backgroundColor, const spk::Color& p_frontgroundColor);
		ColorBox();

		const spk::Geometry2D& geometry() const;
		const spk::Color& frontgroundColor() const;
		const spk::Color& backgroundColor() const;
		const float& layer() const;
		const spk::Vector2UInt& cornerSize() const;

		void setGeometry(const spk::Geometry2D& p_geometry);
		void setCornerSize(const spk::Vector2UInt& p_cornerSize);
		void setColors(const spk::Color& p_backgroundColor, const spk::Color& p_frontgroundColor);
		void setFrontgroundColor(const spk::Color& p_frontgroundColor);
		void setBackgroundColors(const spk::Color& p_backgroundColor);
		void setLayer(const float& p_layer);

		void render();
	};
}