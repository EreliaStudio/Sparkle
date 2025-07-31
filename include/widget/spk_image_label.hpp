#pragma once

#include "widget/spk_widget.hpp"

#include "structure/graphics/renderer/spk_nine_slice_renderer.hpp"
#include "structure/graphics/renderer/spk_texture_renderer.hpp"
#include "structure/graphics/texture/spk_texture.hpp"

namespace spk
{
	class ImageLabel : public spk::Widget
	{
	private:
		spk::NineSliceRenderer _backgroundRenderer;
		spk::TextureRenderer _textureRenderer;

		const spk::SafePointer<spk::Texture> _texture = nullptr;
		spk::Texture::Section _textureSection = {{0.0f, 0.0f}, {1.0f, 1.0f}};

		spk::Vector2UInt _cornerSize{10, 10};

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent &p_event) override;

	public:
		ImageLabel(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void setTexture(const spk::SafePointer<spk::Texture> &p_texture);
		const spk::SafePointer<spk::Texture> &texture() const;

		void setSection(const spk::Texture::Section &p_section);
		const spk::Texture::Section &section() const;

		void setNineSlice(spk::SafePointer<const spk::SpriteSheet> p_spriteSheet);
		void setCornerSize(const spk::Vector2UInt &p_cornerSize);
		const spk::Vector2UInt &cornerSize() const;
	};
}