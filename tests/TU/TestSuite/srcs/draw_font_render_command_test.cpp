#include <gtest/gtest.h>

#include <GL/glew.h>

#include <cstddef>
#include <cstdint>
#include <utility>

#include "rendering/command/draw_font_render_command.hpp"
#include "render_command_test_utils.hpp"
#include "ui/widget.hpp"

namespace test = render_command_test;

static_assert(spk::DrawFontRenderCommand::AtlasSamplerBindingPoint == 0);
static_assert(spk::DrawFontRenderCommand::FontDataUBOBindingPoint == 1);
static_assert(sizeof(spk::DrawFontRenderCommand::FontRenderData) == 48);

namespace
{
	spk::TextureMesh2D glyphMesh(const spk::Font::Glyph &glyph, spk::Vector2Int anchor, float depth = 0)
	{
		spk::TextureMesh2D::Builder builder;
		const auto vertex = [&](unsigned int index) {
			return spk::Texture2DVertex{
				{static_cast<float>(anchor.x + glyph.positions[index].x), static_cast<float>(anchor.y + glyph.positions[index].y)},
				depth,
				glyph.uvs[index]};
		};
		builder.addShape(vertex(0), vertex(1), vertex(3), vertex(2));
		return std::move(builder).build();
	}

	std::size_t countDominant(const sparkle_test::FramebufferImage &image, unsigned int channel)
	{
		std::size_t result = 0;
		for (std::size_t index = 0; index < image.pixels.size(); index += 4)
		{
			const auto value = image.pixels[index + channel];
			result += value > 32 && value > image.pixels[index + ((channel + 1) % 3)] * 2 && value > image.pixels[index + ((channel + 2) % 3)] * 2;
		}
		return result;
	}
}

TEST(DrawFontRenderCommandTest, FontRenderDataHasExpectedBindingLayout)
{
	EXPECT_EQ(spk::DrawFontRenderCommand::AtlasSamplerBindingPoint, 0u);
	EXPECT_EQ(spk::DrawFontRenderCommand::FontDataUBOBindingPoint, 1u);
	EXPECT_EQ(sizeof(spk::DrawFontRenderCommand::FontRenderData), 48u);
}

TEST(DrawFontRenderCommandTest, GlyphMeshSamplesAtlasAndRendersGlyphColor)
{
	auto &atlas = spk::Widget::defaultStyle->font->atlas({48, 4});
	const auto &glyph = atlas[U'A'];
	test::Target target({128, 96});
	target.clear();
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {24, 24}), {0, 1, 0, 1}, {1, 0, 0, 1}, 0).execute(target.context());
	const auto image = target.capture();
	EXPECT_GT(countDominant(image, 1), 0u);
	EXPECT_EQ(countDominant(image, 0), 0u);
}

TEST(DrawFontRenderCommandTest, OutlineColorAndThicknessAffectRenderedPixels)
{
	auto &atlas = spk::Widget::defaultStyle->font->atlas({48, 6});
	const auto &glyph = atlas[U'A'];
	test::Target target({128, 96});
	target.clear();
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {24, 24}), {0, 1, 0, 1}, {1, 0, 0, 1}, 0.2f).execute(target.context());
	const auto image = target.capture();
	EXPECT_GT(countDominant(image, 0), 0u);
	EXPECT_GT(countDominant(image, 1), 0u);
}

TEST(DrawFontRenderCommandTest, ExecutionUploadsFontUniformData)
{
	auto &atlas = spk::Widget::defaultStyle->font->atlas({32, 3});
	const auto &glyph = atlas[U'A'];
	test::Target target({96, 80});
	target.clear();
	const spk::Color glyphColor{0.1f, 0.2f, 0.3f, 0.4f};
	const spk::Color outlineColor{0.5f, 0.6f, 0.7f, 0.8f};
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {16, 16}), glyphColor, outlineColor, 0.125f).execute(target.context());
	GLint buffer = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, spk::DrawFontRenderCommand::FontDataUBOBindingPoint, &buffer);
	ASSERT_NE(buffer, 0);
	glBindBuffer(GL_UNIFORM_BUFFER, static_cast<GLuint>(buffer));
	spk::DrawFontRenderCommand::FontRenderData actual{};
	glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(actual), &actual);
	EXPECT_EQ(actual.glyphColor, glyphColor);
	EXPECT_EQ(actual.outlineColor, outlineColor);
	EXPECT_FLOAT_EQ(actual.outlineThickness, 0.125f);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(DrawFontRenderCommandTest, ExecutionBindsAtlasSampler)
{
	auto &atlas = spk::Widget::defaultStyle->font->atlas({32, 2});
	const auto &glyph = atlas[U'A'];
	test::Target target({96, 80});
	target.clear();
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {16, 16}), {1, 1, 1, 1}, {}, 0).execute(target.context());
	GLint sampler = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, spk::DrawFontRenderCommand::AtlasSamplerBindingPoint, &sampler);
	glActiveTexture(GL_TEXTURE0 + spk::DrawFontRenderCommand::AtlasSamplerBindingPoint);
	GLint texture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
	EXPECT_NE(sampler, 0);
	EXPECT_NE(texture, 0);
	EXPECT_EQ(glGetError(), GL_NO_ERROR);
}

TEST(DrawFontRenderCommandTest, GlyphMeshDepthParticipatesInDepthTesting)
{
	auto &atlas = spk::Widget::defaultStyle->font->atlas({48, 4});
	const auto &glyph = atlas[U'A'];
	test::Target target({128, 96});
	target.clear();
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {24, 24}, 0.5f), {1, 0, 0, 1}, {}, 0).execute(target.context());
	const auto front = target.capture();
	spk::DrawFontRenderCommand(&atlas, glyphMesh(glyph, {24, 24}, -0.5f), {0, 0, 1, 1}, {}, 0).execute(target.context());
	EXPECT_EQ(target.capture().pixels, front.pixels);
}

TEST(DrawFontRenderCommandTest, NullAtlasIsRejected)
{
	EXPECT_THROW(
		(void)spk::DrawFontRenderCommand(nullptr, spk::TextureMesh2D{}, spk::Color{}, spk::Color{}, 0.0f),
		std::invalid_argument);
}
