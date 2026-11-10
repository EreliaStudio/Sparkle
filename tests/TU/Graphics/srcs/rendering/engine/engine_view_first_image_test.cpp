#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "core/context/render_context.hpp"
#include "engine/entity3d.hpp"
#include "exception.hpp"
#include "graphics/opengl/framebuffer.hpp"
#include "graphics/opengl/state.hpp"
#include "rendering/engine/camera_component3d.hpp"
#include "rendering/engine/engine.hpp"
#include "rendering/engine/rendering_behaviour.hpp"
#include "rendering/render_command.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/engine_widget.hpp"

namespace
{
	constexpr spk::Vector2UInt ImageSize{64, 64};

	struct HDRClear final : spk::RenderCommand
	{
		float intensity;

		explicit HDRClear(float value) :
			intensity(value)
		{
		}

		void execute(spk::RenderContext &) const override
		{
			const GLfloat value[]{intensity, intensity, intensity, 1.0f};
			glClearBufferfv(GL_COLOR, 0, value);
		}
	};

	struct HDRContributor final : spk::RenderingBehaviour
	{
		float value = 0.0f;

		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &) override
		{
			builder.renderPass(spk::EngineView::OpaquePassKey).emplace<HDRClear>(value);
		}
	};

	struct KnownWBOITBuffers final : spk::RenderCommand
	{
		void execute(spk::RenderContext &) const override
		{
			const GLfloat accumulated[]{0.5f, 0.0f, 0.0f, 0.25f};
			const GLfloat revealage[]{0.75f, 0.75f, 0.75f, 0.75f};
			glClearBufferfv(GL_COLOR, 0, accumulated);
			glClearBufferfv(GL_COLOR, 1, revealage);
		}
	};

	struct KnownWBOITContributor final : spk::RenderingBehaviour
	{
		void buildViewRenderSnapshot(
			spk::RenderSnapshot::Builder &builder,
			const spk::EngineView::CaptureContext &) override
		{
			builder.renderPass(spk::EngineView::OpaquePassKey).emplace<HDRClear>(1.0f);
			builder.renderPass(spk::EngineView::WBOITPassKey).emplace<KnownWBOITBuffers>();
		}
	};

	struct Scene
	{
		spk::Engine engine;
		spk::Entity3D cameraEntity{"camera"};
		spk::CameraComponent3D &camera;
		spk::EngineView &view;

		Scene() :
			camera(cameraEntity.addComponent<spk::CameraComponent3D>()),
			view(_initialize())
		{
		}

		spk::EngineView &_initialize()
		{
			engine.addEntity(&cameraEntity);
			return engine.addView("main", &camera, ImageSize);
		}

		~Scene()
		{
			engine.removeEntity(&cameraEntity);
		}
	};

	void prepare(spk::EngineWidget &widget, spk::Engine &engine, spk::Rect2D geometry)
	{
		widget.setEngine(&engine);
		widget.setGeometry(geometry);
		widget.activate();
	}

	[[nodiscard]] spk::RenderSnapshot capture(spk::EngineWidget &widget)
	{
		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		return builder.build();
	}

	[[nodiscard]] std::array<std::uint8_t, 4> readFinal(spk::EngineView &view)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		auto handle = view.output()->latest(gl.surface());
		if (!handle)
		{
			throw std::runtime_error("No final EngineView texture on test surface");
		}
		spk::Framebuffer target(handle->size(), std::vector<spk::Texture::Handle>{handle}, std::nullopt);
		target.activate(gl.renderContext());
		std::array<std::uint8_t, 4> pixel{};
		glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
		return pixel;
	}

	[[nodiscard]] int encodedReinhard(float hdr, float exposure)
	{
		const double x = static_cast<double>(hdr) * exposure;
		const double linear = x / (1.0 + x);
		const double srgb = linear <= 0.0031308
								? linear * 12.92
								: 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
		return static_cast<int>(std::round(srgb * 255.0));
	}

	void expectGolden(const std::string &name)
	{
		auto &gl = sparkle_test::OpenGLTestContext::instance();
		const auto category = std::filesystem::path("rendering/engine/empty_view");
		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto diff = sparkle_test::resultImagePath(category, name + "_difference");
		gl.save(actual);
		ASSERT_TRUE(std::filesystem::exists(expected)) << expected;
		const auto comparison = sparkle_test::compareImages(actual, expected, diff);
		EXPECT_TRUE(comparison.matches) << "Pixel differences: " << comparison.differentPixelCount;
	}
}

