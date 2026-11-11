#include <GL/glew.h>
#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/image.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"
#include "voxel/mesher.hpp"
#include "voxel/palette.hpp"
#include "voxel/renderer.hpp"
#include "voxel/rendering_system.hpp"

namespace
{
	constexpr spk::Vector2UInt ImageSize{320, 240};
	constexpr float CharacterScale = 0.5f;
	constexpr spk::Vector3 CharacterEntityScale{CharacterScale, CharacterScale, CharacterScale};
	constexpr const char *GoldenCategory = "rendering/voxel-scenes";

	enum class Block : std::uint32_t
	{
		Stone = 1,
		Slab = 2,
		Slope = 3,
		Stair = 4,
		Brick = 5,
		Skin = 6,
		Cloth = 7,
		Hair = 8,
		Metal = 9,
		Dark = 10,
		Grass = 11,
		Wood = 12,
		Gold = 13
	};

	using Horizontal = spk::Voxel::Cell::HorizontalOrientation;
	using Vertical = spk::Voxel::Cell::VerticalOrientation;
	using Builder = spk::Voxel::Volume::Builder;

	constexpr std::array<const char *, 16> MaterialNames{"stone", "slab", "slope", "stair", "brick", "skin", "cloth", "hair", "metal", "dark", "grass", "wood", "gold", "slab-profile", "slope-profile", "stair-profile"};

	[[nodiscard]] spk::Image loadAtlas()
	{
		return spk::Image::open(std::filesystem::path(SPARKLE_TEST_RESOURCES_DIR) / "textures/voxel_test_atlas.png");
	}

	[[nodiscard]] spk::Texture::Section section(std::size_t index, spk::Vector2UInt textureSize)
	{
		const float x = static_cast<float>(index % 4) * 16.0f;
		const float y = static_cast<float>(index / 4) * 16.0f;
		const spk::Vector2 anchor{(x + 0.5f) / static_cast<float>(textureSize.x), (y + 0.5f) / static_cast<float>(textureSize.y)};
		const spk::Vector2 size{15.0f / static_cast<float>(textureSize.x), 15.0f / static_cast<float>(textureSize.y)};
		return {anchor, size};
	}

