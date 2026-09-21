#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstdint>

#include "rendering/command/draw_color_mesh_render_command.hpp"
#include "render_command_test_utils.hpp"

namespace test = render_command_test;

TEST(DrawColorMeshRenderCommandTest, EmptyMeshProducesNoVisiblePixels)
{
	test::Target target;
	target.clear({0.2f, 0.4f, 0.6f, 1.0f});
	const auto before = target.capture();
	spk::DrawColorMeshRenderCommand(spk::ColorMesh2D{}).execute(target.context());
	EXPECT_EQ(target.capture().pixels, before.pixels);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(DrawColorMeshRenderCommandTest, SingleMeshRendersVertexColors)
{
	test::Target target;
	target.clear();
	spk::DrawColorMeshRenderCommand(test::colorQuad({.anchor = {8, 10}, .size = {20, 16}}, {1, 0, 0, 1})).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {12, 14}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {2, 2}), (std::array<std::uint8_t, 4>{0, 0, 0, 0}));
}

TEST(DrawColorMeshRenderCommandTest, MultipleMeshesRenderIndependently)
{
	test::Target target;
	target.clear();
	spk::DrawColorMeshRenderCommand(test::colorQuad({.anchor = {4, 4}, .size = {20, 20}}, {1, 0, 0, 1})).execute(target.context());
	spk::DrawColorMeshRenderCommand(test::colorQuad({.anchor = {36, 36}, .size = {20, 20}}, {0, 1, 0, 1})).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {10, 10}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {42, 42}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
}

TEST(DrawColorMeshRenderCommandTest, VertexAlphaBlendsWithExistingColor)
{
	test::Target target;
	target.clear({0, 0, 1, 1});
	spk::DrawColorMeshRenderCommand(test::colorQuad({.anchor = {8, 8}, .size = {24, 24}}, {1, 0, 0, 0.5f})).execute(target.context());
	const auto actual = test::pixel(target.capture(), {12, 12});
	EXPECT_NEAR(actual[0], 128, 1);
	EXPECT_EQ(actual[1], 0);
	EXPECT_NEAR(actual[2], 127, 1);
	EXPECT_EQ(actual[3], 255);
}

TEST(DrawColorMeshRenderCommandTest, VertexDepthParticipatesInDepthTesting)
{
	test::Target target;
	target.clear();
	const spk::Rect2D overlap{.anchor = {8, 8}, .size = {32, 32}};
	spk::DrawColorMeshRenderCommand(test::colorQuad(overlap, {1, 0, 0, 1}, 0.5f)).execute(target.context());
	spk::DrawColorMeshRenderCommand(test::colorQuad(overlap, {0, 0, 1, 1}, -0.5f)).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {20, 20}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(DrawColorMeshRenderCommandTest, RepeatedIndependentCommandsProduceStablePixelsAndRasterState)
{
	test::Target target({32, 16});
	spk::DrawColorMeshRenderCommand red(test::colorQuad({.anchor = {2, 2}, .size = {10, 10}}, {1, 0, 0, 1}));
	spk::DrawColorMeshRenderCommand green(test::colorQuad({.anchor = {20, 2}, .size = {10, 10}}, {0, 1, 0, 1}));
	const auto render = [&] {
		target.clear();
		red.execute(target.context());
		green.execute(target.context());
		return target.capture();
	};

	const auto first = render();
	const auto second = render();
	EXPECT_EQ(second.pixels, first.pixels);
	EXPECT_GT(test::countColor(first, {255, 0, 0, 255}), 0u);
	EXPECT_GT(test::countColor(first, {0, 255, 0, 255}), 0u);

	GLint activeFramebuffer = 0;
	GLint actualViewport[4]{};
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &activeFramebuffer);
	glGetIntegerv(GL_VIEWPORT, actualViewport);
	EXPECT_NE(activeFramebuffer, 0);
	EXPECT_EQ(actualViewport[0], 0);
	EXPECT_EQ(actualViewport[1], 0);
	EXPECT_EQ(actualViewport[2], 32);
	EXPECT_EQ(actualViewport[3], 16);
	EXPECT_TRUE(glIsEnabled(GL_BLEND));
	EXPECT_TRUE(glIsEnabled(GL_DEPTH_TEST));
	EXPECT_FALSE(glIsEnabled(GL_SCISSOR_TEST));
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}
