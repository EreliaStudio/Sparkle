#include <gtest/gtest.h>

#include <GL/glew.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "container/json/object.hpp"
#include "core/platform/window.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "exception.hpp"
#include "graphics/opengl/framebuffer.hpp"
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

	class RedAtlas final : public spk::Texture
	{
	public:
		RedAtlas()
		{
			std::array<std::uint8_t, 4 * 4 * 4> pixels{};
			for (std::size_t offset = 0; offset < pixels.size(); offset += 4)
			{
				pixels[offset] = 255;
				pixels[offset + 1] = 24;
				pixels[offset + 2] = 16;
				pixels[offset + 3] = 255;
			}
			setPixels(std::span<const std::uint8_t>(pixels), {4, 4}, Format::RGBA);
			validate();
		}
	};

	[[nodiscard]] spk::Voxel::Geometry geometry(const std::string &shapeName)
	{
		spk::Voxel::Shape::Builder shapes;
		shapes.addFromJSON(spk::JSON::Value::loadFromFile(std::filesystem::path(SPARKLE_VOXEL_SHAPES_DIR) / (shapeName + ".json")));
		spk::Voxel::Definition::SlotMapping mapping;
		const auto shape = shapes.build();
		for (const auto &slot : shape.slots())
		{
			mapping.emplace(slot.name, spk::Voxel::Definition::Slot{"screen", 1.0f});
		}
		spk::Voxel::Definition::Registry::Builder registry;
		registry.add(1, spk::Voxel::Definition(shape, std::move(mapping)));
		spk::Voxel::Volume::Builder volume({1, 1, 1});
		volume.bind(registry.build());
		volume.set({0, 0, 0}, 1);
		return spk::Voxel::Mesher().bake(volume.build());
	}

	[[nodiscard]] spk::Palette appearance(const spk::Texture &texture)
	{
		spk::Material::Builder material;
		material.add("albedo", texture);
		spk::Voxel::Palette::Builder palette;
		palette.setMaterial("screen", material.build());
		return palette.build();
	}

	[[nodiscard]] spk::Palette appearance(const spk::EngineView &view)
	{
		spk::Material::Builder material;
		material.add("albedo", view);
		spk::Voxel::Palette::Builder palette;
		palette.setMaterial("screen", material.build());
		return palette.build();
	}

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"main camera"};
		spk::CameraComponent3D &camera = cameraEntity.addComponent<spk::CameraComponent3D>();
		spk::Entity3D securityCameraEntity{"security camera"};
		spk::CameraComponent3D &securityCamera = securityCameraEntity.addComponent<spk::CameraComponent3D>();
		spk::EngineView *producer = nullptr;
		spk::EngineView *consumer = nullptr;
		std::vector<std::unique_ptr<spk::Entity3D>> entities;

		static void configureCamera(
			spk::Entity3D &entity, spk::CameraComponent3D &camera, spk::Vector3 eye)
		{
			entity.transform().place(eye);
			entity.transform().setRotation(spk::Quaternion::lookAt(eye, {0, 0, 0}));
			camera.setPerspective(0.78f, static_cast<float>(ImageSize.x) / ImageSize.y, 0.1f, 30.0f);
		}

		Scene()
		{
			configureCamera(cameraEntity, camera, {2.8f, 1.9f, 4.0f});
			configureCamera(securityCameraEntity, securityCamera, {1.8f, 1.5f, 3.2f});
			engine.addEntity(&cameraEntity);
			engine.addEntity(&securityCameraEntity);
			producer = &engine.addView("securityCamera", &securityCamera, ImageSize, -10);
			consumer = &engine.addView("main", &camera, ImageSize, 10);
			producer->setLayers({"security"});
			consumer->setLayers({"main"});
			producer->setBackgroundColor({0.1f, 0.16f, 0.23f});
			consumer->setBackgroundColor({0.01f, 0.02f, 0.035f});
			engine.addSystem<spk::Voxel::RenderingSystem>().setLayers({"security", "main"});
		}

		~Scene()
		{
			for (const auto &entity : entities)
			{
				engine.removeEntity(entity.get());
			}
			engine.removeEntity(&securityCameraEntity);
			engine.removeEntity(&cameraEntity);
		}

		spk::Voxel::Renderer &add(
			const std::string &shape,
			const spk::Palette &palette,
			const std::string &layer,
			spk::Vector3 position = {-0.5f, -0.5f, -0.5f},
			spk::Vector3 rotation = {},
			spk::Vector3 scale = {1.0f, 1.0f, 1.0f})
		{
			auto entity = std::make_unique<spk::Entity3D>("voxel monitor");
			entity->transform().place(position);
			entity->transform().setEulerRotation(rotation);
			entity->transform().rescale(scale);
			auto &renderer = entity->addComponent<spk::Voxel::Renderer>();
			renderer.bind(geometry(shape));
			renderer.bind(palette);
			renderer.setLayers({layer});
			engine.addEntity(entity.get());
			entities.emplace_back(std::move(entity));
			return renderer;
		}

		void configure(const spk::Texture &atlas)
		{
			add("cube", appearance(atlas), "security");
			add("slab", appearance(*producer), "main", {-1.25f, 1.25f, 0.0f}, {90.0f, 0.0f, 0.0f}, {2.5f, 2.5f, 2.5f});
		}

		[[nodiscard]] std::shared_ptr<const spk::RenderSnapshot> capture()
		{
			engine.refreshRenderSnapshot();
			return engine.latestRenderSnapshot();
		}
	};

	void drawMain(Scene &scene)
	{
		spk::EngineWidget widget("security monitor", nullptr);
		widget.setEngine(&scene.engine);
		widget.setGeometry({.anchor = {0, 0}, .size = ImageSize});
		widget.activate();
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		builder.build().execute(sparkle_test::OpenGLTestContext::instance().renderContext());
	}

	void saveGolden(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const std::filesystem::path category{"rendering/voxel-view-material"};
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		gl.save(actual);
		if (!std::filesystem::exists(expected))
		{
			GTEST_SKIP() << "New golden reference awaiting manual approval: " << actual;
		}
		const auto diff = sparkle_test::resultImagePath(category, name + "_difference");
		const auto comparison = sparkle_test::compareImages(actual, expected, diff);
		EXPECT_TRUE(comparison.matches) << name << ": " << comparison.differentPixelCount << " different pixels";
	}
}

