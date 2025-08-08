#include <sparkle.hpp>

class Block
{
public:
	using NeightbourVisibility = std::array<bool, 6>;

	struct FaceData
	{
		std::vector<spk::Vector3> points;	 // >= 3 vertices
		std::vector<spk::Vector2> uvOffsets; // same length as points
		spk::Vector2 uvBase;				 // added during bake

		// Hard-coded, immutable description of a face (positions + normalized UVs).
		struct Template
		{
			uint8_t vertexCount;				  // 3 or 4
			spk::Vector3 positions[4];			  // last entries ignored if triangle
			spk::Vector2 textureCoordinates01[4]; // normalized in [0..1]
			bool present;						  // false => skip this face

			// Build a FaceData from this template + a sprite.
			FaceData materialize(const spk::SpriteSheet::Sprite &p_sprite) const
			{
				FaceData faceData;
				if (!present)
				{
					return faceData; // empty => skipped by _emitFace
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
	// Full-rect _makeQuad from p_a sprite
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

	// Overload if you want p_a custom base and offsets (no sprite)
	static FaceData _makeFace(spk::Vector2 p_uvBase, std::vector<spk::Vector3> p_points, std::vector<spk::Vector2> p_uvOffsets)
	{
		FaceData f{std::move(p_points), std::move(p_uvOffsets), p_uvBase};
		return f;
	}

	static FaceData _emptyFace()
	{
		return FaceData{};
	} // skipped by _emitFace

	// Triangulate (fan) and emit: finalUV = p_uvBase + p_uvOffsets[i]
	static void _emitFace(const FaceData &p_face, const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh)
	{
		const auto &points = p_face.points;
		const auto &uvOffset = p_face.uvOffsets;
		if (points.size() < 3 || points.size() != uvOffset.size())
		{
			return; // or assert/throw
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
	virtual void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_meshToFeed, const NeightbourVisibility &p_neightbourVisibility) = 0;
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

	void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh, const NeightbourVisibility &p_visibility) override
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
};

class SlopeBlock : public Block
{
public:
	enum class Orientation
	{
		NorthToSouth,
		SouthToNorth,
		EastToWest,
		WestToEast
	};

	struct Sprites
	{
		spk::SpriteSheet::Sprite triangles; // triangular sides
		spk::SpriteSheet::Sprite back;		// tall rectangular end
		spk::SpriteSheet::Sprite ramp;		// sloped top
		spk::SpriteSheet::Sprite bottom;	// underside
	};

	enum Face : size_t
	{
		Front = 0,
		Back = 1,
		Left = 2,
		Right = 3,
		Top = 4,
		Down = 5
	};

private:
	struct EnumHash
	{
		size_t operator()(Orientation p_value) const noexcept
		{
			return static_cast<size_t>(p_value);
		}
	};

	static const std::unordered_map<Orientation, std::array<FaceData::Template, 6>, EnumHash> slopeBlockTemplates;

	static const spk::SpriteSheet::Sprite &_selectSpriteForFace(const Sprites &p_sprites, size_t p_faceIndex, const FaceData::Template &p_template)
	{
		if (!p_template.present)
		{
			return p_sprites.bottom;
		}
		if (p_faceIndex == Top)
		{
			return p_sprites.ramp;
		}
		if (p_faceIndex == Down)
		{
			return p_sprites.bottom;
		}

		return (p_template.vertexCount == 3) ? p_sprites.triangles : p_sprites.back;
	}

	Sprites _sprites;
	Orientation _orientation;
	std::array<FaceData, 6> _faces;

	void _rebuildFaces()
	{
		const auto &templates = slopeBlockTemplates.at(_orientation);
		for (size_t i = 0; i < _faces.size(); ++i)
		{
			const auto &faceTemplate = templates[i];
			if (!faceTemplate.present)
			{
				_faces[i] = _emptyFace();
				continue;
			}
			const auto &sprite = _selectSpriteForFace(_sprites, i, faceTemplate);
			_faces[i] = faceTemplate.materialize(sprite);
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

	void bake(const spk::Vector3 &p_position, spk::VoxelMesh &p_mesh, const NeightbourVisibility &p_neightbourVisibility) override
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
};

// ---------- Map initialization (one entry per orientation) ----------
const std::unordered_map<SlopeBlock::Orientation, std::array<Block::FaceData::Template, 6>, SlopeBlock::EnumHash> SlopeBlock::slopeBlockTemplates = {
	{SlopeBlock::Orientation::NorthToSouth,
	 std::array<Block::FaceData::Template, 6>{
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
								   true}}},
	{SlopeBlock::Orientation::SouthToNorth,
	 std::array<Block::FaceData::Template, 6>{
		 // Front (empty)
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 0), spk::Vector3(), spk::Vector3(), spk::Vector3()},
								   {spk::Vector2(0, 0), spk::Vector2(), spk::Vector2(), spk::Vector2()},
								   false},
		 // Back (quad, high) y = 1 - z
		 Block::FaceData::Template{4,
								   {spk::Vector3(1, 0, 0), spk::Vector3(0, 0, 0), spk::Vector3(0, 1, 0), spk::Vector3(1, 1, 0)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true},
		 // Left (triangle)
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 0), spk::Vector3(0, 0, 1), spk::Vector3(0, 1, 0), spk::Vector3()},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Right (triangle)
		 Block::FaceData::Template{3,
								   {spk::Vector3(1, 0, 1), spk::Vector3(1, 0, 0), spk::Vector3(1, 1, 0), spk::Vector3()},
								   {spk::Vector2(1, 1), spk::Vector2(0, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Top (quad ramp)
		 Block::FaceData::Template{4,
								   {spk::Vector3(1, 1, 0), spk::Vector3(0, 1, 0), spk::Vector3(0, 0, 1), spk::Vector3(1, 0, 1)},
								   {spk::Vector2(0, 0), spk::Vector2(1, 0), spk::Vector2(1, 1), spk::Vector2(0, 1)},
								   true},
		 // Down (quad bottom)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 0, 0), spk::Vector3(1, 0, 0), spk::Vector3(1, 0, 1), spk::Vector3(0, 0, 1)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true}}},
	{SlopeBlock::Orientation::EastToWest,
	 std::array<Block::FaceData::Template, 6>{
		 // Front (triangle) y = x
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 1), spk::Vector3(1, 0, 1), spk::Vector3(1, 1, 1), spk::Vector3()},
								   {spk::Vector2(1, 1), spk::Vector2(0, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Back (triangle)
		 Block::FaceData::Template{3,
								   {spk::Vector3(1, 0, 0), spk::Vector3(0, 0, 0), spk::Vector3(1, 1, 0), spk::Vector3()},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Left (empty)
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 0), spk::Vector3(), spk::Vector3(), spk::Vector3()},
								   {spk::Vector2(0, 0), spk::Vector2(), spk::Vector2(), spk::Vector2()},
								   false},
		 // Right (quad, high)
		 Block::FaceData::Template{4,
								   {spk::Vector3(1, 0, 1), spk::Vector3(1, 0, 0), spk::Vector3(1, 1, 0), spk::Vector3(1, 1, 1)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true},
		 // Top (quad ramp)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 0, 1), spk::Vector3(1, 1, 1), spk::Vector3(1, 1, 0), spk::Vector3(0, 0, 0)},
								   {spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0), spk::Vector2(0, 1)},
								   true},
		 // Down (quad bottom)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 0, 0), spk::Vector3(1, 0, 0), spk::Vector3(1, 0, 1), spk::Vector3(0, 0, 1)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true}}},
	{SlopeBlock::Orientation::WestToEast,
	 std::array<Block::FaceData::Template, 6>{
		 // Front (triangle)
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 1), spk::Vector3(1, 0, 1), spk::Vector3(0, 1, 1), spk::Vector3()},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Back (triangle)
		 Block::FaceData::Template{3,
								   {spk::Vector3(1, 0, 0), spk::Vector3(0, 0, 0), spk::Vector3(0, 1, 0), spk::Vector3()},
								   {spk::Vector2(1, 1), spk::Vector2(0, 1), spk::Vector2(0, 0), spk::Vector2()},
								   true},
		 // Left (quad, high)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 0, 0), spk::Vector3(0, 0, 1), spk::Vector3(0, 1, 1), spk::Vector3(0, 1, 0)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true},
		 // Right (empty)
		 Block::FaceData::Template{3,
								   {spk::Vector3(0, 0, 0), spk::Vector3(), spk::Vector3(), spk::Vector3()},
								   {spk::Vector2(0, 0), spk::Vector2(), spk::Vector2(), spk::Vector2()},
								   false},
		 // Top (quad ramp)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 1, 1), spk::Vector3(1, 0, 1), spk::Vector3(1, 0, 0), spk::Vector3(0, 1, 0)},
								   {spk::Vector2(0, 0), spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0)},
								   true},
		 // Down (quad bottom)
		 Block::FaceData::Template{4,
								   {spk::Vector3(0, 0, 0), spk::Vector3(1, 0, 0), spk::Vector3(1, 0, 1), spk::Vector3(0, 0, 1)},
								   {spk::Vector2(0, 1), spk::Vector2(1, 1), spk::Vector2(1, 0), spk::Vector2(0, 0)},
								   true}}}};

int main()
{
	spk::GraphicalApplication app;
	auto window = app.createWindow(L"Playground", {{0, 0}, {800, 600}});
	window->setUpdateTimer(1);
	window->requestMousePlacement({400, 300});

	spk::SafePointer<spk::GameEngine> engine = new spk::GameEngine();
	spk::GameEngineWidget engineWidget(L"EngineWidget", window->widget());
	engineWidget.setGeometry({0, 0}, window->geometry().size);
	engineWidget.setGameEngine(engine);
	engineWidget.activate();

	// --- Player + camera ---
	spk::SafePointer<spk::Entity> player = new spk::Entity(L"Player");
	player->activate();
	engine->addEntity(player);

	auto cameraComponent = player->addComponent<spk::CameraComponent>(L"Player/CameraComponent");
	cameraComponent->setPerspective(60.0f, static_cast<float>(window->geometry().size.x) / static_cast<float>(window->geometry().size.y));
	auto playerController = player->addComponent<spk::FreeViewController>(L"Player/FreeViewController");
	player->transform().place({5.0f, 5.0f, 5.0f});
	player->transform().lookAt({0.0f, 0.0f, 0.0f});

	// --- One voxel block entity ---
	spk::SafePointer<spk::Entity> blockEntity = new spk::Entity(L"BlockMap");
	blockEntity->activate();
	engine->addEntity(blockEntity);

	// Renderer
	auto renderer = blockEntity->addComponent<spk::VoxelMeshRenderer>(L"Block/VoxelMeshRenderer");

	// Texture (use whatever atlas you want; full-UV sprite below)
	spk::SpriteSheet atlas("playground/resources/texture/CubeTexture.png", {4, 1});
	renderer->setTexture(&atlas);

	// Build the mesh from p_a FullBlock with the BOTTOM p_face hidden
	spk::SafePointer<spk::VoxelMesh> mesh = new spk::VoxelMesh();

	// Full-_makeQuad sprite 0/0
	FullBlock block(atlas.sprite({0, 0}));

	Block::NeightbourVisibility visibleFaces = {
		true, // FRONT
		true, // BACK
		true, // LEFT
		true, // RIGHT
		true, // TOP
		false // DOWN (hidden)
	};

	blockEntity->transform().place({0.0f, 0.0f, 0.0f});

	SlopeBlock::Sprites slopeSprites;
	slopeSprites.triangles = atlas.sprite({1, 0}); // both p_side triangles
	slopeSprites.back = atlas.sprite({2, 0});	   // tall end
	slopeSprites.ramp = atlas.sprite({2, 0});	   // sloped p_top
	slopeSprites.bottom = atlas.sprite({3, 0});

	SlopeBlock slopeNS(slopeSprites, SlopeBlock::Orientation::NorthToSouth);
	SlopeBlock slopeSN(slopeSprites, SlopeBlock::Orientation::SouthToNorth);
	SlopeBlock slopeEW(slopeSprites, SlopeBlock::Orientation::EastToWest);
	SlopeBlock slopeWE(slopeSprites, SlopeBlock::Orientation::WestToEast);

	visibleFaces = {
		true,
		true,
		true,
		true, // sides
		true,
		true // p_top, bottom
	};

	slopeNS.bake({3, 0, 3}, *mesh, visibleFaces);
	slopeSN.bake({5, 0, 3}, *mesh, visibleFaces);
	slopeEW.bake({7, 0, 3}, *mesh, visibleFaces);
	slopeWE.bake({9, 0, 3}, *mesh, visibleFaces);
	block.bake({0, 0, 0}, *mesh, visibleFaces);

	renderer->setMesh(mesh);

	return app.run();
}