#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_polygon.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <sparkle.hpp>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

class Block
{
public:
	using ID = short;

	enum class HorizontalOrientation
	{
		XPositive,
		ZPositive,
		XNegative,
		ZNegative
	};

	enum class VerticalOrientation
	{
		YPositive,
		YNegative
	};

	struct Orientation
	{
		HorizontalOrientation horizontalOrientation;
		VerticalOrientation verticalOrientation;

		bool operator<(const Orientation &p_other) const
		{
			if (horizontalOrientation != p_other.horizontalOrientation)
			{
				return horizontalOrientation < p_other.horizontalOrientation;
			}
			return verticalOrientation < p_other.verticalOrientation;
		}
	};

	static inline const std::vector<spk::Vector3> neightbourCoordinates = {{0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {0, 0, 1}, {-1, 0, 0}, {0, 0, -1}};

	using Specifier = std::pair<Block::ID, Block::Orientation>;
	using Describer = std::pair<spk::SafePointer<const Block>, Block::Orientation>;
	using NeightbourDescriber = std::array<Block::Describer, 6>;
	using Footprint = spk::Polygon;

protected:
	using Type = std::wstring; // Each block type can be identified by its type : FullBlock, Slope, HalfBlock, Fence, for exemple
	// Multiple block can shared the same type, as long as they are the same shape in 3D, with just different sprite and interaction

	virtual const spk::ObjMesh &_mesh() const = 0;

	static void _applySprite(spk::ObjMesh::Shape &p_shape, const spk::SpriteSheet::Sprite &p_sprite)
	{
		auto transform = [&](spk::Vertex &p_v) {
			if (p_v.uv == -1)
			{
				return ;
			}
			p_v.uv = (p_v.uv * p_sprite.size) + p_sprite.anchor;
			};
		std::visit(
			[&](auto &p_face)
			{
				transform(p_face.a);
				transform(p_face.b);
				transform(p_face.c);
				if constexpr (std::is_same_v<std::decay_t<decltype(p_face)>, spk::ObjMesh::Quad>)
				{
					transform(p_face.d);
				}
			},
			p_shape);
	}

private:
	struct Face
	{
		Footprint footprint;
		spk::ObjMesh mesh;
		bool full = false;
	};

	class Cache
	{
	public:
		struct Entry;

		bool hasCase(const Orientation &p_orientation) const
		{
			return _cache.contains(p_orientation);
		}

		void addCase(const Orientation &p_orientation, const spk::ObjMesh &p_mesh)
		{
			if (_cache.contains(p_orientation) == false)
			{
				_cache.emplace(p_orientation, _compute(p_mesh, p_orientation));
			}
		}

		const Entry &getCase(const Orientation &p_orientation) const
		{
			return _cache.at(p_orientation);
		}

		struct Entry
		{
			spk::ObjMesh innerMesh;
			std::unordered_map<spk::Vector3, Face> faces;

			void applyInnerMesh(spk::ObjMesh &p_target, const spk::Vector3 &p_offset) const
			{
				_appendMesh(p_target, innerMesh, p_offset);
			}

			void applyFace(spk::ObjMesh &p_target, const spk::Vector3 &p_offset, const spk::Vector3 &p_normal) const
			{
				auto it = faces.find(p_normal);
				if (it != faces.end())
				{
					_appendMesh(p_target, it->second.mesh, p_offset);
				}
			}

		private:
			static void _appendMesh(spk::ObjMesh &p_target, const spk::ObjMesh &p_source, const spk::Vector3 &p_offset)
			{
				auto add = [&](auto p_shape)
				{
					p_shape.a.position += p_offset;
					p_shape.b.position += p_offset;
					p_shape.c.position += p_offset;
					if constexpr (std::is_same_v<std::decay_t<decltype(p_shape)>, spk::ObjMesh::Quad>)
					{
						p_shape.d.position += p_offset;
					}
					p_target.addShape(p_shape);
				};

				for (const auto &shape : p_source.shapes())
				{
					std::visit(add, shape);
				}
			}
		};

	private:
		static spk::Vector3 _applyOrientation(const spk::Vector3 &p_position, const Orientation &p_orientation)
		{
			spk::Vector3 result = p_position - spk::Vector3(0.5f, 0.5f, 0.5f);
			constexpr std::array<float, 4> rotations = {-90.0f, 0.0f, 90.0f, 180.0f};
			result = result.rotate({0, rotations[static_cast<size_t>(p_orientation.horizontalOrientation)], 0});

			if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
			{
				result.y = -result.y;
			}

			return result + spk::Vector3(0.5f, 0.5f, 0.5f);
		}

		static bool _isFullQuad(const std::vector<spk::Vertex> &p_vertices, const spk::Vector3 &p_normal)
		{
			if (p_vertices.size() != 4)
			{
				return false;
			}

			auto getAB = [&](const spk::Vector3 &p_point) -> std::pair<float, float>
			{
				if (std::abs(p_normal.y) > 0.0f)
				{
					return {p_point.x, p_point.z};
				}
				else if (std::abs(p_normal.x) > 0.0f)
				{
					return {p_point.y, p_point.z};
				}
				else
				{
					return {p_point.x, p_point.y};
				}
			};

			float minA = std::numeric_limits<float>::max();
			float maxA = std::numeric_limits<float>::lowest();
			float minB = std::numeric_limits<float>::max();
			float maxB = std::numeric_limits<float>::lowest();

			for (const auto &v : p_vertices)
			{
				auto [a, b] = getAB(v.position);
				minA = std::min(minA, a);
				maxA = std::max(maxA, a);
				minB = std::min(minB, b);
				maxB = std::max(maxB, b);
			}

			return (
				(std::abs(minA - 0.0f) < std::numeric_limits<float>::epsilon()) && (std::abs(maxA - 1.0f) < std::numeric_limits<float>::epsilon()) &&
				(std::abs(minB - 0.0f) < std::numeric_limits<float>::epsilon()) && (std::abs(maxB - 1.0f) < std::numeric_limits<float>::epsilon()));
		}

		static std::vector<spk::Vertex> _extractVertices(const spk::ObjMesh::Shape &p_shape)
		{
			if (std::holds_alternative<spk::ObjMesh::Triangle>(p_shape))
			{
				const auto &tri = std::get<spk::ObjMesh::Triangle>(p_shape);
				return {tri.a, tri.b, tri.c};
			}
			else
			{
				const auto &quad = std::get<spk::ObjMesh::Quad>(p_shape);
				return {quad.a, quad.b, quad.c, quad.d};
			}
		}

		static void _applyOrientationToVertices(std::vector<spk::Vertex> &p_vertices, const Orientation &p_orientation)
		{
			for (auto &v : p_vertices)
			{
				v.position = _applyOrientation(v.position, p_orientation);
			}
			if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
			{
				std::reverse(p_vertices.begin(), p_vertices.end());
			}
		}

		static bool _isAxisAlignedFace(const std::vector<spk::Vertex> &p_vertices, spk::Vector3 &p_outNormal)
		{
			auto eq = [&](float p_a, float p_b) { return std::abs(p_a - p_b) <= std::numeric_limits<float>::epsilon(); };

			bool allX0 = true, allX1 = true, allY0 = true, allY1 = true, allZ0 = true, allZ1 = true;
			for (const auto &v : p_vertices)
			{
				allX0 = allX0 && eq(v.position.x, 0.0f);
				allX1 = allX1 && eq(v.position.x, 1.0f);
				allY0 = allY0 && eq(v.position.y, 0.0f);
				allY1 = allY1 && eq(v.position.y, 1.0f);
				allZ0 = allZ0 && eq(v.position.z, 0.0f);
				allZ1 = allZ1 && eq(v.position.z, 1.0f);
			}

			if (allX0)
			{
				p_outNormal = spk::Vector3(-1, 0, 0);
				return true;
			}
			if (allX1)
			{
				p_outNormal = spk::Vector3(1, 0, 0);
				return true;
			}
			if (allY0)
			{
				p_outNormal = spk::Vector3(0, -1, 0);
				return true;
			}
			if (allY1)
			{
				p_outNormal = spk::Vector3(0, 1, 0);
				return true;
			}
			if (allZ0)
			{
				p_outNormal = spk::Vector3(0, 0, -1);
				return true;
			}
			if (allZ1)
			{
				p_outNormal = spk::Vector3(0, 0, 1);
				return true;
			}

			return false;
		}

		static void _addVerticesToMesh(spk::ObjMesh &p_mesh, const std::vector<spk::Vertex> &p_vertices)
		{
			if (p_vertices.size() == 3)
			{
				p_mesh.addShape(p_vertices[0], p_vertices[1], p_vertices[2]);
			}
			else
			{
				p_mesh.addShape(p_vertices[0], p_vertices[1], p_vertices[2], p_vertices[3]);
			}
		}

		static Entry _compute(const spk::ObjMesh &p_mesh, const Orientation &p_orientation)
		{
			Entry result;

			for (const auto &shape : p_mesh.shapes())
			{
				auto vertices = _extractVertices(shape);
				_applyOrientationToVertices(vertices, p_orientation);

				spk::Vector3 normal;
				if (_isAxisAlignedFace(vertices, normal))
				{
					Face &face = result.faces[normal];
					for (auto &v : vertices)
					{
						face.footprint.points.push_back(v.position);
					}
					_addVerticesToMesh(face.mesh, vertices);

					if (vertices.size() == 4 && _isFullQuad(vertices, normal))
					{
						face.full = true;
					}
				}
				else
				{
					_addVerticesToMesh(result.innerMesh, vertices);
				}
			}

			return result;
		}

		std::map<Orientation, Entry> _cache;
	};

	mutable Cache _cache;

	const Cache::Entry &_ensureCacheCase(const Orientation &p_orientation) const
	{
		if (_cache.hasCase(p_orientation) == false)
		{
			_cache.addCase(p_orientation, _mesh());
		}
		return _cache.getCase(p_orientation);
	}

	std::array<const Face *, 6> _gatherNeighbourFaces(const NeightbourDescriber &p_neightbourDescriber) const
	{
		std::array<const Face *, 6> neighFaces{};
		for (size_t i = 0; i < 6; ++i)
		{
			neighFaces[i] = nullptr;
			const auto &desc = p_neightbourDescriber[i];
			if (desc.first == nullptr)
			{
				continue;
			}

			const Block *neigh = desc.first;
			const Orientation &neighOrientation = desc.second;
			if (neigh->_cache.hasCase(neighOrientation) == false)
			{
				neigh->_cache.addCase(neighOrientation, neigh->_mesh());
			}

			const Cache::Entry &neighData = neigh->_cache.getCase(neighOrientation);
			const spk::Vector3 oppositeNormal = -neightbourCoordinates[i];
			if (auto it = neighData.faces.find(oppositeNormal); it != neighData.faces.end())
			{
				neighFaces[i] = &it->second;
			}
		}
		return neighFaces;
	}

	static bool _allSixFull(const std::array<const Face *, 6> &p_neighFaces)
	{
		for (const Face *nf : p_neighFaces)
		{
			if (nf == nullptr || nf->full == false)
			{
				return false;
			}
		}
		return true;
	}

	static void _emitInnerIfNeeded(
		spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const std::array<const Face *, 6> &p_neighFaces)
	{
		if (_allSixFull(p_neighFaces) == false)
		{
			p_data.applyInnerMesh(p_toFill, p_position);
		}
	}

	static spk::Polygon _translated(const spk::Polygon &p_poly, const spk::Vector3 &p_delta)
	{
		spk::Polygon out;
		out.points.reserve(p_poly.points.size());
		std::transform(
			p_poly.points.begin(),
			p_poly.points.end(),
			std::back_inserter(out.points),
			[&](const spk::Vector3 &p_point) { return p_point + p_delta; });
		return out;
	}

	static void _emitVisibleFaces(
		spk::ObjMesh &p_toFill, const Cache::Entry &p_data, const spk::Vector3 &p_position, const std::array<const Face *, 6> &p_neighFaces)
	{
		for (size_t i = 0; i < 6; ++i)
		{
			const spk::Vector3 normal = neightbourCoordinates[i];

			auto faceIt = p_data.faces.find(normal);
			if (faceIt == p_data.faces.end())
			{
				continue;
			}

			const Face &ourFace = faceIt->second;
			if (ourFace.mesh.shapes().empty() == true)
			{
				continue;
			}

			const Face *neigh = p_neighFaces[i];
			bool visible = true;

			if (neigh != nullptr && neigh->mesh.shapes().empty() == false)
			{
				const spk::Vector3 toOurLocal = normal;
				spk::Polygon neighInOurSpace = _translated(neigh->footprint, toOurLocal);

				if (neighInOurSpace.contains(ourFace.footprint) == true)
				{
					visible = false;
				}
			}

			if (visible == true)
			{
				p_data.applyFace(p_toFill, p_position, normal);
			}
		}
	}

public:
	virtual ~Block() = default;

	void bake(
		spk::ObjMesh &p_toFill,
		const NeightbourDescriber &p_neightbourDescriber,
		const spk::Vector3 &p_position,
		const Orientation &p_orientation) const
	{
		const Cache::Entry &data = _ensureCacheCase(p_orientation);

		const auto neighFaces = _gatherNeighbourFaces(p_neightbourDescriber);

		_emitInnerIfNeeded(p_toFill, data, p_position, neighFaces);

		_emitVisibleFaces(p_toFill, data, p_position, neighFaces);
	}
};

struct FullBlock : public Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite front;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite top;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;
	static inline std::string objMeshCode = R"(v 0.0 0.0 0.0  # 1 bottom-left-back
v 1.0 0.0 0.0  # 2 bottom-right-back
v 1.0 0.0 1.0  # 3 bottom-right-front
v 0.0 0.0 1.0  # 4 bottom-left-front
v 0.0 1.0 0.0  # 5 top-left-back
v 1.0 1.0 0.0  # 6 top-right-back
v 1.0 1.0 1.0  # 7 top-right-front
v 0.0 1.0 1.0  # 8 top-left-front

# Texture coordinates (quad)
vt 0.0 1.0  # 1
vt 1.0 1.0  # 2
vt 1.0 0.0  # 3
vt 0.0 0.0  # 4

# Normals
vn  0.0  0.0  1.0  # Front
vn  0.0  0.0 -1.0  # Back
vn  1.0  0.0  0.0  # Right
vn -1.0  0.0  0.0  # Left
vn  0.0  1.0  0.0  # Top
vn  0.0 -1.0  0.0  # Bottom

# Faces (two triangles per face, CCW winding)

# Front (z = 1) -> uses normal 1
f 4/1/1 3/2/1 7/3/1 8/4/1

# Back (z = 0) -> normal 2
f 2/2/2 1/1/2 5/4/2 6/3/2

# Right (x = 1) -> normal 3
f 3/1/3 2/2/3 6/3/3 7/4/3

# Left (x = 0) -> normal 4
f 1/2/4 4/1/4 8/4/4 5/3/4

# Top (y = 1) -> normal 5
f 5/1/5 8/2/5 7/3/5 6/4/5

# Bottom (y = 0) -> normal 6
f 1/1/6 2/2/6 3/3/6 4/4/6)";

