#include "widget/spk_frame.hpp"

#include "structure/graphics/spk_viewport.hpp"

namespace spk
{
	Frame::Frame(const std::wstring &p_name, const spk::SafePointer<spk::Widget> &p_parent) :
		Widget(p_name, p_parent),
		_cornerSize(8, 8)
	{
		setNineSlice(Widget::defaultNineSlice());
	}

	void Frame::setNineSlice(const SafePointer<const spk::SpriteSheet> &p_spriteSheet)
	{
		_frameRenderer.setSpriteSheet(p_spriteSheet);
	}

	SafePointer<const spk::SpriteSheet> Frame::nineslice() const
	{
		return (_frameRenderer.spriteSheet());
	}

	void Frame::setCornerSize(const spk::Vector2Int &p_cornerSize)
	{
		_cornerSize = p_cornerSize;
		requireGeometryUpdate();
	}

	const spk::Vector2Int &Frame::cornerSize() const
	{
		return _cornerSize;
	}

	void Frame::_onGeometryChange()
	{
		_frameRenderer.clear();

		_frameRenderer.prepare(geometry().atOrigin(), layer(), _cornerSize);

		_frameRenderer.validate();
	}

	void Frame::_onPaintEvent(PaintEvent &p_event)
	{
		_frameRenderer.render();
	}
}
