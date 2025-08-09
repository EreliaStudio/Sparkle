#include <sparkle.hpp>

class Block
{
public:
	using ID = short;

	using NeightbourVisibility = std::array<bool, 6>;

	enum class Side : size_t
	{
		Front = 0,
		Back = 1,
		Left = 2,
		Right = 3,
		Top = 4,
		Down = 5
	};

	static inline Side opposite(Side p_side)
	{
		switch (p_side)
		{
		case Side::Front:
			return Side::Back;
		case Side::Back:
			return Side::Front;
		case Side::Left:
			return Side::Right;
		case Side::Right:
			return Side::Left;
		case Side::Top:
			return Side::Down;
		case Side::Down:
			return Side::Top;
		}
		return Side::Front;
	}

	struct FaceData
	{
		std::vector<spk::Vector3> points;
		std::vector<spk::Vector2> uvOffsets;
		spk::Vector2 uvBase;

		struct Template
		{
			uint8_t vertexCount;
			spk::Vector3 positions[4];
			spk::Vector2 textureCoordinates01[4];
			bool present;

			FaceData materialize(const spk::SpriteSheet::Sprite &p_sprite) const
			{
				FaceData faceData;
				if (!present)
				{
					return faceData;
				}

				faceData.uvBase = p_sprite.anchor;
				faceData.points.reserve(vertexCount);
				faceData.uvOffsets.reserve(vertexCount);

				for (uint8_t i = 0; i < vertexCount; ++i)
				{
					faceData.points.push_back(positions[i]);
					faceData.uvOffsets.push_back({textureCoordinates01[i].x * p_sprite.size.x, textureCoordinates01[i].y * p_sprite.size.y});
				}
				return faceData;
			}
		};
	};

protected:
	static FaceData _makeQuad(const spk::SpriteSheet::Sprite &p_sprite,
							  const spk::Vector3 &p_a,
							  const spk::Vector2 &p_uvA01,
							  const spk::Vector3 &p_b,
							  const spk::Vector2 &p_uvB01,
							  const spk::Vector3 &p_c,
							  const spk::Vector2 &p_uvC01,
							  const spk::Vector3 &p_d,
							  const spk::Vector2 &p_uvD01)
	{
		return FaceData{{p_a, p_b, p_c, p_d},
						{p_uvA01 * p_sprite.size, p_uvB01 * p_sprite.size, p_uvC01 * p_sprite.size, p_uvD01 * p_sprite.size},
						p_sprite.anchor};
	}

	static FaceData _makeTri(const spk::SpriteSheet::Sprite &p_sprite,
							 const spk::Vector3 &p_a,
							 const spk::Vector2 &p_uvA01,
							 const spk::Vector3 &p_b,
							 const spk::Vector2 &p_uvB01,
							 const spk::Vector3 &p_c,
							 const spk::Vector2 &p_uvC01)
	{
		return FaceData{{p_a, p_b, p_c}, {p_uvA01 * p_sprite.size, p_uvB01 * p_sprite.size, p_uvC01 * p_sprite.size}, p_sprite.anchor};
	}

	static FaceData _makeFace(spk::Vector2 p_uvBase, std::vector<spk::Vector3> p_points, std::vector<spk::Vector2> p_uvOffsets)
	{
		FaceData face{std::move(p_points), std::move(p_uvOffsets), p_uvBase};
		return face;
	}

	static FaceData _emptyFace()
	{
		return FaceData{};
	}

	static void _emitFace(const FaceData &p_face, const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh)
	{
		const auto &points = p_face.points;
		const auto &uvOffset = p_face.uvOffsets;
		if (points.size() < 3 || points.size() != uvOffset.size())
		{
			return;
		}

		for (size_t i = 1; i + 1 < points.size(); ++i)
		{
			p_mesh.addShape(spk::VoxelVertex{points[0] + p_position, p_face.uvBase + uvOffset[0]},
							spk::VoxelVertex{points[i] + p_position, p_face.uvBase + uvOffset[i]},
							spk::VoxelVertex{points[i + 1] + p_position, p_face.uvBase + uvOffset[i + 1]});
		}
	}

public:
	virtual ~Block() = default;
	virtual void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_meshToFeed, const NeightbourVisibility &p_neightbourVisibility) const = 0;

	virtual bool facePresent(Side p_side) const = 0;
	virtual bool blocksViewFrom(Side p_fromSide, const Block *p_sourceBlock) const
	{
		// Default: fully opaque if the face exists.
		(void)p_sourceBlock;
		return facePresent(p_fromSide);
	}
};

