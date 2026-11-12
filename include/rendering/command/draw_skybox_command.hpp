#pragma once

#include <array>

#include "geometry/texture_mesh_3d.hpp"
#include "graphics/opengl/program.hpp"
#include "graphics/opengl/sampler.hpp"
#include "math/matrix.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class DrawSkyboxCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t FaceCount = 6;
		using Sections = std::array<Texture::Section, FaceCount>;

	private:
		TextureMesh3D _mesh;
		Program _program;
		Sampler _albedo;
		Matrix4x4 _projection;
		Matrix4x4 _viewRotation;

		[[nodiscard]] static Program &_sharedProgram();
		[[nodiscard]] static TextureMesh3D _buildMesh(const Sections &sections);

	public:
		DrawSkyboxCommand(
			Texture::Handle albedo,
			const Sections &sections,
			Matrix4x4 projection,
			Matrix4x4 view);

		void execute(RenderContext &context) const override;
	};
}
