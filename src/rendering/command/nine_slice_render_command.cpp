#include "rendering/command/nine_slice_render_command.hpp"
#include <array>
#include <stdexcept>
namespace spk
{
	const SpriteSheet &NineSliceRenderCommand::_sheet(const SpriteSheet *spriteSheet)
	{
		if (!spriteSheet)
		{
			throw std::invalid_argument("NineSliceRenderCommand sprite sheet cannot be null");
		}
		if (spriteSheet->nbSprite() != Vector2UInt{3, 3})
		{
			throw std::invalid_argument("NineSliceRenderCommand requires a 3x3 sprite sheet");
		}
		return *spriteSheet;
	}
	TextureMesh2D NineSliceRenderCommand::_mesh(const SpriteSheet &spriteSheet, Rect2D rect, Vector2UInt corner, float depth)
	{
		if (corner.x > rect.width / 2 || corner.y > rect.height / 2)
		{
			throw std::invalid_argument("Nine-slice corners exceed half the destination size");
		}

		std::array<float, 4> xs{
			float(rect.x),
			float(rect.x + corner.x),
			float(rect.x + rect.width - corner.x),
			float(rect.x + rect.width)};

		std::array<float, 4> ys{
			float(rect.y),
			float(rect.y + corner.y),
			float(rect.y + rect.height - corner.y),
			float(rect.y + rect.height)};

		TextureMesh2D::Builder meshBuilder;
		meshBuilder.reserve(36, 54);
		for (unsigned y = 0; y < 3; y++)
		{
			for (unsigned x = 0; x < 3; x++)
			{
				auto section = spriteSheet.sprite({x, y});

				meshBuilder.addShape(
					{{xs[x], ys[y]}, depth, section.anchor},
					{{xs[x], ys[y + 1]}, depth, {section.anchor.x, section.anchor.y + section.size.y}},
					{{xs[x + 1], ys[y + 1]}, depth, section.anchor + section.size},
					{{xs[x + 1], ys[y]}, depth, {section.anchor.x + section.size.x, section.anchor.y}}
				);
			}
		}
		return std::move(meshBuilder).build();
	}
	NineSliceRenderCommand::NineSliceRenderCommand(const SpriteSheet *spriteSheet, Rect2D rect, Vector2UInt cornerSize, float depth) :
		_command(&_sheet(spriteSheet), _mesh(_sheet(spriteSheet), rect, cornerSize, depth))
	{
	}
	void NineSliceRenderCommand::execute(RenderContext &context) const
	{
		_command.execute(context);
	}
}