TEST(EngineViewFirstImageTest, BlackEmptyViewProducesOpaquePixelsAndGolden)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	spk::EngineWidget widget("view", nullptr);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(widget).execute(gl.renderContext());
	const auto image = gl.capture();
	const auto pixel = image.pixel({32, 32});
	EXPECT_EQ(pixel[0], 0);
	EXPECT_EQ(pixel[1], 0);
	EXPECT_EQ(pixel[2], 0);
	EXPECT_EQ(pixel[3], 255);
	expectGolden("black");
}

TEST(EngineViewFirstImageTest, CustomBackgroundAndExposureProduceOpaqueGolden)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	scene.view.setBackgroundColor({1.0f, 0.0f, 0.0f});
	scene.view.setExposure(1.0e10f);
	spk::EngineWidget widget("view", nullptr);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(widget).execute(gl.renderContext());
	const auto image = gl.capture();
	const auto pixel = image.pixel({32, 32});
	EXPECT_EQ(pixel[0], 255);
	EXPECT_EQ(pixel[1], 0);
	EXPECT_EQ(pixel[2], 0);
	EXPECT_EQ(pixel[3], 255);
	expectGolden("custom_red");
}

TEST(EngineViewFirstImageTest, ReinhardMatchesReferenceHDRValuesOnGPU)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	auto &contributor = scene.engine.root().addBehaviour<HDRContributor>();
	spk::EngineWidget widget("render", nullptr, spk::EngineWidget::Type::Render);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	const std::array<std::pair<float, float>, 5> cases{{{0.0f, 1.0f}, {1.0f, 1.0f}, {2.0f, 1.0f}, {4.0f, 1.0f}, {2.0f, 2.0f}}};
	for (const auto &[hdr, exposure] : cases)
	{
		contributor.value = hdr;
		scene.view.setExposure(exposure);
		capture(widget).execute(gl.renderContext());
		const auto color = readFinal(scene.view);
		EXPECT_NEAR(color[0], encodedReinhard(hdr, exposure), 1);
		EXPECT_NEAR(color[1], encodedReinhard(hdr, exposure), 1);
		EXPECT_NEAR(color[2], encodedReinhard(hdr, exposure), 1);
		EXPECT_EQ(color[3], 255);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
	}
	gl.reset();
}

TEST(EngineViewFirstImageTest, RenderOnlyAndDrawOnlyAreIndependentOnOneSurface)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	scene.view.setBackgroundColor({1.0f, 0.0f, 0.0f});
	scene.view.setExposure(1.0e10f);
	spk::EngineWidget producer("producer", nullptr, spk::EngineWidget::Type::Render);
	prepare(producer, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(producer).execute(gl.renderContext());
	ASSERT_TRUE(scene.view.output()->latest(gl.surface()));
	gl.reset(ImageSize);
	spk::EngineWidget consumer("consumer", nullptr, spk::EngineWidget::Type::Draw);
	prepare(consumer, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	const auto published = scene.engine.latestRenderSnapshot();
	capture(consumer).execute(gl.renderContext());
	EXPECT_EQ(scene.engine.latestRenderSnapshot(), published);
	const auto image = gl.capture();
	const auto pixel = image.pixel({32, 32});
	EXPECT_EQ(pixel[0], 255);
	EXPECT_EQ(pixel[3], 255);
}

TEST(EngineViewFirstImageTest, ResolutionChangesKeepOlderRenderSnapshotsAndHandles)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	spk::EngineWidget widget("producer", nullptr, spk::EngineWidget::Type::Render);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	auto oldSnapshot = capture(widget);
	oldSnapshot.execute(gl.renderContext());
	const auto previous = scene.view.output()->latest(gl.surface());
	ASSERT_TRUE(previous);
	EXPECT_EQ(previous->size(), ImageSize);
	scene.view.setResolution({32, 32});
	auto newSnapshot = capture(widget);
	newSnapshot.execute(gl.renderContext());
	const auto updated = scene.view.output()->latest(gl.surface());
	ASSERT_TRUE(updated);
	EXPECT_NE(previous.identifier(), updated.identifier());
	EXPECT_EQ(updated->size(), (spk::Vector2UInt{32, 32}));
	oldSnapshot.execute(gl.renderContext());
	EXPECT_EQ(scene.view.output()->latest(gl.surface()).identifier(), previous.identifier());
	gl.reset();
}

