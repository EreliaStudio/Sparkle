#include <gtest/gtest.h>

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "graphics/opengl/cube_map.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/state.hpp"
#include "light/directional.hpp"
#include "light/point.hpp"
#include "light/spot.hpp"
#include "rendering/command/draw_engine_view_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/render_snapshot.hpp"
#include "shadow_map/contributor.hpp"
#include "shadow_map/rendering_system.hpp"
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
	constexpr std::uint32_t ShadowSize = 512;
	constexpr const char *Category = "rendering/shadow-maps";

	class Atlas final : public spk::Texture
	{
	public:
		Atlas()
		{
			std::array<std::uint8_t, 16 * 4 * 4> pixels{};
			constexpr std::array<std::uint8_t, 4> red{224, 92, 148, 185};
			constexpr std::array<std::uint8_t, 4> green{191, 166, 195, 130};
			constexpr std::array<std::uint8_t, 4> blue{145, 210, 140, 180};
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

	class DepthPreview final : public spk::Texture
	{
	public:
		void saveDepth(std::span<const float> depth, std::uint32_t size, const std::filesystem::path &path)
		{
			std::vector<std::uint8_t> rgba(size * size * 4);
			for (std::size_t i = 0; i < depth.size(); ++i)
			{
				const float distance = std::clamp((1.0f - depth[i]) * 128.0f, 0.0f, 1.0f);
				const auto value = static_cast<std::uint8_t>(std::lround(distance * 255.0f));
				rgba[4 * i] = value;
				rgba[4 * i + 1] = value;
				rgba[4 * i + 2] = value;
				rgba[4 * i + 3] = 255;
			}
			setPixels(std::span<const std::uint8_t>(rgba), {size, size}, Format::RGBA);
			saveAsPng(path);
		}
	};

	[[nodiscard]] spk::Voxel::Geometry geometry(spk::Vector3UInt dimensions, const char *shapeFile = "cube.json")
	{
		spk::Voxel::Shape::Builder shapeBuilder;
		shapeBuilder.addFromJSON(
			spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / shapeFile));
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

	// A single mesh containing cube, slab, slope and stair cells validates both
	// the visible triangles and the independent opaque depth-mesh output.
	[[nodiscard]] spk::Voxel::Geometry mixedGeometry()
	{
		spk::Voxel::Definition::Registry::Builder registry;
		constexpr std::array<const char *, 4> files{"cube.json", "slab.json", "slope.json", "stair.json"};
		for (std::size_t index = 0; index < files.size(); ++index)
		{
			spk::Voxel::Shape::Builder builder;
			builder.addFromJSON(
				spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / files[index]));
			const auto shape = builder.build();
			spk::Voxel::Definition::SlotMapping slots;
			for (const auto &slot : shape.slots())
			{
				slots.emplace(slot.name, spk::Voxel::Definition::Slot{"paint", 1.0f});
			}
			registry.add(static_cast<std::uint32_t>(index + 1), spk::Voxel::Definition(shape, std::move(slots)));
		}
		spk::Voxel::Volume::Builder volume({4, 1, 4});
		volume.bind(registry.build());
		using Horizontal = spk::Voxel::Shape::HorizontalOrientation;
		constexpr std::array<Horizontal, 4> directions{
			Horizontal::XPositive, Horizontal::ZPositive, Horizontal::XNegative, Horizontal::ZNegative};
		for (std::uint32_t z = 0; z < 4; ++z)
		{
			for (std::uint32_t x = 0; x < 4; ++x)
			{
				volume.set({x, 0, z}, (x + z) % 4 + 1, directions[z]);
			}
		}
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Palette palette(const Atlas &atlas, std::uint32_t tile)
	{
		spk::Material::Builder material;
		material.add("albedo", atlas, spk::Texture::Section::fromPixels(atlas.size(), {tile * 4u, 0u}, {4, 4}));
		spk::Voxel::Palette::Builder builder;
		builder.setMaterial("paint", material.build());
		return builder.build();
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"Shadow test camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::Vector2UInt imageSize;
		spk::EngineView &view;
		spk::Voxel::RenderingSystem &voxels;
		spk::ShadowMap::RenderingSystem &shadows;
		Atlas atlas;
		spk::Voxel::Geometry cube = geometry({1, 1, 1});
		spk::Voxel::Geometry slab = geometry({1, 1, 1}, "slab.json");
		spk::Voxel::Geometry slope = geometry({1, 1, 1}, "slope.json");
		spk::Voxel::Geometry stair = geometry({1, 1, 1}, "stair.json");
		spk::Voxel::Geometry mixed = mixedGeometry();
		spk::Voxel::Geometry floor;
		std::vector<std::unique_ptr<spk::Entity3D>> objects;
		std::vector<std::unique_ptr<spk::Entity3D>> lights;

		explicit Scene(bool wide = false) :
			imageSize(wide ? spk::Vector2UInt{960, 720} : ImageSize),
			view(_initializeView()),
			voxels(engine.addSystem<spk::Voxel::RenderingSystem>()),
			shadows(engine.addSystem<spk::ShadowMap::RenderingSystem>()),
			floor(geometry(wide ? spk::Vector3UInt{48, 1, 32} : spk::Vector3UInt{7, 1, 7}))
		{
			view.ambientLight().setColor({1.0f, 1.0f, 1.0f});
			view.ambientLight().setIntensity(0.12f);
			add(floor, 3, wide ? spk::Vector3{-24.0f, -1.2f, -16.0f} : spk::Vector3{-3.5f, -1.2f, -3.5f}, spk::ShadowMap::Mode::None);
		}

		spk::EngineView &_initializeView()
		{
			const spk::Vector3 eye{6.0f, 4.2f, 8.0f};
			cameraEntity.transform().place(eye);
			cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, -0.4f, 0}));
			camera.setPerspective(0.78f, static_cast<float>(imageSize.x) / imageSize.y, 0.1f, 40.0f);
			engine.addEntity(&cameraEntity);
			return engine.addView("shadow", &camera, imageSize);
		}

		spk::Entity3D &add(
			const spk::Voxel::Geometry &mesh,
			std::uint32_t tile,
			spk::Vector3 position,
			spk::ShadowMap::Mode mode)
		{
			auto object = std::make_unique<spk::Entity3D>("Shadow test voxel");
			object->transform().place(position);
			auto &visual = object->addComponent<spk::Voxel::Renderer>();
			visual.bind(mesh);
			visual.bind(palette(atlas, tile));
			auto &caster = object->addComponent<spk::ShadowMap::Contributor>();
			caster.bind(mesh.depthMesh());
			caster.setShadowMode(mode);
			engine.addEntity(object.get());
			auto &result = *object;
			objects.push_back(std::move(object));
			return result;
		}

		template <typename TLight>
		TLight &addLight(spk::Vector3 position, spk::Vector3 target, spk::RGB color, float intensity)
		{
			auto object = std::make_unique<spk::Entity3D>("Shadow test light");
			object->transform().place(position);
			object->transform().setRotation(spk::Quaternion::lookAt(position, target));
			auto &light = object->addComponent<TLight>();
			light.setColor(color);
			light.setIntensity(intensity);
			light.setCastsShadows(true);
			light.setShadowResolution(ShadowSize);
			if constexpr (std::is_same_v<TLight, spk::Light::Spot>)
			{
				light.setRange(12.0f);
				light.setAngle(0.6f);
			}
			else if constexpr (std::is_same_v<TLight, spk::Light::Directional>)
			{
				light.setShadowDistance(24.0f);
			}
			else
			{
				light.setRange(12.0f);
			}
			engine.addEntity(object.get());
			lights.push_back(std::move(object));
			return light;
		}

		[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> capture()
		{
			engine.refreshRenderSnapshot();
			return engine.latestRenderSnapshot();
		}

		~Scene()
		{
			for (const auto &object : objects)
			{
				engine.removeEntity(object.get());
			}
			for (const auto &light : lights)
			{
				engine.removeEntity(light.get());
			}
			engine.removeEntity(&cameraEntity);
		}
	};

	void compareGolden(const std::filesystem::path &category, const std::string &name, const std::filesystem::path &actual)
	{
		const auto expected = sparkle_test::expectedImagePath(category, name);
		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing shadow golden image. Inspect CI result " << actual << " before accepting a reference";
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		const auto comparison = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(comparison.matches)
			<< name << ": " << comparison.differentPixelCount << " different pixels; actual " << actual;
	}

	void compareDepth(const spk::ShadowMap::Result::Entry &entry, const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		ASSERT_EQ(entry.resolution, ShadowSize);
		ASSERT_TRUE(entry.depthTexture);
		const std::array<const char *, 6> suffixes{
			"positive_x", "negative_x", "positive_y", "negative_y", "positive_z", "negative_z"};
		bool foundDepth = false;
		for (std::size_t i = 0; i < entry.faceCount; ++i)
		{
			spk::Framebuffer framebuffer;
			if (entry.faceCount == 6)
			{
				framebuffer = spk::Framebuffer({entry.resolution, entry.resolution}, std::vector<spk::Framebuffer::RenderAttachment>{}, spk::Framebuffer::RenderAttachment(entry.depthTexture, static_cast<spk::CubeMap::Face>(i)));
			}
			else
			{
				framebuffer = spk::Framebuffer({entry.resolution, entry.resolution}, std::vector<spk::Texture::Handle>{}, entry.depthTexture);
			}
			framebuffer.activate(gl.renderContext());
			std::vector<float> depths(entry.resolution * entry.resolution);
			glReadPixels(0, 0, entry.resolution, entry.resolution, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
			EXPECT_EQ(glGetError(), GL_NO_ERROR);
			foundDepth |= *std::min_element(depths.begin(), depths.end()) < 1.0f;
			const auto category = std::filesystem::path(Category) / "depth";
			const std::string imageName = entry.faceCount == 6 ? name + "_" + suffixes[i] : name;
			const auto actual = sparkle_test::resultImagePath(category, imageName);
			DepthPreview{}.saveDepth(depths, entry.resolution, actual);
			compareGolden(category, imageName, actual);
		}
		EXPECT_TRUE(foundDepth);
	}

	void render(Scene &scene, const std::string &name, const std::string &depthName = {})
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		const auto snapshot = scene.capture();
		ASSERT_NE(snapshot, nullptr);
		snapshot->execute(gl.renderContext());
		spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = ImageSize}).execute(gl.renderContext());
		spk::DrawEngineViewCommand(scene.view.output(), {.anchor = {0, 0}, .size = ImageSize})
			.execute(gl.renderContext());
		const auto actual = sparkle_test::resultImagePath(Category, name);
		gl.save(actual);
		compareGolden(Category, name, actual);
		if (!depthName.empty())
		{
			const auto selection = scene.view.lightChannel().current();
			ASSERT_NE(selection, nullptr);
			const auto &light = selection->local.empty() ? selection->directional.front() : selection->local.front();
			const auto published = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
			ASSERT_NE(published, nullptr);
			const auto result = published->entries.find(light.registrationOrder);
			ASSERT_NE(result, published->entries.end());
			EXPECT_EQ(result->second.admission, light.admission);
			compareDepth(result->second, depthName);
		}
		EXPECT_NO_THROW(gl.requireNoErrors());
		gl.reset();
	}
	// Sample the floor in world space, not arbitrary screen pixels. Compare
	// identical geometry with shadows enabled and disabled, independently of
	// the golden image, so an incorrect new baseline cannot hide a short shadow.
	void checkFullDirectionalShadows(const std::string &name, float runPerHeight, spk::Light::ShadowFilter filter)
	{
		Scene scene(true);
		const spk::Vector3 eye{0, 35, 24};
		scene.cameraEntity.transform().place(eye);
		scene.cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, -0.2f, 0}));
		scene.camera.setOrthographic(-26.0f, 26.0f, -19.5f, 19.5f, 0.1f, 80.0f);

		const std::array<std::uint32_t, 3> heights{3, 6, 9};
		const std::array<float, 3> lanes{-8.0f, 0.0f, 8.0f};
		std::array<spk::Voxel::Geometry, 3> pillars{
			geometry({2, 3, 2}), geometry({2, 6, 2}), geometry({2, 9, 2})};
		std::vector<spk::Vector3> probes;
		for (std::size_t i = 0; i < heights.size(); ++i)
		{
			scene.add(pillars[i], 1, {-12.0f, -0.2f, lanes[i] - 1.0f}, spk::ShadowMap::Mode::Static);
			const float length = static_cast<float>(heights[i]) * runPerHeight;
			// +X is the light travel direction. The trailing edge is x=-10.
			probes.push_back({-14.0f, -0.2f, lanes[i]});
			probes.push_back({-10.0f + 0.75f, -0.2f, lanes[i]});
			probes.push_back({-10.0f + length * 0.5f, -0.2f, lanes[i]});
			probes.push_back({-10.0f + length - 0.75f, -0.2f, lanes[i]});
			probes.push_back({-10.0f + length + 1.5f, -0.2f, lanes[i]});
		}

		auto &sun = scene.addLight<spk::Light::Directional>({-runPerHeight, 1, 0}, {0, 0, 0}, {1, 1, 1}, 2.0f);
		sun.setShadowDistance(80.0f);
		sun.setShadowResolution(2048);
		sun.setShadowFilter(filter);
		const auto direction = sun.worldDirection();
		EXPECT_GT(direction.x, 0.0f);
		EXPECT_LT(direction.y, 0.0f);

		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const auto capture = [&](bool enabled, const std::string &imageName, std::vector<int> &values) {
			sun.setCastsShadows(enabled);
			gl.reset(scene.imageSize);
			const auto snapshot = scene.capture();
			ASSERT_NE(snapshot, nullptr);
			snapshot->execute(gl.renderContext());
			spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = scene.imageSize}).execute(gl.renderContext());
			spk::DrawEngineViewCommand(scene.view.output(), {.anchor = {0, 0}, .size = scene.imageSize})
				.execute(gl.renderContext());
			gl.save(sparkle_test::resultImagePath(Category, imageName));
			for (const auto &probe : probes)
			{
				const auto clip = scene.camera.viewProjectionMatrix() * spk::Vector4(probe, 1.0f);
				ASSERT_GT(clip.w, 0.0f);
				const auto x = static_cast<GLint>((clip.x / clip.w * 0.5f + 0.5f) * scene.imageSize.x);
				const auto y = static_cast<GLint>((clip.y / clip.w * 0.5f + 0.5f) * scene.imageSize.y);
				ASSERT_GE(x, 0);
				ASSERT_GE(y, 0);
				ASSERT_LT(x, static_cast<GLint>(scene.imageSize.x));
				ASSERT_LT(y, static_cast<GLint>(scene.imageSize.y));
				std::array<std::uint8_t, 4> pixel{};
				glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
				values.push_back(pixel[0] + pixel[1] + pixel[2]);
			}
			EXPECT_NO_THROW(gl.requireNoErrors());
			gl.reset();
		};

		std::vector<int> unshadowed;
		std::vector<int> shadowed;
		capture(false, name + "_no_shadows", unshadowed);
		capture(true, name, shadowed);
		ASSERT_EQ(unshadowed.size(), probes.size());
		ASSERT_EQ(shadowed.size(), probes.size());
		for (std::size_t i = 0; i < heights.size(); ++i)
		{
			SCOPED_TRACE(::testing::Message() << "Pillar height " << heights[i] << ", expected shadow extension "
											 << heights[i] * runPerHeight);
			const std::size_t start = i * 5;
			for (std::size_t j = 0; j < 5; ++j)
			{
				const auto index = start + j;
				EXPECT_GT(unshadowed[index], 60) << "Floor probe " << j << " must be illuminated";
				if (j == 0 || j == 4)
				{
					EXPECT_NEAR(shadowed[index], unshadowed[index], 24) << "Unexpected shadow outside its footprint at probe " << j;
				}
				else
				{
					EXPECT_LT(shadowed[index], unshadowed[index] * 0.8f) << "Missing shadow at interior probe " << j;
				}
			}
		}

		const auto actual = sparkle_test::resultImagePath(Category, name);
		const auto unshadowedPath = sparkle_test::resultImagePath(Category, name + "_no_shadows");
		sparkle_test::ImageComparisonOptions tolerance;
		tolerance.rgbTolerance = 8;
		const auto difference = sparkle_test::compareImages(
			actual, unshadowedPath, sparkle_test::resultImagePath(Category, name + "_shadow_difference"), tolerance);
		EXPECT_GT(difference.differentPixelCount, 0u);
		compareGolden(Category, name, actual);
	}

}