class FullBlock : public Block
{
private:
	std::array<FaceData, 6> _faces;

	static inline Block::FaceData::Template fullBlockTemplates[6] = {
		// Front (z = 1)
		{4,
		 {spk::Vector3(0, 0, 1), spk::Vector3(1, 0, 1), spk::Vector3(1, 1, 1), spk::Vector3(0, 1, 1)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
		// Back (z = 0)
		{4,
		 {spk::Vector3(1, 0, 0), spk::Vector3(0, 0, 0), spk::Vector3(0, 1, 0), spk::Vector3(1, 1, 0)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
		// Left (x = 0)
		{4,
		 {spk::Vector3(0, 0, 0), spk::Vector3(0, 0, 1), spk::Vector3(0, 1, 1), spk::Vector3(0, 1, 0)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
		// Right (x = 1)
		{4,
		 {spk::Vector3(1, 0, 1), spk::Vector3(1, 0, 0), spk::Vector3(1, 1, 0), spk::Vector3(1, 1, 1)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
		// Top (y = 1)
		{4,
		 {spk::Vector3(0, 1, 1), spk::Vector3(1, 1, 1), spk::Vector3(1, 1, 0), spk::Vector3(0, 1, 0)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
		// Down (y = 0)
		{4,
		 {spk::Vector3(0, 0, 0), spk::Vector3(1, 0, 0), spk::Vector3(1, 0, 1), spk::Vector3(0, 0, 1)},
		 {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
		 true},
	};

public:
	explicit FullBlock(const spk::SpriteSheet::Sprite &p_faceSprite)
	{
		for (int i = 0; i < 6; ++i)
		{
			_faces[i] = fullBlockTemplates[i].materialize(p_faceSprite);
		}
	}

	FullBlock(const spk::SpriteSheet::Sprite &p_topSprite,
			  const spk::SpriteSheet::Sprite &p_bottomSprite,
			  const spk::SpriteSheet::Sprite &p_sideSprite)
	{
		_faces[0] = fullBlockTemplates[0].materialize(p_sideSprite);
		_faces[1] = fullBlockTemplates[1].materialize(p_sideSprite);
		_faces[2] = fullBlockTemplates[2].materialize(p_sideSprite);
		_faces[3] = fullBlockTemplates[3].materialize(p_sideSprite);
		_faces[4] = fullBlockTemplates[4].materialize(p_topSprite);
		_faces[5] = fullBlockTemplates[5].materialize(p_bottomSprite);
	}

	explicit FullBlock(const std::array<spk::SpriteSheet::Sprite, 6> &p_faceSprites)
	{
		for (int i = 0; i < 6; ++i)
		{
			_faces[i] = fullBlockTemplates[i].materialize(p_faceSprites[i]);
		}
	}

	void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh, const NeightbourVisibility &p_visibility) const override
	{
		for (size_t i = 0; i < _faces.size(); ++i)
		{
			if (!p_visibility[i])
			{
				continue;
			}
			_emitFace(_faces[i], p_position, p_mesh);
		}
	}

	bool facePresent(Side) const override
	{
		return true;
	}
	bool blocksViewFrom(Side, const Block *) const override
	{
		return true;
	}
};

class SlopeBlock : public Block
{
public:
	enum class Orientation
	{
		NorthToSouth = 0,
		EastToWest = 1,
		SouthToNorth = 2,
		WestToEast = 3
	};

	struct Sprites
	{
		spk::SpriteSheet::Sprite triangles;
		spk::SpriteSheet::Sprite back;
		spk::SpriteSheet::Sprite ramp;
		spk::SpriteSheet::Sprite bottom;
	};

private:
	static inline std::array<Block::FaceData::Template, 6> canonicalNorthToSouth = {
		// Front (quad, high) y = z
		Block::FaceData::Template{4,
								  {spk::Vector3(0, 0, 1), spk::Vector3(1, 0, 1), spk::Vector3(1, 1, 1), spk::Vector3(0, 1, 1)},
								  {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								  true},
		// Back (empty)
		Block::FaceData::Template{3,
								  {spk::Vector3(0, 0, 0), spk::Vector3(), spk::Vector3(), spk::Vector3()},
								  {spk::Vector2(0, 0), spk::Vector2(), spk::Vector2(), spk::Vector2()},
								  false},
		// Left (triangle)
		Block::FaceData::Template{3,
								  {spk::Vector3(0, 0, 0), spk::Vector3(0, 0, 1), spk::Vector3(0, 1, 1), spk::Vector3()},
								  {spk::Vector2(1, 1), spk::Vector2(0, 1), spk::Vector2(0, 0), spk::Vector2()},
								  true},
		// Right (triangle)
		Block::FaceData::Template{3,
								  {spk::Vector3(1, 0, 1), spk::Vector3(1, 0, 0), spk::Vector3(1, 1, 1), spk::Vector3()},
								  {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(0, 0), spk::Vector2()},
								  true},
		// Top (quad ramp)
		Block::FaceData::Template{4,
								  {spk::Vector3(0, 1, 1), spk::Vector3(1, 1, 1), spk::Vector3(1, 0, 0), spk::Vector3(0, 0, 0)},
								  {spk::Vector2(0, 0), spk::Vector2(1, 0), spk::Vector2(1, 1), spk::Vector2(0, 1)},
								  true},
		// Down (quad bottom)
		Block::FaceData::Template{4,
								  {spk::Vector3(0, 0, 0), spk::Vector3(1, 0, 0), spk::Vector3(1, 0, 1), spk::Vector3(0, 0, 1)},
								  {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								  true}};

	static spk::Vector3 _rotateQuarterTurnsY(const spk::Vector3 &p_value, Orientation p_orientation)
	{
		spk::Vector3 centered = p_value - spk::Vector3(0.5f, 0.0f, 0.5f);
		spk::Vector3 rotated;
		switch (p_orientation)
		{
		case Orientation::NorthToSouth:
			rotated = centered;
			break; // 0°
		case Orientation::EastToWest:
			rotated = {centered.z, centered.y, -centered.x};
			break; // 90°
		case Orientation::SouthToNorth:
			rotated = {-centered.x, centered.y, -centered.z};
			break; // 180°
		case Orientation::WestToEast:
			rotated = {-centered.z, centered.y, centered.x};
			break; // 270°
		}
		return rotated + spk::Vector3(0.5f, 0.0f, 0.5f);
	}

	static Side _remapSideAfterQuarterTurnsY(Side p_face, Orientation p_orientation)
	{
		if (p_face == Side::Top || p_face == Side::Down)
		{
			return p_face;
		}

		auto ringIndexOf = [](Side p_face) -> int
		{
			switch (p_face)
			{
			case Side::Front:
				return 0;
			case Side::Right:
				return 1;
			case Side::Back:
				return 2;
			case Side::Left:
				return 3;
			default:
				return -1;
			}
		};
		auto faceOfRingIndex = [](int p_faceIndex) -> Side
		{
			switch (p_faceIndex & 3)
			{
			case 0:
				return Side::Front;
			case 1:
				return Side::Right;
			case 2:
				return Side::Back;
			default:
				return Side::Left;
			}
		};

		const int index = ringIndexOf(p_face);
		if (index < 0)
		{
			return p_face;
		}

		int turns = 0;
		switch (p_orientation)
		{
		case Orientation::NorthToSouth:
			turns = 0;
			break;
		case Orientation::EastToWest:
			turns = 1;
			break;
		case Orientation::SouthToNorth:
			turns = 2;
			break;
		case Orientation::WestToEast:
			turns = 3;
			break;
		}
		return faceOfRingIndex(index + turns);
	}

	static const spk::SpriteSheet::Sprite &
	_selectSpriteForFace(const Sprites &p_sprites, size_t p_faceIndex, const Block::FaceData::Template &p_template)
	{
		if (p_template.present == false)
		{
			return p_sprites.bottom;
		}
		if (p_faceIndex == static_cast<size_t>(Side::Top))
		{
			return p_sprites.ramp;
		}
		if (p_faceIndex == static_cast<size_t>(Side::Down))
		{
			return p_sprites.bottom;
		}
		return (p_template.vertexCount == 3) ? p_sprites.triangles : p_sprites.back;
	}

	Sprites _sprites;
	Orientation _orientation;
	std::array<FaceData, 6> _faces;
	std::array<bool, 6> _faceMask{};

	void _rebuildFaces()
	{
		_faces.fill(_emptyFace());
		_faceMask.fill(false); // reset mask

		for (size_t srcFace = 0; srcFace < canonicalNorthToSouth.size(); ++srcFace)
		{
			const auto &src = canonicalNorthToSouth[srcFace];
			if (!src.present)
			{
				continue;
			}

			Block::FaceData::Template rotated{};
			rotated.vertexCount = src.vertexCount;
			rotated.present = true;
			for (int i = 0; i < 4; ++i)
			{
				rotated.positions[i] = _rotateQuarterTurnsY(src.positions[i], _orientation);
				rotated.textureCoordinates01[i] = src.textureCoordinates01[i];
			}

			const Side dst = _remapSideAfterQuarterTurnsY(static_cast<Side>(srcFace), _orientation);

			const auto &sprite = _selectSpriteForFace(_sprites, static_cast<size_t>(dst), rotated);
			_faces[static_cast<size_t>(dst)] = rotated.materialize(sprite);

			_faceMask[static_cast<size_t>(dst)] = true;
		}
	}

public:
	SlopeBlock(const Sprites &p_sprites, Orientation p_orientation) :
		_sprites(p_sprites),
		_orientation(p_orientation)
	{
		_rebuildFaces();
	}

	void setOrientation(Orientation p_orientation)
	{
		_orientation = p_orientation;
		_rebuildFaces();
	}

	void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh, const NeightbourVisibility &p_neightbourVisibility) const override
	{
		for (size_t i = 0; i < _faces.size(); ++i)
		{
			if (!p_neightbourVisibility[i])
			{
				continue;
			}
			_emitFace(_faces[i], p_position, p_mesh);
		}
	}

	bool facePresent(Side p_side) const override
	{
		return _faceMask[static_cast<size_t>(p_side)];
	}

	bool blocksViewFrom(Side p_fromSide, const Block *) const override
	{
		return facePresent(p_fromSide);
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
			spk::SafePointer<spk::VoxelMeshRenderer> _renderer;

			spk::SafePointer<BlockMap> _blockMap;
			Block::ID _content[ChunkSizeX][ChunkSizeY][ChunkSizeZ];

			bool _isBaked = false;
			spk::VoxelMesh _mesh;

			void _bake()
			{
				_mesh.clear();

				auto isInside = [](int p_x, int p_y, int p_z) -> bool
				{ return (p_x >= 0 && p_x < size.x && p_y >= 0 && p_y < size.y && p_z >= 0 && p_z < size.z); };

				auto isFilled = [&](int p_x, int p_y, int p_z) -> bool
				{
					return (p_x >= 0 && p_x < size.x && p_y >= 0 && p_y < size.y && p_z >= 0 && p_z < size.z &&
							_content[p_x][p_y][p_z] != static_cast<Block::ID>(-1));
				};

				auto blockAt = [&](int p_x, int p_y, int p_z) -> const Block *
				{
					if (!isFilled(p_x, p_y, p_z))
					{
						return nullptr;
					}
					Block::ID id = _content[p_x][p_y][p_z];
					return (_blockMap ? _blockMap->blockById(id) : nullptr);
				};

				// NEW: neighbor-side aware occlusion test
				auto isBlocking = [&](int p_nx, int p_ny, int p_nz, Block::Side p_mySide, const Block *p_self) -> bool
				{
					const Block *neighbor = blockAt(p_nx, p_ny, p_nz);
					if (!neighbor)
					{
						return false;
					}
					const Block::Side fromSide = Block::opposite(p_mySide);
					return neighbor->blocksViewFrom(fromSide, p_self);
				};

				for (int z = 0; z < size.z; ++z)
				{
					for (int y = 0; y < size.y; ++y)
					{
						for (int x = 0; x < size.x; ++x)
						{
							Block::ID id = _content[x][y][z];
							if (id == static_cast<Block::ID>(-1))
							{
								continue;
							}

							const Block *block = (_blockMap ? _blockMap->blockById(id) : nullptr);
							if (!block)
							{
								continue;
							}

							Block::NeightbourVisibility visibility;
							visibility[(size_t)Block::Side::Front] =
								block->facePresent(Block::Side::Front) && !isBlocking(x, y, z + 1, Block::Side::Front, block);

							visibility[(size_t)Block::Side::Back] =
								block->facePresent(Block::Side::Back) && !isBlocking(x, y, z - 1, Block::Side::Back, block);

							visibility[(size_t)Block::Side::Left] =
								block->facePresent(Block::Side::Left) && !isBlocking(x - 1, y, z, Block::Side::Left, block);

							visibility[(size_t)Block::Side::Right] =
								block->facePresent(Block::Side::Right) && !isBlocking(x + 1, y, z, Block::Side::Right, block);

							visibility[(size_t)Block::Side::Top] =
								block->facePresent(Block::Side::Top) && !isBlocking(x, y + 1, z, Block::Side::Top, block);

							visibility[(size_t)Block::Side::Down] =
								block->facePresent(Block::Side::Down) && !isBlocking(x, y - 1, z, Block::Side::Down, block);

							spk::Vector3 blockPosition(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));

							block->bake(blockPosition, _mesh, visibility);
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
							_content[x][y][z] = p_id;
						}
					}
				}
				_isBaked = false;
			}

			void setContent(int p_x, int p_y, int p_z, Block::ID p_id)
			{
				_content[p_x][p_y][p_z] = p_id;
				_isBaked = false;
			}

			void start()
			{
				_renderer = owner()->template getComponent<spk::VoxelMeshRenderer>();
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

			spk::SafePointer<spk::VoxelMesh> mesh()
			{
				return (&_mesh);
			}
			const spk::SafePointer<const spk::VoxelMesh> mesh() const
			{
				return (&_mesh);
			}
		};

		spk::SafePointer<spk::VoxelMeshRenderer> _renderer;
		spk::SafePointer<Data> _data;

	public:
		Chunk(const std::wstring &p_name, spk::SafePointer<BlockMap> p_parent) :
			spk::Entity(p_name, p_parent)
		{
			_renderer = addComponent<spk::VoxelMeshRenderer>(p_name + L"/VoxelMeshRenderer");
			_data = addComponent<Data>(p_name + L"/Data");
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

		void setContent(spk::Vector3Int p_position, Block::ID p_id)
		{
			setContent(p_position.x, p_position.y, p_position.z, p_id);
		}
		void setContent(spk::Vector2Int p_position, int p_z, Block::ID p_id)
		{
			setContent(p_position.x, p_position.y, p_z, p_id);
		}

		void setContent(int p_x, int p_y, int p_z, Block::ID p_id)
		{
			_data->setContent(p_x, p_y, p_z, p_id);
		}

		bool isBaked() const
		{
			return _data.isBaked();
		}

		spk::SafePointer<spk::VoxelMesh> mesh()
		{
			return (_data.mesh());
		}
		const spk::SafePointer<const spk::VoxelMesh> mesh() const
		{
			return (_data.mesh());
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
				if (i == 0 || j == 0)
				{
					p_chunkToFill.setContent(i, 1, j, 1);
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
						_chunks[chunkPosition] = _generateChunk(chunkPosition);
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
	spk::SafePointer<spk::CameraComponent> _cameraComponent;
	spk::SafePointer<spk::FreeViewController> _freeViewController;

public:
	Player(const std::wstring &p_name, spk::SafePointer<spk::Entity> p_parent) :
		spk::Entity(p_name, p_parent),
		_cameraComponent(addComponent<spk::CameraComponent>(L"Player/CameraComponent")),
		_freeViewController(addComponent<spk::FreeViewController>(L"Player/FreeViewController"))
	{
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

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(1);
	window->requestMousePlacement({400, 300});

	spk::GameEngine engine;
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(&engine);
	engineWidget.activate();

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
	blockMap.addBlockByID(0, std::make_unique<FullBlock>(fullBlockSprite));

	SlopeBlock::Sprites slopeSprites;
	slopeSprites.triangles = blockMapTilemap.sprite({1, 0});
	slopeSprites.back = blockMapTilemap.sprite({2, 0});
	slopeSprites.ramp = blockMapTilemap.sprite({2, 0});
	slopeSprites.bottom = blockMapTilemap.sprite({3, 0});
	blockMap.addBlockByID(1, std::make_unique<SlopeBlock>(slopeSprites, SlopeBlock::Orientation::NorthToSouth));

	blockMap.setChunkRange({-3, 0, -3}, {3, 0, 3});

	return app.run();
}