TEST(EngineViewFirstImageTest, DetachedOutputPoliciesAndAbsentView)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	spk::EngineWidget widget("producer", nullptr, spk::EngineWidget::Type::Render);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(widget).execute(gl.renderContext());
	const auto previous = scene.view.output()->latest(gl.surface());
	ASSERT_TRUE(previous);
	scene.view.detachCamera();
	EXPECT_FALSE(scene.view.output()->latest(gl.surface()));
	scene.view.setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::KeepLastFrame);
	EXPECT_EQ(scene.view.output()->latest(gl.surface()).identifier(), previous.identifier());
	auto emptyOutput = std::make_shared<spk::EngineView::Output>();
	emptyOutput->setDetached(true, spk::EngineView::DetachedOutputPolicy::KeepLastFrame, {});
	EXPECT_THROW((void)emptyOutput->latest(gl.surface()), spk::Exception);
	const auto fallback = spk::Framebuffer::RenderAttachment::create2D(ImageSize, spk::Texture::Format::SRGB8_ALPHA8).texture();
	scene.view.setFallbackTexture(fallback);
	scene.view.setDetachedOutputPolicy(spk::EngineView::DetachedOutputPolicy::DefaultTexture);
	EXPECT_EQ(scene.view.output()->latest(gl.surface()).identifier(), fallback.identifier());
	spk::EngineWidget missing("missing", nullptr, spk::EngineWidget::Type::Draw);
	prepare(missing, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	missing.setViewName("absent");
	EXPECT_NO_THROW(capture(missing).execute(gl.renderContext()));
	EXPECT_THROW((void)scene.engine.view("absent"), spk::Exception);
	gl.reset();
}

TEST(EngineViewFirstImageTest, RenderingRestoresUIRasterAndFramebufferState)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	spk::EngineWidget widget("producer", nullptr, spk::EngineWidget::Type::Render);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	glViewport(3, 4, 21, 19);
	glScissor(5, 6, 11, 12);
	glEnable(GL_SCISSOR_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE);
	scene.engine.refreshRenderSnapshot();
	spk::RenderSnapshot::Builder builder;
	scene.engine.buildRenderSnapshot(builder);
	builder.build().execute(gl.renderContext());
	std::array<GLint, 4> viewport{}, scissor{};
	std::array<GLboolean, 4> mask{};
	glGetIntegerv(GL_VIEWPORT, viewport.data());
	glGetIntegerv(GL_SCISSOR_BOX, scissor.data());
	glGetBooleanv(GL_COLOR_WRITEMASK, mask.data());
	EXPECT_EQ(viewport, (std::array<GLint, 4>{3, 4, 21, 19}));
	EXPECT_EQ(scissor, (std::array<GLint, 4>{5, 6, 11, 12}));
	EXPECT_EQ(glIsEnabled(GL_SCISSOR_TEST), GL_TRUE);
	EXPECT_EQ(glIsEnabled(GL_DEPTH_TEST), GL_FALSE);
	EXPECT_EQ(glIsEnabled(GL_CULL_FACE), GL_FALSE);
	EXPECT_EQ(glIsEnabled(GL_BLEND), GL_FALSE);
	EXPECT_EQ(mask, (std::array<GLboolean, 4>{GL_TRUE, GL_FALSE, GL_TRUE, GL_FALSE}));
	gl.reset();
}

