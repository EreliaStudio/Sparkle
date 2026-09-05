#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "graphics/sprite_sheet.hpp"
#include "rendering/render_snapshot.hpp"
#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"
#include "sparkle_test/paths.hpp"
#include "ui/widget/image_label.hpp"

namespace
{
	void expectWidgetImage(spk::Widget &widget, const std::filesystem::path &category, const std::string &name)
	{
		auto &context = sparkle_test::OpenGLTestContext::instance();
		context.reset();

		spk::RenderSnapshot::Builder builder;
		widget.buildRenderSnapshot(builder);
		spk::RenderSnapshot snapshot = builder.build();
		snapshot.execute(context.renderContext());

		const auto actual = sparkle_test::resultImagePath(category, name);
		const auto expected = sparkle_test::expectedImagePath(category, name);
		const auto difference = sparkle_test::resultImagePath(category, name + "_difference");
		context.save(actual);

		ASSERT_TRUE(std::filesystem::exists(expected))
			<< "Missing golden image: " << expected << "\n"
			<< "The current render was saved to: " << actual;

		const sparkle_test::ImageComparisonResult result =
			sparkle_test::compareImages(actual, expected, difference);
		EXPECT_TRUE(result.matches)
			<< "Image mismatch for [" << category.string() << "/" << name << "]\n"
			<< "Different pixels: " << result.differentPixelCount << "\n"
			<< "Actual size: " << result.actualWidth << "x" << result.actualHeight << "\n"
			<< "Expected size: " << result.expectedWidth << "x" << result.expectedHeight << "\n"
			<< "Difference image: " << difference;
	}

	[[nodiscard]] const spk::Widget::Style &defaultStyle()
	{
		return spk::Widget::defaultStyle.get();
	}
}

TEST(ImageLabelTest, DefaultState)
{
	spk::ImageLabel label("Image");
	EXPECT_EQ(label.texture(), defaultStyle().iconsetImage.get());
	EXPECT_EQ(label.section(), spk::Texture::Section::whole);
	EXPECT_FLOAT_EQ(label.depth(), 0.0f);
}

TEST(ImageLabelTest, TextureConstructorAndSetterRoundTrip)
{
	const spk::Texture *texture = defaultStyle().iconset.get();
	spk::ImageLabel fromConstructor("Image", texture);
	EXPECT_EQ(fromConstructor.texture(), texture);
	spk::ImageLabel fromSetter("Image");
	fromSetter.setTexture(texture);
	EXPECT_EQ(fromSetter.texture(), texture);
}

TEST(ImageLabelTest, ApplyStyleUsesStyleImage)
{
	spk::ImageLabel label("Image");
	label.applyStyle(defaultStyle());
	EXPECT_NE(label.texture(), nullptr);
}

TEST(ImageLabelTest, SectionAndDepthRoundTrip)
{
	spk::ImageLabel label("Image", defaultStyle().iconset.get());
	const auto section = defaultStyle().iconset->sprite(12);
	label.setSection(section);
	label.setDepth(4.25f);
	EXPECT_EQ(label.section(), section);
	EXPECT_FLOAT_EQ(label.depth(), 4.25f);
}

TEST(ImageLabelTest, NullTextureIsRejectedWithoutChangingCurrentTexture)
{
	const spk::Texture *texture = defaultStyle().iconset.get();
	spk::ImageLabel label("Image", texture);
	EXPECT_THROW(label.setTexture(nullptr), std::invalid_argument);
	EXPECT_EQ(label.texture(), texture);
}

TEST(ImageLabelTest, EmptyEdgeAndOutOfRangeSectionsAreStoredVerbatim)
{
	spk::ImageLabel label("Image", defaultStyle().iconset.get());
	const std::vector<spk::Texture::Section> sections{
		{{0.0f, 0.0f}, {0.0f, 0.0f}},
		{{1.0f, 1.0f}, {1.0f, 1.0f}},
		{{-1.0f, -2.0f}, {3.0f, 4.0f}}};
	for (const auto &section : sections)
	{
		label.setSection(section);
		EXPECT_EQ(label.section(), section);
	}
}

TEST(ImageLabelRenderTest, DISABLED_WholeTexture)
{
	spk::ImageLabel label("Image", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {60, 50}, .size = {240, 180}});
	label.activate();
	expectWidgetImage(label, "ui/widget/image_label", "whole_texture");
}

TEST(ImageLabelRenderTest, DISABLED_SpriteSection)
{
	spk::ImageLabel label("Image", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {60, 50}, .size = {180, 180}});
	label.setSection(defaultStyle().iconset->sprite(12));
	label.activate();
	expectWidgetImage(label, "ui/widget/image_label", "sprite_section");
}

TEST(ImageLabelRenderTest, DISABLED_WideGeometry)
{
	spk::ImageLabel label("Image", defaultStyle().iconset.get());
	label.setGeometry({.anchor = {40, 40}, .size = {340, 100}});
	label.setSection(defaultStyle().iconset->sprite(3));
	label.activate();
	expectWidgetImage(label, "ui/widget/image_label", "wide_geometry");
}
