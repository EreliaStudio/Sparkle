#pragma once

#include "structure/graphics/painter/spk_texture_painter.hpp"
#include "structure/graphics/texture/spk_sprite_sheet.hpp"
#include "structure/system/time/spk_timer.hpp"
#include "widget/spk_widget.hpp"

namespace spk
{
	class AnimationLabel : public spk::Widget
	{
	private:
		spk::TexturePainter _textureRenderer;
		spk::SafePointer<spk::SpriteSheet> _spriteSheet;
		size_t _currentSprite = 0;
		size_t _rangeStart = 0;
		size_t _rangeEnd = 0;
		spk::Timer _timer = spk::Timer(125_ms);

		void _onGeometryChange() override;
		void _onPaintEvent(spk::PaintEvent &) override;
		void _onUpdateEvent(spk::UpdateEvent &) override;

	public:
		AnimationLabel(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent);

		void setSpriteSheet(const spk::SafePointer<spk::SpriteSheet> &p_spriteSheet);
		spk::SafePointer<spk::SpriteSheet> spriteSheet() const;

		void setLoopSpeed(const spk::Duration &p_duration);

		void setAnimationRange(size_t start, size_t end);

		size_t currentFrame() const;
	};
}