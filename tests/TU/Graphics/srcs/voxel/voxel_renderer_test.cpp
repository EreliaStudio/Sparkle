#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "graphics/image.hpp"
#include "rendering/command/draw_voxel_bucket_command.hpp"
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
	constexpr spk::Vector2UInt ImageSize{160, 120};

	class TestAtlas final : public spk::Texture
	{
	public:
		TestAtlas()
		{
			std::array<std::uint8_t, 8 * 8 * 4> pixels{};
			for (std::size_t y = 0; y < 8; ++y)
			{
				for (std::size_t x = 0; x < 8; ++x)
				{
					const auto offset = (y * 8 + x) * 4;
					pixels[offset] = x < 4 ? 255 : 0;
					pixels[offset + 1] = y < 4 ? 255 : 0;
					pixels[offset + 2] = x >= 4 ? 255 : 0;
					pixels[offset + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {8, 8}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Geometry namedGeometry(const std::string &shapeName, const std::string &materialName)
	{
		spk::Voxel::Shape::Builder shapes;
		shapes.addFromJSON(spk::JSON::Value::loadFromFile(
			std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (shapeName + ".json")));
		const auto shape = shapes.build();
		spk::Voxel::Definition::SlotMapping mapping;
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{materialName, 1.0f});
		}
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, spk::Voxel::Definition(shape, std::move(mapping)));
		spk::Voxel::Volume::Builder volume({1, 1, 1});
		volume.bind(registry.build());
		volume.set({0, 0, 0}, 1);
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Voxel::Geometry cubeGeometry()
	{
		return namedGeometry("cube", "stone");
	}

	[[nodiscard]] spk::Palette makePalette(const spk::Texture &atlas, spk::Texture::Section section)
	{
		spk::Material::Builder material;
		material.add("albedo", atlas, section);
		spk::Voxel::Palette::Builder palette;
		palette.setMaterial("stone", material.build());
		return palette.build();
	}

	[[nodiscard]] spk::Palette sharedPalette(const spk::Texture &atlas)
	{
		spk::Voxel::Palette::Builder builder;
		for (const auto &[name, section] : std::array<std::pair<const char *, spk::Texture::Section>, 2>{
				{{"stone", {{0.0f, 0.0f}, {0.5f, 0.5f}}},
				 {"wood", {{0.5f, 0.0f}, {0.5f, 0.5f}}}}})
		{
			spk::Material::Builder material;
			material.add("albedo", atlas, section);
			builder.setMaterial(name, material.build());
		}
		return builder.build();
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"camera"};
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
			const spk::Vector3 eye{3.0f, 2.4f, 5.0f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
			camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		spk::Voxel::Renderer &add(
			const spk::Voxel::Geometry &geometry,
			const spk::Palette &palette,
			spk::Vector3 position,
			spk::Vector3 rotation = {})
		{
			auto entity = std::make_unique<spk::Entity3D>("voxel");
			entity->transform().place(position);
			entity->transform().setEulerRotation(rotation);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry);
			renderer.bind(palette);
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

		[[nodiscard]] std::size_t bucketCount()
		{
			spk::RenderSnapshot::Builder builder;
			spk::EngineView::CaptureContext context;
			system.buildViewRenderSnapshot(builder, context);
			return builder.renderPass(spk::EngineView::OpaquePassKey).commandCount();
		}
	};

	[[nodiscard]] spk::RenderSnapshot capturedWidget(spk::EngineWidget &widget)
	{
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		return builder.build();
	}

	void saveAndCheck(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const std::filesystem::path category{"rendering/voxel"};
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		gl.save(actual);
		const auto image = gl.capture();
		EXPECT_EQ(image.size, ImageSize);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		bool hasVisiblePixel = false;
		for (std::uint32_t y = 0; y < ImageSize.y && !hasVisiblePixel; ++y)
		{
			for (std::uint32_t x = 0; x < ImageSize.x && !hasVisiblePixel; ++x)
			{
				const auto pixel = image.pixel({x, y});
				hasVisiblePixel = pixel[0] > 20 || pixel[1] > 20 || pixel[2] > 20;
			}
		}
		EXPECT_TRUE(hasVisiblePixel) << "Voxel renderer produced an entirely black image: " << name;
		const auto diff = sparkle_test::resultImagePath(category, name + "_difference");
		const auto comparison = sparkle_test::compareImages(actual, expected, diff);
		EXPECT_TRUE(comparison.matches) << "Voxel golden mismatch: " << name << ", pixels: " << comparison.differentPixelCount;
	}

	void drawScene(Scene &scene)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		ASSERT_GT(scene.bucketCount(), 0u);
		spk::EngineWidget widget("voxel", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		capturedWidget(widget).execute(gl.renderContext());
	}

	void renderScene(Scene &scene, const std::string &name)
	{
		drawScene(scene);
		saveAndCheck(name);
	}
}

TEST(VoxelRendererTest, RealVolumeBakesOpaqueGeometryAndSharesItsMesh)
{
	const auto geometry = cubeGeometry();
	ASSERT_GT(geometry.opaqueMesh().indexCount(), 0u);
	EXPECT_EQ(geometry.opaqueMesh().indexCount(), 36u);
	const auto copied = geometry;
	EXPECT_EQ(
		copied.opaqueMesh().layout().vertexBuffer().identifier(),
		geometry.opaqueMesh().layout().vertexBuffer().identifier());
}

TEST(VoxelRendererTest, MissingMaterialNamesAreReportedWithoutPublishingInvalidBinding)
{
	TestAtlas atlas;
	const auto palette = makePalette(atlas, spk::Texture::Section::whole);
	spk::Voxel::Renderer renderer;
	renderer.bind(palette);
	const spk::Voxel::Geometry invalid({}, {}, {}, {{"missingOne", 1.0f}, {"missingTwo", 1.0f}});
	try
	{
		renderer.bind(invalid);
		FAIL() << "Expected a descriptive missing-slot exception";
	} catch (const spk::Exception &error)
	{
		const std::string message = error.what();
		EXPECT_NE(message.find("missingOne"), std::string::npos);
		EXPECT_NE(message.find("missingTwo"), std::string::npos);
	}
	EXPECT_EQ(renderer.geometry(), nullptr);
	EXPECT_NE(renderer.palette(), nullptr);
	EXPECT_TRUE(renderer.materialRemap().empty());
}

TEST(VoxelRendererTest, BucketsCombineMatchingPaletteAndSeparateDifferentMaterials)
{
	TestAtlas atlas;
	const auto geometry = cubeGeometry();
	const auto first = makePalette(atlas, spk::Texture::Section::whole);
	const auto second = makePalette(atlas, {{0.5f, 0.0f}, {0.5f, 0.5f}});
	Scene scene;
	scene.add(geometry, first, {-1, 0, 0});
	scene.add(geometry, first, {1, 0, 0});
	EXPECT_EQ(scene.bucketCount(), 1u);
	scene.add(geometry, second, {0, 0, -2});
	EXPECT_EQ(scene.bucketCount(), 2u);
}

TEST(VoxelRendererTest, DistinctMeshesResolveLocalZeroToDifferentSharedPaletteIndices)
{
	TestAtlas atlas;
	const auto palette = sharedPalette(atlas);
	const auto stone = namedGeometry("cube", "stone");
	const auto wood = namedGeometry("slab", "wood");
	ASSERT_EQ(stone.materialIndices().at("stone"), 0u);
	ASSERT_EQ(wood.materialIndices().at("wood"), 0u);
	spk::Voxel::Renderer renderer;
	renderer.bind(stone);
	renderer.bind(palette);
	ASSERT_EQ(renderer.materialRemap().size(), 1u);
	EXPECT_EQ(renderer.materialRemap()[0], 0u);
	renderer.bind(wood);
	ASSERT_EQ(renderer.materialRemap().size(), 1u);
	EXPECT_EQ(renderer.materialRemap()[0], 1u);
	EXPECT_THROW(renderer.bind(namedGeometry("cube", "missing")), spk::Exception);
	ASSERT_NE(renderer.geometry(), nullptr);
	EXPECT_EQ(renderer.geometry()->materialIndices().at("wood"), 0u);
	EXPECT_EQ(renderer.materialRemap()[0], 1u);
}

TEST(VoxelRendererTest, DifferentMeshesSharingOnePaletteRenderDistinctMaterials)
{
	TestAtlas atlas;
	const auto palette = sharedPalette(atlas);
	Scene scene;
	auto &first = scene.add(namedGeometry("cube", "stone"), palette, {-1.0f, 0, 0});
	auto &second = scene.add(namedGeometry("slab", "wood"), palette, {1.0f, 0, 0});
	ASSERT_EQ(scene.bucketCount(), 2u);
	EXPECT_EQ(first.materialRemap().at(0), 0u);
	EXPECT_EQ(second.materialRemap().at(0), 1u);
	drawScene(scene);
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	const auto image = gl.capture();
	std::size_t yellowPixels = 0;
	std::size_t cyanPixels = 0;
	for (std::uint32_t y = 0; y < ImageSize.y; ++y)
	{
		for (std::uint32_t x = 0; x < ImageSize.x; ++x)
		{
			const auto pixel = image.pixel({x, y});
			yellowPixels += pixel[0] > pixel[2] + 35 && pixel[1] > pixel[2] + 35;
			cyanPixels += pixel[2] > pixel[0] + 35 && pixel[1] > pixel[0] + 35;
		}
	}
	EXPECT_GT(yellowPixels, 30u);
	EXPECT_GT(cyanPixels, 30u);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.save(sparkle_test::resultImagePath("rendering/voxel", "shared_palette_distinct_meshes"));
}

TEST(VoxelRendererTest, InvalidScaleIsRejectedBeforeSnapshotPublication)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	auto &renderer = scene.add(
		geometry, makePalette(atlas, spk::Texture::Section::whole), {}, {});
	auto &transform = renderer.owner()->transform();
	const spk::Vector3 rejectedScales[] = {{0.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, 2.0f, 1.0f}};
	for (const spk::Vector3 &scale : rejectedScales)
	{
		transform.rescale(scale);
		EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	}
	transform.rescale({2.0f, 2.0f, 2.0f});
	EXPECT_NO_THROW(scene.engine.refreshRenderSnapshot());
}

TEST(VoxelRendererTest, InstancedCommandUsesStd430AndRebindsSsboAfterThirdParty)
{
	static_assert(sizeof(spk::DrawVoxelBucketCommand::InstanceData) == 64);
	static_assert(sizeof(spk::DrawVoxelBucketCommand::MaterialData) == 16);
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	TestAtlas atlas;
	const auto palette = makePalette(atlas, spk::Texture::Section::whole);
	spk::DrawVoxelBucketCommand command(
		cubeGeometry(),
		palette,
		{0u},
		{spk::Matrix4x4::identity(), spk::Matrix4x4::translation(2, 0, 0)},
		spk::Matrix4x4::identity());
	EXPECT_EQ(command.instanceCount(), 2u);
	command.execute(gl.renderContext());
	GLint first = 0;
	GLint material = 0;
	GLint remap = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &first);
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 1, &material);
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 2, &remap);
	EXPECT_GT(first, 0);
	EXPECT_GT(material, 0);
	EXPECT_GT(remap, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
	command.execute(gl.renderContext());
	GLint rebound = 0;
	GLint reboundMaterial = 0;
	GLint reboundRemap = 0;
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 0, &rebound);
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 1, &reboundMaterial);
	glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, 2, &reboundRemap);
	EXPECT_EQ(rebound, first);
	EXPECT_EQ(reboundMaterial, material);
	EXPECT_EQ(reboundRemap, remap);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(VoxelRendererGoldenTest, CubeTextured)
{
	TestAtlas atlas;
	Scene scene;
	scene.add(cubeGeometry(), makePalette(atlas, spk::Texture::Section::whole), {});
	renderScene(scene, "cube_textured");
}

