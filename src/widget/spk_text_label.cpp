#include "widget/spk_text_label.hpp"

namespace spk
{
	void TextLabel::_onGeometryChange()
	{
		_textRenderer.setGeometry(geometry());
	}

	void TextLabel::_onPaintEvent(spk::PaintEvent& p_event)
	{
		_textRenderer.render();
	}

	TextLabel::TextLabel(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent) :
		spk::Widget(p_name, p_parent)
	{
		_textRenderer.setLayer(0.0f);
	}

	const WidgetComponent::TextRenderer& TextLabel::renderer() const
	{
		return (_textRenderer);
	}
	
	WidgetComponent::TextRenderer& TextLabel::renderer()
	{
		return (_textRenderer);
	}
}