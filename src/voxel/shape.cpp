#include "voxel/shape.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
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

	spk::Vector3 readPosition(const spk::JSON::Value &value)
	{
		const auto &coordinates = value.asArray();
		if (coordinates.size() != 3)
		{
			throw spk::Exception("Voxel Shape JSON position requires exactly three coordinates");
		}
		return {coordinates[0].as<float>(), coordinates[1].as<float>(), coordinates[2].as<float>()};
	}

	spk::Vector2 readUV(const spk::JSON::Value &value)
	{
		const auto &coordinates = value.asArray();
		if (coordinates.size() != 2)
		{
			throw spk::Exception("Voxel Shape JSON UV requires exactly two coordinates");
		}
		return {coordinates[0].as<float>(), coordinates[1].as<float>()};
	}

	spk::Voxel::Shape::Vertex readVertex(const spk::JSON::Value &value)
	{
		spk::Voxel::Shape::Vertex vertex;
		if (value.isArray())
		{
			vertex.position = readPosition(value);
			return vertex;
		}
		if (!value.isObject())
		{
			throw spk::Exception("Voxel Shape JSON vertex must be an XYZ array or an object");
		}
		vertex.position = readPosition(value.at("position"));
		if (value.contains("uv"))
		{
			vertex.uv = readUV(value.at("uv"));
		}
		if (value.contains("normal"))
		{
			vertex.normal = readPosition(value.at("normal"));
		}
		return vertex;
	}

	spk::Vector3 rotateY(spk::Vector3 value, unsigned turns, bool position) noexcept
	{
		for (unsigned index = 0; index < turns; ++index)
		{
			const float x = value.x;
			value.x = value.z;
			value.z = position ? 1.0f - x : -x;
		}
		return value;
	}

	spk::Vector3 transformPosition(spk::Vector3 value, unsigned turns, bool flipped) noexcept
	{
		value = rotateY(value, turns, true);
		if (flipped)
		{
			value.y = 1.0f - value.y;
		}
		return value;
	}

	spk::Vector3 transformNormal(spk::Vector3 value, unsigned turns, bool flipped) noexcept
	{
		value = rotateY(value, turns, false);
		if (flipped)
		{
			value.y = -value.y;
		}
		return value;
	}

	spk::Voxel::Shape::Polygon orientedPolygon(
		spk::Voxel::Shape::Polygon polygon, unsigned turns, bool flipped)
	{
		for (auto &vertex : polygon.vertices)
		{
			vertex.position = transformPosition(vertex.position, turns, flipped);
			if (vertex.normal)
			{
				vertex.normal = transformNormal(*vertex.normal, turns, flipped);
			}
		}
		polygon.geometricNormal = transformNormal(polygon.geometricNormal, turns, flipped);
		if (flipped)
		{
			std::reverse(polygon.vertices.begin(), polygon.vertices.end());
		}
		polygon.boundary = boundaryOf(polygon.vertices);
		polygon.shell = polygon.boundary ? spk::Voxel::Shape::Shell::Outer : spk::Voxel::Shape::Shell::Inner;
		return polygon;
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
		std::array<Instance, 8> instances;
	};

	Shape::Instance::Instance(std::vector<Polygon> polygons) :
		_polygons(std::move(polygons))
	{
		for (const Polygon &polygon : _polygons)
		{
			for (std::size_t i = 1; i + 1 < polygon.vertices.size(); ++i)
			{
				Polygon triangle = polygon;
				triangle.vertices = {polygon.vertices[0], polygon.vertices[i], polygon.vertices[i + 1]};
				_triangles.push_back(std::move(triangle));
			}
		}
	}

	const std::vector<Shape::Polygon> &Shape::Instance::polygons() const noexcept
	{
		return _polygons;
	}

	const std::vector<Shape::Polygon> &Shape::Instance::triangles() const noexcept
	{
		return _triangles;
	}

	bool Shape::Instance::empty() const noexcept
	{
		return _polygons.empty();
	}

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

	const Shape::Instance &Shape::instance() const
	{
		return instance(Orientation{});
	}

	const Shape::Instance &Shape::instance(Orientation orientation) const
	{
		const unsigned degrees = static_cast<unsigned>(orientation.horizontal);
		const unsigned vertical = static_cast<unsigned>(orientation.vertical);
		if (degrees % 90 != 0 || degrees > 270 || vertical > 1)
		{
			throw spk::Exception("Invalid Voxel Shape orientation");
		}
		return _data->instances[(vertical * 4) + (degrees / 90)];
	}

	bool Shape::empty() const noexcept
	{
		return _data->instances[0].empty();
	}

	bool Shape::operator==(const Shape &other) const noexcept
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

	void Shape::Builder::addSlot(std::string name)
	{
		if (name.empty())
		{
			throw spk::Exception("Voxel Shape slot requires a name");
		}
		for (const Slot &slot : _slots)
		{
			if (slot.name == name)
			{
				throw spk::Exception("Voxel Shape slot name already exists");
			}
		}
		_slots.push_back(Slot{std::move(name)});
	}

	void Shape::Builder::addPolygon(std::string slotKey, std::vector<Vertex> vertices)
	{
		bool found = false;
		for (const Slot &entry : _slots)
		{
			found = found || entry.name == slotKey;
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
		polygon.slotKey = std::move(slotKey);
		polygon.geometricNormal = normal;
		polygon.boundary = boundaryOf(polygon.vertices);
		polygon.shell = polygon.boundary ? Shell::Outer : Shell::Inner;
		_polygons.push_back(std::move(polygon));

		if (warnUV)
		{
			SPK_LOG(Warning) << "Voxel Shape polygon contains UV outside [0,1]" << std::endl;
		}
	}

	void Shape::Builder::addFromJSON(const spk::JSON::Value &value)
	{
		Builder pending = *this;
		try
		{
			const auto &data = value.contains("data") ? value.at("data") : value;
			for (const auto &[name, ignored] : data.at("slots").asObject())
			{
				(void)ignored;
				pending.addSlot(name);
			}
			for (const auto &entry : data.at("polygons").asArray())
			{
				const auto slotKey = entry.at("slot").as<std::string>();
				std::vector<Vertex> vertices;
				const auto &source = entry.at("vertices").asArray();
				vertices.reserve(source.size());
				for (const auto &item : source)
				{
					vertices.push_back(readVertex(item));
				}
				pending.addPolygon(slotKey, std::move(vertices));
			}
		} catch (...)
		{
			throw spk::Exception("Invalid Voxel Shape JSON description", std::current_exception());
		}
		_slots.swap(pending._slots);
		_polygons.swap(pending._polygons);
	}

	Shape Shape::Builder::build()
	{
		auto data = std::make_shared<Data>();
		data->slots = _slots;
		for (unsigned flipped = 0; flipped < 2; ++flipped)
		{
			for (unsigned turns = 0; turns < 4; ++turns)
			{
				std::vector<Polygon> polygons;
				polygons.reserve(_polygons.size());
				for (const Polygon &polygon : _polygons)
				{
					polygons.push_back(orientedPolygon(polygon, turns, flipped != 0));
				}
				data->instances[flipped * 4 + turns] = Instance(std::move(polygons));
			}
		}
		return Shape(std::move(data));
	}
}
