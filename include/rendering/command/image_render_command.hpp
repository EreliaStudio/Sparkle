#pragma once

#include "rendering/command/draw_texture_mesh_render_command.hpp"
#include "math/rect2d.hpp"
#include "graphics/opengl/texture.hpp"

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
