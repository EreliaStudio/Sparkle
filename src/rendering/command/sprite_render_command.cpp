#include "rendering/command/sprite_render_command.hpp"
#include <stdexcept>
namespace spk
{
	const SpriteSheet &SpriteRenderCommand::_sheet(const SpriteSheet *spriteSheet)
	{
		if (!spriteSheet)
		{
			throw std::invalid_argument("SpriteRenderCommand sprite sheet cannot be null");
		}
		return *spriteSheet;
	}
	SpriteRenderCommand::SpriteRenderCommand(const SpriteSheet *spriteSheet, Vector2UInt spriteCoordinate, Rect2D rect, float depth) :
		_command(&_sheet(spriteSheet), _sheet(spriteSheet).sprite(spriteCoordinate), rect, depth)
	{
	}
	void SpriteRenderCommand::execute(RenderContext &spriteCoordinate) const
	{
		_command.execute(spriteCoordinate);
	}
}