public:
	explicit FullBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromString(objMeshCode);

		_applySprite(_objMesh.shapes()[0], _configuration.front);
		_applySprite(_objMesh.shapes()[1], _configuration.back);
		_applySprite(_objMesh.shapes()[2], _configuration.left);
		_applySprite(_objMesh.shapes()[3], _configuration.right);
		_applySprite(_objMesh.shapes()[4], _configuration.top);
		_applySprite(_objMesh.shapes()[5], _configuration.bottom);
	}
};

struct SlopeBlock : public Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite triangleLeft;
		spk::SpriteSheet::Sprite triangleRight;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite ramp;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;
	static inline std::string objMeshCode = R"(v 0.0 0.0 0.0
v 1.0 0.0 0.0
v 1.0 0.0 1.0
v 0.0 0.0 1.0
v 0.0 1.0 1.0
v 1.0 1.0 1.0

vt 1.0 1.0   # 1
vt 0.0 1.0   # 2
vt 0.0 0.0   # 3
vt 1.0 0.0   # 4

vn  0.0 -1.0  0.0
vn  0.0  0.0  1.0
vn -1.0  0.0  0.0
vn  1.0  0.0  0.0
vn  0.0  0.70710678 -0.70710678

f 1/2/1 2/1/1 3/4/1 4/3/1
f 4/2/2 3/1/2 6/4/2 5/3/2
f 1/1/3 4/2/3 5/3/3
f 2/1/4 6/3/4 3/2/4
f 1/2/5 5/3/5 6/4/5 2/1/5)";

