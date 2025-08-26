#include "structure/math/spk_plane.hpp"
#include "structure/math/spk_polygon.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
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

	static const spk::SpriteSheet &spriteSheet()
	{
		return (_spriteSheet);
	}

	static spk::SafePointer<const spk::Texture> texture()
	{
		return (&_spriteSheet);
	}

protected:
	using Type = std::wstring; // Each block type can be identified by its type : FullBlock, Slope, HalfBlock, Fence, for exemple
	// Multiple block can shared the same type, as long as they are the same shape in 3D, with just different sprite and interaction

	virtual const spk::ObjMesh &_mesh() const = 0;

	static inline spk::SpriteSheet _spriteSheet = spk::SpriteSheet("playground/resources/texture/CubeTexture.png", {9, 1});

	static void _applySprite(spk::ObjMesh::Shape &p_shape, const spk::SpriteSheet::Sprite &p_sprite)
	{
		auto transform = [&](spk::Vertex &p_v)
		{
			if (p_v.uv == -1)
			{
				return;
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
				(std::abs(minA - 0.0f) < spk::Constants::pointPrecision) && (std::abs(maxA - 1.0f) < spk::Constants::pointPrecision) &&
				(std::abs(minB - 0.0f) < spk::Constants::pointPrecision) && (std::abs(maxB - 1.0f) < spk::Constants::pointPrecision));
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
			if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
			{
				float minY = std::numeric_limits<float>::max();
				float maxY = std::numeric_limits<float>::lowest();

				for (const auto &v : p_vertices)
				{
					if (v.uv != -1)
					{
						minY = std::min(minY, v.uv.y);
						maxY = std::max(maxY, v.uv.y);
					}
				}
				for (auto &v : p_vertices)
				{
					v.position = _applyOrientation(v.position, p_orientation);
					if (v.uv != -1)
					{
						v.uv.y = minY + maxY - v.uv.y;
					}
				}
				std::reverse(p_vertices.begin(), p_vertices.end());
			}
			else
			{
				for (auto &v : p_vertices)
				{
					v.position = _applyOrientation(v.position, p_orientation);
				}
			}
		}

		static bool _isAxisAlignedFace(const std::vector<spk::Vertex> &p_vertices, spk::Vector3 &p_outNormal)
		{
			auto eq = [&](float p_a, float p_b) { return std::abs(p_a - p_b) <= spk::Constants::pointPrecision; };

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
				if (_isAxisAlignedFace(vertices, normal) == true)
				{
					Face &face = result.faces[normal];
					spk::Polygon current;
					if (vertices.size() == 3)
					{
						current = spk::Polygon::makeTri(vertices[0].position, vertices[1].position, vertices[2].position);
					}
					else
					{
						current = spk::Polygon::makeSquare(vertices[0].position, vertices[1].position, vertices[2].position, vertices[3].position);
					}

					if (face.footprint.points().empty() == true)
					{
						face.footprint = current;
					}
					else
					{
						face.footprint = face.footprint.fuze(current);
					}

					_addVerticesToMesh(face.mesh, vertices);

					if (vertices.size() == 4 && _isFullQuad(vertices, normal) == true)
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
		out.points().reserve(p_poly.points().size());
		std::transform(
			p_poly.points().begin(),
			p_poly.points().end(),
			std::back_inserter(out.points()),
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

public:
	explicit FullBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/full_block.obj");

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

public:
	explicit SlopeBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/slope_block.obj");

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

public:
	explicit HalfBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/half_block.obj");

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

public:
	explicit StairBlock(const Configuration &p_configuration) :
		_configuration(p_configuration)
	{
		_objMesh = spk::ObjMesh::loadFromFile("playground/resources/obj/stair_block.obj");

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
			spk::SafePointer<spk::CollisionMeshRenderer> _collisionRenderer;
			spk::SafePointer<spk::RigidBody> _rigidBody;

			spk::SafePointer<BlockMap> _blockMap;
			std::array<std::array<std::array<Block::Specifier, ChunkSizeX>, ChunkSizeY>, ChunkSizeZ> _content;

			bool _isBaked = false;
			spk::ObjMesh _mesh;
			spk::CollisionMesh _collisionMesh;

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
				try
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

									try
									{
										currentBlock->bake(_mesh, neightbourSpecifiers, {x, y, z}, currentSpecifier.second);
									}
									catch(const std::exception& e)
									{
										PROPAGATE_ERROR("Error while baking block ID [" + std::to_string(x) + "][" + std::to_string(y) + "][" + std::to_string(z) + "] of type [" + std::to_string(currentSpecifier.first) + "]", e);
									}
									catch (...)
									{
										GENERATE_ERROR("Unknow error while baking block ID [" + std::to_string(x) + "][" + std::to_string(y) + "][" + std::to_string(z) + "] of type [" + std::to_string(currentSpecifier.first) + "]");
									}
								}
							}
						}
					}
				}
				catch(const std::exception& e)
				{
					PROPAGATE_ERROR("Error while constructing the mesh", e);
				}
				catch (...)
				{
					GENERATE_ERROR("Unknow error while constructing the mesh");
				}

				try
				{
					_collisionMesh = spk::CollisionMesh::fromObjMesh(&_mesh);
				}
				catch(const std::exception& e)
				{
					PROPAGATE_ERROR("Error while constructing the collision mesh", e);
				}
				catch (...)
				{
					GENERATE_ERROR("Unknow error while constructing the collision mesh");
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
				_collisionRenderer = owner()->template getComponent<spk::CollisionMeshRenderer>();
				_rigidBody = owner()->template getComponent<spk::RigidBody>();
			}

			void onPaintEvent(spk::PaintEvent &p_event) override
			{
				
				
				if (_isBaked == false)
				{
					try
					{
						_bake();
					}
					catch(const std::exception& e)
					{
						PROPAGATE_ERROR("Error while baking the mesh and collider", e);
					}
					catch (...)
					{
						GENERATE_ERROR("Unknow error while baking the mesh and collider");
					}

					if (_renderer != nullptr)
					{
						_renderer->setMesh(mesh());
						_collisionRenderer->setMesh(collisionMesh());
						_rigidBody->setCollider(collisionMesh());
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

			spk::SafePointer<spk::CollisionMesh> collisionMesh()
			{
				return (&_collisionMesh);
			}
			const spk::SafePointer<const spk::CollisionMesh> collisionMesh() const
			{
				return (&_collisionMesh);
			}
		};

		spk::SafePointer<spk::ObjMeshRenderer> _renderer;
		spk::SafePointer<spk::CollisionMeshRenderer> _collisionRenderer;
		spk::SafePointer<spk::RigidBody> _rigidBody;
		spk::SafePointer<Data> _data;

	public:
		Chunk(const std::wstring &p_name, spk::SafePointer<BlockMap> p_parent) :
			spk::Entity(p_name, p_parent)
		{
			_renderer = this->template addComponent<spk::ObjMeshRenderer>(p_name + L"/ObjMeshRenderer");
			_collisionRenderer = this->template addComponent<spk::CollisionMeshRenderer>(p_name + L"/CollisionMeshRenderer");
			_rigidBody = this->template addComponent<spk::RigidBody>(p_name + L"/RigidBody");
			_data = this->template addComponent<Data>(p_name + L"/Data");
			_data->setBlockMap(p_parent);

			_renderer->setPriority(100);
			_collisionRenderer->setPriority(100);
			_collisionRenderer->setWireframe(true);
			_collisionRenderer->deactivate();
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

		void useCollisionRenderer(bool p_use)
		{
			if (p_use == true)
			{
				_renderer->deactivate();
				_collisionRenderer->activate();
			}
			else
			{
				_collisionRenderer->deactivate();
				_renderer->activate();
			}
		}
	};

private:
	spk::SafePointer<const spk::Texture> _texture;

	std::unordered_map<Block::ID, std::unique_ptr<Block>> _availableBlocks;
	std::unordered_map<spk::Vector3Int, std::unique_ptr<Chunk>> _chunks;

	std::vector<spk::SafePointer<Chunk>> _activeChunks;

	bool _useCollisionRenderer = false;

	std::unique_ptr<Chunk> _generateChunk(const spk::Vector3Int &p_chunkCoordinate)
	{
		const std::wstring chunkName = name() + L"/Chunk[" + p_chunkCoordinate.to_wstring() + L"]";

		std::unique_ptr<Chunk> newChunk = std::make_unique<Chunk>(chunkName, this);

		newChunk->transform().place(p_chunkCoordinate * Chunk::size);
		newChunk->setTexture(_texture);
		newChunk->fill(-1);

		_onChunkGeneration(p_chunkCoordinate, *newChunk);

		newChunk->activate();

		newChunk->useCollisionRenderer(_useCollisionRenderer);

		return newChunk;
	}

	virtual void _onChunkGeneration(const spk::Vector3Int &p_chunkCoordinate, Chunk &p_chunkToFill)
	{
		for (size_t i = 0; i < Chunk::size.x; i++)
		{
			for (size_t j = 0; j < Chunk::size.z; j++)
			{
				p_chunkToFill.setContent(i, 0, j, 0);
				if (i == 0 && j != 0)
				{
					p_chunkToFill.setContent(
						i, 1, j, 2, Block::Orientation{Block::HorizontalOrientation::ZNegative, Block::VerticalOrientation::YPositive});
				}
				if (j == 0 && i != 0)
				{
					p_chunkToFill.setContent(
						i, 1, j, 3, Block::Orientation{Block::HorizontalOrientation::ZPositive, Block::VerticalOrientation::YNegative});
				}
				if (i == 1 && j != 1)
				{
					p_chunkToFill.setContent(
						i, 1, j, 1, Block::Orientation{Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive});
				}
				if (j == 1 && i != 1)
				{
					p_chunkToFill.setContent(
						i, 1, j, 1, Block::Orientation{Block::HorizontalOrientation::ZPositive, Block::VerticalOrientation::YPositive});
				}
				if (i == 0 && j == 0)
				{
					p_chunkToFill.setContent(i, 1, j, 0);
					p_chunkToFill.setContent(i, 2, j, 0);
					p_chunkToFill.setContent(i, 3, j, 0);
					p_chunkToFill.setContent(i, 4, j, 0);
				}
			}
		}
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
			chunk->useCollisionRenderer(_useCollisionRenderer);
		}
	}
	void useCollisionRenderer(bool p_use)
	{
		_useCollisionRenderer = p_use;
		for (auto &chunk : _activeChunks)
		{
			chunk->useCollisionRenderer(_useCollisionRenderer);
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

template <size_t ChunkSizeX, size_t ChunkSizeY, size_t ChunkSizeZ>
class CollisionRenderToggler : public spk::Component
{
private:
	spk::SafePointer<BlockMap<ChunkSizeX, ChunkSizeY, ChunkSizeZ>> _blockMap;
	bool _useCollisionRenderer = false;

public:
	CollisionRenderToggler(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	void setBlockMap(spk::SafePointer<BlockMap<ChunkSizeX, ChunkSizeY, ChunkSizeZ>> p_blockMap)
	{
		_blockMap = p_blockMap;
	}

	void onKeyboardEvent(spk::KeyboardEvent &p_event) override
	{
		if (p_event.type == spk::KeyboardEvent::Type::Press && p_event.key == spk::Keyboard::F1)
		{
			_useCollisionRenderer = (_useCollisionRenderer == false);
			if (_blockMap != nullptr)
			{
				_blockMap->useCollisionRenderer(_useCollisionRenderer);
			}
		}
	}
};

class RayCastPrinter : public spk::Component
{
private:
	spk::SafePointer<spk::CameraComponent> _cameraComponent;
	spk::Entity _cubeEntity;
	spk::ColorMesh _cubeMesh;
	bool _isCasting = false;

	void _updateCubePosition(const spk::Vector2Int &p_mousePosition)
	{
		spk::Vector3 cameraDirection = _cameraComponent->camera().convertScreenToCamera(spk::Viewport::convertScreenToOpenGL(p_mousePosition));

		const auto &camMtx = _cameraComponent->owner()->transform().model();
		spk::Vector3 worldDirection = (camMtx * spk::Vector4(cameraDirection, 0.0f)).xyz().normalize();

		auto hit = spk::RayCast::launch(owner(), worldDirection, 1000.0f);
		if (hit.entity != nullptr)
		{
			_cubeEntity.transform().place(hit.position);
		}
	}

public:
	RayCastPrinter(const std::wstring &p_name) :
		spk::Component(p_name)
	{
	}

	spk::SafePointer<spk::Entity> cubeEntity()
	{
		return (&_cubeEntity);
	}

	void start() override
	{
		_cameraComponent = owner()->getComponent<spk::CameraComponent>();

		owner()->engine()->addEntity(&_cubeEntity);

		_cubeEntity.transform().place({3, 3, 3});
		_cubeEntity.setName(L"RayCastCube");
		_cubeEntity.activate();

		auto renderer = _cubeEntity.addComponent<spk::ColorMeshRenderer>(L"RayCastCube/ColorMeshRenderer");
		renderer->activate();

		float s = 0.1f;
		spk::Color color = spk::Color::red;

		using CV = spk::ColorVertex;
		CV v1{{-s, -s, -s}, color};
		CV v2{{s, -s, -s}, color};
		CV v3{{s, s, -s}, color};
		CV v4{{-s, s, -s}, color};
		CV v5{{-s, -s, s}, color};
		CV v6{{s, -s, s}, color};
		CV v7{{s, s, s}, color};
		CV v8{{-s, s, s}, color};

		_cubeMesh.clear();

		_cubeMesh.addShape(v1, v4, v3, v2); // Back
		_cubeMesh.addShape(v5, v6, v7, v8); // Front
		_cubeMesh.addShape(v1, v5, v8, v4); // Left
		_cubeMesh.addShape(v2, v3, v7, v6); // Right
		_cubeMesh.addShape(v4, v8, v7, v3); // Top
		_cubeMesh.addShape(v1, v2, v6, v5); // Bottom

		renderer->setMesh(&_cubeMesh);
	}

	void onMouseEvent(spk::MouseEvent &p_event) override
	{
		if (_cameraComponent == nullptr)
		{
			return;
		}

		switch (p_event.type)
		{
		case spk::MouseEvent::Type::Press:
			if (p_event.button == spk::Mouse::Button::Right)
			{
				_isCasting = true;
				_updateCubePosition(p_event.mouse->position());
			}
			break;
		case spk::MouseEvent::Type::Motion:
			if (_isCasting == true)
			{
				_updateCubePosition(p_event.position);
			}
			break;
		case spk::MouseEvent::Type::Release:
			if (p_event.button == spk::Mouse::Button::Right)
			{
				_isCasting = false;
			}
			break;
		default:
			break;
		}
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

	auto rayCastPrinter = player.addComponent<RayCastPrinter>(L"Player/RayCastPrinter");
	rayCastPrinter->activate();

	player.cameraComponent()->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	player.transform().place({5.0f, 5.0f, 5.0f});
	player.transform().lookAt({3.0f, 3.0f, 3.0f});

	BlockMap<16, 16, 16> blockMap = BlockMap<16, 16, 16>(L"BlockMap", nullptr);
	blockMap.setTexture(Block::texture());
	blockMap.activate();
	engine.addEntity(&blockMap);

	auto fullBlockSprite = Block::spriteSheet().sprite({0, 0});
	FullBlock::Configuration fullConfiguration;
	fullConfiguration.front = fullBlockSprite;
	fullConfiguration.back = fullBlockSprite;
	fullConfiguration.left = fullBlockSprite;
	fullConfiguration.right = fullBlockSprite;
	fullConfiguration.top = fullBlockSprite;
	fullConfiguration.bottom = fullBlockSprite;
	blockMap.addBlockByID(0, std::make_unique<FullBlock>(fullConfiguration));

	SlopeBlock::Configuration slopeConfiguration;
	slopeConfiguration.triangleLeft = Block::spriteSheet().sprite({1, 0});
	slopeConfiguration.triangleRight = Block::spriteSheet().sprite({1, 0});
	slopeConfiguration.back = Block::spriteSheet().sprite({2, 0});
	slopeConfiguration.ramp = Block::spriteSheet().sprite({2, 0});
	slopeConfiguration.bottom = Block::spriteSheet().sprite({3, 0});
	blockMap.addBlockByID(1, std::make_unique<SlopeBlock>(slopeConfiguration));

	StairBlock::Configuration stairConfiguration;
	stairConfiguration.staircaseTop = Block::spriteSheet().sprite({7, 0});
	stairConfiguration.staircaseFront = Block::spriteSheet().sprite({7, 0});
	stairConfiguration.left = Block::spriteSheet().sprite({6, 0});
	stairConfiguration.right = Block::spriteSheet().sprite({6, 0});
	stairConfiguration.back = Block::spriteSheet().sprite({8, 0});
	stairConfiguration.bottom = Block::spriteSheet().sprite({8, 0});
	blockMap.addBlockByID(2, std::make_unique<StairBlock>(stairConfiguration));

	HalfBlock::Configuration halfConfiguration;
	halfConfiguration.front = Block::spriteSheet().sprite({4, 0});
	halfConfiguration.back = Block::spriteSheet().sprite({4, 0});
	halfConfiguration.left = Block::spriteSheet().sprite({4, 0});
	halfConfiguration.right = Block::spriteSheet().sprite({4, 0});
	halfConfiguration.top = Block::spriteSheet().sprite({5, 0});
	halfConfiguration.bottom = Block::spriteSheet().sprite({5, 0});
	blockMap.addBlockByID(3, std::make_unique<HalfBlock>(halfConfiguration));

	blockMap.setChunkRange({-0, 0, -0}, {0, 0, 0});

	auto collisionRenderToggler = player.addComponent<CollisionRenderToggler<16, 16, 16>>(L"Player/CollisionRenderToggler");
	collisionRenderToggler->setBlockMap(&blockMap);

	return app.run();
}