TEST(VoxelRendererGoldenTest, CubeWithRotation)
{
	TestAtlas atlas;
	Scene scene;
	scene.add(cubeGeometry(), makePalette(atlas, spk::Texture::Section::whole), {}, {0, 45, 22});
	renderScene(scene, "cube_rotated");
}

TEST(VoxelRendererGoldenTest, TwoInstancesShareGeometry)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	const auto palette = makePalette(atlas, spk::Texture::Section::whole);
	scene.add(geometry, palette, {-1, 0, 0}, {0, 20, 0});
	scene.add(geometry, palette, {1, 0, 0}, {0, -20, 0});
	EXPECT_EQ(scene.bucketCount(), 1u);
	renderScene(scene, "shared_mesh_instanced");
}

TEST(VoxelRendererGoldenTest, DifferentPalettesStayIndependent)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	scene.add(geometry, makePalette(atlas, spk::Texture::Section::whole), {-1, 0, 0});
	scene.add(geometry, makePalette(atlas, {{0.5f, 0}, {0.5f, 0.5f}}), {1, 0, 0});
	EXPECT_EQ(scene.bucketCount(), 2u);
	renderScene(scene, "two_palettes");
}

TEST(VoxelRendererGoldenTest, VoxelRoomThroughEngineWidget)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	const auto stone = makePalette(atlas, spk::Texture::Section::whole);
	const auto decorated = makePalette(atlas, {{0.5f, 0}, {0.5f, 0.5f}});
	for (int x = -2; x <= 2; ++x)
	{
		for (int z = -2; z <= 2; ++z)
		{
			scene.add(geometry, stone, {static_cast<float>(x), -1.5f, static_cast<float>(z)});
		}
	}
	for (int x = -2; x <= 2; ++x)
	{
		scene.add(geometry, stone, {static_cast<float>(x), -0.5f, -2});
	}
	scene.add(geometry, decorated, {-0.5f, 0, 0}, {0, 35, 0});
	scene.add(geometry, decorated, {1, 0, 1}, {15, -25, 0});
	renderScene(scene, "voxel_room_widget");
}
