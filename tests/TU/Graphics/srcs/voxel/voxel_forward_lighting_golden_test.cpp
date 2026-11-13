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
#include "graphics/image.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/state.hpp"
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
	constexpr float SoftFillAmbientIntensity = 0.20f;

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

	[[nodiscard]] spk::Voxel::Geometry geometry(spk::Vector3UInt dimensions, const std::string &shapeName = "cube")
	{
		spk::Voxel::Shape::Builder shapeBuilder;
		shapeBuilder.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (shapeName + ".json")));
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
		material.add("albedo", atlas, spk::Texture::Section::fromPixels(atlas.size(), {tile * 4u, 0u}, {4, 4}));
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
		spk::Voxel::Geometry stair = geometry({1, 1, 1}, "stair");
		spk::Voxel::Geometry slope = geometry({1, 1, 1}, "slope");
		spk::Voxel::Geometry floor = geometry({7, 1, 5});
		std::vector<std::unique_ptr<spk::Entity3D>> objects;
		std::vector<std::unique_ptr<spk::Entity3D>> lights;

		explicit Scene(bool includeProps = true) :
			view(_makeView()),
			voxelSystem(engine.addSystem<spk::Voxel::RenderingSystem>())
		{
			add(floor, 3, {-3.5f, -1.2f, -2.5f});
			if (includeProps)
			{
				add(cube, 0, {-2.0f, -0.2f, -0.5f});
				add(stair, 1, {-0.5f, -0.2f, -0.5f});
				add(slope, 2, {1.0f, -0.2f, -0.5f});
			}
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

		spk::Entity3D &add(
			const spk::Voxel::Geometry &mesh,
			std::uint32_t tile,
			spk::Vector3 position,
			spk::Entity3D *parent = nullptr)
		{
			auto entity = std::make_unique<spk::Entity3D>("Forward voxel");
			entity->transform().place(position);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(mesh);
			renderer.bind(palette(atlas, tile));
			if (parent != nullptr)
			{
				entity->setParent(*parent);
			}
			else
			{
				engine.addEntity(entity.get());
			}
			auto &result = *entity;
			objects.push_back(std::move(entity));
			return result;
		}

		spk::Entity3D &joint(spk::Entity3D &parent, spk::Vector3 offset)
		{
			auto child = std::make_unique<spk::Entity3D>("Articulated voxel joint", &parent);
			child->transform().place(offset);
			auto &result = *child;
			objects.push_back(std::move(child));
			return result;
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
		EXPECT_NO_THROW(gl.requireNoErrors());
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

TEST(VoxelForwardLightingGolden, ObliqueSpotCastsRoundedOvalAcrossDifferentShapes)
{
	Scene scene;
	// Shine diagonally across the floor, rather than straight down: the
	// projected cone becomes an elongated, rounded oval around all three shapes.
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	auto &spot = scene.addLight<spk::Light::Spot>(
		{3.8f, 3.4f, 1.9f},
		{0.0f, -0.2f, -0.2f},
		{1.0f, 0.9f, 0.72f},
		9.0f,
		11.5f);
	spot.setAngle(0.29f);
	expectGolden(scene, "spot_oblique_oval_cube_stair_slope");
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
	spk::OpenGL::State::Viewport{{0, 0, static_cast<int>(ImageSize.x), static_cast<int>(ImageSize.y)}}.apply(gl.renderContext());
	spk::OpenGL::State::Clear::color(gl.renderContext(), 0, {0, 0, 0, 1});
	spk::OpenGL::State::Clear::depth(gl.renderContext());

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

	const auto pixels = gl.captureFloatTexture(hdr.texture(), ImageSize);
	float brightest = 0.0f;
	for (std::size_t i = 0; i < pixels.size(); i += 4)
	{
		brightest = std::max(brightest, std::max(pixels[i], std::max(pixels[i + 1], pixels[i + 2])));
	}
	EXPECT_GT(brightest, 1.0f) << "Opaque lighting was clamped before the final Reinhard pass";
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}

namespace
{
	struct ArticulatedCharacter
	{
		spk::Entity3D &root;
		spk::Entity3D &head;
		spk::Entity3D &leftShoulder;
		spk::Entity3D &rightShoulder;
		spk::Entity3D &leftHip;
		spk::Entity3D &rightHip;
		spk::Entity3D &rightWrist;
	};

	[[nodiscard]] ArticulatedCharacter addArticulatedCharacter(Scene &scene, bool withSword = false)
	{
		auto rootEntity = std::make_unique<spk::Entity3D>("Voxel character root");
		rootEntity->transform().place({0, 1.83f, 0});
		rootEntity->transform().rescale({0.45f, 0.45f, 0.45f});
		auto &root = *rootEntity;
		scene.engine.addEntity(&root);
		scene.objects.push_back(std::move(rootEntity));

		scene.add(geometry({3, 4, 1}), 1, {-1.5f, -1.5f, -0.5f}, &root);
		auto &head = scene.joint(root, {0, 2.8f, 0});
		scene.add(geometry({2, 2, 2}), 3, {-1, 0, -1}, &head);
		scene.add(scene.cube, 0, {-0.5f, 0.5f, 1}, &head);

		auto &leftShoulder = scene.joint(root, {-1.8f, 2.2f, 0});
		auto &rightShoulder = scene.joint(root, {1.8f, 2.2f, 0});
		scene.add(geometry({1, 3, 1}), 2, {-0.5f, -3, -0.5f}, &leftShoulder);
		scene.add(geometry({1, 3, 1}), 2, {-0.5f, -3, -0.5f}, &rightShoulder);

		auto &leftHip = scene.joint(root, {-0.85f, -1.5f, 0});
		auto &rightHip = scene.joint(root, {0.85f, -1.5f, 0});
		scene.add(geometry({1, 3, 1}), 0, {-0.5f, -3, -0.5f}, &leftHip);
		scene.add(geometry({1, 3, 1}), 0, {-0.5f, -3, -0.5f}, &rightHip);

		auto &rightWrist = scene.joint(rightShoulder, {0, -2.7f, 0});
		if (withSword)
		{
			rightWrist.transform().setEulerRotation({90, 0, 0});
			scene.add(scene.cube, 0, {-0.5f, -0.5f, -0.5f}, &rightWrist);
			scene.add(geometry({1, 4, 1}), 2, {-0.5f, 0, -0.5f}, &rightWrist);
		}

		const spk::Vector3 eye{7, 5.5f, 9};
		scene.cameraEntity.transform().place(eye);
		scene.cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, 1.65f, 0}));
		return {root, head, leftShoulder, rightShoulder, leftHip, rightHip, rightWrist};
	}

	void addCharacterSun(Scene &scene, spk::Vector3 position, spk::RGB color = {1, 1, 1}, float intensity = 1.4f)
	{
		scene.addLight<spk::Light::Directional>(position, {0, 1.7f, 0}, color, intensity);
	}

	void addCharacterPointLeft(Scene &scene)
	{
		scene.addLight<spk::Light::Point>({-3.5f, 3.8f, 3}, {0, 1.5f, 0}, {1, 0.25f, 0.12f}, 6.0f, 10.0f);
	}

	void addCharacterPointRight(Scene &scene)
	{
		scene.addLight<spk::Light::Point>({3.5f, 3.8f, 3}, {0, 1.5f, 0}, {0.15f, 0.35f, 1}, 6.0f, 10.0f);
	}

	void addCharacterSpot(Scene &scene, spk::Vector3 origin, spk::RGB color, float angle = 0.5f)
	{
		auto &spot = scene.addLight<spk::Light::Spot>(origin, {0, 1.2f, 0}, color, 8.0f, 12.0f);
		spot.setAngle(angle);
	}

	[[nodiscard]] sparkle_test::FramebufferImage renderCapture(
		const spk::RenderSnapshot &snapshot, const spk::EngineView &view)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		snapshot.execute(gl.renderContext());
		spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = ImageSize}).execute(gl.renderContext());
		spk::DrawEngineViewCommand(view.output(), {.anchor = {0, 0}, .size = ImageSize}).execute(gl.renderContext());
		auto image = gl.capture();
		gl.reset();
		return image;
	}

	[[nodiscard]] std::size_t differentPixels(
		const sparkle_test::FramebufferImage &before, const sparkle_test::FramebufferImage &after)
	{
		if (before.size != after.size || before.pixels.size() != after.pixels.size())
		{
			return 0;
		}
		std::size_t count = 0;
		for (std::size_t i = 0; i < before.pixels.size(); i += 4)
		{
			if (before.pixels[i] != after.pixels[i] ||
				before.pixels[i + 1] != after.pixels[i + 1] ||
				before.pixels[i + 2] != after.pixels[i + 2])
			{
				++count;
			}
		}
		return count;
	}

	enum class CharacterLighting
	{
		AmbientOnly,
		DirectionalFront,
		DirectionalBehind,
		PointLeft,
		PointRight,
		ArmUpPointLeft,
		ArmUpPointRight,
		BothArmsSplitPoints,
		WalkingSpotLeft,
		WalkingSpotRight,
		HeadYawDirectional,
		RootYawDirectional,
		SwordForwardSpot,
		SwordRaisedSpot,
		SpotAcrossShapes,
		ThreeLightMix
	};

	struct CharacterLightingCase
	{
		const char *name;
		CharacterLighting lighting;
	};

	class ArticulatedVoxelLightingGolden : public ::testing::TestWithParam<CharacterLightingCase>
	{
	};
}

