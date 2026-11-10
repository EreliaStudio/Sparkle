#pragma once

#include "geometry/texture_mesh_2d.hpp"
#include "graphics/opengl/sampler.hpp"
#include "math/vector2.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class Program;

	class EngineViewCompositionCommand final : public RenderCommand
	{
	private:
		static Program &_sharedProgram();
		static TextureMesh2D _quad(Vector2UInt resolution);

		TextureMesh2D _mesh;
		Sampler _opaque;
		Sampler _accumulation;
		Sampler _revealage;
		ViewportUniformRenderCommand _viewport;
		float _exposure;

	public:
		EngineViewCompositionCommand(
			Texture::Handle opaque,
			Texture::Handle accumulation,
			Texture::Handle revealage,
			Vector2UInt resolution,
			float exposure);
		void execute(RenderContext &context) const override;
	};
}
