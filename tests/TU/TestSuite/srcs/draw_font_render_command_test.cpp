#include <gtest/gtest.h>

#include <cstddef>
#include "rendering/command/draw_font_render_command.hpp"

static_assert(spk::DrawFontRenderCommand::AtlasSamplerBindingPoint == 0);
static_assert(spk::DrawFontRenderCommand::FontDataUBOBindingPoint == 1);
static_assert(sizeof(spk::DrawFontRenderCommand::FontRenderData) == 48);

TEST(DrawFontRenderCommandTest, FontRenderDataHasExpectedBindingLayout)
{
	EXPECT_EQ(spk::DrawFontRenderCommand::AtlasSamplerBindingPoint, 0u);
	EXPECT_EQ(spk::DrawFontRenderCommand::FontDataUBOBindingPoint, 1u);
	EXPECT_EQ(sizeof(spk::DrawFontRenderCommand::FontRenderData), 48u);
}

TEST(DrawFontRenderCommandTest, DISABLED_GlyphMeshSamplesAtlasAndRendersGlyphColor)
{
	GTEST_SKIP() << "Requires Font::Atlas creation, TextureMesh2D construction and offscreen pixel readback APIs not included in section 10.";
	// Intended assertion: deterministic glyph coverage samples the atlas and outputs the requested glyph color.
}

TEST(DrawFontRenderCommandTest, DISABLED_OutlineColorAndThicknessAffectRenderedPixels)
{
	GTEST_SKIP() << "Requires Font::Atlas creation, TextureMesh2D construction and offscreen pixel readback APIs not included in section 10.";
	// Intended assertion: outline-only pixels use outlineColor and respond to outlineThickness.
}

TEST(DrawFontRenderCommandTest, DISABLED_ExecutionUploadsFontUniformData)
{
	GTEST_SKIP() << "Requires RenderContext plus UniformBuffer/OpenGL buffer introspection from the main repository, which are not included in section 10.";
	// Intended assertion: the UBO at FontDataUBOBindingPoint contains glyphColor, outlineColor and outlineThickness.
}

TEST(DrawFontRenderCommandTest, DISABLED_ExecutionBindsAtlasSampler)
{
	GTEST_SKIP() << "Requires RenderContext/OpenGL sampler introspection and a real Font::Atlas, neither included in section 10.";
	// Intended assertion: AtlasSamplerBindingPoint references the supplied atlas.
}

TEST(DrawFontRenderCommandTest, DISABLED_GlyphMeshDepthParticipatesInDepthTesting)
{
	GTEST_SKIP() << "Requires a real Font::Atlas, TextureMesh2D and deterministic offscreen depth testing not included in section 10.";
	// Intended assertion: overlapping glyph meshes at different depths obey the established depth convention.
}

TEST(DrawFontRenderCommandTest, DISABLED_NullAtlasIsRejected)
{
	GTEST_SKIP() << "The null-atlas constructor also requires a TextureMesh2D value, but its construction API is a transitive dependency not included in section 10.";
	// Intended assertion: EXPECT_THROW(DrawFontRenderCommand(nullptr, validMesh, ...), std::invalid_argument).
}