TEST_P(ArticulatedVoxelLightingGolden, ArticulatedNormalsFollowHierarchyAndLights)
{
	const auto &param = GetParam();
	const bool withSword =
		param.lighting == CharacterLighting::SwordForwardSpot ||
		param.lighting == CharacterLighting::SwordRaisedSpot;
	Scene scene(false);
	auto rig = addArticulatedCharacter(scene, withSword);
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	switch (param.lighting)
	{
	case CharacterLighting::AmbientOnly:
		scene.view.ambientLight().setIntensity(0.75f);
		break;
	case CharacterLighting::DirectionalFront:
		addCharacterSun(scene, {5, 7, 7});
		break;
	case CharacterLighting::DirectionalBehind:
		addCharacterSun(scene, {-4, 6, -6});
		break;
	case CharacterLighting::PointLeft:
		addCharacterPointLeft(scene);
		break;
	case CharacterLighting::PointRight:
		addCharacterPointRight(scene);
		break;
	case CharacterLighting::ArmUpPointLeft:
		rig.leftShoulder.transform().setEulerRotation({0, 0, -75});
		addCharacterPointLeft(scene);
		break;
	case CharacterLighting::ArmUpPointRight:
		rig.leftShoulder.transform().setEulerRotation({0, 0, -75});
		addCharacterPointRight(scene);
		break;
	case CharacterLighting::BothArmsSplitPoints:
		rig.leftShoulder.transform().setEulerRotation({0, 0, -75});
		rig.rightShoulder.transform().setEulerRotation({0, 0, 75});
		addCharacterPointLeft(scene);
		addCharacterPointRight(scene);
		break;
	case CharacterLighting::WalkingSpotLeft:
		rig.leftHip.transform().setEulerRotation({25, 0, 0});
		rig.rightHip.transform().setEulerRotation({-25, 0, 0});
		addCharacterSpot(scene, {-4, 5, 4}, {1, 0.65f, 0.2f});
		break;
	case CharacterLighting::WalkingSpotRight:
		rig.leftHip.transform().setEulerRotation({-25, 0, 0});
		rig.rightHip.transform().setEulerRotation({25, 0, 0});
		addCharacterSpot(scene, {4, 5, 4}, {0.25f, 0.6f, 1});
		break;
	case CharacterLighting::HeadYawDirectional:
		rig.head.transform().setEulerRotation({0, 65, 0});
		addCharacterSun(scene, {5, 7, 7});
		break;
	case CharacterLighting::RootYawDirectional:
		rig.root.transform().setEulerRotation({0, 75, 0});
		addCharacterSun(scene, {5, 7, 7});
		break;
	case CharacterLighting::SwordForwardSpot:
		addCharacterSpot(scene, {4, 5, 4}, {1, 0.8f, 0.3f});
		break;
	case CharacterLighting::SwordRaisedSpot:
		rig.rightShoulder.transform().setEulerRotation({0, 0, 70});
		addCharacterSpot(scene, {-4, 5, 4}, {0.35f, 0.65f, 1});
		break;
	case CharacterLighting::SpotAcrossShapes:
		scene.add(scene.stair, 1, {-3.0f, -0.2f, 0});
		scene.add(scene.slope, 2, {2.5f, -0.2f, 0});
		addCharacterSpot(scene, {4, 4.5f, 4}, {1, 0.85f, 0.6f}, 0.8f);
		break;
	case CharacterLighting::ThreeLightMix:
		rig.leftShoulder.transform().setEulerRotation({0, 0, -65});
		rig.rightHip.transform().setEulerRotation({-20, 0, 0});
		addCharacterSun(scene, {2, 8, -4}, {0.3f, 0.5f, 1}, 0.7f);
		addCharacterPointLeft(scene);
		addCharacterSpot(scene, {3, 5, 4}, {1, 0.8f, 0.4f});
		break;
	}
	expectGolden(scene, std::string("articulated_") + param.name);
}

