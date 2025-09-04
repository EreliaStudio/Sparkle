#include "structure/engine/spk_primitive_2d.hpp"

#include <cmath>
#include <numbers>
#include <vector>

namespace spk
{
	namespace Primitive2D
	{
		spk::Mesh2D makeSquare(const spk::Vector2 &p_size, const spk::SpriteSheet::Section &p_uv)
		{
			spk::Mesh2D mesh;
			const spk::Vector2 half = p_size / 2.0f;

			mesh.addShape(
				{{{-half.x, -half.y}, {p_uv.anchor.x, p_uv.anchor.y + p_uv.size.y}},
				 {{half.x, -half.y}, {p_uv.anchor.x + p_uv.size.x, p_uv.anchor.y + p_uv.size.y}},
				 {{half.x, half.y}, {p_uv.anchor.x + p_uv.size.x, p_uv.anchor.y}},
				 {{-half.x, half.y}, {p_uv.anchor.x, p_uv.anchor.y}}});

			return mesh;
		}

		spk::Mesh2D makeCircle(float p_radius, size_t p_pointCount, const spk::SpriteSheet::Section &p_uv)
		{
			spk::Mesh2D mesh;
			if (p_pointCount < 3)
			{
				return mesh;
			}

			std::vector<spk::Vertex2D> vertices;
			vertices.reserve(p_pointCount);

			const float step = 2.0f * std::numbers::pi_v<float> / static_cast<float>(p_pointCount);
			for (size_t i = 0; i < p_pointCount; ++i)
			{
				float angle = step * static_cast<float>(i);
				float x = std::cos(angle) * p_radius;
				float y = std::sin(angle) * p_radius;

				float u = p_uv.anchor.x + ((x / (p_radius * 2.0f) + 0.5f) * p_uv.size.x);
				float v = p_uv.anchor.y + ((1.0f - (y / (p_radius * 2.0f) + 0.5f)) * p_uv.size.y);

				vertices.push_back({{x, y}, {u, v}});
			}

			mesh.addShape(vertices);
			return mesh;
		}
	}
}
