#pragma once

#include <cstddef>

#include "rendering/render_command.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/mesh/texture_mesh_2d.hpp"

namespace spk
{
	class Program;
	class Texture;

	class DrawTextureMeshRenderCommand final : public RenderCommand
	{
	private:
		static Program &_sharedProgram();

		const Texture *_texture;
		TextureMesh2D _mesh;
		Sampler _sampler;

	public:
		static constexpr std::size_t TextureSamplerBindingPoint = 0;
		DrawTextureMeshRenderCommand(const Texture *texture, TextureMesh2D mesh);
		void execute(RenderContext &) const override;
	};
}
