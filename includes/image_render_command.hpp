#pragma once

#include "draw_texture_mesh_render_command.hpp"
#include "rect2d.hpp"
#include "texture.hpp"

namespace spk
{
	class ImageRenderCommand final : public RenderCommand
	{
	private:
		DrawTextureMeshRenderCommand _command;
		static TextureMesh2D _mesh(Texture::Section, Rect2D, float);

	public:
		ImageRenderCommand(const Texture *, Texture::Section, Rect2D, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