public:
	explicit SlopeBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromString(objMeshCode);

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.back);
		_applySprite(_objMesh.shapes()[2], _configuration.triangleLeft);
		_applySprite(_objMesh.shapes()[3], _configuration.triangleRight);
		_applySprite(_objMesh.shapes()[4], _configuration.ramp);
	}
};

struct HalfBlock : public Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite front;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite top;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;
	static inline std::string objMeshCode = R"(v 0.0 0.0 0.0
v 0.0 0.0 1.0
v 1.0 0.0 1.0
v 1.0 0.0 0.0
v 0.0 0.5 0.0
v 0.0 0.5 1.0
v 1.0 0.5 1.0
v 1.0 0.5 0.0

vt 0.0 0.0 // 1
vt 0.5 0.0 // 2
vt 1.0 0.0 // 3
vt 0.0 0.5 // 4
vt 0.5 0.5 // 5
vt 1.0 0.5 // 6
vt 0.0 1.0 // 7
vt 0.5 1.0 // 8
vt 1.0 1.0 // 9
vt -1.0 -1.0 // 10

vn  0.0  0.0  1.0
vn  0.0  0.0 -1.0
vn  1.0  0.0  0.0
vn -1.0  0.0  0.0
vn  0.0  1.0  0.0
vn  0.0 -1.0  0.0