TEST(VoxelViewMaterialTest, MaterialPreservesIdentityThroughResizeAndDoesNotRebindHomonyms)
{
	Scene scene;
	spk::Material::Builder builder;
	builder.add("albedo", *scene.producer);
	const auto material = builder.build();
	const auto *binding = material.find("albedo");
	ASSERT_NE(binding, nullptr);
	EXPECT_TRUE(binding->isViewSource());
	EXPECT_TRUE(binding->valid());
	const auto identity = binding->viewIdentity;
	scene.producer->setResolution({80, 60});
	EXPECT_EQ(binding->viewIdentity, identity);
	EXPECT_EQ(binding->viewOutput.lock(), scene.producer->output());
	scene.engine.removeView("securityCamera");
	auto &replacement = scene.engine.addView("securityCamera", &scene.camera, ImageSize, -10);
	EXPECT_NE(replacement.identity(), identity);
	EXPECT_NE(binding->viewOutput.lock(), replacement.output());
	EXPECT_FALSE(binding->valid());
}

TEST(VoxelViewMaterialTest, MaterialBuilderPreservesStaticTexturesAndRejectsDuplicates)
{
	Scene scene;
	RedAtlas atlas;
	spk::Material::Builder material;
	material.add("albedo", atlas);
	EXPECT_THROW(material.add("albedo", *scene.producer), spk::Exception);
	const auto built = material.build();
	ASSERT_NE(built.find("albedo"), nullptr);
	EXPECT_FALSE(built.find("albedo")->isViewSource());
	EXPECT_EQ(built.find("albedo")->texture.identifier(), atlas.handle().identifier());
}

TEST(VoxelViewMaterialTest, InvalidProducerOrderRetainsLastPublishedSnapshot)
{
	Scene scene;
	RedAtlas atlas;
	scene.configure(atlas);
	const auto old = scene.capture();
	ASSERT_NE(old, nullptr);
	scene.consumer->setOrder(-20);
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), old);
}

