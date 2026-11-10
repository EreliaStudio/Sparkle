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

	constexpr std::array<const char *, 13> MaterialNames{
		"stone", "slab", "slope", "stair", "brick", "skin", "cloth", "hair", "metal", "dark", "grass", "wood", "gold"};

	[[nodiscard]] spk::Image loadAtlas()
	{
		return spk::Image::open(std::filesystem::path(SPARKLE_TEST_RESOURCES_DIR) / "textures/voxel_test_atlas.png");
	}

	[[nodiscard]] spk::Texture::Section section(std::size_t index)
	{
		const float x = static_cast<float>(index % 4) * 0.25f;
		const float y = static_cast<float>(index / 4) * 0.25f;
		return {{x, y}, {0.25f, 0.25f}};
	}

	[[nodiscard]] spk::Palette palette(const spk::Image &atlas, bool alternate = false)
	{
		constexpr std::array<std::size_t, 13> Tiles{
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
		spk::Voxel::Palette::Builder builder;
		for (std::size_t i = 0; i < MaterialNames.size(); ++i)
		{
			spk::Material::Builder material;
			const auto index = alternate ? (Tiles[i] + 3) % 16 : Tiles[i];
			material.add("albedo", atlas, section(index));
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
		const spk::Voxel::Shape &shape, const char *material)
	{
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{material, 1.0f});
		}
		return {shape, std::move(mapping)};
	}

	[[nodiscard]] spk::Voxel::Definition::Registry registry()
	{
		const auto cube = shape("cube");
		spk::Voxel::Definition::Registry::Builder builder;
		builder.add(1, definition(cube, "stone"));
		builder.add(2, definition(shape("slab"), "slab"));
		builder.add(3, definition(shape("slope"), "slope"));
		builder.add(4, definition(shape("stair"), "stair"));
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
			spk::Vector3 rotation = {})
		{
			auto entity = std::make_unique<spk::Entity3D>("voxel scene object");
			entity->transform().place(position);
			entity->transform().setEulerRotation(rotation);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry);
			renderer.bind(appearance);
			engine.addEntity(entity.get());
			entities.emplace_back(std::move(entity));
			return renderer;
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
		if (!std::filesystem::exists(expected))
		{
			GTEST_SKIP() << "Reference requires visual approval: " << expected << "; actual: " << actual;
		}
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
		put(builder, 2, 1, 2, Block::Stone);
		switch (arrangement)
		{
		case Arrangement::AdjacentCubes:
			put(builder, 3, 1, 2, Block::Brick);
			break;
		case Arrangement::CubesWithGap:
			put(builder, 4, 1, 2, Block::Brick);
			break;
		case Arrangement::CubeAgainstSlab:
			put(builder, 3, 1, 2, Block::Slab);
			break;
		case Arrangement::CubeAgainstSlope:
			put(builder, 3, 1, 2, Block::Slope);
			break;
		case Arrangement::CubeAgainstStair:
			put(builder, 3, 1, 2, Block::Stair);
			break;
		case Arrangement::SlopesInterlocked:
			for (std::uint32_t i = 0; i < 4; ++i)
			{
				put(builder, 1 + i, 2, 2, Block::Slope, Orientations[i]);
			}
			break;
		case Arrangement::StairsFourDirections:
			for (std::uint32_t i = 0; i < 4; ++i)
			{
				put(builder, 1 + i, 2, 2, Block::Stair, Orientations[i]);
			}
			break;
		case Arrangement::SlabsStacked:
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
				for (std::uint32_t y = 0; y < x / 2; ++y)
				{
					put(builder, x, y, 2, Block::Brick);
				}
				put(builder, x, x / 2, 2, x % 2 == 0 ? Block::Stair : Block::Slab);
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
						put(builder, x, 2, z, Block::Slab);
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
						put(builder, x, 5, z, x == 1 || x == 6 ? Block::Slope : Block::Slab);
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
		const auto leftStart = pose == Pose::LeftArmRaised || pose == Pose::BothArmsRaised ? 9u : 5u;
		const auto rightStart = pose == Pose::BothArmsRaised ? 9u : 5u;
		for (std::uint32_t y = leftStart; y < leftStart + 5; ++y)
		{
			put(builder, 2, y, 2, y == leftStart ? Block::Cloth : Block::Skin);
		}
		for (std::uint32_t y = rightStart; y < rightStart + 5; ++y)
		{
			put(builder, 8, y, 2, y == rightStart ? Block::Cloth : Block::Skin);
		}
		if (pose == Pose::Sword)
		{
			put(builder, 9, 8, 2, Block::Wood);
			for (std::uint32_t y = 9; y <= 14; ++y)
			{
				put(builder, 9, y, 2, Block::Metal);
			}
			put(builder, 10, 9, 2, Block::Gold);
		}
	}

	[[nodiscard]] spk::Voxel::Geometry character(Pose pose)
	{
		return bake({11, 16, 5}, [pose](Builder &builder) {
			fillCharacter(builder, pose);
		});
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
		scene.lookAt({14, 12, 25}, {0, 0, 0});
		const auto model = character(param.pose);
		ASSERT_GT(model.opaqueMesh().indexCount(), 0u);
		scene.add(model, palette(atlas), {-5.5f, -7.5f, -2.5f});
		render(scene, std::string("character_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		CharacterPoses, VoxelCharacterGoldenTest, ::testing::Values(CharacterCase{"neutral", Pose::Neutral}, CharacterCase{"left_arm_raised", Pose::LeftArmRaised}, CharacterCase{"both_arms_raised", Pose::BothArmsRaised}, CharacterCase{"walking", Pose::Walking}, CharacterCase{"with_sword", Pose::Sword}), [](const ::testing::TestParamInfo<CharacterCase> &info) {
			return std::string(info.param.name);
		});

	void fillRoom(Builder &builder, std::uint32_t width, std::uint32_t height)
	{
		for (std::uint32_t x = 0; x < width; ++x)
		{
			for (std::uint32_t z = 0; z < width; ++z)
			{
				put(builder, x, 0, z, (x + z) % 2 == 0 ? Block::Stone : Block::Brick);
				if ((x == 0 || x + 1 == width || z == 0 || z + 1 == width))
				{
					for (std::uint32_t y = 1; y < height; ++y)
					{
						const bool door = z + 1 == width && x >= width / 2 - 1 &&
										  x <= width / 2 + 1 && y <= 3;
						const bool window = x == 0 && z >= 3 && z <= 5 && y >= 2 && y <= 4;
						if (!door && !window)
						{
							put(builder, x, y, z, Block::Brick);
						}
					}
				}
				if ((x + z) % 3 == 0 && x != 0 && z != 0 && x + 1 != width && z + 1 != width)
				{
					put(builder, x, height - 1, z, Block::Wood);
				}
			}
		}
	}

	[[nodiscard]] spk::Voxel::Geometry room(std::uint32_t width, std::uint32_t height)
	{
		return bake({width, height, width}, [&](Builder &builder) {
			fillRoom(builder, width, height);
		});
	}

	enum class RoomView
	{
		Exterior,
		InteriorCenter,
		InteriorCorner,
		InteriorWallClose,
		InteriorWindow,
		InsideCharacter,
		ExteriorCharacters,
		TwoPalettes,
		SharedMeshes
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
		const auto height = tall ? 18u : 8u;
		const auto geometry = room(width, height);
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
		case RoomView::InsideCharacter:
			scene.add(character(Pose::Sword), appearance, {-5.5f, -floor + 1, -2.5f});
			scene.lookAt({1, 0, 6}, {0, 0, 0});
			break;
		case RoomView::ExteriorCharacters:
			scene.add(character(Pose::Neutral), appearance, {-13.5f, -floor + 1, -2.5f});
			scene.add(character(Pose::LeftArmRaised), appearance, {2.5f, -floor + 1, -2.5f});
			scene.lookAt({23, 13, 25}, {0, 1, 0});
			break;
		case RoomView::TwoPalettes:
			scene.add(geometry, palette(atlas, true), {center + 1, -floor, -center});
			scene.lookAt({23, 14, 25}, {4, 0, 0});
			break;
		case RoomView::SharedMeshes:
			scene.add(geometry, appearance, {center + 1, -floor, -center});
			scene.lookAt({23, 14, 25}, {4, 0, 0});
			break;
		}
		render(scene, std::string("room_") + param.name);
	}

	INSTANTIATE_TEST_SUITE_P(
		OutsideAndInsideCameras, VoxelRoomGoldenTest, ::testing::Values(RoomCase{"exterior", RoomView::Exterior}, RoomCase{"interior_center", RoomView::InteriorCenter}, RoomCase{"interior_corner", RoomView::InteriorCorner}, RoomCase{"interior_wall_close", RoomView::InteriorWallClose}, RoomCase{"interior_window", RoomView::InteriorWindow}, RoomCase{"inside_character", RoomView::InsideCharacter}, RoomCase{"exterior_characters", RoomView::ExteriorCharacters}, RoomCase{"two_palettes", RoomView::TwoPalettes}, RoomCase{"shared_meshes", RoomView::SharedMeshes}), [](const ::testing::TestParamInfo<RoomCase> &info) {
			return std::string(info.param.name);
		});

	TEST(VoxelSceneFunctionalTest, AdjacentCubesCullBothInternalFaces)
	{
		const auto geometry = bake({2, 1, 1}, [](Builder &builder) {
			put(builder, 0, 0, 0, Block::Stone);
			put(builder, 1, 0, 0, Block::Brick);
		});
		EXPECT_EQ(geometry.opaqueMesh().indexCount(), 60u);
		EXPECT_EQ(geometry.depthMesh().indexCount(), 60u);
	}

	TEST(VoxelSceneFunctionalTest, RoomMeshCullsInternalCellBoundaries)
	{
		const auto geometry = room(13, 8);
		ASSERT_GT(geometry.opaqueMesh().indexCount(), 0u);
		EXPECT_TRUE(geometry.transparentMesh().empty());
		EXPECT_GT(geometry.depthMesh().indexCount(), 0u);
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
