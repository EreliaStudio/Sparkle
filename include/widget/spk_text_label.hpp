#pragma once

#include "widget/spk_widget.hpp"

#include "widget/component/spk_text_renderer.hpp"

namespace spk
{
	class TextLabel : public spk::Widget
	{
	private:
		WidgetComponent::TextRenderer _textRenderer;

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;

	public:
		TextLabel(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent);

		const WidgetComponent::TextRenderer& renderer() const;
		WidgetComponent::TextRenderer& renderer();
	};
}