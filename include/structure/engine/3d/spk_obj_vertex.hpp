#pragma once

#include <functional>

#include "structure/math/spk_vector2.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct ObjVertex
	{
		spk::Vector3 position = {0, 0, 0};
		spk::Vector2 uv = {-1, -1};
		spk::Vector3 normal = {0, 0, 1};

		bool operator==(const ObjVertex &p_other) const
		{
			return position == p_other.position && uv == p_other.uv && normal == p_other.normal;
		}

		bool operator<(const ObjVertex &p_other) const
		{
			if (position != p_other.position)
			{
				return position < p_other.position;
			}
			if (uv != p_other.uv)
			{
				return uv < p_other.uv;
			}
			return normal < p_other.normal;
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::ObjVertex>
	{
		std::size_t operator()(const spk::ObjVertex &p_v) const
		{
			std::size_t h1 = std::hash<spk::Vector3>{}(p_v.position);
			std::size_t h2 = std::hash<spk::Vector2>{}(p_v.uv);
			std::size_t h3 = std::hash<spk::Vector3>{}(p_v.normal);

			std::size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}