TEST(ShadowRenderingGolden, SpotStaticCasterHasVisibleShadowAndD32FDepth)
{
	Scene scene;
	scene.add(scene.cube, 0, {-0.8f, -0.2f, -0.4f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 1, {0.7f, -0.2f, -0.8f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Spot>({-2, 4, 3}, {0, -1, -0.5f}, {1, 0.92f, 0.78f}, 9.0f);
	render(scene, "spot_static", "spot_static");
}

TEST(ShadowRenderingGolden, DirectionalStaticCasterHasVisibleShadowAndD32FDepth)
{
	Scene scene;
	scene.add(scene.cube, 1, {-1.0f, -0.2f, -0.5f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {1.0f, -0.2f, 0.7f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Directional>({-5, 8, 5}, {0, 0, 0}, {1, 0.95f, 0.8f}, 1.8f);
	render(scene, "directional_static", "directional_static");
}

TEST(ShadowRenderingGolden, StaticAndDynamicCastersShareFinalDepthWithoutSharingStaticLayer)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.2f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {1.2f, -0.2f, 0}, spk::ShadowMap::Mode::Dynamic);
	scene.addLight<spk::Light::Spot>({0, 5, 3}, {0, -1, 0}, {1, 1, 1}, 9.0f);
	render(scene, "mixed_static_dynamic", "mixed_static_dynamic");
}

TEST(ShadowRenderingGolden, DynamicCasterMovesAndRegeneratesFinalDepth)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.3f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	auto &dynamic = scene.add(scene.cube, 1, {1.2f, -0.2f, 0}, spk::ShadowMap::Mode::Dynamic);
	scene.addLight<spk::Light::Spot>({-1, 5, 2}, {0, -1, 0}, {1, 1, 1}, 9.0f);
	render(scene, "dynamic_before", "dynamic_before");
	const auto first = scene.view.lightChannel().shadowResultCollection().acquireLatest(
		sparkle_test::OpenGLTestContext::instance().surface());
	ASSERT_NE(first, nullptr);
	ASSERT_EQ(first->entries.size(), 1u);
	const auto generation = first->entries.begin()->second.generation;
	dynamic.transform().place({0.4f, -0.2f, -1.7f});
	render(scene, "dynamic_after", "dynamic_after");
	const auto second = scene.view.lightChannel().shadowResultCollection().acquireLatest(
		sparkle_test::OpenGLTestContext::instance().surface());
	ASSERT_NE(second, nullptr);
	ASSERT_EQ(second->entries.size(), 1u);
	EXPECT_GT(second->entries.begin()->second.generation, generation);
}

TEST(ShadowRenderingGolden, NoneModeDoesNotOccludeShadowMap)
{
	Scene scene;
	scene.add(scene.cube, 1, {-1.0f, -0.2f, 0}, spk::ShadowMap::Mode::None);
	scene.add(scene.cube, 0, {1.0f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Directional>({-5, 8, 5}, {0, 0, 0}, {1, 1, 1}, 2.0f);
	render(scene, "none_caster");
}

TEST(ShadowRenderingGolden, SpotAndDirectionalUseIndependentProjectorsAndPCF)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.0f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {0.9f, -0.2f, 0.6f}, spk::ShadowMap::Mode::Dynamic);
	auto &spot = scene.addLight<spk::Light::Spot>({2, 4, 2}, {0, -1, 0}, {0.3f, 0.65f, 1.0f}, 8.0f);
	auto &directional = scene.addLight<spk::Light::Directional>({-5, 8, 4}, {0, 0, 0}, {1.0f, 0.9f, 0.6f}, 1.7f);
	spot.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	directional.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	render(scene, "spot_directional_pcf");
}

TEST(ShadowRenderingGolden, SpotHardAndDirectionalSoftUseIndependentFilters)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.0f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {0.9f, -0.2f, 0.6f}, spk::ShadowMap::Mode::Dynamic);
	auto &spot = scene.addLight<spk::Light::Spot>({2, 4, 2}, {0, -1, 0}, {0.3f, 0.65f, 1.0f}, 8.0f);
	auto &directional = scene.addLight<spk::Light::Directional>({-5, 8, 4}, {0, 0, 0}, {1.0f, 0.9f, 0.6f}, 1.7f);
	EXPECT_EQ(spot.shadowFilter(), spk::Light::ShadowFilter::Hard);
	directional.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	render(scene, "mixed_shadow_filters");
}

TEST(ShadowRenderingGolden, PointCubeProjectsOmnidirectionalHardShadowsAndSixDepthFaces)
{
	Scene scene;
	scene.add(scene.cube, 0, {-0.8f, -0.2f, -0.4f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 1, {0.7f, -0.2f, -0.8f}, spk::ShadowMap::Mode::Static);
	auto &light = scene.addLight<spk::Light::Point>({0, 2.5f, 1}, {0, 0, 0}, {1, 0.92f, 0.75f}, 7.0f);
	EXPECT_EQ(light.shadowFilter(), spk::Light::ShadowFilter::Hard);
	render(scene, "point_hard_floor");
}

TEST(ShadowRenderingGolden, PointSixDepthFacesHaveIndependentCardinalCasters)
{
	Scene scene;
	const std::array<spk::Vector3, 6> positions{{{2, 2, 0}, {-2, 2, 0}, {0, 4, 0}, {0, 0, 0}, {0, 2, 2}, {0, 2, -2}}};
	for (std::size_t i = 0; i < positions.size(); ++i)
	{
		scene.add(scene.cube, i % 3, positions[i], spk::ShadowMap::Mode::Static);
	}
	scene.addLight<spk::Light::Point>({0, 2, 0}, {0, 0, 0}, {1, 0.92f, 0.78f}, 7.0f);
	render(scene, "point_six_face_occluders", "point_faces");
}

TEST(ShadowRenderingGolden, PointSurroundingCubesCastShadowsOnAllSides)
{
	Scene scene;
	const std::array<spk::Vector3, 6> positions{{{2, -0.2f, 0}, {-2, -0.2f, 0}, {0, -0.2f, 2}, {0, -0.2f, -2}, {1.2f, 1.3f, 0}, {-1.2f, 1.3f, 0}}};
	for (const spk::Vector3 &position : positions)
	{
		scene.add(scene.cube, 1, position, spk::ShadowMap::Mode::Static);
	}
	scene.addLight<spk::Light::Point>({0, 1, 0}, {0, 0, 0}, {1, 0.9f, 0.78f}, 8.0f);
	render(scene, "point_surrounding_cubes");
}

TEST(ShadowRenderingGolden, PointInsideRoomLightsMultipleWallFaces)
{
	Scene scene;
	auto backWall = geometry({7, 3, 1});
	auto leftWall = geometry({1, 3, 7});
	scene.add(backWall, 0, {-3.5f, -1.2f, -3.5f}, spk::ShadowMap::Mode::Static);
	scene.add(leftWall, 2, {-3.5f, -1.2f, -3.5f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.stair, 1, {-0.8f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slope, 2, {0.8f, -0.2f, 0.2f}, spk::ShadowMap::Mode::Static);
	const spk::Vector3 eye{2.7f, 2.4f, 3.0f};
	scene.cameraEntity.transform().place(eye);
	scene.cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {-0.2f, -0.3f, -0.4f}));
	scene.addLight<spk::Light::Point>({0, 1, 0}, {0, 0, 0}, {1, 0.78f, 0.65f}, 8.0f);
	render(scene, "point_room");
}

TEST(ShadowRenderingGolden, PointDynamicCasterMovesWithoutChangingHistoricalDepth)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.2f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	auto &dynamic = scene.add(scene.cube, 1, {1.2f, -0.2f, 0}, spk::ShadowMap::Mode::Dynamic);
	auto &point = scene.addLight<spk::Light::Point>({0, 3, 1}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	render(scene, "point_dynamic_before");
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	const auto first = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(first, nullptr);
	ASSERT_EQ(first->entries.size(), 1u);
	const auto entry = first->entries.begin()->second;
	ASSERT_EQ(entry.faceCount, 6);
	dynamic.transform().place({0.3f, -0.2f, -1.5f});
	render(scene, "point_dynamic_after");
	const auto second = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(second, nullptr);
	EXPECT_GT(second->entries.begin()->second.generation, entry.generation);
	EXPECT_NE(second->entries.begin()->second.depthTexture.identifier(), entry.depthTexture.identifier());
	EXPECT_EQ(first->entries.begin()->second.depthTexture.identifier(), entry.depthTexture.identifier());
	point.setShadowResolution(256);
	render(scene, "point_dynamic_resized");
	const auto third = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(third, nullptr);
	ASSERT_EQ(third->entries.size(), 1u);
	EXPECT_EQ(third->entries.begin()->second.resolution, 256u);
	EXPECT_GT(third->entries.begin()->second.generation, second->entries.begin()->second.generation);
	EXPECT_NE(third->entries.begin()->second.depthTexture.identifier(), second->entries.begin()->second.depthTexture.identifier());

	// A failed GPU cube allocation must leave the last completely published
	// six-face generation untouched, even after Light::Channel invalidates admission.
	GLint maximum = 0;
	glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maximum);
	ASSERT_GT(maximum, 0);
	ASSERT_LT(maximum, std::numeric_limits<std::int32_t>::max());
	point.setShadowResolution(static_cast<std::uint32_t>(maximum) + 1u);
	gl.reset(ImageSize);
	const auto failed = scene.capture();
	ASSERT_NE(failed, nullptr);
	EXPECT_THROW(failed->execute(gl.renderContext()), spk::Exception);
	EXPECT_EQ(scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface()).get(), third.get());
	gl.reset();
}

