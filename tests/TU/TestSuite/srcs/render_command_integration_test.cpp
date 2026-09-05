#include <GL/glew.h>
#include <gtest/gtest.h>

#include "core/platform/window.hpp"
#include "rendering/command/clear_render_command.hpp"
#include "rendering/command/draw_color_mesh_render_command.hpp"
#include "rendering/command/draw_font_render_command.hpp"
#include "rendering/command/draw_texture_mesh_render_command.hpp"
#include "rendering/command/image_render_command.hpp"
#include "rendering/command/nine_slice_render_command.hpp"
#include "rendering/command/scissor_render_command.hpp"
#include "rendering/command/sprite_render_command.hpp"
#include "rendering/command/text_render_command.hpp"
#include "rendering/command/viewport_render_command.hpp"
#include "rendering/command/viewport_uniform_render_command.hpp"
#include "sparkle_test.hpp"
#include "ui/widget.hpp"

namespace
{
	const spk::Rect2D viewport{.anchor = {0, 0}, .size = {640, 480}};
	spk::TextureMesh2D textureQuad(float x, float y, float size, float depth = 0)
	{
		spk::TextureMesh2D::Builder builder;
		builder.addShape({{x, y}, depth, {0, 0}}, {{x, y + size}, depth, {0, 1}}, {{x + size, y + size}, depth, {1, 1}}, {{x + size, y}, depth, {1, 0}});
		return std::move(builder).build();
	}
	spk::ColorMesh2D colorQuad(float x, float y, float size, spk::Color color, float depth = 0)
	{
		spk::ColorMesh2D::Builder builder;
		builder.addShape({{x, y}, depth, color}, {{x, y + size}, depth, color}, {{x + size, y + size}, depth, color}, {{x + size, y}, depth, color});
		return std::move(builder).build();
	}
	void viewCommands(spk::RenderPass &pass)
	{
		pass.emplace<spk::ClearRenderCommand>(spk::Color{0, 0, 0, 0}, spk::ClearRenderCommand::Mask::All);
		pass.emplace<spk::ViewportRenderCommand>(viewport);
		pass.emplace<spk::ViewportUniformRenderCommand>(viewport);
		pass.emplace<spk::ScissorRenderCommand>(viewport);
	}
	spk::RenderSnapshot scene()
	{
		const auto &style = spk::Widget::defaultStyle.get();
		spk::RenderSnapshot::Builder builder;
		auto &pass = builder.renderPass({"all-command-families", 0});
		viewCommands(pass);
		pass.emplace<spk::DrawColorMeshRenderCommand>(colorQuad(20, 20, 80, {1, 0, 0, 1}));
		pass.emplace<spk::DrawTextureMeshRenderCommand>(style.iconsetImage.get(), textureQuad(120, 20, 80));
		pass.emplace<spk::ImageRenderCommand>(style.iconsetImage.get(), spk::Texture::Section::whole, spk::Rect2D{.anchor = {220, 20}, .size = {80, 80}});
		pass.emplace<spk::SpriteRenderCommand>(style.iconset.get(), spk::Vector2UInt{1, 0}, spk::Rect2D{.anchor = {320, 20}, .size = {80, 80}});
		pass.emplace<spk::NineSliceRenderCommand>(style.nineSlice.get(), spk::Rect2D{.anchor = {420, 20}, .size = {160, 80}}, spk::Vector2UInt{8, 8});
		pass.emplace<spk::TextRenderCommand>(style.font.get(), spk::Font::Size{32, 1}, "Sparkle", spk::TextRenderCommand::Anchor{{20, 140}, {spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top}}, spk::Color{1, 1, 1, 1}, spk::Color{0, 0, 1, 1});
		auto &atlas = style.font->atlas({48, 2});
		const auto &glyph = atlas[U'A'];
		spk::TextureMesh2D::Builder glyphMesh;
		const auto vertex = [&](unsigned i) {
			return spk::Texture2DVertex{{float(240 + glyph.positions[i].x), float(190 + glyph.positions[i].y)}, 0, glyph.uvs[i]};
		};
		glyphMesh.addShape(vertex(0), vertex(1), vertex(3), vertex(2));
		pass.emplace<spk::DrawFontRenderCommand>(&atlas, std::move(glyphMesh).build(), spk::Color{0, 1, 0, 1}, spk::Color{1, 0, 0, 1}, 0.1f);
		// Final mixed-depth overlap: the nearer blue color mesh must remain visible.
		pass.emplace<spk::DrawColorMeshRenderCommand>(colorQuad(320, 140, 80, {0, 0, 1, 1}, 0.5f));
		pass.emplace<spk::ImageRenderCommand>(style.nineSlice.get(), spk::Texture::Section::whole, spk::Rect2D{.anchor = {320, 140}, .size = {80, 80}}, -0.5f);
		return builder.build();
	}
}

