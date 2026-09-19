#pragma once

#include "geometry/mesh.hpp"
#include "math/vector2.hpp"
#include "math/vector3.hpp"

namespace spk
{
	struct Texture3DVertex
	{
		Vector3 position;
		Vector3 normal;
		Vector2 uv;
	};

	class TextureMesh3D : public Mesh<Texture3DVertex>
	{
	public:
		class Builder : public Mesh<Texture3DVertex>::Builder<TextureMesh3D>
		{
		private:
			void _setupAttributes(VertexBuffer &vertexBuffer) override;
		};
	};
}