f 2/9/1 1/7/1 4/1/1 3/3/1
f 5/9/1 6/7/1 7/1/1 8/3/1
f 7/4/1 6/6/1 2/9/1 3/7/1
f 6/4/1 5/6/1 1/9/1 2/7/1
f 5/4/1 8/6/1 4/9/1 1/7/1
f 8/4/1 7/6/1 3/9/1 4/7/1)";

public:
	explicit HalfBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromString(objMeshCode);

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.top);
		_applySprite(_objMesh.shapes()[2], _configuration.left);
		_applySprite(_objMesh.shapes()[3], _configuration.left);
		_applySprite(_objMesh.shapes()[4], _configuration.left);
		_applySprite(_objMesh.shapes()[5], _configuration.left);
	}
};

struct StairBlock : public Block
{
public:
	struct Configuration
	{
		spk::SpriteSheet::Sprite bottom;
		spk::SpriteSheet::Sprite staircaseTop;
		spk::SpriteSheet::Sprite staircaseFront;
		spk::SpriteSheet::Sprite left;
		spk::SpriteSheet::Sprite right;
		spk::SpriteSheet::Sprite back;
	};

private:
	const spk::ObjMesh &_mesh() const override
	{
		return (_objMesh);
	}

	spk::ObjMesh _objMesh;
	Configuration _configuration;
	static inline std::string objMeshCode = R"(v 0.0 0.0 0.0 // 1
v 0.0 0.0 1.0 // 2
v 1.0 0.0 1.0 // 3
v 1.0 0.0 0.0 // 4
v 0.5 0.5 0.0 // 5
v 0.5 0.5 1.0 // 6
v 1.0 0.5 1.0 // 7
v 1.0 0.5 0.0 // 8
v 0.0 1.0 0.0 // 9
v 0.0 1.0 1.0 // 10
v 0.5 1.0 1.0 // 11
v 0.5 1.0 0.0 // 12
v 0.5 0.0 1.0 // 13
v 0.5 0.0 0.0 // 14

vt 0.0 0.0 // 1
vt 0.5 0.0 // 2
vt 1.0 0.0 // 3
vt 0.0 0.5 // 4
vt 0.5 0.5 // 5
vt 1.0 0.5 // 6
vt 0.0 1.0 // 7
vt 0.5 1.0 // 8
vt 1.0 1.0 // 9
vt -1.0 -1.0 // 10

vn  0.0 -1.0  0.0
vn  0.0  1.0  0.0
vn  0.0  0.0 -1.0
vn  0.0  0.0  1.0
vn -1.0  0.0  0.0
vn  1.0  0.0  0.0

f 4/3/1 3/1/1 2/7/1 1/9/1
f 5/7/2 6/9/2 7/6/2 8/4/2
f 9/7/2 10/9/2 11/6/2 12/4/2
f 12/4/6 11/6/6 6/9/6 5/7/6
f 8/4/6 7/6/6 3/9/6 4/7/6
f 10/1/5 9/3/5 1/9/5 2/7/5
f 7/4/4 6/5/4 13/8/4 3/7/4
f 11/2/4 10/3/4 2/9/4 13/8/4
f 9/3/3 12/2/3 14/8/3 1/9/3
f 5/5/3 8/4/3 4/7/3 14/8/3
)";

