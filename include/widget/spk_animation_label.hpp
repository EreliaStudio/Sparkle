#pragma once

#include "widget/spk_widget.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/graphics/renderer/spk_texture_renderer.hpp"
#include "structure/system/time/spk_timer.hpp"

namespace spk
{
	class AnimationLabel : public spk::Widget
	{
	private:
		spk::TextureRenderer _textureRenderer;
		spk::SafePointer<spk::SpriteSheet> _spriteSheet;
		size_t _currentSprite = 0;
		size_t _rangeStart = 0;
		size_t _rangeEnd = 0;
		spk::Timer _timer = spk::Timer(125_ms);

		void _onGeometryChange() override
		{
			_textureRenderer.clear();
			if (_spriteSheet && _spriteSheet->sprites().size() > 0)
			{
				_textureRenderer.prepare(geometry(), _spriteSheet->sprite(_currentSprite), layer());
				_textureRenderer.validate();
			}
		}

		void _onPaintEvent(spk::PaintEvent&) override
		{
			_textureRenderer.render();
		}

		void _onUpdateEvent(spk::UpdateEvent&) override
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

	public:
		AnimationLabel(const std::wstring& p_name, spk::SafePointer<spk::Widget> p_parent) :
			spk::Widget(p_name, p_parent)
		{
			setSpriteSheet(nullptr);
		}

		void setSpriteSheet(const spk::SafePointer<spk::SpriteSheet>& p_spriteSheet)
		{
			_spriteSheet = p_spriteSheet;
			_textureRenderer.setTexture(p_spriteSheet);
			_currentSprite = 0;
			_rangeStart = 0;
			_rangeEnd = p_spriteSheet && p_spriteSheet->sprites().size() > 0 ? p_spriteSheet->sprites().size() - 1 : 0;
			_timer.stop();
		}

		spk::SafePointer<spk::SpriteSheet> spriteSheet() const
		{
			return _spriteSheet;
		}

		void setLoopSpeed(const spk::Duration& p_duration)
		{
			_timer = spk::Timer(p_duration);
			_currentSprite = _rangeStart;
		}

		void setAnimationRange(size_t start, size_t end)
		{
			_rangeStart = start;
			_rangeEnd = end;
			_currentSprite = start;
		}

		size_t currentFrame() const
		{
			return _currentSprite;
		}
	};
}