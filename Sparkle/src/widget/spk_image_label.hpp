#pragma once

#include "widget/spk_widget.hpp"
#include "widget/components/spk_texture_renderer.hpp"

namespace spk
{
	class ImageLabel : public spk::Widget
	{
	private:
		WidgetComponent::TextureRenderer _renderer;

		void _onGeometryChange();
		void _onRender();

	public:
		ImageLabel(spk::Widget* p_parent = nullptr);
		ImageLabel(const std::string& p_name, spk::Widget* p_parent = nullptr);

		WidgetComponent::TextureRenderer& label();
		const WidgetComponent::TextureRenderer& label() const;
	};
}