public:
	explicit StairBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromString(objMeshCode);

		_applySprite(_objMesh.shapes()[0], _configuration.bottom);
		_applySprite(_objMesh.shapes()[1], _configuration.staircaseTop);
		_applySprite(_objMesh.shapes()[2], _configuration.staircaseTop);
		_applySprite(_objMesh.shapes()[3], _configuration.staircaseFront);
		_applySprite(_objMesh.shapes()[4], _configuration.staircaseFront);
		_applySprite(_objMesh.shapes()[5], _configuration.back);
		_applySprite(_objMesh.shapes()[6], _configuration.left);
		_applySprite(_objMesh.shapes()[7], _configuration.left);
		_applySprite(_objMesh.shapes()[8], _configuration.right);
		_applySprite(_objMesh.shapes()[9], _configuration.right);
	}
};

template <size_t ChunkSizeX, size_t ChunkSizeY, size_t ChunkSizeZ>
class BlockMap : public spk::Entity
{
public:
	class Chunk : public spk::Entity
	{
	public:
		static inline const spk::Vector3Int size = spk::Vector3Int(ChunkSizeX, ChunkSizeY, ChunkSizeZ);

	private:
		class Data : public spk::Component
		{
		private:
			spk::SafePointer<spk::ObjMeshRenderer> _renderer;

			spk::SafePointer<BlockMap> _blockMap;
			std::array<std::array<std::array<Block::Specifier, ChunkSizeX>, ChunkSizeY>, ChunkSizeZ> _content;

			bool _isBaked = false;
			spk::ObjMesh _mesh;

