#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstddef>
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
#include "graphics/material.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/skybox.hpp"
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
	constexpr spk::Vector2UInt ImageSize{192, 144};
	using Face = spk::Skybox::Face;

	class Atlas final : public spk::Texture
	{
	public:
		Atlas()
		{
			constexpr std::array<std::array<std::uint8_t, 3>, 6> colors{{
				{{255, 32, 32}}, {{32, 255, 32}}, {{32, 32, 255}},
				{{255, 255, 32}}, {{255, 32, 255}}, {{32, 255, 255}}
			}};
			std::array<std::uint8_t, 12 * 2 * 4> pixels{};
			for (std::size_t y = 0; y < 2; ++y)
			{
				for (std::size_t x = 0; x < 12; ++x)
				{
					const auto color = colors[x / 2];
					const auto at = (y * 12 + x) * 4;
					pixels[at] = color[0];
					pixels[at + 1] = color[1];
					pixels[at + 2] = color[2];
					pixels[at + 3] = 255;
				}
			}
			setPixels(std::span<const std::uint8_t>(pixels), {12, 2}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Texture::Section region(std::size_t index)
	{
		constexpr float width = 1.0f / 6.0f;
		return {{static_cast<float>(index) * width, 0.0f}, {width, 1.0f}};
	}

	[[nodiscard]] spk::Material material(const spk::Texture &texture, std::size_t index)
	{
		spk::Material::Builder builder;
		builder.add("albedo", texture, region(index));
		return builder.build();
	}

	void populate(spk::Skybox &skybox, const spk::Texture &texture)
	{
		for (std::size_t i = 0; i < spk::Skybox::FaceCount; ++i)
		{
			skybox.setMaterial(static_cast<Face>(i), material(texture, i));
		}
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"skybox camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::Entity3D skyEntity{"skybox"};
		spk::Skybox &skybox = skyEntity.addComponent<spk::Skybox>();
		spk::EngineView *view = nullptr;
		std::vector<std::unique_ptr<spk::Entity3D>> objects;

		Scene()
		{
			camera.setPerspective(0.88f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 40.0f);
			engine.addEntity(&cameraEntity);
			engine.addEntity(&skyEntity);
			view = &engine.addView("main", &camera, ImageSize);
		}

		~Scene()
		{
			for (const auto &object : objects)
			{
				engine.removeEntity(object.get());
			}
			engine.removeEntity(&skyEntity);
			engine.removeEntity(&cameraEntity);
		}

		[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> capture()
		{
			engine.refreshRenderSnapshot();
			return engine.latestRenderSnapshot();
		}

		void addOpaqueCubes(const Atlas &atlas)
		{
			spk::Voxel::Shape::Builder shapes;
			shapes.addFromJSON(spk::JSON::Value::loadFromFile(
				std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / "cube.json"));
			auto shape = shapes.build();
			spk::Voxel::Definition::SlotMapping mapping;
			for (const auto &slot : shape.slots())
			{
				mapping.emplace(slot.name, spk::Voxel::Definition::Slot{"stone", 1.0f});
			}
			spk::Voxel::Definition::Registry::Builder definitions;
			definitions.add(1, spk::Voxel::Definition(shape, std::move(mapping)));
			spk::Voxel::Volume::Builder volume({1, 1, 1});
			volume.bind(definitions.build());
			volume.set({0, 0, 0}, 1);
			const auto geometry = spk::Voxel::Mesher().bake(volume.build());
			spk::Voxel::Palette::Builder appearance;
			appearance.setMaterial("stone", material(atlas, 3));
			const auto palette = appearance.build();
			engine.addSystem<spk::Voxel::RenderingSystem>();
			for (const spk::Vector3 &location : {spk::Vector3{-0.8f, -0.5f, -3.0f}, spk::Vector3{0.5f, -0.4f, -4.0f}})
			{
				auto entity = std::make_unique<spk::Entity3D>("skybox foreground cube");
				entity->transform().place(location);
				auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
				renderer.bind(geometry);
				renderer.bind(palette);
				engine.addEntity(entity.get());
				objects.push_back(std::move(entity));
			}
		}
	};

	void show(const spk::Engine &engine)
	{
		spk::EngineWidget widget("skybox display", nullptr, spk::EngineWidget::Type::Draw);
		widget.setEngine(const_cast<spk::Engine *>(&engine));
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(sparkle_test::OpenGLTestContext::instance().renderContext());
	}

	[[nodiscard]] std::array<std::uint8_t, 4> finalPixel(const spk::EngineView &view)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const auto output = view.output()->latest(gl.surface());
		if (!output)
		{
			throw spk::Exception("Skybox test: missing final view output");
		}
		spk::Framebuffer framebuffer(output->size(), {output}, std::nullopt);
		framebuffer.activate(gl.renderContext());
		std::array<std::uint8_t, 4> pixel{};
		glReadPixels(
			static_cast<GLint>(output->size().x / 2), static_cast<GLint>(output->size().y / 2),
			1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
		return pixel;
	}

	void render(Scene &scene, const std::shared_ptr<const spk::RenderSnapshot> &snapshot)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		gl.reset(ImageSize);
		snapshot->execute(gl.renderContext());
		show(scene.engine);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
	}

	void golden(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const std::filesystem::path category{"rendering/skybox"};
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		gl.save(actual);
		const auto image = gl.capture();
		ASSERT_EQ(image.size, ImageSize);
		for (std::size_t index = 3; index < image.pixels.size(); index += 4)
		{
			ASSERT_EQ(image.pixels[index], 255) << "Nonopaque skybox at pixel " << index / 4;
		}
		if (!std::filesystem::exists(expected))
		{
			GTEST_SKIP() << "OpenGL capture awaits approved golden reference: " << actual;
		}
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		const auto result = sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches) << name << ": " << result.differentPixelCount << " pixel differences";
	}
}

TEST(SkyboxTest, SixSeparateMaterialsPreserveSharedTextureIdentityAndSections)
{
	Atlas atlas;
	spk::Skybox skybox;
	populate(skybox, atlas);
	for (std::size_t i = 0; i < spk::Skybox::FaceCount; ++i)
	{
		const auto *binding = skybox.material(static_cast<Face>(i)).find("albedo");
		ASSERT_NE(binding, nullptr);
		EXPECT_EQ(binding->texture.identifier(), atlas.handle().identifier());
		EXPECT_EQ(binding->section, region(i));
	}
	spk::RenderSnapshot::Builder builder;
	skybox.buildViewRenderSnapshot(builder, {});
	EXPECT_EQ(builder.renderPass(spk::EngineView::SkyboxPassKey).commandCount(), 1u);
	EXPECT_EQ(builder.passKeys().size(), 1u);
}

TEST(SkyboxTest, MissingAlbedoOrIncompleteFacesFailBeforePublication)
{
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	const auto previous = scene.capture();
	scene.skybox.setMaterial(Face::PositiveX, {});
	EXPECT_THROW((void)scene.capture(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), previous);
}

TEST(SkyboxTest, DifferentTextureStatesFailEvenWithIdenticalPixels)
{
	Atlas first;
	Atlas second;
	Scene scene;
	populate(scene.skybox, first);
	const auto stable = scene.capture();
	scene.skybox.setMaterial(Face::NegativeY, material(second, 3));
	EXPECT_THROW((void)scene.capture(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), stable);
}

TEST(SkyboxTest, InvalidFaceIsRejected)
{
	spk::Skybox skybox;
	EXPECT_THROW(skybox.setMaterial(static_cast<Face>(255), {}), spk::Exception);
	EXPECT_THROW((void)skybox.material(static_cast<Face>(255)), spk::Exception);
}

TEST(SkyboxTest, InvalidUVSectionIsRejectedByOrdinaryMaterialBuilder)
{
	Atlas atlas;
	spk::Material::Builder builder;
	EXPECT_THROW(builder.add("albedo", atlas, {{0.9f, 0.0f}, {0.2f, 1.0f}}), spk::Exception);
	EXPECT_THROW(builder.add("albedo", atlas, {{0.0f, 0.0f}, {0.0f, 1.0f}}), spk::Exception);
}

TEST(SkyboxTest, DisabledEntityAndComponentDoNotContribute)
{
	Atlas atlas;
	Scene scene;
	scene.skyEntity.deactivate();
	EXPECT_NO_THROW((void)scene.capture());
	scene.skyEntity.activate();
	scene.skybox.deactivate();
	EXPECT_NO_THROW((void)scene.capture());
	scene.skybox.activate();
	EXPECT_THROW((void)scene.capture(), spk::Exception);
	populate(scene.skybox, atlas);
	EXPECT_NO_THROW((void)scene.capture());
}

TEST(SkyboxTest, FiltersWorkIndependentlyForSeveralEngineViews)
{
	Atlas atlas;
	Scene scene;
	scene.skybox.setLayers({"sky"});
	scene.view->setLayers({"other"});
	auto &second = scene.engine.addView("second", &scene.camera, ImageSize);
	second.setLayers({"other"});
	EXPECT_NO_THROW((void)scene.capture());
	second.setLayers({"sky"});
	EXPECT_THROW((void)scene.capture(), spk::Exception);
	populate(scene.skybox, atlas);
	EXPECT_NO_THROW((void)scene.capture());
}

TEST(SkyboxTest, RemovingComponentOnlyAffectsLaterCaptures)
{
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	const auto old = scene.capture();
	scene.skyEntity.removeComponent(scene.skybox);
	EXPECT_NO_THROW((void)scene.capture());
	EXPECT_NE(scene.engine.latestRenderSnapshot(), old);
	EXPECT_EQ(old.use_count(), 1);
}

TEST(SkyboxTest, SkyboxNeverContributesToShadowOpaqueOrLightingPasses)
{
	Atlas atlas;
	spk::Skybox skybox;
	populate(skybox, atlas);
	spk::EngineView::CaptureContext context;
	context.ambientRGB = {10.0f, 5.0f, 2.0f};
	spk::RenderSnapshot::Builder builder;
	skybox.buildViewRenderSnapshot(builder, context);
	ASSERT_EQ(builder.passKeys().size(), 1u);
	EXPECT_EQ(builder.passKeys().at(0).name, spk::EngineView::SkyboxPassKey.name);
}

TEST(SkyboxTest, ViewTextureUsesUnmodifiedMaterialApiAndHasDistinctIdentity)
{
	Atlas atlas;
	Scene scene;
	auto &producer = scene.engine.addView("producer", &scene.camera, ImageSize, -10);
	for (std::size_t i = 0; i < spk::Skybox::FaceCount; ++i)
	{
		scene.skybox.setMaterial(static_cast<Face>(i), material(producer.texture(), i));
	}
	const auto first = scene.skybox.material(Face::PositiveX).find("albedo")->texture.identifier();
	EXPECT_EQ(first, producer.texture().handle().identifier());
	scene.skybox.setMaterial(Face::NegativeY, material(atlas, 3));
	EXPECT_THROW((void)scene.capture(), spk::Exception);
}

TEST(SkyboxGPU, SixFacesAndCameraRotationUseRealOpenGL)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	const std::array<std::pair<const char *, spk::Vector3>, 6> views{{
		{"negative_z", {0, 0, 0}}, {"positive_x", {0, -90, 0}},
		{"negative_x", {0, 90, 0}}, {"positive_y", {-90, 0, 0}},
		{"negative_y", {90, 0, 0}}, {"positive_z", {0, 180, 0}}
	}};
	for (const auto &[name, orientation] : views)
	{
		scene.cameraEntity.transform().setEulerRotation(orientation);
		const auto captured = scene.capture();
		render(scene, captured);
		golden(std::string("face_") + name);
	}
	gl.reset();
}