TEST(RenderCommandIntegrationTest, EveryFamilyRendersOneSceneWithDeterministicDepthAndRepeatability)
{
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.reset();
	const auto snapshot = scene();
	snapshot.execute(context.renderContext());
	const auto first = context.capture();
	EXPECT_EQ(first.pixel({60, 60})[0], 255);
	EXPECT_EQ(first.pixel({350, 170})[2], 255);
	for (const auto bounds : {spk::Rect2D{.anchor = {120, 20}, .size = {80, 80}}, {.anchor = {220, 20}, .size = {80, 80}}, {.anchor = {320, 20}, .size = {80, 80}}, {.anchor = {420, 20}, .size = {160, 80}}, {.anchor = {20, 140}, .size = {150, 60}}, {.anchor = {230, 130}, .size = {70, 80}}})
	{
		unsigned visible = 0;
		for (unsigned y = bounds.y; y < bounds.y + bounds.height; ++y)
		{
			for (unsigned x = bounds.x; x < bounds.x + bounds.width; ++x)
			{
				visible += first.pixel({x, y})[3] != 0;
			}
		}
		EXPECT_GT(visible, 0u) << bounds.x;
	}
	context.reset();
	snapshot.execute(context.renderContext());
	EXPECT_EQ(context.capture().pixels, first.pixels);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderCommandIntegrationTest, MixedCommandsRebindProgramsTexturesAndPreserveConfiguredRasterState)
{
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.reset();
	auto &renderContext = context.renderContext();
	spk::ViewportRenderCommand(viewport).execute(renderContext);
	spk::ViewportUniformRenderCommand(viewport).execute(renderContext);
	const spk::Rect2D clip{.anchor = {10, 10}, .size = {100, 100}};
	spk::ScissorRenderCommand(clip).execute(renderContext);
	const auto &style = spk::Widget::defaultStyle.get();
	spk::DrawColorMeshRenderCommand color(colorQuad(20, 20, 40, {1, 0, 0, 1}));
	spk::ImageRenderCommand image(style.iconset.get(), style.iconset->sprite(1), {.anchor = {60, 20}, .size = {40, 40}});
	spk::TextRenderCommand text(style.font.get(), {20}, "A", {{20, 70}, {spk::Alignment::Horizontal::Left, spk::Alignment::Vertical::Top}}, {1, 1, 1, 1});
	GLint colorProgram = 0, imageProgram = 0, textProgram = 0;
	color.execute(renderContext);
	glGetIntegerv(GL_CURRENT_PROGRAM, &colorProgram);
	image.execute(renderContext);
	glGetIntegerv(GL_CURRENT_PROGRAM, &imageProgram);
	text.execute(renderContext);
	glGetIntegerv(GL_CURRENT_PROGRAM, &textProgram);
	EXPECT_NE(colorProgram, 0);
	EXPECT_NE(imageProgram, colorProgram);
	EXPECT_NE(textProgram, imageProgram);
	image.execute(renderContext);
	GLint rebound = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &rebound);
	EXPECT_EQ(rebound, imageProgram);
	color.execute(renderContext);
	glGetIntegerv(GL_CURRENT_PROGRAM, &rebound);
	EXPECT_EQ(rebound, colorProgram);
	GLint actualViewport[4]{}, scissor[4]{}, depth = 0, blend = 0, vao = 0;
	glGetIntegerv(GL_VIEWPORT, actualViewport);
	glGetIntegerv(GL_SCISSOR_BOX, scissor);
	glGetIntegerv(GL_DEPTH_FUNC, &depth);
	glGetIntegerv(GL_BLEND_SRC_RGB, &blend);
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
	EXPECT_EQ(actualViewport[2], 640);
	EXPECT_EQ(actualViewport[3], 480);
	EXPECT_EQ(scissor[0], 10);
	EXPECT_EQ(scissor[1], 370);
	EXPECT_EQ(scissor[2], 100);
	EXPECT_EQ(scissor[3], 100);
	EXPECT_TRUE(glIsEnabled(GL_BLEND));
	EXPECT_TRUE(glIsEnabled(GL_DEPTH_TEST));
	EXPECT_TRUE(glIsEnabled(GL_SCISSOR_TEST));
	EXPECT_EQ(depth, GL_LEQUAL);
	EXPECT_EQ(blend, GL_SRC_ALPHA);
	EXPECT_NE(vao, 0);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(RenderCommandIntegrationTest, SnapshotResourcesRecreateAfterReleaseAndInIndependentNativeContext)
{
	auto &shared = sparkle_test::OpenGLTestContext::instance();
	shared.reset();
	const auto snapshot = scene();
	snapshot.execute(shared.renderContext());
	const auto first = shared.capture();
	shared.surface()._gpuResources().clear();
	shared.reset();
	snapshot.execute(shared.renderContext());
	EXPECT_EQ(shared.capture().pixels, first.pixels);
	{
		spk::WinAPI::Window::Class windowClass("Sparkle_SecondaryRenderIntegrationContext");
		spk::Window::Native native("SecondaryNative");
		spk::Window::Surface surface("SecondarySurface");
		spk::WinAPI::Window::CreationInfo creation;
		creation.title = "Sparkle hidden integration test";
		creation.width = 640;
		creation.height = 480;
		creation.visible = false;
		native.window().create(windowClass, creation);
		surface.create(native.window());
		surface.setGeometry(viewport);
		surface.makeCurrent();
		spk::RenderContext context{.targetSurface = &surface};
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);
		snapshot.execute(context);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		GLint program = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &program);
		EXPECT_NE(program, 0);
		surface._gpuResources().clear();
		snapshot.execute(context);
		EXPECT_EQ(glGetError(), GL_NO_ERROR);
		surface.destroy();
		native.window().destroy();
	}
	shared.reset();
	snapshot.execute(shared.renderContext());
	EXPECT_EQ(shared.capture().pixels, first.pixels);
}

TEST(RenderCommandIntegrationTest, CompleteGoldenScene)
{
	auto &context = sparkle_test::OpenGLTestContext::instance();
	context.reset();
	const auto snapshot = scene();
	snapshot.execute(context.renderContext());
	const auto actual = sparkle_test::resultImagePath("rendering/integration", "complete_scene");
	const auto expected = sparkle_test::expectedImagePath("rendering/integration", "complete_scene");
	context.save(actual);
	ASSERT_TRUE(std::filesystem::exists(expected)) << "Missing golden: " << expected << "; rendered: " << actual;
	const auto result = sparkle_test::compareImages(actual, expected, sparkle_test::resultImagePath("rendering/integration", "complete_scene_difference"));
	EXPECT_TRUE(result.matches) << result.differentPixelCount << " pixels differ";
}