			Block::NeightbourDescriber _computeNeightbourSpecifiers(int p_x, int p_y, int p_z)
			{
				Block::NeightbourDescriber result;

				for (size_t i = 0; i < Block::neightbourCoordinates.size(); i++)
				{
					spk::Vector3 tmpCoord = Block::neightbourCoordinates[i] + spk::Vector3{p_x, p_y, p_z};

					Block::Specifier specifier = content(tmpCoord);

					spk::SafePointer<const Block> block = nullptr;
					if (specifier.first != -1)
					{
						block = _blockMap->blockById(specifier.first);
					}

					result[i] = std::make_pair(block, specifier.second);
				}

				return (result);
			}

			void _bake()
			{
				_mesh.clear();

				for (int z = 0; z < size.z; ++z)
				{
					for (int y = 0; y < size.y; ++y)
					{
						for (int x = 0; x < size.x; ++x)
						{
							Block::Specifier &currentSpecifier = _content[x][y][z];

							if (currentSpecifier.first != -1)
							{
								Block::NeightbourDescriber neightbourSpecifiers = _computeNeightbourSpecifiers(x, y, z);

								spk::SafePointer<const Block> currentBlock = _blockMap->blockById(currentSpecifier.first);

								currentBlock->bake(_mesh, neightbourSpecifiers, {x, y, z}, currentSpecifier.second);
							}
						}
					}
				}

				_isBaked = true;
			}

		public:
			Data(const std::wstring &p_name) :
				spk::Component(p_name)
			{
			}

			void setBlockMap(spk::SafePointer<BlockMap> p_blockMap)
			{
				_blockMap = p_blockMap;
			}

			void fill(Block::ID p_id)
			{
				for (int z = 0; z < size.z; ++z)
				{
					for (int y = 0; y < size.y; ++y)
					{
						for (int x = 0; x < size.x; ++x)
						{
							_content[x][y][z] = std::make_pair(
								p_id, Block::Orientation{Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive});
						}
					}
				}
				_isBaked = false;
			}

			void setContent(
				int p_x,
				int p_y,
				int p_z,
				const Block::ID &p_data,
				const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
			{
				_content[p_x][p_y][p_z] = std::make_pair(p_data, p_orientation);
				_isBaked = false;
			}

			Block::Specifier content(const spk::Vector3Int &p_coord) const
			{
				return (content(p_coord.x, p_coord.y, p_coord.z));
			}

			Block::Specifier content(const spk::Vector2Int &p_coord, int p_z) const
			{
				return (content(p_coord.x, p_coord.y, p_z));
			}

			Block::Specifier content(int p_x, int p_y, int p_z) const
			{
				if (p_x < 0 || p_x >= size.x || p_y < 0 || p_y >= size.y || p_z < 0 || p_z >= size.z)
				{
					return std::make_pair(-1, Block::Orientation{});
				}
				return (_content[p_x][p_y][p_z]);
			}

			void start() override
			{
				_renderer = owner()->template getComponent<spk::ObjMeshRenderer>();
			}

			void onPaintEvent(spk::PaintEvent &p_event) override
			{
				if (_isBaked == false)
				{
					_bake();

					if (_renderer != nullptr)
					{
						_renderer->setMesh(mesh());
						p_event.requestPaint();
					}
				}
			}

			spk::SafePointer<spk::ObjMesh> mesh()
			{
				return (&_mesh);
			}
			const spk::SafePointer<const spk::ObjMesh> mesh() const
			{
				return (&_mesh);
			}
		};

		spk::SafePointer<spk::ObjMeshRenderer> _renderer;
		spk::SafePointer<Data> _data;

	public:
		Chunk(const std::wstring &p_name, spk::SafePointer<BlockMap> p_parent) :
			spk::Entity(p_name, p_parent)
		{
			_renderer = this->template addComponent<spk::ObjMeshRenderer>(p_name + L"/ObjMeshRenderer");
			_data = this->template addComponent<Data>(p_name + L"/Data");
			_data->setBlockMap(p_parent);

			_renderer->setPriority(100);
			_data->setPriority(0);
		}

		void setTexture(spk::SafePointer<const spk::Texture> p_texture)
		{
			_renderer->setTexture(p_texture);
		}

		void fill(Block::ID p_id)
		{
			_data->fill(p_id);
		}

