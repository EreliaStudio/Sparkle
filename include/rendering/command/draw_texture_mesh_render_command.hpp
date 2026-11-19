#pragma once

#include <cstddef>

#include "geometry/texture_mesh_2d.hpp"
#include "graphics/opengl/sampler.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class Program;
	class Texture;

	class DrawTextureMeshRenderCommand final : public RenderCommand
	{
	private:
		static Program &_sharedProgram();

		TextureMesh2D _mesh;
		Sampler _sampler;
		bool _presentSRGB = false;

	public:
		static constexpr std::size_t TextureSamplerBindingPoint = 0;
		DrawTextureMeshRenderCommand(Texture::Handle texture, TextureMesh2D mesh, bool presentSRGB = false);
		DrawTextureMeshRenderCommand(const Texture *texture, TextureMesh2D mesh, bool presentSRGB = false);
		void execute(RenderContext &) const override;
	};
}
