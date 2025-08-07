#pragma once

#include "structure/engine/spk_mesh.hpp"
#include "structure/engine/spk_voxel_vertex.hpp"

namespace spk
{
        class VoxelMesh : public TMesh<VoxelVertex>
        {
        public:
                VoxelMesh() = default;
        };
}
