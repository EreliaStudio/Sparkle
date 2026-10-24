#pragma once

#include <cstddef>

#include "geometry/texture_mesh_2d.hpp"
#include "graphics/opengl/sampler.hpp"
#include "graphics/sprite_sheet.hpp"
#include "math/rect2d.hpp"
#include "math/vector4.hpp"
#include "rendering/render_command.hpp"

namespace spk
{
	class Program;
	class UniformBuffer;

	class NineSliceRenderCommand final : public RenderCommand
	{
	public:
		static constexpr std::size_t TextureSamplerBindingPoint = 0;
		static constexpr std::size_t RenderDataUBOBindingPoint = 1;

	private:
		struct RenderData
		{
			Vector4Int geometry;
			Vector4UInt corner;
		};
		static_assert(sizeof(RenderData) == 32);

		static const SpriteSheet &_sheet(const SpriteSheet *);
		static TextureMesh2D _mesh(Rect2D, Vector2UInt, float);
		static Program &_sharedProgram();
		static UniformBuffer &_sharedBuffer();

		TextureMesh2D _meshData;
		RenderData _data;
		Sampler _sampler;

	public:
		NineSliceRenderCommand(const SpriteSheet *, Rect2D, Vector2UInt cornerSize, float depth = 0);
		void execute(RenderContext &) const override;
	};
}