		void setContent(
			spk::Vector3Int p_position,
			Block::ID p_id,
			const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
		{
			setContent(p_position.x, p_position.y, p_position.z, p_id, p_orientation);
		}
		void setContent(
			spk::Vector2Int p_position,
			int p_z,
			Block::ID p_id,
			const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
		{
			setContent(p_position.x, p_position.y, p_z, p_id, p_orientation);
		}

		void setContent(
			int p_x,
			int p_y,
			int p_z,
			Block::ID p_id,
			const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
		{
			_data->setContent(p_x, p_y, p_z, p_id, p_orientation);
		}

		bool isBaked() const
		{
			return _data->isBaked();
		}

		spk::SafePointer<spk::ObjMesh> mesh()
		{
			return (_data->mesh());
		}
		const spk::SafePointer<const spk::ObjMesh> mesh() const
		{
			return (_data->mesh());
		}
	};

private:
	spk::SafePointer<const spk::Texture> _texture;

	std::unordered_map<Block::ID, std::unique_ptr<Block>> _availableBlocks;
	std::unordered_map<spk::Vector3Int, std::unique_ptr<Chunk>> _chunks;

	std::vector<spk::SafePointer<Chunk>> _activeChunks;

	std::unique_ptr<Chunk> _generateChunk(const spk::Vector3Int &p_chunkCoordinate)
	{
		const std::wstring chunkName = name() + L"/Chunk[" + p_chunkCoordinate.to_wstring() + L"]";

		std::unique_ptr<Chunk> newChunk = std::make_unique<Chunk>(chunkName, this);

		newChunk->transform().place(p_chunkCoordinate * Chunk::size);
		newChunk->setTexture(_texture);
		newChunk->fill(-1);

		_onChunkGeneration(p_chunkCoordinate, *newChunk);

		newChunk->activate();

		return newChunk;
	}

	virtual void _onChunkGeneration(const spk::Vector3Int &p_chunkCoordinate, Chunk &p_chunkToFill)
	{
		p_chunkToFill.setContent(2, 2, 2, 2);
		p_chunkToFill.setContent(3, 2, 2, 2);
		p_chunkToFill.setContent(4, 2, 2, 2);
		p_chunkToFill.setContent(5, 2, 2, 2);
		p_chunkToFill.setContent(2, 2, 3, 2);
		p_chunkToFill.setContent(2, 2, 4, 2);
		p_chunkToFill.setContent(2, 2, 5, 2);

		p_chunkToFill.setContent(4, 2, 4, 3);
		p_chunkToFill.setContent(5, 2, 4, 3);
		p_chunkToFill.setContent(6, 2, 4, 3);
		p_chunkToFill.setContent(7, 2, 4, 3);
		p_chunkToFill.setContent(4, 2, 5, 3);
		p_chunkToFill.setContent(4, 2, 6, 3);
		p_chunkToFill.setContent(4, 2, 7, 3);
	}

public:
	BlockMap(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent)
	{
	}

	void setTexture(spk::SafePointer<const spk::Texture> p_texture)
	{
		_texture = p_texture;

		for (auto &[key, value] : _chunks)
		{
			value->setTexture(_texture);
		}
	}

	void addBlockByID(const Block::ID &p_id, std::unique_ptr<Block> &&p_block)
	{
		if (_availableBlocks.contains(p_id) == true)
		{
			GENERATE_ERROR("Block ID [" + std::to_string(p_id) + "] already exist in BlockMap [" + spk::StringUtils::wstringToString(name()) + "]");
		}
		_availableBlocks[p_id] = std::move(p_block);
	}

	spk::SafePointer<const Block> blockById(Block::ID p_id) const
	{
		if (_availableBlocks.contains(p_id) == false)
		{
			return (nullptr);
		}
		return (_availableBlocks.at(p_id).get());
	}

	void setChunkRange(const spk::Vector3Int &p_start, const spk::Vector3Int &p_end)
	{
		for (auto &chunk : _activeChunks)
		{
			chunk->deactivate();
		}
		_activeChunks.clear();

		for (int i = p_start.x; i <= p_end.x; i++)
		{
			for (int j = p_start.y; j <= p_end.y; j++)
			{
				for (int k = p_start.z; k <= p_end.z; k++)
				{
					spk::Vector3Int chunkPosition = {i, j, k};
					if (_chunks.contains(chunkPosition) == false)
					{
						_chunks.emplace(chunkPosition, std::move(_generateChunk(chunkPosition)));
					}

					_activeChunks.push_back((_chunks[chunkPosition].get()));
				}
			}
		}

		for (auto &chunk : _activeChunks)
		{
			chunk->activate();
		}
	}
};

class Player : public spk::Entity
{
private:
	spk::SafePointer<spk::CameraComponent> _cameraComponent = nullptr;
	spk::SafePointer<spk::FreeViewController> _freeViewController = nullptr;

public:
	Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent)
	{
		_cameraComponent = addComponent<spk::CameraComponent>(L"Player/CameraComponent");
		_freeViewController = addComponent<spk::FreeViewController>(L"Player/FreeViewController");
	}

	spk::SafePointer<spk::CameraComponent> cameraComponent()
	{
		return (_cameraComponent);
	}

	spk::SafePointer<spk::FreeViewController> freeViewController()
	{
		return (_freeViewController);
	}
};

class DebugOverlayManager : public spk::Widget
{
private:
	spk::Profiler::Instanciator _profilerInstanciator;
	spk::DebugOverlay<3, 20> _debugOverlay;