	[[nodiscard]] spk::Palette palette(const spk::Image &atlas, bool alternate = false)
	{
		constexpr std::array<std::size_t, 16> Tiles{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 17, 18};
		spk::Voxel::Palette::Builder builder;
		for (std::size_t i = 0; i < MaterialNames.size(); ++i)
		{
			spk::Material::Builder material;
			const auto index = alternate && i < 13 ? (Tiles[i] + 3) % 16 : Tiles[i];
			material.add("albedo", atlas, section(index, atlas.size()));
			builder.setMaterial(MaterialNames[i], material.build());
		}
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Shape shape(const char *name)
	{
		spk::Voxel::Shape::Builder builder;
		builder.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (std::string(name) + ".json")));
		return builder.build();
	}

	[[nodiscard]] spk::Voxel::Definition definition(
		const spk::Voxel::Shape &shape, const char *material, const char *profileMaterial = nullptr)
	{
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{slot.name == "profile" ? profileMaterial : material, 1.0f});
		}
		return {shape, std::move(mapping)};
	}

	[[nodiscard]] spk::Voxel::Definition::Registry registry()
	{
		const auto cube = shape("cube");
		spk::Voxel::Definition::Registry::Builder builder;
		builder.add(1, definition(cube, "stone"));
		builder.add(2, definition(shape("slab"), "slab", "slab-profile"));
		builder.add(3, definition(shape("slope"), "slope", "slope-profile"));
		builder.add(4, definition(shape("stair"), "stair", "stair-profile"));
		builder.add(5, definition(cube, "brick"));
		builder.add(6, definition(cube, "skin"));
		builder.add(7, definition(cube, "cloth"));
		builder.add(8, definition(cube, "hair"));
		builder.add(9, definition(cube, "metal"));
		builder.add(10, definition(cube, "dark"));
		builder.add(11, definition(cube, "grass"));
		builder.add(12, definition(cube, "wood"));
		builder.add(13, definition(cube, "gold"));
		return builder.build();
	}

	void put(
		Builder &builder, std::uint32_t x, std::uint32_t y, std::uint32_t z, Block block, Horizontal horizontal = Horizontal::XPositive, Vertical vertical = Vertical::YPositive)
	{
		builder.set({x, y, z}, static_cast<std::uint32_t>(block), horizontal, vertical);
	}

	template <typename Filler>
	[[nodiscard]] spk::Voxel::Geometry bake(spk::Vector3UInt size, Filler &&fill)
	{
		Builder builder(size);
		builder.bind(registry());
		fill(builder);
		return spk::Voxel::Mesher().bake(builder.build());
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"Voxel scene camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &system;
		std::vector<std::unique_ptr<spk::Entity3D>> entities;

		Scene() :
			view(_createView()),
			system(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
		}

		spk::EngineView &_createView()
		{
			lookAt({9, 7, 14}, {0, 0, 0});
			camera.setPerspective(0.85f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.05f, 80.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		void lookAt(spk::Vector3 eye, spk::Vector3 target)
		{
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, target));
		}

		spk::Voxel::Renderer &add(
			const spk::Voxel::Geometry &geometry,
			const spk::Palette &appearance,
			spk::Vector3 position,
			spk::Vector3 rotation = {},
			spk::Vector3 scale = {1.0f, 1.0f, 1.0f},
			spk::Entity3D *parent = nullptr)
		{
			auto entity = std::make_unique<spk::Entity3D>("voxel scene object");
			entity->transform().place(position);
			entity->transform().setEulerRotation(rotation);
			entity->transform().rescale(scale);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry);
			renderer.bind(appearance);
			if (parent == nullptr)
			{
				engine.addEntity(entity.get());
			}
			else
			{
				entity->setParent(*parent);
			}
			entities.emplace_back(std::move(entity));
			return renderer;
		}

		spk::Entity3D &addJoint(spk::Entity3D &parent, spk::Vector3 position, spk::Vector3 rotation)
		{
			auto joint = std::make_unique<spk::Entity3D>("Right-hand weapon articulation", &parent);
			joint->transform().place(position);
			joint->transform().setEulerRotation(rotation);
			auto &result = *joint;
			entities.emplace_back(std::move(joint));
			return result;
		}

		~Scene()
		{
			for (const auto &entity : entities)
			{
				engine.removeEntity(entity.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	void expectVisibleImage(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const std::filesystem::path category{GoldenCategory};
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		gl.save(actual);
		const auto image = gl.capture();
		ASSERT_EQ(image.size, ImageSize);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		bool visible = false;
		for (std::uint32_t y = 0; y < ImageSize.y && !visible; ++y)
		{
			for (std::uint32_t x = 0; x < ImageSize.x && !visible; ++x)
			{
				const auto pixel = image.pixel({x, y});
				visible |= pixel[0] > 18 || pixel[1] > 18 || pixel[2] > 18;
			}
		}
		ASSERT_TRUE(visible) << name << ": rendered image is empty";
		const auto diff = sparkle_test::resultImagePath(category, name + "_difference");
		const auto comparison = sparkle_test::compareImages(actual, expected, diff);
		EXPECT_TRUE(comparison.matches) << name << ": " << comparison.differentPixelCount << " different pixels";
	}

	void render(Scene &scene, const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		spk::EngineWidget widget("voxel scene", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(gl.renderContext());
		expectVisibleImage(name);
		gl.reset();
	}

	struct ShapeCase
	{
		const char *name;
		Block block;
		Horizontal horizontal = Horizontal::XPositive;
		Vertical vertical = Vertical::YPositive;
		spk::Vector3 rotation{};
	};

	class VoxelShapeGoldenTest : public ::testing::TestWithParam<ShapeCase>
	{
	};

	TEST_P(VoxelShapeGoldenTest, RendersShapeThroughEngineView)
	{
		const auto &param = GetParam();
		auto atlas = loadAtlas();
		Scene scene;
		scene.lookAt({2.8f, 2.1f, 4.0f}, {0, 0, 0});
		const auto geometry = bake({1, 1, 1}, [&](Builder &builder) {
			put(builder, 0, 0, 0, param.block, param.horizontal, param.vertical);
		});
		ASSERT_GT(geometry.opaqueMesh().indexCount(), 0u);
		scene.add(geometry, palette(atlas), {-0.5f, -0.5f, -0.5f}, param.rotation);
		render(scene, std::string("shapes_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		AllShapeOrientations, VoxelShapeGoldenTest, ::testing::Values(ShapeCase{"cube_front", Block::Stone}, ShapeCase{"cube_yaw", Block::Stone, Horizontal::XPositive, Vertical::YPositive, {0, 45, 0}}, ShapeCase{"cube_tilt", Block::Stone, Horizontal::XPositive, Vertical::YPositive, {25, 40, -15}}, ShapeCase{"slab_lower", Block::Slab}, ShapeCase{"slab_inverted", Block::Slab, Horizontal::XPositive, Vertical::YNegative}, ShapeCase{"slope_east", Block::Slope}, ShapeCase{"slope_north", Block::Slope, Horizontal::ZPositive}, ShapeCase{"slope_west", Block::Slope, Horizontal::XNegative}, ShapeCase{"slope_south", Block::Slope, Horizontal::ZNegative}, ShapeCase{"slope_inverted", Block::Slope, Horizontal::XPositive, Vertical::YNegative}, ShapeCase{"stair_east", Block::Stair}, ShapeCase{"stair_north", Block::Stair, Horizontal::ZPositive}, ShapeCase{"stair_west", Block::Stair, Horizontal::XNegative}, ShapeCase{"stair_south", Block::Stair, Horizontal::ZNegative}, ShapeCase{"stair_inverted", Block::Stair, Horizontal::XPositive, Vertical::YNegative}, ShapeCase{"stair_rotated", Block::Stair, Horizontal::ZPositive, Vertical::YPositive, {12, 31, 0}}), [](const ::testing::TestParamInfo<ShapeCase> &info) {
			return std::string(info.param.name);
		});

	enum class Arrangement
	{
		AdjacentCubes,
		CubesWithGap,
		CubeAgainstSlab,
		CubeAgainstSlope,
		CubeAgainstStair,
		SlopesInterlocked,
		StairsFourDirections,
		SlabsStacked,
		Staircase,
		PitchedRoof,
		StoneArch,
		MixedPavilion,
		CheckerboardFloor,
		LayeredTerrain,
		WindowedWall,
		MazeOverhead
	};

	struct ArrangementCase
	{
		const char *name;
		Arrangement arrangement;
	};

	void buildSmall(Builder &builder, Arrangement arrangement)
	{
		constexpr std::array<Horizontal, 4> Orientations{
			Horizontal::XPositive, Horizontal::ZPositive, Horizontal::XNegative, Horizontal::ZNegative};
		switch (arrangement)
		{
		case Arrangement::AdjacentCubes:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 3, 1, 2, Block::Brick);
			break;
		case Arrangement::CubesWithGap:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 4, 1, 2, Block::Brick);
			break;
		case Arrangement::CubeAgainstSlab:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 3, 1, 2, Block::Slab);
			break;
		case Arrangement::CubeAgainstSlope:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 3, 1, 2, Block::Slope);
			break;
		case Arrangement::CubeAgainstStair:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 3, 1, 2, Block::Stair);
			break;
		case Arrangement::SlopesInterlocked:
			put(builder, 2, 1, 2, Block::Stone);
			for (std::uint32_t i = 0; i < 4; ++i)
			{
				put(builder, 1 + i, 2, 2, Block::Slope, Orientations[i]);
			}
			break;
		case Arrangement::StairsFourDirections:
			put(builder, 2, 1, 2, Block::Stone);
			for (std::uint32_t i = 0; i < 4; ++i)
			{
				put(builder, 1 + i, 2, 2, Block::Stair, Orientations[i]);
			}
			break;
		case Arrangement::SlabsStacked:
			put(builder, 2, 1, 2, Block::Stone);
			put(builder, 2, 2, 2, Block::Slab);
			put(builder, 2, 3, 2, Block::Slab, Horizontal::XPositive, Vertical::YNegative);
			break;
		default:
			break;
		}
	}

	void buildArchitecture(Builder &builder, Arrangement arrangement)
	{
		switch (arrangement)
		{
		case Arrangement::Staircase:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				const auto level = (x + (x % 2)) / 2;
				for (std::uint32_t y = 0; y < level; ++y)
				{
					put(builder, x, y, 2, Block::Brick);
				}
				put(builder, x, level, 2, x % 2 == 0 ? Block::Stair : Block::Slab);
			}
			break;
		case Arrangement::PitchedRoof:
			for (std::uint32_t x = 1; x < 7; ++x)
			{
				for (std::uint32_t z = 1; z < 7; ++z)
				{
					put(builder, x, 1, z, Block::Brick);
					if (x == 1 || x == 6)
					{
						put(builder, x, 2, z, Block::Slope, x == 1 ? Horizontal::XPositive : Horizontal::XNegative);
					}
					else
					{
						put(builder, x, 2, z, Block::Slab, Horizontal::XPositive, Vertical::YNegative);
					}
				}
			}
			break;
		case Arrangement::StoneArch:
			for (std::uint32_t y = 0; y < 5; ++y)
			{
				put(builder, 1, y, 3, Block::Stone);
				put(builder, 6, y, 3, Block::Stone);
			}
			for (std::uint32_t x = 1; x <= 6; ++x)
			{
				put(builder, x, 5, 3, Block::Brick);
			}
			put(builder, 2, 4, 3, Block::Slope);
			put(builder, 5, 4, 3, Block::Slope, Horizontal::XNegative);
			break;
		case Arrangement::MixedPavilion:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				for (std::uint32_t z = 0; z < 8; ++z)
				{
					put(builder, x, 0, z, Block::Stone);
					if ((x == 0 || x == 7) && (z == 0 || z == 7))
					{
						for (std::uint32_t y = 1; y < 5; ++y)
						{
							put(builder, x, y, z, Block::Wood);
						}
					}
					if (x > 0 && x < 7 && z > 0 && z < 7)
					{
						if (x == 1 || x == 6)
						{
							put(builder, x, 5, z, Block::Slope, x == 1 ? Horizontal::XPositive : Horizontal::XNegative);
						}
						else
						{
							put(builder, x, 5, z, Block::Slab, Horizontal::XPositive, Vertical::YNegative);
						}
					}
				}
			}
			break;
		case Arrangement::CheckerboardFloor:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				for (std::uint32_t z = 0; z < 8; ++z)
				{
					put(builder, x, 0, z, (x + z) % 2 == 0 ? Block::Stone : Block::Brick);
				}
			}
			break;
		case Arrangement::LayeredTerrain:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				for (std::uint32_t z = 0; z < 8; ++z)
				{
					const auto level = (x / 3 + z / 3) % 3;
					for (std::uint32_t y = 0; y <= level; ++y)
					{
						put(builder, x, y, z, y == level ? Block::Grass : Block::Stone);
					}
				}
			}
			break;
		case Arrangement::WindowedWall:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				for (std::uint32_t y = 0; y < 6; ++y)
				{
					if (x >= 3 && x <= 4 && y >= 2 && y <= 4)
					{
						continue;
					}
					put(builder, x, y, 3, Block::Brick);
				}
			}
			break;
		case Arrangement::MazeOverhead:
			for (std::uint32_t x = 0; x < 8; ++x)
			{
				for (std::uint32_t z = 0; z < 8; ++z)
				{
					put(builder, x, 0, z, Block::Stone);
					if ((x % 3 == 0 && z < 6) || (z % 3 == 0 && x > 1))
					{
						for (std::uint32_t y = 1; y < 4; ++y)
						{
							put(builder, x, y, z, Block::Brick);
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}

	[[nodiscard]] spk::Voxel::Geometry arrangementGeometry(Arrangement arrangement)
	{
		return bake({8, 7, 8}, [arrangement](Builder &builder) {
			buildSmall(builder, arrangement);
			buildArchitecture(builder, arrangement);
		});
	}

	class VoxelArrangementGoldenTest : public ::testing::TestWithParam<ArrangementCase>
	{
	};

	TEST_P(VoxelArrangementGoldenTest, RendersComplexOcclusion)
	{
		const auto &param = GetParam();
		auto atlas = loadAtlas();
		Scene scene;
		scene.lookAt({11, 9, 13}, {0, -0.5f, 0});
		const auto geometry = arrangementGeometry(param.arrangement);
		ASSERT_GT(geometry.opaqueMesh().indexCount(), 0u);
		scene.add(geometry, palette(atlas), {-4, -3, -4});
		render(scene, std::string("arrangement_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		OcclusionAndTerrain, VoxelArrangementGoldenTest, ::testing::Values(ArrangementCase{"adjacent_cubes", Arrangement::AdjacentCubes}, ArrangementCase{"cubes_with_gap", Arrangement::CubesWithGap}, ArrangementCase{"cube_against_slab", Arrangement::CubeAgainstSlab}, ArrangementCase{"cube_against_slope", Arrangement::CubeAgainstSlope}, ArrangementCase{"cube_against_stair", Arrangement::CubeAgainstStair}, ArrangementCase{"slopes_interlocked", Arrangement::SlopesInterlocked}, ArrangementCase{"stairs_four_directions", Arrangement::StairsFourDirections}, ArrangementCase{"slabs_stacked", Arrangement::SlabsStacked}, ArrangementCase{"staircase", Arrangement::Staircase}, ArrangementCase{"pitched_roof", Arrangement::PitchedRoof}, ArrangementCase{"stone_arch", Arrangement::StoneArch}, ArrangementCase{"mixed_pavilion", Arrangement::MixedPavilion}, ArrangementCase{"checkerboard_floor", Arrangement::CheckerboardFloor}, ArrangementCase{"layered_terrain", Arrangement::LayeredTerrain}, ArrangementCase{"windowed_wall", Arrangement::WindowedWall}, ArrangementCase{"maze_overhead", Arrangement::MazeOverhead}), [](const ::testing::TestParamInfo<ArrangementCase> &info) {
			return std::string(info.param.name);
		});

	enum class Pose
	{
		Neutral,
		LeftArmRaised,
		BothArmsRaised,
		Walking,
		Sword
	};

	void fillCharacter(Builder &builder, Pose pose)
	{
		for (std::uint32_t y = 0; y < 5; ++y)
		{
			put(builder, 4, y, pose == Pose::Walking ? 3 : 2, Block::Cloth);
			put(builder, 6, y, pose == Pose::Walking ? 1 : 2, Block::Cloth);
		}
		for (std::uint32_t x = 3; x <= 7; ++x)
		{
			for (std::uint32_t y = 5; y <= 9; ++y)
			{
				for (std::uint32_t z = 1; z <= 3; ++z)
				{
					put(builder, x, y, z, Block::Cloth);
				}
			}
		}
		for (std::uint32_t x = 4; x <= 6; ++x)
		{
			for (std::uint32_t y = 10; y <= 12; ++y)
			{
				for (std::uint32_t z = 1; z <= 3; ++z)
				{
					put(builder, x, y, z, Block::Skin);
				}
			}
			put(builder, x, 13, 2, Block::Hair);
		}
		for (std::uint32_t x : {4u, 6u})
		{
			put(builder, x, 11, 4, Block::Dark);
		}
		put(builder, 5, 10, 4, Block::Brick);
		const bool leftRaised = pose == Pose::LeftArmRaised || pose == Pose::BothArmsRaised;
		const bool rightRaised = pose == Pose::BothArmsRaised;
		const auto leftStart = leftRaised ? 9u : 5u;
		const auto rightStart = rightRaised ? 9u : 5u;
		for (std::uint32_t y = leftStart; y < leftStart + 5; ++y)
		{
			put(builder, 2, y, 2, y == (leftRaised ? leftStart : leftStart + 4) ? Block::Cloth : Block::Skin);
		}
		for (std::uint32_t y = rightStart; y < rightStart + 5; ++y)
		{
			put(builder, 8, y, 2, y == (rightRaised ? rightStart : rightStart + 4) ? Block::Cloth : Block::Skin);
		}
	}

	[[nodiscard]] spk::Voxel::Geometry character(Pose pose)
	{
		return bake({11, 16, 5}, [pose](Builder &builder) {
			fillCharacter(builder, pose);
		});
	}

	void fillSword(Builder &builder)
	{
		put(builder, 1, 0, 0, Block::Wood);
		put(builder, 1, 1, 0, Block::Wood);
		for (std::uint32_t x = 0; x < 3; ++x)
		{
			put(builder, x, 2, 0, Block::Gold);
		}
		for (std::uint32_t y = 3; y < 9; ++y)
		{
			put(builder, 1, y, 0, Block::Metal);
		}
	}

	[[nodiscard]] spk::Voxel::Geometry sword()
	{
		return bake({3, 9, 1}, [](Builder &builder) {
			fillSword(builder);
		});
	}

	constexpr spk::Vector3 RightHandAttachment{8.5f, 5.5f, 2.5f};
	constexpr spk::Vector3 WeaponForwardRotation{90.0f, 0.0f, 0.0f};
	constexpr spk::Vector3 GripOrigin{-1.5f, 0.5f, -0.5f};

	spk::Voxel::Renderer &attachSwordToHand(
		Scene &scene,
		spk::Voxel::Renderer &characterRenderer,
		const spk::Palette &appearance)
	{
		auto &joint = scene.addJoint(*characterRenderer.owner(), RightHandAttachment, WeaponForwardRotation);
		return scene.add(sword(), appearance, GripOrigin, {}, {1.0f, 1.0f, 1.0f}, &joint);
	}

	struct CharacterCase
	{
		const char *name;
		Pose pose;
	};

	class VoxelCharacterGoldenTest : public ::testing::TestWithParam<CharacterCase>
	{
	};

	TEST_P(VoxelCharacterGoldenTest, RendersProgrammaticCharacter)
	{
		const auto &param = GetParam();
		auto atlas = loadAtlas();
		Scene scene;
		scene.lookAt({8, 7, 14}, {0, 0, 0});
		const auto model = character(param.pose);
		ASSERT_GT(model.opaqueMesh().indexCount(), 0u);
		const auto appearance = palette(atlas);
		auto &renderer = scene.add(model, appearance, {-5.5f * CharacterScale, -7.5f * CharacterScale, -2.5f * CharacterScale}, {}, CharacterEntityScale);
		if (param.pose == Pose::Sword)
		{
			attachSwordToHand(scene, renderer, appearance);
		}
		render(scene, std::string("character_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		CharacterPoses, VoxelCharacterGoldenTest, ::testing::Values(CharacterCase{"neutral", Pose::Neutral}, CharacterCase{"left_arm_raised", Pose::LeftArmRaised}, CharacterCase{"both_arms_raised", Pose::BothArmsRaised}, CharacterCase{"walking", Pose::Walking}, CharacterCase{"with_sword", Pose::Sword}), [](const ::testing::TestParamInfo<CharacterCase> &info) {
			return std::string(info.param.name);
		});

	void fillRoom(Builder &builder, std::uint32_t width, std::uint32_t height, bool stoneHouse = false)
	{
		for (std::uint32_t x = 0; x < width; ++x)
		{
			for (std::uint32_t z = 0; z < width; ++z)
			{
				put(builder, x, 0, z, stoneHouse ? Block::Wood : ((x + z) % 2 == 0 ? Block::Stone : Block::Brick));
				if ((x == 0 || x + 1 == width || z == 0 || z + 1 == width))
				{
					for (std::uint32_t y = 1; y < height; ++y)
					{
						const bool door = z + 1 == width && x >= width / 2 - 1 &&
										  x <= width / 2 + 1 && y <= 3;
						const bool window = x == 0 && z >= 3 && z <= 5 && y >= 2 && y <= 4;
						if (!door && !window)
						{
							put(builder, x, y, z, stoneHouse ? Block::Stone : Block::Brick);
						}
					}
				}
				if ((x + z) % 3 == 0 && x != 0 && z != 0 && x + 1 != width && z + 1 != width)
				{
					put(builder, x, height - 1, z, stoneHouse ? Block::Grass : Block::Wood);
				}
			}
		}
	}

	[[nodiscard]] spk::Voxel::Geometry room(std::uint32_t width, std::uint32_t height, bool raisedFloor = false, bool stoneHouse = false)
	{
		return bake({width, height, width}, [&](Builder &builder) {
			fillRoom(builder, width, height, stoneHouse);
			if (raisedFloor)
			{
				for (std::uint32_t x = 3; x + 3 < width; x += 2)
				{
					for (std::uint32_t z = 3; z + 3 < width; z += 2)
					{
						put(builder, x, 1, z, (x + z) % 4 == 0 ? Block::Grass : Block::Stone);
					}
				}
			}
		});
	}

	enum class RoomView
	{
		Exterior,
		InteriorCenter,
		InteriorCorner,
		InteriorWallClose,
		InteriorWindow,
		CameraInsideWall,
		CameraInsideFloor,
		CameraFloorGrazing,
		InsideCharacter,
		ExteriorCharacters,
		TwoPalettes,
		SharedPaletteDifferentHouses
	};

	struct RoomCase
	{
		const char *name;
		RoomView view;
	};

	class VoxelRoomGoldenTest : public ::testing::TestWithParam<RoomCase>
	{
	};

	TEST_P(VoxelRoomGoldenTest, RendersVoxelRoomWithCameraOcclusion)
	{
		const auto &param = GetParam();
		auto atlas = loadAtlas();
		Scene scene;
		const bool tall = param.view == RoomView::InsideCharacter;
		const auto width = tall ? 19u : 13u;
		const auto height = tall ? 12u : 8u;
		const bool raisedFloor = param.view == RoomView::CameraInsideFloor || param.view == RoomView::CameraFloorGrazing;
		const auto geometry = room(width, height, raisedFloor);
		const float center = static_cast<float>(width) / 2;
		const float floor = static_cast<float>(height) / 2;
		const auto appearance = palette(atlas);
		scene.add(geometry, appearance, {-center, -floor, -center});
		switch (param.view)
		{
		case RoomView::Exterior:
			scene.lookAt({22, 15, 24}, {0, -1, 0});
			break;
		case RoomView::InteriorCenter:
			scene.lookAt({0, -1, 0}, {0, -1, -5});
			break;
		case RoomView::InteriorCorner:
			scene.lookAt({-3, -1, -3}, {2, -0.5f, 3});
			break;
		case RoomView::InteriorWallClose:
			scene.lookAt({0, -1, -4.5f}, {0, 0, -6.5f});
			break;
		case RoomView::InteriorWindow:
			scene.lookAt({-3, -1, 0}, {-6, 0, 0});
			break;
		case RoomView::CameraInsideWall:
			scene.lookAt({5.95f, -1.25f, -0.75f}, {-1.5f, -2.5f, -4.5f});
			break;
		case RoomView::CameraInsideFloor:
			scene.lookAt({0.1f, -3.75f, -0.2f}, {0.0f, -2.45f, -4.5f});
			break;
		case RoomView::CameraFloorGrazing:
			scene.lookAt({0.1f, -2.78f, -0.2f}, {0.0f, -2.95f, -5.0f});
			break;
		case RoomView::InsideCharacter:
		{
			auto &renderer = scene.add(character(Pose::Sword), appearance, {-5.5f * CharacterScale, -floor + 1, -2.5f * CharacterScale}, {}, CharacterEntityScale);
			attachSwordToHand(scene, renderer, appearance);
			scene.lookAt({2.0f, -0.5f, 8.0f}, {0.0f, -1.5f, 0.0f});
			break;
		}
		case RoomView::ExteriorCharacters:
			scene.add(character(Pose::Neutral), appearance, {-9.0f, -floor, 7.5f}, {}, CharacterEntityScale);
			scene.add(character(Pose::LeftArmRaised), appearance, {4.0f, -floor, 7.5f}, {}, CharacterEntityScale);
			scene.lookAt({19.0f, 11.0f, 25.0f}, {0.0f, 0.0f, 4.0f});
			break;
		case RoomView::TwoPalettes:
			scene.add(geometry, palette(atlas, true), {center + 1, -floor, -center});
			scene.lookAt({23, 14, 25}, {4, 0, 0});
			break;
		case RoomView::SharedPaletteDifferentHouses:
		{
			const auto secondHouse = room(width, height, false, true);
			ASSERT_NE(geometry.opaqueMesh().layout().vertexBuffer().identifier(), secondHouse.opaqueMesh().layout().vertexBuffer().identifier());
			ASSERT_NE(geometry.materialSlots().front().name, secondHouse.materialSlots().front().name);
			scene.add(secondHouse, appearance, {center + 1, -floor, -center});
			scene.lookAt({23, 14, 25}, {4, 0, 0});
			break;
		}
		}
		render(scene, std::string("room_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		OutsideAndInsideCameras, VoxelRoomGoldenTest, ::testing::Values(RoomCase{"exterior", RoomView::Exterior}, RoomCase{"interior_center", RoomView::InteriorCenter}, RoomCase{"interior_corner", RoomView::InteriorCorner}, RoomCase{"interior_wall_close", RoomView::InteriorWallClose}, RoomCase{"interior_window", RoomView::InteriorWindow}, RoomCase{"camera_inside_wall", RoomView::CameraInsideWall}, RoomCase{"camera_inside_floor", RoomView::CameraInsideFloor}, RoomCase{"camera_floor_grazing", RoomView::CameraFloorGrazing}, RoomCase{"inside_character", RoomView::InsideCharacter}, RoomCase{"exterior_characters", RoomView::ExteriorCharacters}, RoomCase{"two_palettes", RoomView::TwoPalettes}, RoomCase{"shared_palette_different_houses", RoomView::SharedPaletteDifferentHouses}), [](const ::testing::TestParamInfo<RoomCase> &info) {
			return std::string(info.param.name);
		});

	TEST(VoxelSceneFunctionalTest, ShapeProfileTilesContainOnlyTheirVisibleSilhouettes)
	{
		const auto atlas = loadAtlas();
		const auto opacity = [&](std::uint32_t tile, std::uint32_t x, std::uint32_t y) {
			return atlas.pixels().at((((tile / 4) * 16 + y) * atlas.size().x + (tile % 4) * 16 + x) * 4 + 3);
		};
		EXPECT_GT(opacity(13, 8, 13), 0);
		EXPECT_GT(opacity(16, 8, 3), 0);
		EXPECT_EQ(opacity(16, 8, 13), 0);
		EXPECT_GT(opacity(17, 12, 8), 0);
		EXPECT_EQ(opacity(17, 3, 8), 0);
		EXPECT_GT(opacity(18, 3, 3), 0);
		EXPECT_EQ(opacity(18, 3, 12), 0);
		EXPECT_GT(opacity(18, 12, 12), 0);
	}

	TEST(VoxelSceneFunctionalTest, AdjacentCubesCullBothInternalFaces)
	{
		const auto geometry = bake({2, 1, 1}, [](Builder &builder) {
			put(builder, 0, 0, 0, Block::Stone);
			put(builder, 1, 0, 0, Block::Brick);
		});
		EXPECT_EQ(geometry.opaqueMesh().indexCount(), 60u);
		EXPECT_EQ(geometry.depthMesh().indexCount(), 60u);
	}

	TEST(VoxelSceneFunctionalTest, FloorCullsSharedFacesBetweenAdjacentVoxels)
	{
		const auto geometry = bake({3, 1, 3}, [](Builder &builder) {
			for (std::uint32_t x = 0; x < 3; ++x)
			{
				for (std::uint32_t z = 0; z < 3; ++z)
				{
					put(builder, x, 0, z, Block::Stone);
				}
			}
		});
		constexpr std::uint32_t ExposedFaces = 3 * 3 * 2 + 3 * 4;
		EXPECT_EQ(geometry.opaqueMesh().indexCount(), ExposedFaces * 6);
		EXPECT_EQ(geometry.depthMesh().indexCount(), ExposedFaces * 6);
	}

	TEST(VoxelSceneFunctionalTest, RoomMeshCullsInternalCellBoundaries)
	{
		const auto geometry = room(13, 8);
		ASSERT_GT(geometry.opaqueMesh().indexCount(), 0u);
		EXPECT_TRUE(geometry.transparentMesh().empty());
		EXPECT_GT(geometry.depthMesh().indexCount(), 0u);
	}

	TEST(VoxelSceneFunctionalTest, SwordIsVerticalInItsOwnGeometry)
	{
		Builder builder({3, 9, 1});
		builder.bind(registry());
		fillSword(builder);
		const auto volume = builder.build();
		const auto material = [&](std::uint32_t x, std::uint32_t y) {
			return volume.at({x, y, 0}).definition().slot("side").name;
		};
		EXPECT_EQ(material(1, 0), "wood");
		EXPECT_EQ(material(1, 1), "wood");
		EXPECT_EQ(material(0, 2), "gold");
		EXPECT_EQ(material(1, 2), "gold");
		EXPECT_EQ(material(2, 2), "gold");
		EXPECT_EQ(material(1, 3), "metal");
		EXPECT_EQ(material(1, 8), "metal");
	}

	TEST(VoxelSceneFunctionalTest, SwordIsNotEmbeddedInCharacter)
	{
		Builder builder({11, 16, 5});
		builder.bind(registry());
		fillCharacter(builder, Pose::Sword);
		const auto volume = builder.build();
		EXPECT_EQ(volume.at({8, 5, 2}).definition().slot("side").name, "skin");
		EXPECT_TRUE(volume.at({8, 4, 2}).isAir());
		EXPECT_TRUE(volume.at({8, 3, 2}).isAir());
		EXPECT_TRUE(volume.at({9, 3, 2}).isAir());
	}

	TEST(VoxelSceneFunctionalTest, HandArticulationOrientsAndFollowsCharacter)
	{
		auto atlas = loadAtlas();
		Scene scene;
		const auto appearance = palette(atlas);
		auto &characterRenderer = scene.add(character(Pose::Sword), appearance, {1, -2, 3}, {}, CharacterEntityScale);
		auto &weaponRenderer = attachSwordToHand(scene, characterRenderer, appearance);
		auto *weaponEntity = weaponRenderer.owner();
		ASSERT_NE(weaponEntity, nullptr);
		ASSERT_NE(weaponEntity->parent(), nullptr);
		EXPECT_EQ(weaponEntity->parent()->parent(), characterRenderer.owner());
		const auto point = [&](float y) {
			return weaponEntity->transform().modelMatrix() * spk::Vector4{1.5f, y, 0.5f, 1.0f};
		};
		const auto grip = point(0.5f);
		const auto tip = point(8.5f);
		EXPECT_NEAR(grip.x, 1 + RightHandAttachment.x * CharacterScale, 1.0e-4f);
		EXPECT_NEAR(grip.y, -2 + RightHandAttachment.y * CharacterScale, 1.0e-4f);
		EXPECT_NEAR(grip.z, 3 + (RightHandAttachment.z + 1.0f) * CharacterScale, 1.0e-4f);
		EXPECT_NEAR(tip.x, grip.x, 1.0e-4f);
		EXPECT_NEAR(tip.y, grip.y, 1.0e-4f);
		EXPECT_NEAR(tip.z - grip.z, 8.0f * CharacterScale, 1.0e-4f);
		const auto handleBack = point(0.0f);
		EXPECT_NEAR(handleBack.z, 3 + (RightHandAttachment.z + 0.5f) * CharacterScale, 1.0e-4f);
		characterRenderer.owner()->transform().move({2, 0, 0});
		EXPECT_NEAR(point(0.5f).x, grip.x + 2.0f, 1.0e-4f);
	}
	TEST(VoxelSceneFunctionalTest, DistinctPalettesKeepSharedMesh)
	{
		auto atlas = loadAtlas();
		const auto geometry = character(Pose::Neutral);
		const auto a = palette(atlas);
		const auto b = palette(atlas, true);
		Scene scene;
		auto &left = scene.add(geometry, a, {-8, -7, -2});
		auto &right = scene.add(geometry, b, {2, -7, -2});
		ASSERT_NE(left.geometry(), nullptr);
		ASSERT_NE(right.geometry(), nullptr);
		EXPECT_EQ(
			left.geometry()->opaqueMesh().layout().vertexBuffer().identifier(),
			right.geometry()->opaqueMesh().layout().vertexBuffer().identifier());
	}
}
