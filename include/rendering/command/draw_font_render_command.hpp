#pragma once

#include <array>
#include <cstddef>

#include "graphics/color.hpp"
#include "graphics/font.hpp"
#include "rendering/render_command.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/mesh/texture_mesh_2d.hpp"

namespace spk
{
	class Program;
	class UniformBuffer;

	class DrawFontRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t AtlasSamplerBindingPoint = 0;
		static constexpr std::size_t FontDataUBOBindingPoint = 1;

		struct FontRenderData
		{
			Color glyphColor;
			Color outlineColor;
			float outlineThickness;
			std::array<float, 3> padding{};
		};

	private:
		static Program &_sharedProgram();
		static UniformBuffer &_sharedBuffer();

		const Font::Atlas *_atlas;
		TextureMesh2D _mesh;
		FontRenderData _data;
		Sampler _sampler;

	public:
		DrawFontRenderCommand(const Font::Atlas *, TextureMesh2D, Color glyphColor, Color outlineColor, float outlineThickness);
		void execute(RenderContext &) const override;
	};

	static_assert(sizeof(DrawFontRenderCommand::FontRenderData) == 48);
}
