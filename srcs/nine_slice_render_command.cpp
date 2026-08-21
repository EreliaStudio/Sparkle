#include "nine_slice_render_command.hpp"
#include <array>
#include <stdexcept>
namespace spk
{
	const SpriteSheet &NineSliceRenderCommand::_sheet(const SpriteSheet *s)
	{
		if (!s)
		{
			throw std::invalid_argument("NineSliceRenderCommand sprite sheet cannot be null");
		}
		if (s->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("NineSliceRenderCommand requires a 3x3 sprite sheet");
		}
		return *s;
	}
	TextureMesh2D NineSliceRenderCommand::_mesh(const SpriteSheet &s, Rect2D r, Vector2UInt corner, float d)
	{
		if (corner.x > r.width / 2 || corner.y > r.height / 2)
		{
			throw std::invalid_argument("Nine-slice corners exceed half the destination size");
		}
		std::array<float, 4> xs{float(r.x), float(r.x + corner.x), float(r.x + r.width - corner.x), float(r.x + r.width)}, ys{float(r.y), float(r.y + corner.y), float(r.y + r.height - corner.y), float(r.y + r.height)};
		TextureMesh2D::Builder m;
		m.reserve(36, 54);
		for (unsigned y = 0; y < 3; y++)
		{
			for (unsigned x = 0; x < 3; x++)
			{
				auto q = s.sprite({x, y});
				m.addShape({{xs[x], ys[y]}, d, q.anchor}, {{xs[x], ys[y + 1]}, d, {q.anchor.x, q.anchor.y + q.size.y}}, {{xs[x + 1], ys[y + 1]}, d, q.anchor + q.size}, {{xs[x + 1], ys[y]}, d, {q.anchor.x + q.size.x, q.anchor.y}});
			}
		}
		return std::move(m).build();
	}
	NineSliceRenderCommand::NineSliceRenderCommand(const SpriteSheet *s, Rect2D r, Vector2UInt c, float d) :
		_command(&_sheet(s), _mesh(_sheet(s), r, c, d))
	{
	}
	void NineSliceRenderCommand::execute(RenderContext &c) const
	{
		_command.execute(c);
	}
}
