#pragma once

#include "graphics/sprite_sheet.hpp"
#include "rendering/command/image_render_command.hpp"

namespace spk
{
	class SpriteRenderCommand final : public RenderCommand
	{
	private:
		static const SpriteSheet &_sheet(const SpriteSheet *);

		ImageRenderCommand _command;

	public:
		SpriteRenderCommand(const SpriteSheet *, Vector2UInt, Rect2D, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
