#pragma once

#include "structure/graphics/spk_color.hpp"
#include "structure/math/spk_vector3.hpp"

namespace spk
{
	struct ColorVertex
	{
		spk::Vector3 position = {0, 0, 0};
		spk::Color color = spk::Color::white;

		bool operator==(const ColorVertex &p_other) const
		{
			return (position == p_other.position && color == p_other.color);
		}

		bool operator<(const ColorVertex &p_other) const
		{
			if ((position != p_other.position) == true)
			{
				return (position < p_other.position);
			}
			if ((color.r != p_other.color.r) == true)
			{
				return (color.r < p_other.color.r);
			}
			if ((color.g != p_other.color.g) == true)
			{
				return (color.g < p_other.color.g);
			}
			if ((color.b != p_other.color.b) == true)
			{
				return (color.b < p_other.color.b);
			}
			return (color.a < p_other.color.a);
		}
	};
}

namespace std
{
	template <>
	struct hash<spk::ColorVertex>
	{
		size_t operator()(const spk::ColorVertex &p_vertex) const
		{
			size_t h1 = std::hash<spk::Vector3>{}(p_vertex.position);
			size_t h2 = std::hash<spk::Color>{}(p_vertex.color);
			size_t seed = h1;
			seed ^= h2 + 0x9e3779b9 + (seed << 6) + (seed >> 2);
			return (seed);
		}
	};
}
