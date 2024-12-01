#include "widget/component/spk_color_box.hpp"

namespace spk::WidgetComponent
{
	void ColorBox::updateFrontgroundGeometry()
	{
		spk::Geometry2D frontgeometry = { _backgroundColorArea.geometry().anchor + _cornerSize, _backgroundColorArea.geometry().size - spk::Vector2Int::min(_cornerSize * 2, _backgroundColorArea.geometry().size) };

		_frontgroundColorArea.setGeometry(frontgeometry);
	}

	ColorBox::ColorBox(const spk::Color& p_backgroundColor, const spk::Color& p_frontgroundColor) :
		_frontgroundColorArea(p_frontgroundColor),
		_backgroundColorArea(p_backgroundColor)
	{
		_backgroundColorArea.setLayer(0.0f);
		_frontgroundColorArea.setLayer(0.1f);
	}

	ColorBox::ColorBox() :
		ColorBox(spk::Color(158, 157, 155), spk::Color(194, 193, 190))
	{

	}

	const spk::Geometry2D& ColorBox::geometry() const
	{
		return (_backgroundColorArea.geometry());
	}

	const spk::Color& ColorBox::frontgroundColor() const
	{
		return (_frontgroundColorArea.color());
	}

	const spk::Color& ColorBox::backgroundColor() const
	{
		return (_backgroundColorArea.color());
	}

	const float& ColorBox::layer() const
	{
		return (_backgroundColorArea.layer());
	}

	const spk::Vector2UInt& ColorBox::cornerSize() const
	{
		return (_cornerSize);
	}

	void ColorBox::setGeometry(const spk::Geometry2D& p_geometry)
	{
		_backgroundColorArea.setGeometry(p_geometry);
		updateFrontgroundGeometry();
	}

	void ColorBox::setCornerSize(const spk::Vector2UInt& p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		updateFrontgroundGeometry();
	}

	void ColorBox::setColors(const spk::Color& p_backgroundColor, const spk::Color& p_frontgroundColor)
	{
		setFrontgroundColor(p_frontgroundColor);
		setBackgroundColors(p_backgroundColor);
	}

	void ColorBox::setFrontgroundColor(const spk::Color& p_frontgroundColor)
	{
		_frontgroundColorArea.setColor(p_frontgroundColor);
	}

	void ColorBox::setBackgroundColors(const spk::Color& p_backgroundColor)
	{
		_backgroundColorArea.setColor(p_backgroundColor);
	}

	void ColorBox::setLayer(const float& p_layer)
	{
		_frontgroundColorArea.setLayer(p_layer);
		_backgroundColorArea.setLayer(p_layer);
	}

	void ColorBox::render()
	{
		_backgroundColorArea.render();
		_frontgroundColorArea.render();
	}
}