#include <gtest/gtest.h>

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
#include "graphics/opengl/sampler.hpp"
#include "graphics/opengl/shader_storage_buffer.hpp"
#include "light/directional.hpp"
#include "lighting/system.hpp"
#include "math/camera_3d.hpp"
#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/command/draw_voxel_command.hpp"
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

	class EdgeAtlas final : public spk::Texture
	{
	public:
		EdgeAtlas()
		{
			std::array<std::uint8_t, 8 * 8 * 4> pixels{};
			for (std::size_t y = 0; y < 8; ++y)
			{
				for (std::size_t x = 0; x < 8; ++x)
				{
					const std::size_t index = (y * 8 + x) * 4;
					pixels[index] = x == 0 ? 255 : 0;
					pixels[index + 1] = x == 0 ? 0 : 255;
					pixels[index + 2] = x == 0 ? 0 : 255;
					pixels[index + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {8, 8}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Geometry namedGeometry(const std::string &shapeName, const std::string &materialName)
	{
		spk::Voxel::Shape::Builder shapes;
		shapes.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (shapeName + ".json")));
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
		spk::Entity3D sunEntity{"PR13 golden sunlight"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &system;
		std::vector<std::unique_ptr<spk::Entity3D>> entities;

		Scene() :
			view(_createView()),
			system(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
			sunEntity.transform().setRotation(spk::Quaternion::lookAt({0, 0, 0}, {-0.35f, -0.85f, -0.45f}));
			auto &sun = sunEntity.addComponent<spk::Light::Directional>();
			sun.setIntensity(0.62f);
			engine.addEntity(&sunEntity);
			view.ambientLight().setIntensity(0.38f);
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
			engine.removeEntity(&sunEntity);
			engine.removeEntity(&cameraEntity);
		}

		[[nodiscard]] std::size_t commandCount()
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
		EXPECT_NO_THROW(gl.requireNoErrors());
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
		ASSERT_GT(scene.commandCount(), 0u);
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

TEST(VoxelRendererTest, MatchingSnapshotsBatchAndDifferentPalettesSeparate)
{
	TestAtlas atlas;
	const auto geometry = cubeGeometry();
	const auto first = makePalette(atlas, spk::Texture::Section::whole);
	const auto second = makePalette(atlas, {{0.5f, 0.0f}, {0.5f, 0.5f}});
	Scene scene;
	scene.add(geometry, first, {-1, 0, 0});
	scene.add(geometry, first, {1, 0, 0});
	EXPECT_EQ(scene.commandCount(), 1u);
	scene.add(geometry, second, {0, 0, -2});
	EXPECT_EQ(scene.commandCount(), 2u);
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

TEST(VoxelRendererTest, SharedMeshWithDifferentSlotMappingsDoesNotBatch)
{
	TestAtlas atlas;
	const auto appearance = sharedPalette(atlas);
	const auto stone = cubeGeometry();
	const spk::Voxel::Geometry wood(stone.opaqueMesh(), {}, {}, {{"wood", 1.0f}});
	Scene scene;
	scene.add(stone, appearance, {-1, 0, 0});
	scene.add(wood, appearance, {1, 0, 0});
	EXPECT_EQ(scene.commandCount(), 2u);
}

TEST(VoxelRendererTest, NearestAtlasSamplingStaysInsideOneTexelSections)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	EdgeAtlas atlas;
	const auto geometry = cubeGeometry();
	const auto lighting = std::make_shared<const spk::Lighting::System>(
		spk::Light::Selection{}, spk::RGB{1, 1, 1});

	spk::Camera3D camera;
	const spk::Vector3 eye{2, 2, 4};
	camera.setPosition(eye);
	camera.setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
	camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
	const auto cameraBinding = std::make_shared<const spk::CameraUniformRenderCommand>(camera);

	const auto drawSection = [&](spk::Texture::Section section) {
		gl.reset(ImageSize);
		spk::DrawVoxelCommand command(
			geometry,
			makePalette(atlas, section),
			{0u},
			spk::Matrix4x4::translation(-0.5f, -0.5f, -0.5f),
			lighting,
			cameraBinding);
		command.execute(gl.renderContext());
		const auto image = gl.capture();
		gl.requireNoErrors();
		return image;
	};

	const auto redTile = drawSection({{0, 0}, {0.125f, 1}});
	const auto cyanTile = drawSection({{0.125f, 0}, {0.125f, 1}});
	std::size_t redPixels = 0;
	std::size_t cyanPixels = 0;
	std::size_t wrongRedPixels = 0;
	std::size_t wrongCyanPixels = 0;
	for (std::uint32_t y = 0; y < ImageSize.y; ++y)
	{
		for (std::uint32_t x = 0; x < ImageSize.x; ++x)
		{
			const auto *red = redTile.pixel({x, y});
			const auto *cyan = cyanTile.pixel({x, y});
			redPixels += red[0] > 100 && red[1] < 10 && red[2] < 10;
			cyanPixels += cyan[0] < 10 && cyan[1] > 100 && cyan[2] > 100;
			wrongRedPixels += red[1] > 30 || red[2] > 30;
			wrongCyanPixels += cyan[0] > 30;
		}
	}
	EXPECT_GT(redPixels, 100u);
	EXPECT_GT(cyanPixels, 100u);
	EXPECT_EQ(wrongRedPixels, 0u);
	EXPECT_EQ(wrongCyanPixels, 0u);
	gl.reset();
}

TEST(VoxelRendererTest, DrawCommandRejectsMissingLighting)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	TestAtlas atlas;
	const auto appearance = makePalette(atlas, spk::Texture::Section::whole);
	const std::shared_ptr<const spk::Lighting::System> noLighting;
	EXPECT_THROW(
		spk::DrawVoxelCommand(cubeGeometry(), appearance, {0u}, spk::Matrix4x4::identity(), noLighting),
		spk::Exception);
	EXPECT_THROW(
		spk::DrawVoxelCommand(
			cubeGeometry(), appearance, {0u}, std::vector<spk::Matrix4x4>{spk::Matrix4x4::identity()}, noLighting),
		spk::Exception);
	gl.reset();
}

TEST(VoxelRendererTest, InstancedDrawRendersBothModelPositions)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	TestAtlas atlas;
	const std::vector<spk::Matrix4x4> models{
		spk::Matrix4x4::translation(-1.5f, -0.5f, -0.5f),
		spk::Matrix4x4::translation(0.5f, -0.5f, -0.5f)};
	const auto lighting = std::make_shared<const spk::Lighting::System>(
		spk::Light::Selection{}, spk::RGB{1, 1, 1});

	spk::Camera3D camera;
	const spk::Vector3 eye{0, 1.5f, 7};
	camera.setPosition(eye);
	camera.setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
	camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
	auto cameraBinding = std::make_shared<const spk::CameraUniformRenderCommand>(camera);
	spk::DrawVoxelCommand command(
		cubeGeometry(), makePalette(atlas, spk::Texture::Section::whole), {0u}, models, lighting, cameraBinding);
	command.execute(gl.renderContext());

	const auto image = gl.capture();
	std::size_t leftPixels = 0;
	std::size_t rightPixels = 0;
	for (std::uint32_t y = 0; y < ImageSize.y; ++y)
	{
		for (std::uint32_t x = 0; x < ImageSize.x; ++x)
		{
			const auto *pixel = image.pixel({x, y});
			if (pixel[0] > 20 || pixel[1] > 20 || pixel[2] > 20)
			{
				(x < ImageSize.x / 2 ? leftPixels : rightPixels)++;
			}
		}
	}
	EXPECT_GT(leftPixels, 80u);
	EXPECT_GT(rightPixels, 80u);
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}

TEST(VoxelRendererTest, DifferentMeshesSharingOnePaletteRenderDistinctMaterials)
{
	TestAtlas atlas;
	const auto palette = sharedPalette(atlas);
	Scene scene;
	auto &first = scene.add(namedGeometry("cube", "stone"), palette, {-1.0f, 0, 0});
	auto &second = scene.add(namedGeometry("slab", "wood"), palette, {1.0f, 0, 0});
	ASSERT_EQ(scene.commandCount(), 2u);
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
	EXPECT_NO_THROW(gl.requireNoErrors());
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

TEST(VoxelRendererTest, SingleDrawCommandRestoresItsOwnResourcesAfterAnotherPipeline)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);

	TestAtlas atlas;
	const auto palette = sharedPalette(atlas);
	const auto lighting = std::make_shared<const spk::Lighting::System>(
		spk::Light::Selection{}, spk::RGB{1, 1, 1});
	spk::Camera3D camera;
	const spk::Vector3 eye{2, 2, 4};
	camera.setPosition(eye);
	camera.setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
	camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
	const auto cameraBinding = std::make_shared<const spk::CameraUniformRenderCommand>(camera);
	spk::DrawVoxelCommand command(
		cubeGeometry(),
		palette,
		{0u},
		spk::Matrix4x4::translation(-0.5f, -0.5f, -0.5f),
		lighting,
		cameraBinding);
	command.execute(gl.renderContext());
	const auto reference = gl.capture();
	const auto isVisible = [](const sparkle_test::FramebufferImage &image) {
		for (std::size_t i = 0; i < image.pixels.size(); i += 4)
		{
			if (image.pixels[i] > 20 || image.pixels[i + 1] > 20 || image.pixels[i + 2] > 20)
			{
				return true;
			}
		}
		return false;
	};
	ASSERT_TRUE(isVisible(reference));

	gl.reset(ImageSize);
	const std::array<spk::Matrix4x4, 1> shiftedModels{spk::Matrix4x4::translation(50, 0, 0)};
	spk::ShaderStorageBuffer otherInstances(0, 0, sizeof(spk::Matrix4x4));
	otherInstances.resize(1);
	otherInstances.setDynamicData(std::span<const spk::Matrix4x4>(shiftedModels));
	otherInstances.validate();
	otherInstances.activate(gl.renderContext());

	const std::array<std::array<float, 4>, 2> otherUVs{{{0, 0, 0, 0}, {0, 0, 0, 0}}};
	spk::ShaderStorageBuffer otherMaterials(1, 0, sizeof(otherUVs[0]));
	otherMaterials.resize(2);
	otherMaterials.setDynamicData(std::span<const std::array<float, 4>>(otherUVs));
	otherMaterials.validate();
	otherMaterials.activate(gl.renderContext());

	const std::array<std::uint32_t, 1> otherIndices{1};
	spk::ShaderStorageBuffer otherRemap(2, 0, sizeof(std::uint32_t));
	otherRemap.resize(1);
	otherRemap.setDynamicData(std::span<const std::uint32_t>(otherIndices));
	otherRemap.validate();
	otherRemap.activate(gl.renderContext());

	const spk::Lighting::System otherLighting(spk::Light::Selection{}, spk::RGB{0, 0, 0});
	otherLighting.activate(gl.renderContext());
	spk::CameraUniformRenderCommand otherCamera(spk::Camera3D{});
	otherCamera.execute(gl.renderContext());
	spk::Sampler otherSampler(0, spk::Sampler::Filtering::Linear);
	otherSampler.setTexture(atlas.handle());
	otherSampler.validate();
	otherSampler.activate(gl.renderContext());

	command.execute(gl.renderContext());
	const auto rebound = gl.capture();
	EXPECT_EQ(rebound.size, reference.size);
	EXPECT_EQ(rebound.pixels, reference.pixels);
	EXPECT_NO_THROW(gl.requireNoErrors());
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

TEST(VoxelRendererGoldenTest, SharedGeometryRendersWithInstancing)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	const auto palette = makePalette(atlas, spk::Texture::Section::whole);
	scene.add(geometry, palette, {-1, 0, 0}, {0, 20, 0});
	scene.add(geometry, palette, {1, 0, 0}, {0, -20, 0});
	EXPECT_EQ(scene.commandCount(), 1u);
	renderScene(scene, "shared_mesh_instanced");
}

TEST(VoxelRendererGoldenTest, DifferentPalettesStayIndependent)
{
	TestAtlas atlas;
	Scene scene;
	const auto geometry = cubeGeometry();
	scene.add(geometry, makePalette(atlas, spk::Texture::Section::whole), {-1, 0, 0});
	scene.add(geometry, makePalette(atlas, {{0.5f, 0}, {0.5f, 0.5f}}), {1, 0, 0});
	EXPECT_EQ(scene.commandCount(), 2u);
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