TEST(VoxelViewMaterialTest, SelfReferenceAndCyclesAreRejectedBeforePublication)
{
	Scene scene;
	RedAtlas atlas;
	scene.configure(atlas);
	auto &self = scene.add("cube", appearance(*scene.producer), "security");
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	self.setLayers({"unused"});
	const auto old = scene.capture();
	auto &other = scene.add("cube", appearance(*scene.consumer), "security");
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), old);
	other.setLayers({"unused"});
	EXPECT_NO_THROW(scene.engine.refreshRenderSnapshot());
}

TEST(VoxelViewMaterialTest, RemovedAndRecreatedProducerDoesNotRebindMaterial)
{
	Scene scene;
	RedAtlas atlas;
	scene.configure(atlas);
	const auto old = scene.capture();
	scene.engine.removeView("securityCamera");
	auto &replacement = scene.engine.addView("securityCamera", &scene.camera, ImageSize, -10);
	replacement.setLayers({"security"});
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), old);
}

TEST(VoxelViewMaterialTest, CapturedOutputCannotBeReadFromAnUnproducedSurface)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	auto source = scene.producer->output();
	auto attachment = spk::Framebuffer::RenderAttachment::create2D(ImageSize, spk::Texture::Format::SRGB8_ALPHA8);
	auto expected = attachment.texture();
	spk::EngineView::Source resolved{spk::EngineView::Source::Kind::Produced, source, expected};
	auto &first = gl.surface();
	spk::Window::Surface second{"second surface"};
	EXPECT_THROW((void)resolved.onSurface(first), spk::Exception);
	source->publish(first, expected);
	EXPECT_EQ(resolved.onSurface(first).identifier(), expected.identifier());
	EXPECT_THROW((void)resolved.onSurface(second), spk::Exception);
	gl.reset();
}

TEST(VoxelViewMaterialTest, FirstFrameAlsoProducesBothViewsOnIndependentSurface)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	RedAtlas atlas;
	Scene scene;
	scene.configure(atlas);
	const auto snapshot = scene.capture();
	spk::WinAPI::Window::Class windowClass("Sparkle_SecondaryVoxelViewSource");
	spk::Window::Native native("secondary material native");
	spk::Window::Surface surface("secondary material surface");
	spk::WinAPI::Window::CreationInfo creation;
	creation.title = "Sparkle material source test";
	creation.width = static_cast<int>(ImageSize.x);
	creation.height = static_cast<int>(ImageSize.y);
	creation.visible = false;
	native.window().create(windowClass, creation);
	surface.create(native.window());
	surface.setGeometry({.anchor = {0, 0}, .size = ImageSize});
	surface.makeCurrent();
	spk::RenderContext context{surface};
	snapshot->execute(context);
	const auto produced = scene.producer->output()->latest(surface);
	const auto consumed = scene.consumer->output()->latest(surface);
	ASSERT_TRUE(produced);
	ASSERT_TRUE(consumed);
	EXPECT_EQ(consumed->size(), ImageSize);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	EXPECT_FALSE(scene.producer->output()->latest(gl.surface()));
	surface.destroy();
	native.window().destroy();
	gl.makeCurrent();
	snapshot->execute(gl.renderContext());
	EXPECT_TRUE(scene.producer->output()->latest(gl.surface()));
	EXPECT_TRUE(scene.consumer->output()->latest(gl.surface()));
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(VoxelViewMaterialTest, DetachedPoliciesAreExplicitAndKeepLastFrameIsSurfaceScoped)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	RedAtlas atlas;
	scene.configure(atlas);
	const auto old = scene.capture();
	scene.producer->detachCamera();
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), old);
	scene.producer->setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::KeepLastFrame);
	EXPECT_NO_THROW(scene.engine.refreshRenderSnapshot());
	auto &first = gl.surface();
	spk::Window::Surface second{"last frame second"};
	spk::EngineView::Source last{spk::EngineView::Source::Kind::KeepLastFrame, scene.producer->output(), {}};
	EXPECT_THROW((void)last.onSurface(first), spk::Exception);
	auto frame = spk::Framebuffer::RenderAttachment::create2D(ImageSize, spk::Texture::Format::SRGB8_ALPHA8).texture();
	scene.producer->output()->publish(first, frame);
	EXPECT_EQ(last.onSurface(first).identifier(), frame.identifier());
	EXPECT_THROW((void)last.onSurface(second), spk::Exception);
	scene.producer->setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::DefaultTexture);
	EXPECT_THROW(scene.engine.refreshRenderSnapshot(), spk::Exception);
	scene.producer->setFallbackTexture(atlas.handle());
	EXPECT_NO_THROW(scene.engine.refreshRenderSnapshot());
	spk::EngineView::Source fallback{spk::EngineView::Source::Kind::DefaultTexture, scene.producer->output(), atlas.handle()};
	EXPECT_EQ(fallback.onSurface(second).identifier(), atlas.handle().identifier());
	gl.reset();
}

