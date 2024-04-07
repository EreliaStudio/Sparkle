#include "widget/spk_image_label.hpp"

namespace spk
{
	void ImageLabel::_onGeometryChange()
	{
		_renderer.setGeometry(anchor(), size());
		_renderer.setLayer(layer());
	}
	
	void ImageLabel::_onRender()
	{
		_renderer.render();
	}

	ImageLabel::ImageLabel(spk::Widget* p_parent) :
		spk::Widget(p_parent)
	{
		_renderer.setTextureGeometry(0, 1);
	}

	ImageLabel::ImageLabel(const std::string& p_name, spk::Widget* p_parent) :
		spk::Widget(p_name, p_parent)
	{

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