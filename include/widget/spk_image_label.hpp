#pragma once

#include "widget/spk_widget.hpp"
#include "widget/component/spk_texture_renderer.hpp"

namespace spk
{
	class ImageLabel : public spk::Widget
	{
	private:
		WidgetComponent::TextureRenderer _textureRenderer;

		void _onGeometryChange() override;
		void _onPaintEvent(const spk::PaintEvent& p_event) override;

	public:
		ImageLabel(const std::wstring& p_name, const spk::SafePointer<Widget>& p_parent);

		const WidgetComponent::TextureRenderer& renderer() const;
		WidgetComponent::TextureRenderer& renderer();
	};
}
