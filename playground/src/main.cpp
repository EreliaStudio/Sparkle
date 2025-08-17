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

        virtual Block::Type _type() const = 0;
        virtual const spk::ObjMesh &_mesh() const = 0;

	static void _applySprite(spk::ObjMesh::Shape &p_shape, const spk::SpriteSheet::Sprite &p_sprite)
	{
		auto transform = [&](spk::Vertex &p_v)
		{
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
				for (const auto &shape : p_source.shapes())
				{
					if (std::holds_alternative<spk::ObjMesh::Triangle>(shape))
					{
						spk::ObjMesh::Triangle tri = std::get<spk::ObjMesh::Triangle>(shape);
						tri.a.position += p_offset;
						tri.b.position += p_offset;
						tri.c.position += p_offset;
						p_target.addShape(tri);
					}
					else
					{
						spk::ObjMesh::Quad quad = std::get<spk::ObjMesh::Quad>(shape);
						quad.a.position += p_offset;
						quad.b.position += p_offset;
						quad.c.position += p_offset;
						quad.d.position += p_offset;
						p_target.addShape(quad);
					}
				}
			}
		};

       private:
               static spk::Vector3 _applyOrientation(const spk::Vector3 &p_position, const Orientation &p_orientation)
		{
			spk::Vector3 result = p_position - spk::Vector3(0.5f, 0.5f, 0.5f);
			spk::Vector3 rotation(0, 0, 0);

			switch (p_orientation.horizontalOrientation)
			{
			case HorizontalOrientation::XPositive:
				break;
			case HorizontalOrientation::ZPositive:
				rotation.y = 90.0f;
				break;
			case HorizontalOrientation::XNegative:
				rotation.y = 180.0f;
				break;
			case HorizontalOrientation::ZNegative:
				rotation.y = 270.0f;
				break;
			}

			result = result.rotate(rotation);

			if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
			{
				result.y = -result.y;
			}

			return (result + spk::Vector3(0.5f, 0.5f, 0.5f));
		}

               static Entry _compute(const spk::ObjMesh &p_mesh, const Orientation &p_orientation)
		{
			Entry result;
			for (const auto &shape : p_mesh.shapes())
			{
				std::vector<spk::Vertex> vertices;
				if (std::holds_alternative<spk::ObjMesh::Triangle>(shape))
				{
					const spk::ObjMesh::Triangle &tri = std::get<spk::ObjMesh::Triangle>(shape);
					vertices = {tri.a, tri.b, tri.c};
				}
				else
				{
					const spk::ObjMesh::Quad &quad = std::get<spk::ObjMesh::Quad>(shape);
					vertices = {quad.a, quad.b, quad.c, quad.d};
				}

				for (auto &v : vertices)
				{
					v.position = _applyOrientation(v.position, p_orientation);
				}

				if (p_orientation.verticalOrientation == VerticalOrientation::YNegative)
				{
					std::reverse(vertices.begin(), vertices.end());
				}

                               spk::Polygon polygon;
                               for (const auto &v : vertices)
                               {
                                       polygon.points.push_back(v.position);
                               }

                               spk::Vector3 normal = polygon.normal();
                               spk::Vector3 axisNormal = normal.round();
                               bool isFace = false;
                               if (axisNormal == normal)
                               {
                                       float axisSum = std::abs(axisNormal.x) + std::abs(axisNormal.y) + std::abs(axisNormal.z);
                                       if (std::abs(axisSum - 1.0f) < 0.0001f)
                                       {
                                               normal = axisNormal;
                                               int axis = (axisNormal.x != 0.0f) ? 0 : (axisNormal.y != 0.0f) ? 1 : 2;
                                               float planeValue = (axisNormal.x + axisNormal.y + axisNormal.z > 0.0f) ? 1.0f : 0.0f;
                                               isFace = true;
                                               for (const auto &v : vertices)
                                               {
                                                       float coord = (axis == 0) ? v.position.x : (axis == 1) ? v.position.y : v.position.z;
                                                       if (std::abs(coord - planeValue) > 0.0001f)
                                                       {
                                                               isFace = false;
                                                               break;
                                                       }
                                               }
                                       }
                               }

                               if (isFace == false)
                               {
                                       if (vertices.size() == 3)
                                       {
                                               result.innerMesh.addShape(vertices[0], vertices[1], vertices[2]);
                                       }
                                       else
                                       {
                                               result.innerMesh.addShape(vertices[0], vertices[1], vertices[2], vertices[3]);
                                       }
                               }
                               else
                               {
                                       Face &face = result.faces[normal];
                                       for (const auto &v : vertices)
                                       {
                                               face.footprint.points.push_back(v.position);
                                       }
                                       if (vertices.size() == 3)
                                       {
                                               face.mesh.addShape(vertices[0], vertices[1], vertices[2]);
                                       }
                                       else
                                       {
                                               face.mesh.addShape(vertices[0], vertices[1], vertices[2], vertices[3]);
                                       }

                                       if (vertices.size() == 4)
                                       {
                                               float minA = std::numeric_limits<float>::max();
                                               float maxA = std::numeric_limits<float>::lowest();
                                               float minB = std::numeric_limits<float>::max();
                                               float maxB = std::numeric_limits<float>::lowest();
                                               for (const auto &v : vertices)
                                               {
                                                       float a;
                                                       float b;
                                                       if (normal.y != 0.0f)
                                                       {
                                                               a = v.position.x;
                                                               b = v.position.z;
                                                       }
                                                       else if (normal.x != 0.0f)
                                                       {
                                                               a = v.position.y;
                                                               b = v.position.z;
                                                       }
                                                       else
                                                       {
                                                               a = v.position.x;
                                                               b = v.position.y;
                                                       }
                                                       minA = std::min(minA, a);
                                                       maxA = std::max(maxA, a);
                                                       minB = std::min(minB, b);
                                                       maxB = std::max(maxB, b);
                                               }
                                               if (std::abs(minA - 0.0f) < 0.0001f && std::abs(maxA - 1.0f) < 0.0001f && std::abs(minB - 0.0f) < 0.0001f &&
                                                       std::abs(maxB - 1.0f) < 0.0001f)
                                               {
                                                       face.full = true;
                                               }
                                       }
                               }
			}
			return result;
		}

               std::map<Orientation, Entry> _cache;
       };

       mutable Cache _cache;