INSTANTIATE_TEST_SUITE_P(
	CharacterPosesAndLightTypes,
	ArticulatedVoxelLightingGolden,
	::testing::Values(
		CharacterLightingCase{"ambient_only", CharacterLighting::AmbientOnly},
		CharacterLightingCase{"directional_front", CharacterLighting::DirectionalFront},
		CharacterLightingCase{"directional_behind", CharacterLighting::DirectionalBehind},
		CharacterLightingCase{"point_left", CharacterLighting::PointLeft},
		CharacterLightingCase{"point_right", CharacterLighting::PointRight},
		CharacterLightingCase{"arm_up_point_left", CharacterLighting::ArmUpPointLeft},
		CharacterLightingCase{"arm_up_point_right", CharacterLighting::ArmUpPointRight},
		CharacterLightingCase{"both_arms_split_points", CharacterLighting::BothArmsSplitPoints},
		CharacterLightingCase{"walking_spot_left", CharacterLighting::WalkingSpotLeft},
		CharacterLightingCase{"walking_spot_right", CharacterLighting::WalkingSpotRight},
		CharacterLightingCase{"head_yaw_directional", CharacterLighting::HeadYawDirectional},
		CharacterLightingCase{"root_yaw_directional", CharacterLighting::RootYawDirectional},
		CharacterLightingCase{"sword_forward_spot", CharacterLighting::SwordForwardSpot},
		CharacterLightingCase{"sword_raised_spot", CharacterLighting::SwordRaisedSpot},
		CharacterLightingCase{"spot_across_shapes", CharacterLighting::SpotAcrossShapes},
		CharacterLightingCase{"three_light_mix", CharacterLighting::ThreeLightMix}),
	[](const ::testing::TestParamInfo<CharacterLightingCase> &info) {
		return std::string(info.param.name);
	});

