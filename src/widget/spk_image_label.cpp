#include "widget/spk_image_label.hpp"

namespace spk
{
	void ImageLabel::_onGeometryChange()
	{
		_textureRenderer.setGeometry(geometry());
	}

	void ImageLabel::_onPaintEvent(const spk::PaintEvent& p_event)
	{
		_textureRenderer.render();
	}

	ImageLabel::ImageLabel(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_textureRenderer.setLayer(0.0f);
	}

	const WidgetComponent::TextureRenderer& ImageLabel::renderer() const
	{
		return (_textureRenderer);
	}

	WidgetComponent::TextureRenderer& ImageLabel::renderer()
	{
		return (_textureRenderer);
	}
}
