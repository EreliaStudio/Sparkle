#pragma once

#include "graphics/opengl/texture.hpp"
#include "math/rect2d.hpp"
#include "rendering/command/draw_texture_mesh_render_command.hpp"

namespace spk
{
	class ImageRenderCommand final : public RenderCommand
	{
	private:
		DrawTextureMeshRenderCommand _command;

		static DrawTextureMeshRenderCommand _resolve(const Texture *, Texture::Section, Rect2D, float);
		static Texture::Section _texelCenteredSection(Texture::Section, const Vector2UInt &);
		static TextureMesh2D _mesh(Texture::Section, Rect2D, float, const Vector2UInt &);

	public:
		ImageRenderCommand(Texture::Handle, Texture::Section, Rect2D, float depth = 0);
		ImageRenderCommand(const Texture *, Texture::Section, Rect2D, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
