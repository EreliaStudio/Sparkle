#include "ui/widget/animation_label.hpp"

#include <stdexcept>
#include <utility>

#include "core/context/update_context.hpp"
#include "rendering/command/sprite_render_command.hpp"

namespace spk
{
	AnimationLabel::AnimationLabel(std::string name, Widget *parent) :
		Widget(std::move(name), parent)
	{
		applyStyle(defaultStyle);
		activate();
	}

	AnimationLabel::AnimationLabel(std::string name, const SpriteSheet *spriteSheet, Widget *parent) :
		AnimationLabel(std::move(name), parent)
	{
		setSpriteSheet(spriteSheet);
	}

	void AnimationLabel::applyStyle(const Style &style)
	{
		if (style.iconset != nullptr)
		{
			setSpriteSheet(style.iconset.get());
		}
	}

	std::size_t AnimationLabel::_spriteCount() const noexcept
	{
		if (_spriteSheet == nullptr)
		{
			return 0;
		}
		return static_cast<std::size_t>(_spriteSheet->nbSprite().x) * static_cast<std::size_t>(_spriteSheet->nbSprite().y);
	}

	void AnimationLabel::_advanceFrame()
	{
		const std::size_t count = _spriteCount();
		if (count == 0)
		{
			_currentFrame = 0;
			return;
		}

		const bool rangeIsValid = _rangeStart <= _rangeEnd && _rangeEnd < count;
		if (!rangeIsValid)
		{
			_currentFrame = _currentFrame + 1 < count ? _currentFrame + 1 : 0;
			return;
		}

		if (_currentFrame < _rangeStart || _currentFrame >= _rangeEnd)
		{
			_currentFrame = _rangeStart;
		}
		else
		{
			++_currentFrame;
		}
	}

	void AnimationLabel::_updateState(UpdateContext &context)
	{
		if (_spriteCount() == 0)
		{
			return;
		}

		if (_loopSpeed <= Duration::zero())
		{
			_advanceFrame();
			_elapsed = Duration::zero();
			return;
		}

		_elapsed += context.deltaTime;
		while (_elapsed >= _loopSpeed)
		{
			_elapsed -= _loopSpeed;
			_advanceFrame();
		}
	}

	void AnimationLabel::_buildRenderSnapshot(RenderSnapshot::Builder &builder)
	{
		const std::size_t count = _spriteCount();
		if (_spriteSheet == nullptr || count == 0 || geometry().width == 0 || geometry().height == 0)
		{
			return;
		}

		const std::size_t renderedFrame = _currentFrame < count ? _currentFrame : 0;
		const std::size_t columns = _spriteSheet->nbSprite().x;
		const Vector2UInt coordinates{
			static_cast<unsigned int>(renderedFrame % columns),
			static_cast<unsigned int>(renderedFrame / columns)};
		builder.renderPass(targetRenderPass()).emplace<SpriteRenderCommand>(_spriteSheet, coordinates, Rect2D{Vector2Int{0, 0}, geometry().size}, _depth);
	}

	void AnimationLabel::setSpriteSheet(const SpriteSheet *spriteSheet)
	{
		if (spriteSheet == nullptr)
		{
			throw std::invalid_argument("AnimationLabel sprite sheet cannot be null");
		}

		_spriteSheet = spriteSheet;
		_currentFrame = 0;
		_rangeStart = 0;
		const std::size_t count = _spriteCount();
		_rangeEnd = count == 0 ? 0 : count - 1;
		_elapsed = Duration::zero();
	}

	void AnimationLabel::setLoopSpeed(Duration duration)
	{
		_loopSpeed = duration;
		_currentFrame = _rangeStart;
		_elapsed = Duration::zero();
	}

	void AnimationLabel::setAnimationRange(std::size_t start, std::size_t end)
	{
		if (end < start)
		{
			throw std::invalid_argument("AnimationLabel range end cannot precede its start");
		}
		_rangeStart = start;
		_rangeEnd = end;
		_currentFrame = start;
		_elapsed = Duration::zero();
	}

	void AnimationLabel::setDepth(float depth)
	{
		_depth = depth;
	}

	const SpriteSheet *AnimationLabel::spriteSheet() const noexcept
	{
		return _spriteSheet;
	}

	std::size_t AnimationLabel::currentFrame() const noexcept
	{
		return _currentFrame;
	}

	std::size_t AnimationLabel::rangeStart() const noexcept
	{
		return _rangeStart;
	}

	std::size_t AnimationLabel::rangeEnd() const noexcept
	{
		return _rangeEnd;
	}

	AnimationLabel::Duration AnimationLabel::loopSpeed() const noexcept
	{
		return _loopSpeed;
	}

	float AnimationLabel::depth() const noexcept
	{
		return _depth;
	}
}
