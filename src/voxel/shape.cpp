#include "voxel/shape.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "diagnostics/logger.hpp"
#include "exception.hpp"

namespace
{
	constexpr float PlanarityTolerance = 1.0e-5f;
	constexpr float AreaTolerance = 1.0e-10f;

	bool finite(const spk::Vector3 &value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
	}

	bool finite(const spk::Vector2 &value)
	{
		return std::isfinite(value.x) && std::isfinite(value.y);
	}

	bool inCube(const spk::Vector3 &value)
	{
		return value.x >= 0.0f && value.x <= 1.0f &&
			   value.y >= 0.0f && value.y <= 1.0f &&
			   value.z >= 0.0f && value.z <= 1.0f;
	}

	bool onPlane(const std::vector<spk::Voxel::Shape::Vertex> &vertices, int axis, float value)
	{
		for (const auto &vertex : vertices)
		{
			if (vertex.position.data[axis] != value)
			{
				return false;
			}
		}
		return true;
	}

	std::optional<spk::Voxel::Shape::Face> boundaryOf(const std::vector<spk::Voxel::Shape::Vertex> &vertices)
	{
		using Face = spk::Voxel::Shape::Face;
		for (int axis = 0; axis != 3; ++axis)
		{
			if (onPlane(vertices, axis, 0.0f))
			{
				return static_cast<Face>(axis * 2);
			}
			if (onPlane(vertices, axis, 1.0f))
			{
				return static_cast<Face>(axis * 2 + 1);
			}
		}
		return std::nullopt;
	}

	spk::Vector3 geometricNormal(const std::vector<spk::Voxel::Shape::Vertex> &vertices)
	{
		const spk::Vector3 origin = vertices.front().position;
		for (std::size_t index = 1; index + 1 < vertices.size(); ++index)
		{
			const auto cross = (vertices[index].position - origin).cross(vertices[index + 1].position - origin);
			const float length = static_cast<float>(cross.length());
			if (length > AreaTolerance)
			{
				return cross / spk::Vector3{length, length, length};
			}
		}
		throw spk::Exception("Voxel Shape polygon is degenerate");
	}

	void validateVertices(const std::vector<spk::Voxel::Shape::Vertex> &vertices)
	{
		if (vertices.size() < 3)
		{
			throw spk::Exception("Voxel Shape polygon requires at least three vertices");
		}
		for (std::size_t index = 0; index < vertices.size(); ++index)
		{
			const auto &vertex = vertices[index];
			if (!finite(vertex.position) || !inCube(vertex.position))
			{
				throw spk::Exception("Voxel Shape vertex position must be finite and inside [0,1]^3");
			}
			if (!finite(vertex.uv))
			{
				throw spk::Exception("Voxel Shape UV coordinates must be finite");
			}
			if (vertex.normal && (!finite(*vertex.normal) || vertex.normal->length() <= 0.0))
			{
				throw spk::Exception("Voxel Shape lighting normal must be finite and nonzero");
			}
			for (std::size_t previous = 0; previous < index; ++previous)
			{
				if (vertices[previous].position == vertex.position)
				{
					throw spk::Exception("Voxel Shape polygon has repeated vertices");
				}
			}
		}
	}

	void validateConvexPlanar(
		const std::vector<spk::Voxel::Shape::Vertex> &vertices,
		const spk::Vector3 &normal)
	{
		const auto origin = vertices.front().position;
		for (const auto &vertex : vertices)
		{
			if (std::abs((vertex.position - origin).dot(normal)) > PlanarityTolerance)
			{
				throw spk::Exception("Voxel Shape polygon is not planar");
			}
		}
		for (std::size_t index = 0; index < vertices.size(); ++index)
		{
			const auto &first = vertices[index].position;
			const auto &second = vertices[(index + 1) % vertices.size()].position;
			const auto edge = second - first;
			for (std::size_t other = 0; other < vertices.size(); ++other)
			{
				const auto side = edge.cross(vertices[other].position - first).dot(normal);
				if (side < -PlanarityTolerance)
				{
					throw spk::Exception("Voxel Shape polygon is not convex or has inconsistent winding");
				}
			}
		}
	}

	bool hasUVOutsideUnit(const std::vector<spk::Voxel::Shape::Vertex> &vertices)
	{
		return std::any_of(vertices.begin(), vertices.end(), [](const auto &vertex) {
			return vertex.uv.x < 0.0f || vertex.uv.x > 1.0f ||
				   vertex.uv.y < 0.0f || vertex.uv.y > 1.0f;
		});
	}
}

namespace spk::Voxel
{
	struct Shape::Data
	{
		std::vector<Slot> slots;
		std::vector<Polygon> polygons;
	};

	Shape::Shape(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	Shape::Shape() :
		_data(std::make_shared<const Data>())
	{
	}

	const std::vector<Shape::Slot> &Shape::slots() const noexcept
	{
		return _data->slots;
	}

	const std::vector<Shape::Polygon> &Shape::polygons() const noexcept
	{
		return _data->polygons;
	}

	bool Shape::empty() const noexcept
	{
		return _data->polygons.empty();
	}

	bool Shape::sameIdentity(const Shape &other) const noexcept
	{
		return _data == other._data;
	}

	const Shape::Slot &Shape::slot(std::string_view name) const
	{
		for (const Slot &entry : _data->slots)
		{
			if (entry.name == name)
			{
				return entry;
			}
		}
		throw spk::Exception("Voxel Shape references an unknown geometric slot");
	}

	void Shape::Builder::_check() const
	{
		if (_consumed)
		{
			throw spk::Exception("Voxel Shape Builder has already been consumed");
		}
	}

	void Shape::Builder::addSlot(std::string name, float opacity)
	{
		_check();
		if (name.empty() || !std::isfinite(opacity) || opacity < 0.0f || opacity > 1.0f)
		{
			throw spk::Exception("Voxel Shape slot requires a name and opacity in [0,1]");
		}
		for (const Slot &slot : _slots)
		{
			if (slot.name == name)
			{
				throw spk::Exception("Voxel Shape slot name already exists");
			}
		}
		_slots.push_back(Slot{std::move(name), opacity});
	}

	void Shape::Builder::addPolygon(std::string slot, std::vector<Vertex> vertices)
	{
		_check();
		bool found = false;
		for (const Slot &entry : _slots)
		{
			found = found || entry.name == slot;
		}
		if (!found)
		{
			throw spk::Exception("Voxel Shape polygon refers to an undeclared slot");
		}

		validateVertices(vertices);
		const Vector3 normal = geometricNormal(vertices);
		validateConvexPlanar(vertices, normal);
		const bool warnUV = hasUVOutsideUnit(vertices);
		for (auto &vertex : vertices)
		{
			if (!vertex.normal)
			{
				vertex.normal = normal;
			}
		}

		Polygon polygon;
		polygon.vertices = std::move(vertices);
		polygon.slot = std::move(slot);
		polygon.geometricNormal = normal;
		polygon.boundary = boundaryOf(polygon.vertices);
		polygon.shell = polygon.boundary ? Shell::Outer : Shell::Inner;
		_polygons.push_back(std::move(polygon));

		if (warnUV)
		{
			SPK_LOG(Warning) << "Voxel Shape polygon contains UV outside [0,1]" << std::endl;
		}
	}

	Shape Shape::Builder::build()
	{
		_check();
		auto data = std::make_shared<Data>();
		data->slots = _slots;
		data->polygons = _polygons;
		Shape result(std::move(data));
		_consumed = true;
		return result;
	}
}
