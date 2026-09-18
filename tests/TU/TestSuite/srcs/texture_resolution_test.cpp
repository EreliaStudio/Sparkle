#include "exception.hpp"
#include "graphics/sprite_sheet.hpp"
#include "ui/widget.hpp"
#include <gtest/gtest.h>

namespace
{
	class Pixels : public spk::Texture
	{
	public:
		explicit Pixels(spk::Vector2UInt size)
		{
			setPixels(nullptr, size, Format::RGBA);
		}
		void resize(spk::Vector2UInt size)
		{
			resizePixels(size);
		}
	};
}

TEST(TextureResolutionTest, SelectsLargestFittingVariantRegardlessOfInsertionOrder)
{
	Pixels image({32, 32});
	image.addResolution(Pixels({8, 8}));
	image.addResolution(Pixels({16, 16}));
	EXPECT_EQ(image.resolution(spk::Vector2UInt{18, 18}).size(), (spk::Vector2UInt{16, 16}));
	EXPECT_EQ(image.resolution(spk::Vector2UInt{12, 12}).size(), (spk::Vector2UInt{8, 8}));
	EXPECT_EQ(image.resolution(spk::Vector2UInt{40, 40}).handle().identifier(), image.handle().identifier());
	EXPECT_EQ(image.resolution(spk::Vector2UInt{4, 4}).handle().identifier(), image.handle().identifier());
	EXPECT_EQ(image.resolution([](const auto &texture) {
					   return texture.size().x == 8;
				   })
				  .size()
				  .x,
			  8u);
}

TEST(TextureResolutionTest, RejectsInvalidVariantsAndPreservesExistingOnFailure)
{
	Pixels image({32, 32});
	image.addResolution(Pixels({16, 16}));
	EXPECT_THROW(image.addResolution(Pixels({16, 16})), spk::Exception);
	EXPECT_THROW(image.addResolution(Pixels({8, 4})), spk::Exception);
	EXPECT_THROW(image.addResolution(image), spk::Exception);
	EXPECT_THROW(image.resolution(spk::Texture::ResolutionPredicate{}), spk::Exception);
	EXPECT_EQ(image.resolution(spk::Vector2UInt{18, 18}).size().x, 16u);
}

TEST(TextureResolutionTest, KeepsSingleTextureScalingAndClearsStaleVariants)
{
	Pixels image({32, 32});
	EXPECT_EQ(image.resolve(spk::Texture::Section::whole, {18, 18}).size, (spk::Vector2UInt{18, 18}));
	image.addResolution(Pixels({16, 16}));
	auto selected = image.resolve(spk::Texture::Section::whole, {18, 18});
	EXPECT_EQ(selected.size, (spk::Vector2UInt{16, 16}));
	EXPECT_GT(selected.texture.generation(), 0u);
	image.resize({64, 64});
	EXPECT_EQ(image.resolution(spk::Vector2UInt{18, 18}).handle().identifier(), image.handle().identifier());
	EXPECT_EQ(selected.texture->size(), (spk::Vector2UInt{16, 16}));
}

TEST(TextureResolutionTest, DefaultAtlasSelectsBySpriteDimensionsThroughTextureInterface)
{
	const auto &sheet = *spk::Widget::defaultStyle.get().iconset;
	const spk::Texture &texture = sheet;
	EXPECT_EQ(texture.resolution(spk::Vector2UInt{18, 18}).size(), (spk::Vector2UInt{160, 160}));
	EXPECT_EQ(texture.resolution(spk::Vector2UInt{12, 12}).size(), (spk::Vector2UInt{80, 80}));
	const auto selected = sheet.resolve(sheet.sprite(4), {18, 18});
	EXPECT_EQ(selected.size, (spk::Vector2UInt{16, 16}));
	EXPECT_GT(selected.texture.generation(), 0u);
	EXPECT_EQ(selected.texture->size(), (spk::Vector2UInt{160, 160}));
}

TEST(TextureResolutionTest, SharedFamilySurvivesOriginalAndAllowsSelectingLargerLevels)
{
	auto small = [] {
		Pixels original({32, 32});
		original.addResolution(Pixels({16, 16}));
		return original.resolution(spk::Vector2UInt{16, 16});
	}();
	auto large = small.resolution(spk::Vector2UInt{32, 32});
	EXPECT_EQ(large.size(), (spk::Vector2UInt{32, 32}));
	small.addResolution(Pixels({8, 8}));
	EXPECT_EQ(large.resolution(spk::Vector2UInt{8, 8}).size(), (spk::Vector2UInt{8, 8}));
	small.addResolution(Pixels({64, 64}));
	EXPECT_EQ(large.resolution(spk::Vector2UInt{64, 64}).size(), (spk::Vector2UInt{64, 64}));
}

TEST(TextureResolutionTest, CloneHasIndependentFamilyAndSelectedResource)
{
	Pixels original({32, 32});
	original.addResolution(Pixels({16, 16}));
	auto resource = original.clone();
	auto &copy = dynamic_cast<spk::Texture &>(*resource);
	EXPECT_NE(copy.handle().identifier(), original.handle().identifier());
	EXPECT_EQ(copy.resolution(spk::Vector2UInt{32, 32}).handle().identifier(), copy.handle().identifier());
	copy.addResolution(Pixels({8, 8}));
	EXPECT_EQ(original.resolution(spk::Vector2UInt{8, 8}).size(), (spk::Vector2UInt{32, 32}));
}