public:
	virtual ~Block() = default;

	void bake(spk::ObjMesh &p_toFill,
			  const NeightbourDescriber &p_neightbourDescriber,
			  const spk::Vector3 &p_position,
			  const Orientation &p_orientation) const
	{
               if (_cache.hasCase(p_orientation) == false)
               {
                       _cache.addCase(p_orientation, _mesh());
               }
               const Cache::Entry &data = _cache.getCase(p_orientation);

               std::array<const Face *, 6> neighFaces{};
               for (size_t i = 0; i < 6; ++i)
               {
                       neighFaces[i] = nullptr;
                       if (p_neightbourDescriber[i].first != nullptr)
                       {
                               const Block *neigh = p_neightbourDescriber[i].first;
                               const Orientation &neighOrientation = p_neightbourDescriber[i].second;
                               if (neigh->_cache.hasCase(neighOrientation) == false)
                               {
                                       neigh->_cache.addCase(neighOrientation, neigh->_mesh());
                               }
                               const Cache::Entry &neighData = neigh->_cache.getCase(neighOrientation);
                               spk::Vector3 oppositeNormal = -neightbourCoordinates[i];
                               auto it = neighData.faces.find(oppositeNormal);
                               if (it != neighData.faces.end())
                               {
                                       neighFaces[i] = &it->second;
                               }
                       }
               }

               bool fullyOccluded = true;
               for (size_t i = 0; i < 6; ++i)
               {
                       const Face *nf = neighFaces[i];
                       if (nf == nullptr || nf->full == false)
                       {
                               fullyOccluded = false;
                               break;
                       }
               }

               if (fullyOccluded == false)
               {
                       data.applyInnerMesh(p_toFill, p_position);
               }

               for (size_t i = 0; i < 6; ++i)
               {
                       spk::Vector3 normal = neightbourCoordinates[i];
                       auto faceIt = data.faces.find(normal);
                       if (faceIt == data.faces.end())
                       {
                               continue;
                       }

                       const Face &ourFace = faceIt->second;
                       if (ourFace.mesh.shapes().empty() == true)
                       {
                               continue;
                       }

                       const Face *currentNeighFace = neighFaces[i];
                       bool visible = true;
                       if (currentNeighFace != nullptr && currentNeighFace->mesh.shapes().empty() == false)
                       {
                               if (currentNeighFace->footprint.contains(ourFace.footprint) == true)
                               {
                                       visible = false;
                               }
                       }
                       if (visible == true)
                       {
                               data.applyFace(p_toFill, p_position, normal);
                       }
               }
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
        Block::Type _type() const override
        {
                return L"FullBlock";
        }
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

vt 0 0
vt 1 0
vt 0 1 
vt 1 1

vn  1.0  0.0  0.0
vn -1.0  0.0  0.0
vn  0.0  1.0  0.0
vn  0.0 -1.0  0.0
vn  0.0  0.0  1.0
vn  0.0  0.0 -1.0

f 3/1/1 7/3/1 8/4/1
f 3/3/1 8/2/1 4/4/1
f 1/1/2 5/3/2 6/4/2
f 1/3/2 6/2/2 2/1/2
f 7/1/3 3/3/3 2/4/3
f 7/3/3 2/2/3 6/1/3
f 4/1/4 8/3/4 5/4/4
f 4/3/4 5/2/4 1/1/4
f 8/1/5 7/3/5 6/4/5
f 8/3/5 6/2/5 5/1/5

f 3/1/6 4/3/6 1/4/6
f 3/3/6 1/2/6 2/1/6)";

