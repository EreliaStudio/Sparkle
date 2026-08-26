#include "ui/widget/panel.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

#include "rendering/command/nine_slice_render_command.hpp"

namespace spk
{
	Panel::Panel(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		activate();
	}

	Panel::Panel(std::string name, const SpriteSheet *spriteSheet, Widget *parent) :
		Panel(std::move(name), parent)
	{
		setSpriteSheet(spriteSheet);
	}

	void Panel::applyStyle(const Style &style)
	{
		if (style.nineSlice != nullptr)
		{
			setSpriteSheet(style.nineSlice.get());
		}
	}

	void Panel::_updateSizeHint()
	{
		const Vector2 intrinsicSize{
			static_cast<float>(_cornerSize.x * 2),
			static_cast<float>(_cornerSize.y * 2)};
		SizeHint hint = sizeHint();
		hint.minimal = intrinsicSize;
		hint.preferred = intrinsicSize;
		setSizeHint(hint);
	}

	void Panel::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		if (_spriteSheet == nullptr || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		const Vector2UInt renderedCornerSize{
			std::min(static_cast<unsigned int>(_cornerSize.x), geometry().width / 2),
			std::min(static_cast<unsigned int>(_cornerSize.y), geometry().height / 2)};
		builder.renderPass(targetRenderPass()).emplace<NineSliceRenderCommand>(_spriteSheet, Rect2D{Vector2Int{0, 0}, geometry().size}, renderedCornerSize, _depth);
	}

	void Panel::setSpriteSheet(const SpriteSheet *spriteSheet)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("Panel sprite sheet cannot be null");
		}
		if (spriteSheet->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("Panel requires a 3x3 sprite sheet");
		}

		_spriteSheet = spriteSheet;
		if (!_hasExplicitCornerSize)
		{
			_cornerSize = Vector2Int{
				static_cast<int>(spriteSheet->size().x / 3),
				static_cast<int>(spriteSheet->size().y / 3)};
		}
		_updateSizeHint();
	}

	void Panel::setCornerSize(const Vector2Int &cornerSize)
	{
		if (cornerSize.x < 0 || cornerSize.y < 0)
		{
			throw std::invalid_argument("Panel corner size cannot be negative");
		}
		_cornerSize = cornerSize;
		_hasExplicitCornerSize = true;
		_updateSizeHint();
	}

	void Panel::setDepth(float depth)
	{
		_depth = depth;
	}

	const SpriteSheet *Panel::spriteSheet() const noexcept
	{
		return _spriteSheet;
	}

	const Vector2Int &Panel::cornerSize() const noexcept
	{
		return _cornerSize;
	}

	float Panel::depth() const noexcept
	{
		return _depth;
	}
}
