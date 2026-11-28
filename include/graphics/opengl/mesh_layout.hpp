#pragma once

#include "geometry/mesh.hpp"
#include "graphics/opengl/layout_buffer.hpp"

namespace spk
{
	// Graphics-side layouts are shared by all copies of the same CPU mesh.
	// The returned layout is valid only while an owner of its mesh data exists.
	class MeshLayout
	{
	public:
		[[nodiscard]] static const LayoutBuffer &obtain(const MeshData &mesh);

		template <typename TVertex>
		[[nodiscard]] static const LayoutBuffer &obtain(const Mesh<TVertex> &mesh)
		{
			return obtain(mesh.content());
		}
	};
}