public:
       explicit FullBlock(const Configuration &p_configuration) : _configuration(p_configuration)
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
                spk::SpriteSheet::Sprite triangles;
                spk::SpriteSheet::Sprite back;
                spk::SpriteSheet::Sprite ramp;
                spk::SpriteSheet::Sprite bottom;
        };

private:
        Block::Type _type() const override
        {
                return L"SlopeBlock";
        }
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

vt 0.0 0.0   # 1
vt 1.0 0.0   # 2
vt 0.0 1.0   # 3
vt 1.0 1.0   # 4

vn  0.0 -1.0  0.0
vn  0.0  0.0  1.0
vn -1.0  0.0  0.0
vn  1.0  0.0  0.0
vn  0.0  0.70710678 -0.70710678

f 1/1/1 2/2/1 3/4/1 4/3/1
f 4/1/2 3/2/2 6/4/2 5/3/2
f 1/1/3 4/2/3 5/3/3
f 2/1/4 6/3/4 3/2/4
f 1/1/5 5/3/5 6/4/5 2/2/5)";

public:
       explicit SlopeBlock(const Configuration &p_configuration) : _configuration(p_configuration)
       {
       _objMesh = spk::ObjMesh::loadFromString(objMeshCode);

       _applySprite(_objMesh.shapes()[0], _configuration.bottom);
       _applySprite(_objMesh.shapes()[1], _configuration.back);
       _applySprite(_objMesh.shapes()[2], _configuration.triangles);
       _applySprite(_objMesh.shapes()[3], _configuration.triangles);
       _applySprite(_objMesh.shapes()[4], _configuration.ramp);
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
			Block::Specifier _content[ChunkSizeX][ChunkSizeY][ChunkSizeZ];

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
							_content[x][y][z] = std::make_pair(p_id, Block::Orientation{Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive});
						}
					}
				}
				_isBaked = false;
			}

			void setContent(int p_x,
							int p_y,
							int p_z,
							const Block::ID &p_data,
							const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive,
																	   Block::VerticalOrientation::YPositive})
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

			void start()
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

                void setContent(spk::Vector3Int p_position,
                                Block::ID p_id,
                                const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
                {
                        setContent(p_position.x, p_position.y, p_position.z, p_id, p_orientation);
                }
                void setContent(spk::Vector2Int p_position,
                                int p_z,
                                Block::ID p_id,
                                const Block::Orientation &p_orientation = {Block::HorizontalOrientation::XPositive, Block::VerticalOrientation::YPositive})
                {
                        setContent(p_position.x, p_position.y, p_z, p_id, p_orientation);
                }

                void setContent(int p_x,
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
		for (size_t i = 0; i < Chunk::size.x; i++)
		{
			for (size_t j = 0; j < Chunk::size.z; j++)
			{
				p_chunkToFill.setContent(i, 0, j, 0);
				if (i == 0 && j != 0)
				{
					p_chunkToFill.setContent(i, 1, j, 1);
				}
				if (j == 0 && i != 0)
				{
					p_chunkToFill.setContent(i, 1, j, 2);
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

	spk::SpriteSheet blockMapTilemap = spk::SpriteSheet("playground/resources/texture/CubeTexture.png", {4, 1});

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
	slopeConfiguration.triangles = blockMapTilemap.sprite({1, 0});
	slopeConfiguration.back = blockMapTilemap.sprite({2, 0});
	slopeConfiguration.ramp = blockMapTilemap.sprite({2, 0});
	slopeConfiguration.bottom = blockMapTilemap.sprite({3, 0});

	blockMap.addBlockByID(1, std::make_unique<SlopeBlock>(slopeConfiguration));
	blockMap.addBlockByID(2, std::make_unique<SlopeBlock>(slopeConfiguration));

	blockMap.setChunkRange({-3, 0, -3}, {3, 0, 3});

	spk::Entity supportEntity = spk::Entity(L"SupportA", nullptr);
	supportEntity.activate();
	engine.addEntity(&supportEntity);
	supportEntity.transform().place({2, 2, 2});

	spk::ObjMesh supportMesh = spk::ObjMesh();
	supportMesh.loadFromFile("playground/resources/obj/support.obj");

	auto objRenderer = supportEntity.addComponent<spk::ObjMeshRenderer>(L"ObjMeshRenderer");
	objRenderer->setMesh(&supportMesh);

	return app.run();
}
