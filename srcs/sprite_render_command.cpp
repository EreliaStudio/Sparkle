#include "sprite_render_command.hpp"
#include <stdexcept>
namespace spk
{
	const SpriteSheet &SpriteRenderCommand::_sheet(const SpriteSheet *s)
	{
		if (!s)
		{
			throw std::invalid_argument("SpriteRenderCommand sprite sheet cannot be null");
		}
		return *s;
	}
	SpriteRenderCommand::SpriteRenderCommand(const SpriteSheet *s, Vector2UInt c, Rect2D r, float d) :
		_command(&_sheet(s), _sheet(s).sprite(c), r, d)
	{
	}
	void SpriteRenderCommand::execute(RenderContext &c) const
	{
		_command.execute(c);
	}
}
