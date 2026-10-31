#include "voxel/occlusion/algorithm.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <utility>

#include "exception.hpp"

namespace spk::Voxel::Occlusion
{
	namespace
	{
		using Vertex = Shape::Vertex;
		using Polygon = Shape::Polygon;
		using Face = Shape::Face;

		spk::Vector3 offset(Face face)
		{
			switch (face)
			{
			case Face::NegativeX:
				return {-1, 0, 0};
			case Face::PositiveX:
				return {1, 0, 0};
			case Face::NegativeY:
				return {0, -1, 0};
			case Face::PositiveY:
				return {0, 1, 0};
			case Face::NegativeZ:
				return {0, 0, -1};
			case Face::PositiveZ:
				return {0, 0, 1};
			}
			throw spk::Exception("Invalid voxel face");
		}

		Face opposite(Face face)
		{
			switch (face)
			{
			case Face::NegativeX:
				return Face::PositiveX;
			case Face::PositiveX:
				return Face::NegativeX;
			case Face::NegativeY:
				return Face::PositiveY;
			case Face::PositiveY:
				return Face::NegativeY;
			case Face::NegativeZ:
				return Face::PositiveZ;
			case Face::PositiveZ:
				return Face::NegativeZ;
			}
			throw spk::Exception("Invalid voxel face");
		}

		spk::Vector2 project(const spk::Vector3 &point, Face face)
		{
			if (face == Face::NegativeX || face == Face::PositiveX)
			{
				return {point.y, point.z};
			}
			if (face == Face::NegativeY || face == Face::PositiveY)
			{
				return {point.x, point.z};
			}
			return {point.x, point.y};
		}

		float cross(const spk::Vector2 &a, const spk::Vector2 &b, const spk::Vector2 &c)
		{
			return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
		}

		float area(const std::vector<Vertex> &vertices, Face face)
		{
			float result = 0;
			for (std::size_t i = 0; i < vertices.size(); ++i)
			{
				auto a = project(vertices[i].position, face);
				auto b = project(vertices[(i + 1) % vertices.size()].position, face);
				result += a.x * b.y - a.y * b.x;
			}
			return result * 0.5f;
		}

		Vertex interpolate(const Vertex &a, const Vertex &b, float t)
		{
			Vertex vertex;
			vertex.position = a.position + (b.position - a.position) * spk::Vector3{t, t, t};
			vertex.uv = a.uv + (b.uv - a.uv) * spk::Vector2{t, t};
			if (a.normal && b.normal)
			{
				auto normal = *a.normal + (*b.normal - *a.normal) * spk::Vector3{t, t, t};
				if (normal.length() > 0)
				{
					vertex.normal = normal.normalized();
				}
			}
			return vertex;
		}

		std::vector<Vertex> clip(
			const std::vector<Vertex> &input,
			const spk::Vector2 &a,
			const spk::Vector2 &b,
			Face face,
			float sign,
			bool inside,
			float tolerance)
		{
			std::vector<Vertex> output;
			if (input.empty())
			{
				return output;
			}
			auto distance = [&](const Vertex &vertex) {
				return sign * cross(a, b, project(vertex.position, face));
			};
			for (std::size_t i = 0; i < input.size(); ++i)
			{
				const auto &p = input[i];
				const auto &q = input[(i + 1) % input.size()];
				float dp = distance(p);
				float dq = distance(q);
				bool pin = inside ? dp >= -tolerance : dp <= tolerance;
				bool qin = inside ? dq >= -tolerance : dq <= tolerance;
				if (pin)
				{
					output.push_back(p);
				}
				if (pin != qin && std::abs(dp - dq) > 1.0e-12f)
				{
					output.push_back(interpolate(p, q, std::clamp(dp / (dp - dq), 0.0f, 1.0f)));
				}
			}
			return output;
		}

		std::vector<Polygon> subtract(
			const Polygon &source,
			const Polygon &blocker,
			Face face,
			float tolerance)
		{
			std::vector<Polygon> visible;
			auto remainder = source.vertices;
			const float sign = area(blocker.vertices, face) >= 0 ? 1.0f : -1.0f;
			for (std::size_t i = 0; i < blocker.vertices.size() && remainder.size() >= 3; ++i)
			{
				auto a = project(blocker.vertices[i].position, face);
				auto b = project(blocker.vertices[(i + 1) % blocker.vertices.size()].position, face);
				auto outside = clip(remainder, a, b, face, sign, false, tolerance);
				if (outside.size() >= 3 && std::abs(area(outside, face)) > tolerance * tolerance)
				{
					Polygon fragment = source;
					fragment.vertices = std::move(outside);
					visible.push_back(std::move(fragment));
				}
				remainder = clip(remainder, a, b, face, sign, true, tolerance);
			}
			return visible;
		}
	}

	Result Algorithm::compute(
		const Definition &source,
		Shape::Orientation sourceOrientation,
		const Definition &blocker,
		Shape::Orientation blockerOrientation,
		Shape::Face direction,
		float tolerance)
	{
		if (!std::isfinite(tolerance) || tolerance <= 0)
		{
			throw spk::Exception("Occlusion tolerance must be positive and finite");
		}

		std::vector<Polygon> fragments;
		const auto translation = offset(direction);
		for (const auto &polygon : source.shape().instance(sourceOrientation).triangles())
		{
			if (polygon.shell != Shape::Shell::Outer || polygon.boundary != direction)
			{
				continue;
			}
			std::vector<Polygon> current{polygon};
			for (const auto &occluder : blocker.shape().instance(blockerOrientation).triangles())
			{
				if (occluder.shell != Shape::Shell::Outer || occluder.boundary != opposite(direction))
				{
					continue;
				}
				if (blocker.opacity(occluder.slotKey) < 1.0f)
				{
					continue;
				}
				if (occluder.geometricNormal.dot(translation) >= 0)
				{
					continue;
				}
				Polygon translated = occluder;
				for (auto &vertex : translated.vertices)
				{
					vertex.position += translation;
				}
				std::vector<Polygon> next;
				for (const auto &fragment : current)
				{
					auto pieces = subtract(fragment, translated, direction, tolerance);
					next.insert(next.end(), std::make_move_iterator(pieces.begin()), std::make_move_iterator(pieces.end()));
				}
				current = std::move(next);
				if (current.empty())
				{
					break;
				}
			}
			fragments.insert(fragments.end(), std::make_move_iterator(current.begin()), std::make_move_iterator(current.end()));
		}
		return Result(std::move(fragments));
	}
}
