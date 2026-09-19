#pragma once

#include <string>

#include "graphics/opengl/texture.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class ImageLabel : public Widget
	{
	private:
		const Texture *_texture = nullptr;
		Texture::Section _section = Texture::Section::whole;
		float _depth = 0.0f;

		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		explicit ImageLabel(std::string name, Widget *parent = nullptr);
		ImageLabel(std::string name, const Texture *texture, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setTexture(const Texture *texture);
		void setSection(const Texture::Section &section);
		void setDepth(float depth);

		[[nodiscard]] const Texture *texture() const noexcept;
		[[nodiscard]] const Texture::Section &section() const noexcept;
		[[nodiscard]] float depth() const noexcept;
	};
}