	void _onGeometryChange() override
	{
		_debugOverlay.setGeometry({{0, 0}, geometry().size});
	}

	void _onUpdateEvent(spk::UpdateEvent &p_event) override
	{
		_debugOverlay.setText(0, 0, L"FPS : " + std::to_wstring(spk::Profiler::instance()->counter(L"FPS")->value()));
		_debugOverlay.setText(0, 1, L"UPS : " + std::to_wstring(spk::Profiler::instance()->counter(L"UPS")->value()));
		p_event.requestPaint();
	}

public:
	DebugOverlayManager(const std::wstring &p_name, spk::SafePointer<spk::Widget> p_parent) :
		spk::Widget(p_name, p_parent),
		_debugOverlay(p_name + L"/Overlay", this)
	{
		_debugOverlay.activate();
	}
};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(0);
	window->requestMousePlacement({400, 300});

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(&engine);
	engineWidget.setLayer(0);
	engineWidget.activate();

	DebugOverlayManager debugOverlay(L"DebugOverlay", window->widget());
	debugOverlay.setGeometry({0, 0}, window->geometry().size);
	debugOverlay.setLayer(100);
	debugOverlay.activate();

	Player player = Player(L"Player", nullptr);
	player.activate();
	engine.addEntity(&player);

	player.cameraComponent()->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	player.transform().place({5.0f, 5.0f, 5.0f});
	player.transform().lookAt({0.0f, 0.0f, 0.0f});

	spk::SpriteSheet blockMapTilemap = spk::SpriteSheet("playground/resources/texture/CubeTexture.png", {9, 1});

	BlockMap<16, 16, 16> blockMap = BlockMap<16, 16, 16>(L"BlockMap", nullptr);
	blockMap.setTexture(&blockMapTilemap);
	blockMap.activate();
	engine.addEntity(&blockMap);

	auto fullBlockSprite = blockMapTilemap.sprite({0, 0});
	FullBlock::Configuration fullConfiguration;
	fullConfiguration.front = fullBlockSprite;
	fullConfiguration.back = fullBlockSprite;
	fullConfiguration.left = fullBlockSprite;
	fullConfiguration.right = fullBlockSprite;
	fullConfiguration.top = fullBlockSprite;
	fullConfiguration.bottom = fullBlockSprite;
	blockMap.addBlockByID(0, std::make_unique<FullBlock>(fullConfiguration));

	SlopeBlock::Configuration slopeConfiguration;
	slopeConfiguration.triangleLeft = blockMapTilemap.sprite({1, 0});
	slopeConfiguration.triangleRight = blockMapTilemap.sprite({1, 0});
	slopeConfiguration.back = blockMapTilemap.sprite({2, 0});
	slopeConfiguration.ramp = blockMapTilemap.sprite({2, 0});
	slopeConfiguration.bottom = blockMapTilemap.sprite({3, 0});
	blockMap.addBlockByID(1, std::make_unique<SlopeBlock>(slopeConfiguration));

	
	StairBlock::Configuration stairConfiguration;
	stairConfiguration.staircaseTop = blockMapTilemap.sprite({7, 0});
	stairConfiguration.staircaseFront = blockMapTilemap.sprite({7, 0});
	stairConfiguration.left = blockMapTilemap.sprite({6, 0});
	stairConfiguration.right = blockMapTilemap.sprite({6, 0});
	stairConfiguration.back = blockMapTilemap.sprite({8, 0});
	stairConfiguration.bottom = blockMapTilemap.sprite({8, 0});
	blockMap.addBlockByID(2, std::make_unique<StairBlock>(stairConfiguration));

	HalfBlock::Configuration halfConfiguration;
	halfConfiguration.front = blockMapTilemap.sprite({4, 0});
	halfConfiguration.back = blockMapTilemap.sprite({4, 0});
	halfConfiguration.left = blockMapTilemap.sprite({4, 0});
	halfConfiguration.right = blockMapTilemap.sprite({4, 0});
	halfConfiguration.top = blockMapTilemap.sprite({5, 0});
	halfConfiguration.bottom = blockMapTilemap.sprite({5, 0});
	blockMap.addBlockByID(3, std::make_unique<HalfBlock>(halfConfiguration));

	blockMap.setChunkRange({-0, 0, -0}, {0, 0, 0});
	//blockMap.setChunkRange({-3, 0, -3}, {3, 0, 3});

	return app.run();
}