TEST(ShadowRenderingGolden, PointAndSpotAndDirectionalCanShadowSameForwardScene)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.0f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {0.9f, -0.2f, 0.6f}, spk::ShadowMap::Mode::Dynamic);
	scene.addLight<spk::Light::Point>({-1, 2, 0}, {0, 0, 0}, {1, 0.4f, 0.3f}, 5.0f);
	scene.addLight<spk::Light::Spot>({2, 4, 2}, {0, -1, 0}, {0.3f, 0.65f, 1}, 5.0f);
	scene.addLight<spk::Light::Directional>({-5, 8, 4}, {0, 0, 0}, {0.8f, 0.9f, 1}, 1.5f);
	render(scene, "point_spot_directional");
}

TEST(ShadowRenderingGolden, PointHardAndNineTapPCFHaveDistinctReferences)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.0f, -0.2f, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 1, {0.7f, -0.2f, 0.4f}, spk::ShadowMap::Mode::Dynamic);
	auto &point = scene.addLight<spk::Light::Point>({-1.5f, 3, 1.5f}, {0, 0, 0}, {1, 1, 1}, 9.0f);
	point.setShadowResolution(128);
	render(scene, "point_hard_compare");
	point.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	render(scene, "point_pcf3x3_compare");
}

TEST(ShadowRenderingGolden, PointShadowSeamsAndCornersUseCrossFaceSampling)
{
	Scene scene;
	scene.add(scene.cube, 0, {0.7f, -0.2f, 0.7f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 1, {-0.7f, -0.2f, -0.7f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {1.2f, -0.2f, -1.2f}, spk::ShadowMap::Mode::Static);
	auto &point = scene.addLight<spk::Light::Point>({0, 2, 0}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	point.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	render(scene, "point_face_seams");
}

TEST(ShadowRenderingGolden, PointHalfSlabCastsShorterShadowThanFullCube)
{
	Scene scene;
	scene.add(scene.cube, 0, {-1.25f, -0.2f, 0.0f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slab, 1, {0.75f, -0.2f, 0.0f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({-1.5f, 3.5f, 1.0f}, {0, 0, 0}, {1, 0.9f, 0.7f}, 9.0f);
	render(scene, "point_slab_vs_cube");
}

TEST(ShadowRenderingGolden, PointSlopeCastsDiagonalShadowWithDepthGeometry)
{
	Scene scene;
	scene.add(scene.slope, 0, {-0.9f, -0.2f, 0.0f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.cube, 2, {0.9f, -0.2f, -0.5f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({-1.0f, 3.0f, 2.5f}, {0, 0, 0}, {1, 0.92f, 0.8f}, 8.0f);
	render(scene, "point_slope_diagonal", "point_slope_diagonal");
}

TEST(ShadowRenderingGolden, PointStairCastsSteppedShadowWithDepthGeometry)
{
	Scene scene;
	scene.add(scene.stair, 1, {-0.5f, -0.2f, 0.0f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slab, 2, {1.0f, -0.2f, -0.3f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({-1.4f, 3.0f, 2.3f}, {0, 0, 0}, {1, 0.85f, 0.65f}, 8.0f);
	render(scene, "point_stair_steps", "point_stair_steps");
}

TEST(ShadowRenderingGolden, PointFourDistinctShapeCastersShareOneCubemap)
{
	Scene scene;
	const std::array<const spk::Voxel::Geometry *, 4> shapes{&scene.cube, &scene.slab, &scene.slope, &scene.stair};
	for (std::size_t i = 0; i < shapes.size(); ++i)
	{
		scene.add(*shapes[i], static_cast<std::uint32_t>(i % 3), {-2.25f + static_cast<float>(i) * 1.3f, -0.2f, -0.6f}, spk::ShadowMap::Mode::Static);
	}
	scene.addLight<spk::Light::Point>({0.0f, 3.0f, 2.0f}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	render(scene, "point_all_four_shapes");
}

TEST(ShadowRenderingGolden, PointMixedShapeCellsProduceOneCompositeDepthMesh)
{
	Scene scene;
	scene.add(scene.mixed, 0, {-2.0f, -0.2f, -2.0f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({-0.5f, 3.0f, 0.5f}, {0, 0, 0}, {1, 0.9f, 0.72f}, 8.0f);
	render(scene, "point_mixed_cell_geometry", "point_mixed_cell_geometry");
}

// Non-uniform voxel scaling is intentionally unsupported by the Forward
// renderer. Keep that contract distinct from the valid PointLight shadow test.
TEST(PointShadowRenderingTest, NonUniformStairScaleIsRejected)
{
	Scene scene;
	auto &stair = scene.add(scene.stair, 0, {-0.7f, -0.2f, -0.2f}, spk::ShadowMap::Mode::Static);
	stair.transform().rescale({1.6f, 1.5f, 0.65f});
	scene.addLight<spk::Light::Point>({0.0f, 3.0f, 1.3f}, {0, 0, 0}, {1, 0.95f, 0.8f}, 8.0f);
	EXPECT_THROW((void)scene.capture(), spk::Exception);
}

TEST(PointShadowRenderingTest, UniformStairScaleAndRotationChangeAllFaceDepthAsNeeded)
{
	Scene scene;
	auto &stair = scene.add(scene.stair, 0, {-0.7f, -0.2f, -0.2f}, spk::ShadowMap::Mode::Static);
	stair.transform().rescale({1.5f, 1.5f, 1.5f});
	scene.add(scene.slope, 1, {1.0f, -0.2f, 0.0f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({0.0f, 3.0f, 1.3f}, {0, 0, 0}, {1, 0.95f, 0.8f}, 8.0f);
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	const auto readDepthFaces = [&](const spk::ShadowMap::Result::Entry &entry) {
		std::array<std::vector<float>, 6> depths;
		for (std::size_t face = 0; face < depths.size(); ++face)
		{
			spk::Framebuffer framebuffer(
				{entry.resolution, entry.resolution},
				std::vector<spk::Framebuffer::RenderAttachment>{},
				spk::Framebuffer::RenderAttachment(entry.depthTexture, static_cast<spk::CubeMap::Face>(face)));
			framebuffer.activate(gl.renderContext());
			depths[face].resize(entry.resolution * entry.resolution);
			glReadPixels(0, 0, entry.resolution, entry.resolution, GL_DEPTH_COMPONENT, GL_FLOAT, depths[face].data());
			EXPECT_EQ(glGetError(), GL_NO_ERROR);
		}
		return depths;
	};

	gl.reset(ImageSize);
	const auto firstSnapshot = scene.capture();
	ASSERT_NE(firstSnapshot, nullptr);
	firstSnapshot->execute(gl.renderContext());
	const auto first = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(first, nullptr);
	ASSERT_EQ(first->entries.size(), 1u);
	const auto firstEntry = first->entries.begin()->second;
	ASSERT_EQ(firstEntry.faceCount, 6u);
	const auto firstDepth = readDepthFaces(firstEntry);
	gl.reset();

	stair.transform().setEulerRotation({0, 0.72f, 0});
	gl.reset(ImageSize);
	const auto secondSnapshot = scene.capture();
	ASSERT_NE(secondSnapshot, nullptr);
	secondSnapshot->execute(gl.renderContext());
	const auto second = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(second, nullptr);
	ASSERT_EQ(second->entries.size(), 1u);
	const auto &secondEntry = second->entries.begin()->second;
	ASSERT_EQ(secondEntry.faceCount, 6u);
	EXPECT_GT(secondEntry.generation, firstEntry.generation);
	EXPECT_NE(secondEntry.depthTexture.identifier(), firstEntry.depthTexture.identifier());
	const auto secondDepth = readDepthFaces(secondEntry);
	EXPECT_NE(secondDepth, firstDepth) << "Rotating the stair must alter at least one cubemap depth pixel";
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}

TEST(ShadowRenderingGolden, PointSlabRoofPillarsAndStairsCastArchitecturalShadows)
{
	Scene scene;
	const auto pillar = geometry({1, 3, 1});
	const auto roof = geometry({5, 1, 3}, "slab.json");
	scene.add(pillar, 2, {-2.5f, -0.2f, -1.5f}, spk::ShadowMap::Mode::Static);
	scene.add(pillar, 2, {1.5f, -0.2f, -1.5f}, spk::ShadowMap::Mode::Static);
	scene.add(pillar, 2, {-2.5f, -0.2f, 0.5f}, spk::ShadowMap::Mode::Static);
	scene.add(pillar, 2, {1.5f, -0.2f, 0.5f}, spk::ShadowMap::Mode::Static);
	scene.add(roof, 1, {-2.5f, 2.8f, -1.5f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.stair, 0, {-0.9f, -0.2f, 2.0f}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({0, 1.5f, 0}, {0, 0, 0}, {1, 0.88f, 0.62f}, 10.0f);
	render(scene, "point_slab_roof_pavilion");
}

TEST(ShadowRenderingGolden, PointRotatedSlopesAndStairsCrossCubemapFaceSeams)
{
	Scene scene;
	auto &first = scene.add(scene.slope, 0, {0.55f, -0.2f, 0.55f}, spk::ShadowMap::Mode::Static);
	auto &second = scene.add(scene.stair, 1, {-1.25f, -0.2f, -0.3f}, spk::ShadowMap::Mode::Static);
	first.transform().setEulerRotation({0, 0.785398f, 0});
	second.transform().setEulerRotation({0, -0.785398f, 0});
	scene.add(scene.slab, 2, {0.6f, 0.8f, -1.1f}, spk::ShadowMap::Mode::Static);
	auto &light = scene.addLight<spk::Light::Point>({0, 2.0f, 0}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	light.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	render(scene, "point_non_cube_face_seams");
}

TEST(ShadowRenderingGolden, PointDynamicSlopeMovesBetweenStaticStairsAndSlabs)
{
	Scene scene;
	scene.add(scene.stair, 0, {-1.4f, -0.2f, -0.5f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slab, 1, {1.1f, -0.2f, 0.4f}, spk::ShadowMap::Mode::Static);
	auto &movingSlope = scene.add(scene.slope, 2, {0.5f, -0.2f, -1.0f}, spk::ShadowMap::Mode::Dynamic);
	scene.addLight<spk::Light::Point>({0.0f, 2.6f, 0.0f}, {0, 0, 0}, {1, 0.95f, 0.85f}, 8.0f);
	render(scene, "point_shape_dynamic_before");
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	const auto first = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(first, nullptr);
	ASSERT_EQ(first->entries.size(), 1u);
	const auto generation = first->entries.begin()->second.generation;
	movingSlope.transform().place({-0.4f, -0.2f, 1.2f});
	render(scene, "point_shape_dynamic_after");
	const auto second = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(second, nullptr);
	ASSERT_EQ(second->entries.size(), 1u);
	EXPECT_GT(second->entries.begin()->second.generation, generation);
}

TEST(ShadowRenderingGolden, PointAndSpotMixedShapeCastersUseIndependentFilters)
{
	Scene scene;
	scene.add(scene.stair, 0, {-1.2f, -0.2f, 0.1f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slope, 1, {0.8f, -0.2f, -0.5f}, spk::ShadowMap::Mode::Dynamic);
	scene.add(scene.slab, 2, {1.5f, 0.8f, 0.5f}, spk::ShadowMap::Mode::Static);
	auto &point = scene.addLight<spk::Light::Point>({-1.5f, 3.0f, 1.0f}, {0, 0, 0}, {1, 0.45f, 0.35f}, 7.0f);
	auto &spot = scene.addLight<spk::Light::Spot>({2.0f, 4.0f, 2.0f}, {0, -1, 0}, {0.3f, 0.65f, 1.0f}, 7.0f);
	point.setShadowFilter(spk::Light::ShadowFilter::PCF3x3);
	EXPECT_EQ(spot.shadowFilter(), spk::Light::ShadowFilter::Hard);
	render(scene, "point_spot_non_cube_mixed");
}

TEST(ShadowRenderingGolden, PointNoneModeSlopeRemainsVisibleButCannotBlockShadow)
{
	Scene scene;
	scene.add(scene.slope, 0, {-1.0f, -0.2f, -0.3f}, spk::ShadowMap::Mode::None);
	scene.add(scene.stair, 1, {0.7f, -0.2f, -0.3f}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slab, 2, {-0.2f, 0.6f, 1.1f}, spk::ShadowMap::Mode::Dynamic);
	scene.addLight<spk::Light::Point>({0, 3.0f, 1.0f}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	render(scene, "point_none_mode_slope");
}

// Unlike the golden images, this test checks the GPU-produced D32F data
// directly: non-cubic casters must populate every one of the six faces.
TEST(PointShadowDepthTest, NonCubeCastersPopulateEveryCardinalFace)
{
	Scene scene;
	scene.add(scene.slab, 0, {2, 2, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slope, 1, {-3, 2, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.stair, 2, {0, 4, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slab, 0, {0, 0, 0}, spk::ShadowMap::Mode::Static);
	scene.add(scene.slope, 1, {0, 2, 2}, spk::ShadowMap::Mode::Static);
	scene.add(scene.stair, 2, {0, 2, -3}, spk::ShadowMap::Mode::Static);
	scene.addLight<spk::Light::Point>({0, 2, 0}, {0, 0, 0}, {1, 1, 1}, 8.0f);
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	const auto snapshot = scene.capture();
	ASSERT_NE(snapshot, nullptr);
	snapshot->execute(gl.renderContext());
	const auto latest = scene.view.lightChannel().shadowResultCollection().acquireLatest(gl.surface());
	ASSERT_NE(latest, nullptr);
	ASSERT_EQ(latest->entries.size(), 1u);
	const auto &entry = latest->entries.begin()->second;
	ASSERT_EQ(entry.faceCount, 6u);
	ASSERT_EQ(entry.resolution, ShadowSize);
	ASSERT_TRUE(entry.depthTexture);
	EXPECT_EQ(entry.depthTexture->format(), spk::Texture::Format::Depth32F);
	for (std::size_t face = 0; face < 6; ++face)
	{
		spk::Framebuffer framebuffer(
			{entry.resolution, entry.resolution},
			std::vector<spk::Framebuffer::RenderAttachment>{},
			spk::Framebuffer::RenderAttachment(entry.depthTexture, static_cast<spk::CubeMap::Face>(face)));
		framebuffer.activate(gl.renderContext());
		std::vector<float> depths(entry.resolution * entry.resolution);
		glReadPixels(0, 0, entry.resolution, entry.resolution, GL_DEPTH_COMPONENT, GL_FLOAT, depths.data());
		EXPECT_EQ(glGetError(), GL_NO_ERROR) << "face " << face;
		const auto nearest = std::min_element(depths.begin(), depths.end());
		ASSERT_NE(nearest, depths.end());
		EXPECT_LT(*nearest, 0.99999f) << "No non-cube caster depth in face " << face;
		const auto occupied = std::count_if(depths.begin(), depths.end(), [](float value) {
			return value < 0.99999f;
		});
		EXPECT_GT(occupied, 8) << "No meaningful non-cube coverage in face " << face;
	}
	EXPECT_NO_THROW(gl.requireNoErrors());
	gl.reset();
}

TEST(ShadowRenderingGolden, DirectionalFullShadowAt45Degrees)
{
	checkFullDirectionalShadows("directional_full_shadow_45", 1.0f, spk::Light::ShadowFilter::Hard);
}

TEST(ShadowRenderingGolden, DirectionalFullLongShadowAtLowElevation)
{
	checkFullDirectionalShadows("directional_full_shadow_low", 3.0f, spk::Light::ShadowFilter::Hard);
}

TEST(ShadowRenderingGolden, DirectionalFullLongShadowWithPCF)
{
	checkFullDirectionalShadows("directional_full_shadow_low_pcf", 3.0f, spk::Light::ShadowFilter::PCF3x3);
}

TEST(ShadowRenderingGolden, DirectionalOffscreenCasterStillShadowsVisibleFloor)
{
	Scene scene(true);
	const spk::Vector3 eye{0, 10, 8};
	scene.cameraEntity.transform().place(eye);
	scene.cameraEntity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, -0.2f, 0}));
	scene.camera.setOrthographic(-4.0f, 4.0f, -3.0f, 3.0f, 0.1f, 18.0f);
	const auto pillar = geometry({2, 8, 2});
	scene.add(pillar, 1, {-19.0f, -0.2f, -1.0f}, spk::ShadowMap::Mode::Static);
	auto &sun = scene.addLight<spk::Light::Directional>({-3, 1, 0}, {0, 0, 0}, {1, 1, 1}, 2.0f);
	sun.setShadowDistance(18.0f);
	sun.setShadowResolution(2048);
	sun.setShadowFilter(spk::Light::ShadowFilter::Hard);

	// The ray from the central floor probe towards the sun passes through
	// the pillar at (-18, 5.8, 0), well outside the camera's horizontal extent.
	const spk::Vector3 blocker{-18, 5.8f, 0};
	const auto blockerClip = scene.camera.viewProjectionMatrix() * spk::Vector4(blocker, 1.0f);
	ASSERT_GT(std::abs(blockerClip.x), blockerClip.w);
	const std::array<spk::Vector3, 4> probes{{{0, -0.2f, 0}, {2, -0.2f, 0}, {0, -0.2f, -2}, {0, -0.2f, 2}}};
	const std::string name = "directional_offscreen_caster";
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	const auto capture = [&](bool enabled, const std::string &imageName, std::vector<int> &values) {
		sun.setCastsShadows(enabled);
		gl.reset(scene.imageSize);
		const auto snapshot = scene.capture();
		ASSERT_NE(snapshot, nullptr);
		snapshot->execute(gl.renderContext());
		spk::ViewportUniformRenderCommand({.anchor = {0, 0}, .size = scene.imageSize}).execute(gl.renderContext());
		spk::DrawEngineViewCommand(scene.view.output(), {.anchor = {0, 0}, .size = scene.imageSize})
			.execute(gl.renderContext());
		gl.save(sparkle_test::resultImagePath(Category, imageName));
		for (const auto &probe : probes)
		{
			const auto clip = scene.camera.viewProjectionMatrix() * spk::Vector4(probe, 1.0f);
			ASSERT_GT(clip.w, 0.0f);
			const auto x = static_cast<GLint>((clip.x / clip.w * 0.5f + 0.5f) * scene.imageSize.x);
			const auto y = static_cast<GLint>((clip.y / clip.w * 0.5f + 0.5f) * scene.imageSize.y);
			ASSERT_GE(x, 0);
			ASSERT_GE(y, 0);
			ASSERT_LT(x, static_cast<GLint>(scene.imageSize.x));
			ASSERT_LT(y, static_cast<GLint>(scene.imageSize.y));
			std::array<std::uint8_t, 4> pixel{};
			glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
			values.push_back(pixel[0] + pixel[1] + pixel[2]);
		}
		EXPECT_NO_THROW(gl.requireNoErrors());
		gl.reset();
	};

	std::vector<int> unshadowed;
	std::vector<int> shadowed;
	capture(false, name + "_no_shadows", unshadowed);
	capture(true, name, shadowed);
	ASSERT_EQ(unshadowed.size(), probes.size());
	ASSERT_EQ(shadowed.size(), probes.size());
	for (std::size_t i = 0; i < probes.size(); ++i)
	{
		SCOPED_TRACE(i);
		EXPECT_GT(unshadowed[i], 60);
		if (i < 2)
		{
			EXPECT_LT(shadowed[i], unshadowed[i] * 0.8f) << "Offscreen caster must shadow visible receiver";
		}
		else
		{
			EXPECT_NEAR(shadowed[i], unshadowed[i], 24);
		}
	}
	const auto actual = sparkle_test::resultImagePath(Category, name);
	sparkle_test::ImageComparisonOptions tolerance;
	tolerance.rgbTolerance = 8;
	const auto difference = sparkle_test::compareImages(
		actual, sparkle_test::resultImagePath(Category, name + "_no_shadows"), sparkle_test::resultImagePath(Category, name + "_shadow_difference"), tolerance);
	EXPECT_GT(difference.differentPixelCount, 0u);
	compareGolden(Category, name, actual);
}