TEST(VoxelViewMaterialTest, FirstFrameDisplaysSecondaryViewThroughRealVoxelMeshing)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	RedAtlas atlas;
	Scene scene;
	scene.configure(atlas);
	drawMain(scene);
	ASSERT_TRUE(scene.producer->output()->latest(gl.surface()));
	ASSERT_TRUE(scene.consumer->output()->latest(gl.surface()));
	const auto image = gl.capture();
	const auto pixel = image.pixel({ImageSize.x / 2, ImageSize.y / 2});
	EXPECT_GT(pixel[0], pixel[2]);
	EXPECT_GT(pixel[0], 25u);
	EXPECT_EQ(pixel[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(VoxelViewMaterialTest, SourceSrgbIsDecodedOnceBeforeVoxelLighting)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	scene.producer->setBackgroundColor({1.0f, 1.0f, 1.0f});
	scene.add("cube", appearance(*scene.producer), "main");
	const auto snapshot = scene.capture();
	snapshot->execute(gl.renderContext());
	const auto final = scene.consumer->output()->latest(gl.surface());
	ASSERT_TRUE(final);
	spk::Framebuffer target(final->size(), std::vector<spk::Texture::Handle>{final}, std::nullopt);
	target.activate(gl.renderContext());
	std::vector<std::uint8_t> pixels(ImageSize.x * ImageSize.y * 4);
	glReadPixels(0, 0, ImageSize.x, ImageSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	int brightest = 0;
	for (std::size_t i = 0; i < pixels.size(); i += 4)
	{
		brightest = std::max(brightest, static_cast<int>(pixels[i]));
	}
	const float lightY = 0.85f / std::sqrt(0.35f * 0.35f + 0.85f * 0.85f + 0.45f * 0.45f);
	const float hdr = 0.5f * (0.38f + 0.62f * lightY);
	const float linear = hdr / (1.0f + hdr);
	const float encoded = (1.055f * std::pow(linear, 1.0f / 2.4f) - 0.055f) * 255.0f;
	EXPECT_NEAR(brightest, encoded, 6);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(VoxelViewMaterialTest, ResizePreservesExecutionOfHistoricalMaterialSnapshots)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	RedAtlas atlas;
	Scene scene;
	scene.configure(atlas);
	const auto old = scene.capture();
	old->execute(gl.renderContext());
	const auto initial = scene.producer->output()->latest(gl.surface());
	ASSERT_TRUE(initial);
	scene.producer->setResolution({80, 60});
	const auto updated = scene.capture();
	updated->execute(gl.renderContext());
	const auto resized = scene.producer->output()->latest(gl.surface());
	ASSERT_TRUE(resized);
	EXPECT_NE(initial.identifier(), resized.identifier());
	EXPECT_EQ(resized->size(), (spk::Vector2UInt{80, 60}));
	EXPECT_NO_THROW(old->execute(gl.renderContext()));
	EXPECT_EQ(scene.producer->output()->latest(gl.surface()).identifier(), initial.identifier());
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(VoxelViewMaterialGoldenTest, SecurityCameraVoxelScreen)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	RedAtlas atlas;
	Scene scene;
	scene.configure(atlas);
	drawMain(scene);
	saveGolden("security_camera_voxel_screen");
	gl.reset();
}

TEST(VoxelViewMaterialGoldenTest, SecurityCameraVoxelScreenAfterResolutionChange)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	RedAtlas atlas;
	Scene scene;
	scene.configure(atlas);
	scene.producer->setResolution({80, 60});
	drawMain(scene);
	saveGolden("security_camera_voxel_screen_resized");
	gl.reset();
}
