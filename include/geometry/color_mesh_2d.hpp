#pragma once

#include <cstdint>
#include <memory>

#include "geometry/mesh.hpp"
#include "graphics/color.hpp"
#include "graphics/opengl/layout_buffer.hpp"
#include "math/vector2.hpp"

namespace spk
{
	struct ColorVertex2D
	{
		Vector2 position;
		float depth;
		Color color;
	};

	class ColorMesh2D :
		public Mesh<ColorVertex2D>
	{
	public:
		class Builder :
			public Mesh<ColorVertex2D>::Builder<ColorMesh2D>
		{
		private:
			void _setupAttributes(VertexBuffer &vertexBuffer) override;
			void _setupMesh(ColorMesh2D&) override;
		};

		ColorMesh2D() = default;
	};
}