TEST(EngineViewFirstImageTest, SRGBFinalTextureAndLinearUIPresentationMatch)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	scene.view.setBackgroundColor({1.0f, 1.0f, 1.0f});
	scene.view.setExposure(1.0f);
	spk::EngineWidget widget("srgb", nullptr);
	prepare(widget, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(widget).execute(gl.renderContext());

	const auto displayed = gl.capture();
	const auto finalPixel = readFinal(scene.view);
	const int expected = encodedReinhard(1.0f, 1.0f);
	for (int channel = 0; channel < 3; ++channel)
	{
		EXPECT_NEAR(finalPixel[channel], expected, 1);
		EXPECT_NEAR(displayed.pixel({32, 32})[channel], expected, 2);
	}
	EXPECT_EQ(displayed.pixel({32, 32})[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(OpenGLStateTest, ScopedStateRestoresCustomizedRasterAndBlendConfiguration)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	using State = spk::OpenGL::State;
	auto original = State::capture();
	{
		State::Scope restore;
		auto depth = State::DepthTest{};
		depth.comparison = State::Comparison::Greater;
		depth.write = false;
		depth.clear = 0.25;
		depth.apply();
		auto blending = State::Blend::disabled();
		blending.attachments[0].enabled = true;
		blending.attachments[0].equationRGB = State::BlendEquation::Maximum;
		blending.apply();
		auto changed = State::capture();
		EXPECT_EQ(changed.depthTest.comparison, State::Comparison::Greater);
		EXPECT_FALSE(changed.depthTest.write);
		EXPECT_EQ(changed.blend.attachments[0].equationRGB, State::BlendEquation::Maximum);
	}
	const auto restored = State::capture();
	EXPECT_EQ(restored.depthTest.comparison, original.depthTest.comparison);
	EXPECT_EQ(restored.depthTest.write, original.depthTest.write);
	EXPECT_EQ(restored.blend.attachments[0].equationRGB, original.blend.attachments[0].equationRGB);
	EXPECT_EQ(restored.blend.attachments[0].enabled, original.blend.attachments[0].enabled);
	EXPECT_EQ(restored.bindings.drawFramebuffer, original.bindings.drawFramebuffer);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(EngineViewFirstImageTest, HDRAndWBOITComposeBeforeSingleReinhard)
{
	auto &gl = sparkle_test::OpenGLTestContext::instance();
	gl.reset(ImageSize);
	Scene scene;
	(void)scene.engine.root().addBehaviour<KnownWBOITContributor>();
	spk::EngineWidget producer("wboit", nullptr, spk::EngineWidget::Type::Render);
	prepare(producer, scene.engine, {.anchor = {0, 0}, .size = ImageSize});
	capture(producer).execute(gl.renderContext());
	const auto pixel = readFinal(scene.view);
	EXPECT_NEAR(pixel[0], encodedReinhard(1.25f, 1.0f), 2);
	EXPECT_NEAR(pixel[1], encodedReinhard(0.75f, 1.0f), 2);
	EXPECT_NEAR(pixel[2], encodedReinhard(0.75f, 1.0f), 2);
	EXPECT_EQ(pixel[3], 255);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
	gl.reset();
}

TEST(EngineViewFirstImageTest, ReusesIntermediateTargetsOnlyWhenNoSnapshotLeasesThem)
{
	Scene scene;
	auto output = scene.view.output();
	auto first = output->acquireTargets(ImageSize);
	const auto original = first->opaque.identifier();
	auto concurrent = output->acquireTargets(ImageSize);
	EXPECT_NE(concurrent->opaque.identifier(), original);
	first.reset();
	auto reused = output->acquireTargets(ImageSize);
	EXPECT_EQ(reused->opaque.identifier(), original);
	const auto differentResolution = output->acquireTargets({32, 32});
	EXPECT_NE(differentResolution->opaque.identifier(), reused->opaque.identifier());
}
