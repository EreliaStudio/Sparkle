#include "widget/spk_image_label.hpp"

namespace spk
{
	void ImageLabel::_onGeometryChange()
	{
		_textureRenderer.clear();
		_textureRenderer.prepare(geometry(), _textureSection, layer());
		_textureRenderer.validate();
	}

	void ImageLabel::_onPaintEvent(spk::PaintEvent& p_event)
	{
		_textureRenderer.render();
	}

	ImageLabel::ImageLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_textureRenderer.setTexture(_texture);
	}

	void ImageLabel::setTexture(const spk::SafePointer<spk::Texture>& p_texture)
	{
		if (_texture != p_texture)
		{
			_textureRenderer.setTexture(p_texture);
			requestPaint();
		}
	}

	const spk::SafePointer<spk::Texture>& ImageLabel::texture() const
	{
		return _texture;
	}

	void ImageLabel::setSection(const spk::Texture::Section& p_section)
	{
		if (_textureSection != p_section)
		{
			_textureSection = p_section;
			requireGeometryUpdate();
		}
	}

	const spk::Texture::Section& ImageLabel::section() const
	{
		return _textureSection;
	}
}