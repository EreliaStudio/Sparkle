#include <GL/glew.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "core/platform/window.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/image.hpp"
#include "light/directional.hpp"
#include "light/point.hpp"
#include "light/spot.hpp"
#include "rendering/command/camera_uniform_render_command.hpp"
#include "rendering/command/draw_engine_view_command.hpp"
#include "rendering/command/draw_voxel_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "voxel/mesher.hpp"
#include "voxel/palette.hpp"
#include "voxel/renderer.hpp"
#include "voxel/rendering_system.hpp"

namespace
{
	constexpr spk::Vector2UInt ImageSize{320, 240};
	constexpr const char *Category = "rendering/forward-lighting";

	class Atlas final : public spk::Texture
	{
	public:
		Atlas()
		{
			std::array<std::uint8_t, 16 * 4 * 4> pixels{};
			constexpr std::array<std::uint8_t, 4> red{235, 103, 221, 170};
			constexpr std::array<std::uint8_t, 4> green{110, 200, 201, 174};
			constexpr std::array<std::uint8_t, 4> blue{93, 177, 153, 188};
			for (std::size_t y = 0; y < 4; ++y)
			{
				for (std::size_t x = 0; x < 16; ++x)
				{
					const std::size_t offset = (y * 16 + x) * 4;
					pixels[offset] = red[x / 4];
					pixels[offset + 1] = green[x / 4];
					pixels[offset + 2] = blue[x / 4];
					pixels[offset + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {16, 4}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Geometry geometry(spk::Vector3UInt dimensions)
	{
		spk::Voxel::Shape::Builder shapeBuilder;
		shapeBuilder.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
		const auto shape = shapeBuilder.build();
		spk::Voxel::Definition::SlotMapping slots;
		for (const auto &slot : shape.slots())
		{
			slots.emplace(slot.name, spk::Voxel::Definition::Slot{"paint", 1.0f});
		}
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, spk::Voxel::Definition(shape, std::move(slots)));
		spk::Voxel::Volume::Builder volume(dimensions);
		volume.bind(registry.build());
		for (std::uint32_t z = 0; z < dimensions.z; ++z)
		{
			for (std::uint32_t y = 0; y < dimensions.y; ++y)
			{
				for (std::uint32_t x = 0; x < dimensions.x; ++x)
				{
					volume.set({x, y, z}, 1);
				}
			}
		}
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Palette palette(const Atlas &atlas, std::uint32_t tile)
	{
		spk::Material::Builder material;
		material.add("albedo", atlas, {{static_cast<float>(tile) * 0.25f, 0.0f}, {0.25f, 1.0f}});
		spk::Voxel::Palette::Builder result;
		result.setMaterial("paint", material.build());
		return result.build();
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"Forward camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &voxelSystem;
		Atlas atlas;
		spk::Voxel::Geometry cube = geometry({1, 1, 1});
		spk::Voxel::Geometry floor = geometry({7, 1, 5});
		std::vector<std::unique_ptr<spk::Entity3D>> objects;
		std::vector<std::unique_ptr<spk::Entity3D>> lights;

		Scene() :
			view(_makeView()),
			voxelSystem(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
			add(floor, 3, {-3.5f, -1.2f, -2.5f});
			add(cube, 0, {-2.0f, -0.2f, -0.5f});
			add(cube, 1, {-0.5f, -0.2f, -0.5f});
			add(cube, 2, {1.0f, -0.2f, -0.5f});
		}

		spk::EngineView &_makeView()
		{
			const spk::Vector3 eye{6.0f, 4.2f, 8.0f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
			camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 40.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		void add(const spk::Voxel::Geometry &mesh, std::uint32_t tile, spk::Vector3 position)
		{
			auto entity = std::make_unique<spk::Entity3D>("Forward voxel");
			entity->transform().place(position);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(mesh);
			renderer.bind(palette(atlas, tile));
			engine.addEntity(entity.get());
			objects.push_back(std::move(entity));
		}

		template <typename TLight>
		TLight &addLight(spk::Vector3 position, spk::Vector3 target, spk::RGB color, float intensity, float range = 8.0f)
		{
			auto entity = std::make_unique<spk::Entity3D>("Forward scene light");
			entity->transform().place(position);
			entity->transform().setRotation(spk::Quaternion::lookAt(position, target));
			auto &light = entity->addComponent<TLight>();
			light.setColor(color);
			light.setIntensity(intensity);
			if constexpr (std::is_same_v<TLight, spk::Light::Point> || std::is_same_v<TLight, spk::Light::Spot>)
			{
				light.setRange(range);
			}
			engine.addEntity(entity.get());
			lights.push_back(std::move(entity));
			return light;
		}

		[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> capture()
		{
			engine.refreshRenderSnapshot();
			return engine.latestRenderSnapshot();
		}

		~Scene()
		{
			for (const auto &entity : objects)
			{
				engine.removeEntity(entity.get());
			}
			for (const auto &entity : lights)
			{
				engine.removeEntity(entity.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	void expectGolden(
		const spk::RenderSnapshot &snapshot,
		const spk::EngineView &view,
		const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		snapshot.execute(gl.renderContext());
		spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = ImageSize}).execute(gl.renderContext());
		spk::DrawEngineViewCommand(view.output(), {.anchor = {0, 0}, .size = ImageSize})
			.execute(gl.renderContext());
		const auto actual = sparkle_test::resultImagePath(Category, name);
		const auto expected = sparkle_test::expectedImagePath(Category, name);
		gl.save(actual);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		const auto image = gl.capture();
		EXPECT_EQ(image.size, ImageSize);
		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Forward lighting golden reference missing; inspect " << actual
			<< " and commit an explicitly approved expected image";
		const auto diff = sparkle_test::resultImagePath(Category, name + "_difference");
		const auto comparison = sparkle_test::compareImages(actual, expected, diff);
		EXPECT_TRUE(comparison.matches) << name << ": " << comparison.differentPixelCount << " different pixels";
		gl.reset();
	}

	void expectGolden(Scene &scene, const std::string &name)
	{
		const auto snapshot = scene.capture();
		ASSERT_NE(snapshot, nullptr);
		expectGolden(*snapshot, scene.view, name);
	}
}

TEST(VoxelForwardLightingGolden, AmbientOnly)
{
	Scene scene;
	scene.view.ambientLight().setColor({0.65f, 0.85f, 1.0f});
	scene.view.ambientLight().setIntensity(1.0f);
	expectGolden(scene, "ambient_only");
}

TEST(VoxelForwardLightingGolden, PointOnly)
{
	Scene scene;
	scene.addLight<spk::Light::Point>({-2, 3, 1}, {0, 0, 0}, {1, 0.55f, 0.25f}, 3.0f);
	expectGolden(scene, "point_only");
}

TEST(VoxelForwardLightingGolden, SpotOnly)
{
	Scene scene;
	auto &spot = scene.addLight<spk::Light::Spot>({0, 4, 1}, {-0.2f, -0.5f, -0.5f}, {0.4f, 0.8f, 1}, 5.0f);
	spot.setAngle(0.45f);
	expectGolden(scene, "spot_only");
}

TEST(VoxelForwardLightingGolden, DirectionalOnly)
{
	Scene scene;
	scene.addLight<spk::Light::Directional>({-5, 8, 5}, {0, 0, 0}, {1, 0.95f, 0.8f}, 1.7f);
	expectGolden(scene, "directional_only");
}

TEST(VoxelForwardLightingGolden, AmbientPointSpotDirectionalTogether)
{
	Scene scene;
	scene.view.ambientLight().setIntensity(0.2f);
	scene.addLight<spk::Light::Directional>({-5, 8, 5}, {0, 0, 0}, {1, 0.95f, 0.8f}, 0.6f);
	scene.addLight<spk::Light::Point>({-3, 2, 1}, {0, 0, 0}, {1, 0.1f, 0.1f}, 3.0f);
	auto &spot = scene.addLight<spk::Light::Spot>({3, 3, -1}, {1, -0.5f, -0.5f}, {0.1f, 0.3f, 1}, 4.0f);
	spot.setAngle(0.65f);
	expectGolden(scene, "combined_forward");
}

TEST(VoxelForwardLightingGolden, PointLightHasHardRange)
{
	Scene scene;
	scene.addLight<spk::Light::Point>({-2, 1.3f, 0}, {0, 0, 0}, {1, 1, 1}, 7.0f, 2.5f);
	expectGolden(scene, "point_limited_range");
}

TEST(VoxelForwardLightingGolden, SpotHasVisibleCone)
{
	Scene scene;
	auto &spot = scene.addLight<spk::Light::Spot>({0, 4, 0}, {0, -1, 0}, {1, 1, 1}, 6.0f);
	spot.setAngle(0.32f);
	expectGolden(scene, "spot_visible_cone");
}

TEST(VoxelForwardLightingGolden, TwoViewsHaveIndependentAmbientOnSameObjects)
{
	Scene scene;
	auto &other = scene.engine.addView("warm", &scene.camera, ImageSize);
	scene.view.ambientLight().setColor({0.25f, 0.5f, 1.0f});
	scene.view.ambientLight().setIntensity(1.0f);
	other.ambientLight().setColor({1.0f, 0.4f, 0.2f});
	other.ambientLight().setIntensity(1.0f);
	const auto snapshot = scene.capture();
	ASSERT_NE(snapshot, nullptr);
	expectGolden(*snapshot, scene.view, "ambient_two_views_cool");
	expectGolden(*snapshot, other, "ambient_two_views_warm");
}

TEST(VoxelForwardLightingGolden, LocalTopNChangesTheVisibleResult)
{
	Scene scene;
	scene.view.lightChannel().setLimit(1);
	auto &red = scene.addLight<spk::Light::Point>({-2, 2, 1}, {0, 0, 0}, {1, 0.05f, 0.05f}, 4.0f);
	auto &blue = scene.addLight<spk::Light::Point>({2, 2, 1}, {0, 0, 0}, {0.05f, 0.05f, 1}, 1.0f);
	const auto first = scene.capture();
	ASSERT_NE(first, nullptr);
	auto selected = scene.view.lightChannel().current();
	ASSERT_NE(selected, nullptr);
	ASSERT_EQ(selected->local.size(), 1u);
	EXPECT_EQ(selected->local[0].registrationOrder, red.registrationOrder());
	blue.setIntensity(12.0f);
	const auto second = scene.capture();
	ASSERT_NE(second, nullptr);
	selected = scene.view.lightChannel().current();
	ASSERT_NE(selected, nullptr);
	ASSERT_EQ(selected->local.size(), 1u);
	EXPECT_EQ(selected->local[0].registrationOrder, blue.registrationOrder());
	expectGolden(*first, scene.view, "top_n_red");
	expectGolden(*second, scene.view, "top_n_blue");
}

TEST(VoxelForwardLightingGolden, PreviousCaptureRetainsLightsAndAmbientAfterMutation)
{
	Scene scene;
	scene.view.ambientLight().setIntensity(0.25f);
	auto &sun = scene.addLight<spk::Light::Directional>({-4, 5, 3}, {0, 0, 0}, {1, 1, 1}, 1.0f);
	const auto old = scene.capture();
	ASSERT_NE(old, nullptr);
	scene.view.ambientLight().setColor({1.0f, 0.15f, 0.15f});
	scene.view.ambientLight().setIntensity(0.7f);
	sun.setColor({0.2f, 0.45f, 1.0f});
	sun.setIntensity(2.5f);
	const auto current = scene.capture();
	ASSERT_NE(current, nullptr);
	expectGolden(*old, scene.view, "capture_before_mutation");
	expectGolden(*current, scene.view, "capture_after_mutation");
}

TEST(VoxelForwardLightingIntegration, RenderTargetPreservesHDRAboveOneBeforeToneMapping)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;

	const auto hdr = spk::Framebuffer::RenderAttachment::create2D(ImageSize, spk::Texture::Format::RGBA16F);
	const auto depth = spk::Framebuffer::RenderAttachment::create2D(ImageSize, spk::Texture::Format::Depth24Stencil8);
	spk::Framebuffer target(ImageSize, std::vector<spk::Texture::Handle>{hdr.texture()}, depth.texture());
	const spk::Window::Surface::RenderTarget binding(gl.surface(), target.handle());
	binding.activate(gl.renderContext());
	gl.renderContext().activeRenderTarget = binding;
	glViewport(0, 0, static_cast<GLsizei>(ImageSize.x), static_cast<GLsizei>(ImageSize.y));
	const GLfloat black[]{0.0f, 0.0f, 0.0f, 1.0f};
	glClearBufferfv(GL_COLOR, 0, black);
	glClear(GL_DEPTH_BUFFER_BIT);

	const spk::Vector3 eye{6.0f, 4.2f, 8.0f};
	spk::Camera3D camera;
	camera.setPosition(eye);
	camera.setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
	camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 40.0f);
	const auto cameraBinding = std::make_shared<const spk::CameraUniformRenderCommand>(camera);
	const auto illumination = std::make_shared<const spk::Lighting::System>(
		spk::Light::Selection{}, spk::RGB{8.0f, 8.0f, 8.0f});
	spk::DrawVoxelCommand draw(
		scene.cube,
		palette(scene.atlas, 0),
		{0u},
		spk::Matrix4x4::translation(-0.5f, -0.5f, -0.5f),
		illumination,
		cameraBinding);
	draw.execute(gl.renderContext());

	hdr.texture().activate(gl.renderContext());
	std::vector<float> pixels(static_cast<std::size_t>(ImageSize.x) * ImageSize.y * 4);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());
	float brightest = 0.0f;
	for (std::size_t i = 0; i < pixels.size(); i += 4)
	{
		brightest = std::max(brightest, std::max(pixels[i], std::max(pixels[i + 1], pixels[i + 2])));
	}
	EXPECT_GT(brightest, 1.0f) << "Opaque lighting was clamped before the final Reinhard pass";
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}
