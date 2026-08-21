#pragma once

#include "color_mesh_2d.hpp"
#include "render_command.hpp"

namespace spk
{
	class Program;

	class DrawColorMeshRenderCommand final : public RenderCommand
	{
	private:
		static Program &_sharedProgram();

		ColorMesh2D _mesh;

	public:
		explicit DrawColorMeshRenderCommand(ColorMesh2D mesh);
		void execute(RenderContext &) const override;
	};
}
