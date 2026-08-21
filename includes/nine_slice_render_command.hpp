#pragma once

#include "draw_texture_mesh_render_command.hpp"
#include "rect2d.hpp"
#include "sprite_sheet.hpp"

namespace spk
{
	class NineSliceRenderCommand final : public RenderCommand
	{
	private:
		static const SpriteSheet &_sheet(const SpriteSheet *);
		static TextureMesh2D _mesh(const SpriteSheet &, Rect2D, Vector2UInt, float);

		DrawTextureMeshRenderCommand _command;

	public:
		NineSliceRenderCommand(const SpriteSheet *, Rect2D, Vector2UInt cornerSize, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
