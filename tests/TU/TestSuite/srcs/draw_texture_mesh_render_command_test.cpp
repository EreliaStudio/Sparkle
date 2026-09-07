#include <gtest/gtest.h>

#include <GL/glew.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "rendering/command/draw_texture_mesh_render_command.hpp"
#include "render_command_test_utils.hpp"

namespace test = render_command_test;

static_assert(spk::DrawTextureMeshRenderCommand::TextureSamplerBindingPoint == 0);

TEST(DrawTextureMeshRenderCommandTest, TexturedMeshSamplesExpectedUVs)
{
	const std::array<std::uint8_t, 16> pixels{
		255, 0, 0, 255, 0, 255, 0, 255,
		0, 0, 255, 255, 255, 255, 0, 255};
	test::Texture texture({2, 2}, pixels);
	test::Target target;
	target.clear();
	spk::DrawTextureMeshRenderCommand(&texture, test::textureQuad({.anchor = {8, 8}, .size = {40, 40}})).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {14, 14}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
	EXPECT_EQ(test::pixel(image, {42, 14}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
	EXPECT_EQ(test::pixel(image, {14, 42}), (std::array<std::uint8_t, 4>{0, 0, 255, 255}));
	EXPECT_EQ(test::pixel(image, {42, 42}), (std::array<std::uint8_t, 4>{255, 255, 0, 255}));
}

TEST(DrawTextureMeshRenderCommandTest, TextureAlphaBlendsWithExistingColor)
{
	const std::array<std::uint8_t, 4> pixels{255, 0, 0, 128};
	test::Texture texture({1, 1}, pixels);
	test::Target target;
	target.clear({0, 0, 1, 1});
	spk::DrawTextureMeshRenderCommand(&texture, test::textureQuad({.anchor = {8, 8}, .size = {24, 24}})).execute(target.context());
	const auto actual = test::pixel(target.capture(), {12, 12});
	EXPECT_NEAR(actual[0], 128, 1);
	EXPECT_EQ(actual[1], 0);
	EXPECT_NEAR(actual[2], 127, 1);
	EXPECT_EQ(actual[3], 255);
}

TEST(DrawTextureMeshRenderCommandTest, TextureMeshDepthParticipatesInDepthTesting)
{
	const std::array<std::uint8_t, 4> redPixel{255, 0, 0, 255};
	const std::array<std::uint8_t, 4> bluePixel{0, 0, 255, 255};
	test::Texture red({1, 1}, redPixel);
	test::Texture blue({1, 1}, bluePixel);
	test::Target target;
	target.clear();
	const spk::Rect2D overlap{.anchor = {8, 8}, .size = {32, 32}};
	spk::DrawTextureMeshRenderCommand(&red, test::textureQuad(overlap, spk::Texture::Section::whole, 0.5f)).execute(target.context());
	spk::DrawTextureMeshRenderCommand(&blue, test::textureQuad(overlap, spk::Texture::Section::whole, -0.5f)).execute(target.context());
	EXPECT_EQ(test::pixel(target.capture(), {20, 20}), (std::array<std::uint8_t, 4>{255, 0, 0, 255}));
}

TEST(DrawTextureMeshRenderCommandTest, ExecutionBindsTextureSamplerAtReservedBindingPoint)
{
	const std::array<std::uint8_t, 4> pixels{255, 255, 255, 255};
	test::Texture texture({1, 1}, pixels);
	test::Target target;
	target.clear();
	spk::DrawTextureMeshRenderCommand(&texture, test::textureQuad({.anchor = {0, 0}, .size = {8, 8}})).execute(target.context());
	GLint sampler = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, spk::DrawTextureMeshRenderCommand::TextureSamplerBindingPoint, &sampler);
	glActiveTexture(GL_TEXTURE0 + spk::DrawTextureMeshRenderCommand::TextureSamplerBindingPoint);
	GLint textureName = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureName);
	EXPECT_NE(sampler, 0);
	EXPECT_NE(textureName, 0);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(DrawTextureMeshRenderCommandTest, SourceTextureLifetimeIsExplicitlyExercised)
{
	const std::array<std::uint8_t, 4> pixels{0, 255, 0, 255};
	test::Texture texture({1, 1}, pixels);
	test::Target target;
	target.clear();
	{
		spk::DrawTextureMeshRenderCommand command(&texture, test::textureQuad({.anchor = {4, 4}, .size = {16, 16}}));
		command.execute(target.context());
	}
	spk::DrawTextureMeshRenderCommand(&texture, test::textureQuad({.anchor = {32, 32}, .size = {16, 16}})).execute(target.context());
	const auto image = target.capture();
	EXPECT_EQ(test::pixel(image, {8, 8}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
	EXPECT_EQ(test::pixel(image, {36, 36}), (std::array<std::uint8_t, 4>{0, 255, 0, 255}));
}

TEST(DrawTextureMeshRenderCommandTest, NullTextureIsRejected)
{
	EXPECT_THROW(
		(void)spk::DrawTextureMeshRenderCommand(nullptr, spk::TextureMesh2D{}),
		std::invalid_argument);
}
