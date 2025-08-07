#pragma once

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct VoxelVertex
	{
		spk::Vector3 position = {0, 0, 0};
		spk::Vector2 uv = {0, 0};

		bool operator==(const VoxelVertex &p_other) const
		{
			return (position == p_other.position && uv == p_other.uv);
		}

		bool operator<(const VoxelVertex &p_other) const
		{
			if ((position != p_other.position) == true)
			{
				return (position < p_other.position);
			}
			return (uv < p_other.uv);
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::VoxelVertex>
	{
		std::size_t operator()(const spk::VoxelVertex &p_vertex) const
		{
			std::size_t h1 = std::hash<spk::Vector3>{}(p_vertex.position);
			std::size_t h2 = std::hash<spk::Vector2>{}(p_vertex.uv);
			std::size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return (seed);
		}
	};
}
