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

		struct PolygonPartition
		{
			std::vector<Vertex> outside;
			std::vector<Vertex> inside;
		};

		[[nodiscard]] PolygonPartition split(
			const std::vector<Vertex> &input,
			const spk::Vector2 &a,
			const spk::Vector2 &b,
			Face face,
			float sign,
			float tolerance)
		{
			PolygonPartition result;
			if (input.empty())
			{
				return result;
			}
			result.outside.reserve(input.size() + 1);
			result.inside.reserve(input.size() + 1);
			auto distance = [&](const Vertex &vertex) {
				const float distance = sign * cross(a, b, project(vertex.position, face));
				return std::abs(distance) <= tolerance ? 0.0f : distance;
			};
			const float firstDistance = distance(input.front());
			float dp = firstDistance;
			for (std::size_t i = 0; i < input.size(); ++i)
			{
				const auto &p = input[i];
				const auto &q = input[(i + 1) % input.size()];
				const float dq = i + 1 < input.size() ? distance(q) : firstDistance;
				const bool pin = dp >= 0.0f;
				const bool qin = dq >= 0.0f;
				if (pin)
				{
					result.inside.push_back(p);
				}
				else
				{
					result.outside.push_back(p);
				}
				if (pin != qin && std::abs(dp - dq) > 1.0e-12f)
				{
					const auto intersection = interpolate(p, q, std::clamp(dp / (dp - dq), 0.0f, 1.0f));
					result.inside.push_back(intersection);
					result.outside.push_back(intersection);
				}
				dp = dq;
			}
			return result;
		}

		struct ProjectedOccluder
		{
			std::vector<spk::Vector2> vertices;
			float winding;
		};

		[[nodiscard]] ProjectedOccluder projectOccluder(const Polygon &polygon, Face face)
		{
			ProjectedOccluder result;
			result.vertices.reserve(polygon.vertices.size());
			for (const auto &vertex : polygon.vertices)
			{
				result.vertices.push_back(project(vertex.position, face));
			}
			result.winding = area(polygon.vertices, face) >= 0 ? 1.0f : -1.0f;
			return result;
		}

		std::vector<Polygon> subtract(
			const Polygon &source,
			const ProjectedOccluder &blocker,
			Face face,
			float tolerance)
		{
			std::vector<Polygon> visible;
			visible.reserve(blocker.vertices.size());
			auto remainder = source.vertices;
			for (std::size_t i = 0; i < blocker.vertices.size() && remainder.size() >= 3; ++i)
			{
				const auto &a = blocker.vertices[i];
				const auto &b = blocker.vertices[(i + 1) % blocker.vertices.size()];
				auto pieces = split(remainder, a, b, face, blocker.winding, tolerance);
				if (pieces.outside.size() >= 3 && std::abs(area(pieces.outside, face)) > tolerance * tolerance)
				{
					Polygon fragment = source;
					fragment.vertices = std::move(pieces.outside);
					visible.push_back(std::move(fragment));
				}
				remainder = std::move(pieces.inside);
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
			const auto &sourceSlot = source.slot(polygon.slotKey);
			for (const auto &occluder : blocker.shape().instance(blockerOrientation).triangles())
			{
				if (occluder.shell != Shape::Shell::Outer || occluder.boundary != opposite(direction))
				{
					continue;
				}
				const auto &blockerSlot = blocker.slot(occluder.slotKey);
				// Only a matching translucent surface can hide another translucent surface.
				// Different translucent materials (or opacities) retain their interface.
				if (blockerSlot.opacity < 1.0f &&
					(blockerSlot.name != sourceSlot.name || blockerSlot.opacity != sourceSlot.opacity))
				{
					continue;
				}
				if (occluder.geometricNormal.dot(translation) >= 0)
				{
					continue;
				}
				// Projection omits the face-normal axis, so the neighboring cell's
				// translation does not change these edges.
				const auto projected = projectOccluder(occluder, direction);
				std::vector<Polygon> next;
				next.reserve(current.size());
				for (const auto &fragment : current)
				{
					auto pieces = subtract(fragment, projected, direction, tolerance);
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
		return Result(std::move(fragments), source);
	}
}
