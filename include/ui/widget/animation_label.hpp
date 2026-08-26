#pragma once

#include <chrono>
#include <cstddef>
#include <string>

#include "graphics/sprite_sheet.hpp"
#include "ui/widget.hpp"

namespace spk
{
	class AnimationLabel : public Widget
	{
	public:
		using Duration = std::chrono::steady_clock::duration;

	private:
		const SpriteSheet *_spriteSheet = nullptr;
		std::size_t _currentFrame = 0;
		std::size_t _rangeStart = 0;
		std::size_t _rangeEnd = 0;
		float _depth = 0.0f;
		Duration _loopSpeed = std::chrono::milliseconds(125);
		Duration _elapsed{};

		[[nodiscard]] std::size_t _spriteCount() const noexcept;
		void _advanceFrame();
		void _updateState(UpdateContext &context) override;
		void _buildRenderSnapshot(RenderSnapshot::Builder &builder) override;

	public:
		explicit AnimationLabel(std::string name, Widget *parent = nullptr);
		AnimationLabel(std::string name, const SpriteSheet *spriteSheet, Widget *parent = nullptr);
		void applyStyle(const Style &style) override;

		void setSpriteSheet(const SpriteSheet *spriteSheet);
		void setLoopSpeed(Duration duration);
		void setAnimationRange(std::size_t start, std::size_t end);
		void setDepth(float depth);

		[[nodiscard]] const SpriteSheet *spriteSheet() const noexcept;
		[[nodiscard]] std::size_t currentFrame() const noexcept;
		[[nodiscard]] std::size_t rangeStart() const noexcept;
		[[nodiscard]] std::size_t rangeEnd() const noexcept;
		[[nodiscard]] Duration loopSpeed() const noexcept;
		[[nodiscard]] float depth() const noexcept;
	};
}
