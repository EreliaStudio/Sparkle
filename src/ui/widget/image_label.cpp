#include "ui/widget/image_label.hpp"

#include <stdexcept>
#include <utility>

#include "graphics/image.hpp"
#include "rendering/command/image_render_command.hpp"

namespace spk
{
	ImageLabel::ImageLabel(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		activate();
	}

	ImageLabel::ImageLabel(std::string name, const Texture *texture, Widget *parent) :
		ImageLabel(std::move(name), parent)
	{
		setTexture(texture);
	}

	void ImageLabel::applyStyle(const Style &style)
	{
		if (style.iconsetImage != nullptr)
		{
			setTexture(style.iconsetImage.get());
		}
	}

	void ImageLabel::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (_texture == nullptr || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		builder.renderPass(targetRenderPass()).emplace<ImageRenderCommand>(_texture, _section, Rect2D{Vector2Int{0, 0}, geometry().size}, _depth);
	}

	void ImageLabel::setTexture(const Texture *texture)
	{
		if (texture == nullptr)
		{
			throw std::invalid_argument("ImageLabel texture cannot be null");
		}
		_texture = texture;
	}

	void ImageLabel::setSection(const Texture::Section &section)
	{
		_section = section;
	}

	void ImageLabel::setDepth(float depth)
	{
		_depth = depth;
	}

	const Texture *ImageLabel::texture() const noexcept
	{
		return _texture;
	}

	const Texture::Section &ImageLabel::section() const noexcept
	{
		return _section;
	}

	float ImageLabel::depth() const noexcept
	{
		return _depth;
	}
}
