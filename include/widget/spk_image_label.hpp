#pragma once

#include "widget/spk_widget.hpp"

#include "structure/graphics/texture/spk_texture.hpp"
#include "structure/graphics/renderer/spk_texture_renderer.hpp"

namespace spk
{
	class ImageLabel : public spk::Widget
	{
	private:
		spk::TextureRenderer _textureRenderer;

		const spk::SafePointer<spk::Texture> _texture = nullptr;
		spk::Texture::Section _textureSection = spk::Texture::Section({0.0f, 0.0f}, {1.0f, 1.0f});

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent& p_event) override;

	public:
		ImageLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent);

		void setTexture(const spk::SafePointer<spk::Texture>& p_texture);
		const spk::SafePointer<spk::Texture>& texture() const;

		void setSection(const spk::Texture::Section& p_section);
		const spk::Texture::Section& section() const;
	};
}