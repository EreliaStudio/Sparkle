#pragma once

#include <cstdint>
#include <memory>

#include "geometry/mesh.hpp"
#include "graphics/opengl/layout_buffer.hpp"
#include "math/vector2.hpp"

namespace spk
{
	struct Texture2DVertex
	{
		Vector2 position;
		float depth;
		Vector2 uv;
	};

	class TextureMesh2D :
		public Mesh<Texture2DVertex>
	{
	public:
		class Builder : public Mesh<Texture2DVertex>::Builder<TextureMesh2D>
		{
		private:
			void _setupAttributes(VertexBuffer &vertexBuffer) override;
			void _setupMesh(TextureMesh2D&) override;
		};

		TextureMesh2D() = default;
	};
}
