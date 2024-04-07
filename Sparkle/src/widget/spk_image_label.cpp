#include "widget/spk_image_label.hpp"

namespace spk
{
	void ImageLabel::_onGeometryChange()
	{
		_renderer.setGeometry(anchor(), size());
	}
	
	void ImageLabel::_onRender()
	{
		_renderer.render();
	}

	ImageLabel::ImageLabel(spk::Widget* p_parent)
	{
		_renderer.setTextureGeometry(0, 1);
	}

	WidgetComponent::TextureRenderer& ImageLabel::label()
	{
		return (_renderer);
	}
	
	const WidgetComponent::TextureRenderer& ImageLabel::label() const
	{
		return (_renderer);
	}
}