TEST(SkyboxGPU, CameraTranslationDoesNotCauseParallaxAndRotationChangesImage)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	render(scene, scene.capture());
	const auto initial = gl.capture().pixels;
	golden("camera_initial");
	scene.cameraEntity.transform().place({23, -12, 31});
	render(scene, scene.capture());
	EXPECT_EQ(gl.capture().pixels, initial);
	scene.cameraEntity.transform().setEulerRotation({0, -90, 0});
	render(scene, scene.capture());
	EXPECT_NE(gl.capture().pixels, initial);
	golden("camera_rotated");
	gl.reset();
}

TEST(SkyboxGPU, TwoOpaqueVoxelObjectsAppearInFrontOfSky)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	render(scene, scene.capture());
	const auto background = gl.capture().pixels;
	scene.addOpaqueCubes(atlas);
	render(scene, scene.capture());
	EXPECT_NE(gl.capture().pixels, background);
	golden("sky_with_two_opaque_voxels");
	gl.reset();
}

TEST(SkyboxGPU, AmbientLightChangesDoNotAffectSkyColors)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	scene.view->ambientLight().setIntensity(0.0f);
	render(scene, scene.capture());
	const auto dark = gl.capture().pixels;
	golden("ambient_zero");
	scene.view->ambientLight().setIntensity(4.0f);
	render(scene, scene.capture());
	EXPECT_EQ(gl.capture().pixels, dark);
	golden("ambient_four");
	gl.reset();
}