TEST(VoxelForwardLightingIntegration, ArticulatedArmRotationChangesVisibleLighting)
{
	Scene scene(false);
	auto rig = addArticulatedCharacter(scene);
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	addCharacterPointLeft(scene);
	const auto before = scene.capture();
	ASSERT_NE(before, nullptr);
	const auto beforePixels = renderCapture(*before, scene.view);

	rig.leftShoulder.transform().setEulerRotation({0, 0, -75});
	const auto after = scene.capture();
	ASSERT_NE(after, nullptr);
	const auto afterPixels = renderCapture(*after, scene.view);

	EXPECT_GT(differentPixels(beforePixels, afterPixels), 50u);
	expectGolden(*before, scene.view, "articulated_arm_before_rotation");
	expectGolden(*after, scene.view, "articulated_arm_after_rotation");
}

TEST(VoxelForwardLightingIntegration, CharacterYawChangesWorldSpaceLighting)
{
	Scene scene(false);
	auto rig = addArticulatedCharacter(scene);
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	addCharacterSun(scene, {5, 7, 7});
	const auto before = scene.capture();
	ASSERT_NE(before, nullptr);
	const auto beforePixels = renderCapture(*before, scene.view);

	rig.root.transform().setEulerRotation({0, 65, 0});
	const auto after = scene.capture();
	ASSERT_NE(after, nullptr);
	const auto afterPixels = renderCapture(*after, scene.view);

	EXPECT_GT(differentPixels(beforePixels, afterPixels), 50u);
	expectGolden(*before, scene.view, "articulated_yaw_before");
	expectGolden(*after, scene.view, "articulated_yaw_after");
}

