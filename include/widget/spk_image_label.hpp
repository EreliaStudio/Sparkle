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

		void _onGeometryChange() override
		{
			_textureRenderer.clear();
			_textureRenderer.prepare(geometry(), _textureSection, layer());
			_textureRenderer.validate();
		}

		void _onPaintEvent(spk::PaintEvent& p_event) override
		{
			_textureRenderer.render();
		}

	public:
		ImageLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent)
		{
			_textureRenderer.setTexture(_texture);
		}

		void setTexture(const spk::SafePointer<spk::Texture>& p_texture)
		{
			if (_texture != p_texture)
			{
				_textureRenderer.setTexture(p_texture);
				requestPaint();
			}
		}

		const spk::SafePointer<spk::Texture>& texture() const
		{
			return _texture;
		}

		void setSection(const spk::Texture::Section& p_section)
		{
			if (_textureSection != p_section)
			{
				_textureSection = p_section;
				requireGeometryUpdate();
			}
		}

		const spk::Texture::Section& section() const
		{
			return _textureSection;
		}
	};
}