#include "widget/spk_animation_label.hpp"

namespace spk
{
	void AnimationLabel::_onGeometryChange()
	{
		_textureRenderer.clear();
		if (_spriteSheet && _spriteSheet->sprites().size() > 0)
		{
			_textureRenderer.prepare(geometry().atOrigin(), _spriteSheet->sprite(_currentSprite), layer());
			_textureRenderer.validate();
		}
	}

	void AnimationLabel::_onPaintEvent(spk::PaintEvent&)
	{
		_textureRenderer.render();
	}

	void AnimationLabel::_onUpdateEvent(spk::UpdateEvent&)
	{
		if (_timer.state() != spk::Timer::State::Running && _spriteSheet && _spriteSheet->sprites().size() > 0)
		{
			if (_rangeEnd >= _rangeStart && _rangeEnd < _spriteSheet->sprites().size())
			{
				_currentSprite++;

				if (_currentSprite > _rangeEnd)
				{
					_currentSprite = _rangeStart;
				}
			}
			else
			{
				_currentSprite = (_currentSprite + 1) % _spriteSheet->sprites().size();
			}

			requireGeometryUpdate();
			_timer.start();
		}
	}

	AnimationLabel::AnimationLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent)
	{
		setSpriteSheet(nullptr);
	}

	void AnimationLabel::setSpriteSheet(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet)
	{
		_spriteSheet = p_spriteSheet;
		_textureRenderer.setTexture(p_spriteSheet);
		_currentSprite = 0;
		_rangeStart = 0;
		_rangeEnd = p_spriteSheet && p_spriteSheet->sprites().size() > 0 ? p_spriteSheet->sprites().size() - 1 : 0;
		_timer.stop();
	}

	spk::SafePointer<spk::SpriteSheet> AnimationLabel::spriteSheet() const
	{
		return _spriteSheet;
	}

	void AnimationLabel::setLoopSpeed(const spk::Duration& p_duration)
	{
		_timer = spk::Timer(p_duration);
		_currentSprite = _rangeStart;
	}

	void AnimationLabel::setAnimationRange(size_t p_start, size_t p_end)
	{
		_rangeStart = p_start;
		_rangeEnd = p_end;
		_currentSprite = p_start;
	}

	size_t AnimationLabel::currentFrame() const
	{
		return _currentSprite;
	}
}