TEST(VoxelForwardLightingIntegration, StationaryArticulatedNormalsRespondToLightChange)
{
	Scene scene(false);
	addArticulatedCharacter(scene);
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	auto &left = scene.addLight<spk::Light::Point>(
		{-3.5f, 3.8f, 3}, {0, 1.5f, 0}, {1, 0.2f, 0.1f}, 6.0f, 10.0f);
	auto &right = scene.addLight<spk::Light::Point>(
		{3.5f, 3.8f, 3}, {0, 1.5f, 0}, {0.1f, 0.2f, 1}, 0.0f, 10.0f);
	const auto before = scene.capture();
	ASSERT_NE(before, nullptr);
	const auto beforePixels = renderCapture(*before, scene.view);

	left.setIntensity(0.0f);
	right.setIntensity(6.0f);
	const auto after = scene.capture();
	ASSERT_NE(after, nullptr);
	const auto afterPixels = renderCapture(*after, scene.view);

	EXPECT_GT(differentPixels(beforePixels, afterPixels), 50u);
	expectGolden(*before, scene.view, "articulated_light_before");
	expectGolden(*after, scene.view, "articulated_light_after");
}

namespace
{
	enum class EquivalentLighting
	{
		Directional,
		Point,
		Spot
	};

	struct EquivalentRotationCase
	{
		const char *name;
		bool pitch;
		EquivalentLighting lighting;
	};

	class VoxelWorldNormalRotation : public ::testing::TestWithParam<EquivalentRotationCase>
	{
	};
}

TEST_P(VoxelWorldNormalRotation, EquivalentWorldGeometryKeepsEquivalentShading)
{
	// A cube of uniform material occupies the exact same world volume
	// before and after a quarter-turn. Rotated local normals must still
	// produce the same world-space illumination.
	Scene scene(false);
	auto &cube = scene.add(geometry({3, 3, 3}), 0, {-1.5f, -0.2f, -1.5f});
	scene.view.ambientLight().setIntensity(SoftFillAmbientIntensity);
	switch (GetParam().lighting)
	{
	case EquivalentLighting::Directional:
		addCharacterSun(scene, {4, 7, 5}, {1, 0.8f, 0.5f}, 1.7f);
		break;
	case EquivalentLighting::Point:
		scene.addLight<spk::Light::Point>(
			{3, 4, 5}, {0, 1, 0}, {1, 0.4f, 0.2f}, 7.0f, 12.0f);
		break;
	case EquivalentLighting::Spot:
		addCharacterSpot(scene, {4, 5, 6}, {0.3f, 0.7f, 1}, 0.7f);
		break;
	}

	const auto initial = scene.capture();
	ASSERT_NE(initial, nullptr);
	const auto initialPixels = renderCapture(*initial, scene.view);

	if (GetParam().pitch)
	{
		cube.transform().place({-1.5f, 2.8f, -1.5f});
		cube.transform().setEulerRotation({90, 0, 0});
	}
	else
	{
		cube.transform().place({-1.5f, -0.2f, 1.5f});
		cube.transform().setEulerRotation({0, 90, 0});
	}
	const auto rotated = scene.capture();
	ASSERT_NE(rotated, nullptr);
	const auto rotatedPixels = renderCapture(*rotated, scene.view);
	EXPECT_LE(differentPixels(initialPixels, rotatedPixels), 100u)
		<< GetParam().name << ": normals did not follow the model rotation";
}

INSTANTIATE_TEST_SUITE_P(
	RotationAndLightType,
	VoxelWorldNormalRotation,
	::testing::Values(
		EquivalentRotationCase{"yaw_directional", false, EquivalentLighting::Directional},
		EquivalentRotationCase{"yaw_point", false, EquivalentLighting::Point},
		EquivalentRotationCase{"yaw_spot", false, EquivalentLighting::Spot},
		EquivalentRotationCase{"pitch_directional", true, EquivalentLighting::Directional},
		EquivalentRotationCase{"pitch_point", true, EquivalentLighting::Point},
		EquivalentRotationCase{"pitch_spot", true, EquivalentLighting::Spot}),
	[](const ::testing::TestParamInfo<EquivalentRotationCase> &info) {
		return std::string(info.param.name);
	});
