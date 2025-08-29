#pragma once

#include <functional>

#include "structure/math/spk_vector3.hpp"
#include "structure/math/spk_vector2.hpp"

namespace spk
{
	struct ObjVertex
	{
		spk::Vector3 position = {0, 0, 0};
		spk::Vector2 uv = {-1, -1};
		spk::Vector3 normal = {0, 0, 1};

		bool operator==(const ObjVertex& other) const
		{
			return position == other.position &&
				uv == other.uv &&
				normal == other.normal;
		}

		bool operator<(const ObjVertex& other) const
		{
			if (position != other.position)
				return position < other.position;
			if (uv != other.uv)
				return uv < other.uv;
			return normal < other.normal;
		}
	};
}

namespace std
{
	template<>
	struct hash<spk::ObjVertex>
	{
		std::size_t operator()(const spk::ObjVertex& v) const
		{
			std::size_t h1 = std::hash<spk::Vector3>{}(v.position);
			std::size_t h2 = std::hash<spk::Vector2>{}(v.uv);
			std::size_t h3 = std::hash<spk::Vector3>{}(v.normal);

			std::size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			seed ^= h3 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return seed;
		}
	};
}