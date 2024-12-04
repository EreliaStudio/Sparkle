#include "widget/spk_image_label.hpp"

namespace spk
{
	void ImageLabel::_onGeometryChange()
	{
		_backgroundRenderer.clear();
		_backgroundRenderer.prepare(geometry().atOrigin(), layer(), _cornerSize);
		_backgroundRenderer.validate();

		spk::Geometry2D inner = geometry().atOrigin().shrink(_cornerSize);
		_textureRenderer.clear();
		_textureRenderer.prepare(inner, _textureSection, layer() + 0.01f);
		_textureRenderer.validate();
	}

	void ImageLabel::_onPaintEvent(spk::PaintEvent &p_event)
	{
		_backgroundRenderer.render();
		_textureRenderer.render();
	}

	ImageLabel::ImageLabel(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_backgroundRenderer.setSpriteSheet(Widget::defaultNineSlice());
		_textureRenderer.setTexture(_texture);
	}

	void ImageLabel::setTexture(const spk::SafePointer<spk::Texture> &p_texture)
	{
		if (_texture != p_texture)
		{
			_textureRenderer.setTexture(p_texture);
			_requestPaint();
		}
	}

	const spk::SafePointer<spk::Texture> &ImageLabel::texture() const
	{
		return _texture;
	}

	void ImageLabel::setSection(const spk::Texture::Section &p_section)
	{
		if (_textureSection != p_section)
		{
			_textureSection = p_section;
			requireGeometryUpdate();
		}
	}

	const spk::Texture::Section &ImageLabel::section() const
	{
		return _textureSection;
	}

	void ImageLabel::setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet)
	{
		_backgroundRenderer.setSpriteSheet(p_spriteSheet);
	}

	void ImageLabel::setCornerSize(const spk::Vector2UInt &p_cornerSize)
	{
		if (_cornerSize != p_cornerSize)
		{
			_cornerSize = p_cornerSize;
			requireGeometryUpdate();
		}
	}

	const spk::Vector2UInt &ImageLabel::cornerSize() const
	{
		return _cornerSize;
	}
}