TEST(SkyboxGPU, EarlierProducerViewCanSupplyAllSixMaterials)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Scene scene;
	auto &producer = scene.engine.addView("producer", &scene.camera, ImageSize, -10);
	producer.setBackgroundColor({0.6f, 0.2f, 0.1f});
	producer.setLayers({"producer"});
	scene.view->setLayers({"consumer"});
	scene.skybox.setLayers({"consumer"});
	for (std::size_t i = 0; i < spk::Skybox::FaceCount; ++i)
	{
		scene.skybox.setMaterial(static_cast<Face>(i), material(producer.texture(), i));
	}
	render(scene, scene.capture());
	const auto pixel = finalPixel(*scene.view);
	EXPECT_GT(pixel[0], pixel[2]);
	EXPECT_EQ(pixel[3], 255);
	golden("view_texture_source");
	gl.reset();
}

TEST(SkyboxGPU, RemovalRestoresViewBackgroundButOldSnapshotStillRenders)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	scene.view->setBackgroundColor({0.1f, 0.1f, 0.1f});
	const auto previous = scene.capture();
	render(scene, previous);
	const auto skyPixel = finalPixel(*scene.view);
	scene.skyEntity.removeComponent(scene.skybox);
	const auto without = scene.capture();
	render(scene, without);
	const auto backgroundPixel = finalPixel(*scene.view);
	EXPECT_NE(skyPixel, backgroundPixel);
	golden("removed_background");
	render(scene, previous);
	EXPECT_EQ(finalPixel(*scene.view), skyPixel);
	golden("old_snapshot_after_removal");
	gl.reset();
}

TEST(SkyboxGPU, DisablingSkyboxAlsoRestoresBackgroundOnNextCapture)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	Atlas atlas;
	Scene scene;
	populate(scene.skybox, atlas);
	const auto previous = scene.capture();
	render(scene, previous);
	const auto sky = finalPixel(*scene.view);
	scene.skybox.deactivate();
	render(scene, scene.capture());
	EXPECT_NE(finalPixel(*scene.view), sky);
	render(scene, previous);
	EXPECT_EQ(finalPixel(*scene.view), sky);
	gl.reset();
}
