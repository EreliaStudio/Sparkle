#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <vector>

#include "graphics/image.hpp"
#include "graphics/sprite_sheet.hpp"

namespace
{
	std::vector<std::uint8_t> makePnm(
		char kind,
		unsigned int width,
		unsigned int height,
		std::span<const std::uint8_t> pixels)
	{
		const std::string header = std::string("P") + kind + "\n" + std::to_string(width) + " " + std::to_string(height) + "\n255\n";
		std::vector<std::uint8_t> result(header.begin(), header.end());
		result.insert(result.end(), pixels.begin(), pixels.end());
		return result;
	}

	std::filesystem::path temporaryImagePath(const char *name)
	{
		return std::filesystem::temp_directory_path() / (std::string("sparkle_tu_") + name);
	}
}

TEST(ImageTest, LoadsKnownRGBPixelsFromEncodedBytesAndDoesNotBorrowInput)
{
	const std::vector<std::uint8_t> expected{255, 0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255};
	auto encoded = makePnm('6', 2, 2, expected);
	spk::Image image(encoded);
	encoded.assign(encoded.size(), 0);

	EXPECT_EQ(image.size(), spk::Vector2UInt(2, 2));
	EXPECT_EQ(image.format(), spk::Texture::Format::RGB);
	EXPECT_EQ(image.contentSource(), spk::Texture::ContentSource::PixelData);
	EXPECT_EQ(image.mipmap(), spk::Texture::Mipmap::Enable);
	EXPECT_EQ(image.pixels(), expected);
}

TEST(ImageTest, LoadsGrayscaleTinyImageWithoutExpandingChannels)
{
	const std::vector<std::uint8_t> expected{0, 127, 255};
	spk::Image image(makePnm('5', 3, 1, expected));
	EXPECT_EQ(image.size(), spk::Vector2UInt(3, 1));
	EXPECT_EQ(image.format(), spk::Texture::Format::GreyLevel);
	EXPECT_EQ(image.pixels(), expected);
}

TEST(ImageTest, FileAndMemoryLoadingProduceTheSameImage)
{
	const std::vector<std::uint8_t> expected{1, 2, 3, 4, 5, 6};
	const auto encoded = makePnm('6', 2, 1, expected);
	const auto path = temporaryImagePath("source.ppm");
	{
		std::ofstream stream(path, std::ios::binary);
		stream.write(reinterpret_cast<const char *>(encoded.data()), static_cast<std::streamsize>(encoded.size()));
	}

	const spk::Image fromMemory(encoded);
	const spk::Image fromFile = spk::Image::open(path);
	std::filesystem::remove(path);
	EXPECT_EQ(fromFile.size(), fromMemory.size());
	EXPECT_EQ(fromFile.format(), fromMemory.format());
	EXPECT_EQ(fromFile.pixels(), fromMemory.pixels());
}

TEST(ImageTest, PngExportRoundTripsDimensionsFormatAndPixels)
{
	const std::vector<std::uint8_t> expected{10, 20, 30, 40, 50, 60};
	const spk::Image original(makePnm('6', 2, 1, expected));
	const auto path = temporaryImagePath("round_trip.png");
	original.saveAsPng(path);
	const spk::Image loaded = spk::Image::open(path);
	std::filesystem::remove(path);

	EXPECT_EQ(loaded.size(), original.size());
	EXPECT_EQ(loaded.format(), spk::Texture::Format::RGB);
	EXPECT_EQ(loaded.pixels(), expected);
}

TEST(ImageTest, MoveConstructionPreservesDecodedContent)
{
	const std::vector<std::uint8_t> expected{9, 8, 7};
	spk::Image source(makePnm('6', 1, 1, expected));
	spk::Image moved(std::move(source));
	EXPECT_EQ(moved.size(), spk::Vector2UInt(1, 1));
	EXPECT_EQ(moved.pixels(), expected);
}

TEST(ImageTest, EmptyMissingUndecodableAndTruncatedInputsAreRejected)
{
	EXPECT_THROW(spk::Image(std::span<const std::uint8_t>{}), std::invalid_argument);
	EXPECT_THROW(spk::Image(std::vector<std::uint8_t>{1, 2, 3, 4}), std::runtime_error);
	auto truncated = makePnm('6', 2, 2, std::vector<std::uint8_t>{1, 2, 3});
	EXPECT_THROW((void)spk::Image{truncated}, std::runtime_error);
	EXPECT_THROW((void)spk::Image::open(temporaryImagePath("missing.file")), std::runtime_error);
}

TEST(ImageTest, DISABLED_OversizedEncodedSpanNeedsSyntheticAddressSpaceSeam)
{
	GTEST_SKIP() << "A span larger than INT_MAX requires a genuinely addressable multi-gigabyte range; Image exposes no decoder-size seam.";
}

TEST(ImageTest, DISABLED_DualChannelAndRGBAFixturesNeedDeterministicEncodedAssets)
{
	GTEST_SKIP() << "The repository currently provides no deterministic two-channel or transparent encoded image fixture.";
}

TEST(SpriteSheetTest, NonSquareGridMapsCoordinatesIDsAndPreciseSections)
{
	const auto encoded = makePnm('6', 5, 4, std::vector<std::uint8_t>(5 * 4 * 3, 42));
	spk::SpriteSheet sheet(encoded, {3, 2});
	EXPECT_EQ(sheet.nbSprite(), spk::Vector2UInt(3, 2));
	EXPECT_FLOAT_EQ(sheet.unit().x, 1.0f / 3.0f);
	EXPECT_FLOAT_EQ(sheet.unit().y, 0.5f);
	ASSERT_EQ(sheet.sprites().size(), 6u);
	EXPECT_EQ(sheet.spriteID({0, 0}), 0u);
	EXPECT_EQ(sheet.spriteID({2, 1}), 5u);
	EXPECT_EQ(sheet.sprite({2, 1}), sheet.sprite(5));
	EXPECT_FLOAT_EQ(sheet.sprite(5).anchor.x, 2.0f / 3.0f);
	EXPECT_FLOAT_EQ(sheet.sprite(5).anchor.y, 0.5f);
	EXPECT_EQ(sheet.sprite(5).size, sheet.unit());
}

TEST(SpriteSheetTest, OneByOneGridReturnsTheWholeTextureSection)
{
	const auto encoded = makePnm('5', 1, 1, std::vector<std::uint8_t>{99});
	spk::SpriteSheet sheet(encoded, {1, 1});
	EXPECT_EQ(sheet.sprites().size(), 1u);
	EXPECT_EQ(sheet.sprite(0), spk::Texture::Section::whole);
}

TEST(SpriteSheetTest, ImageDimensionsNeedNotBeDivisibleByLogicalGrid)
{
	const auto encoded = makePnm('6', 5, 7, std::vector<std::uint8_t>(5 * 7 * 3, 7));
	spk::SpriteSheet sheet(encoded, {2, 3});
	EXPECT_EQ(sheet.size(), spk::Vector2UInt(5, 7));
	EXPECT_EQ(sheet.sprites().size(), 6u);
	EXPECT_EQ(sheet.unit(), spk::Vector2(0.5f, 1.0f / 3.0f));
}

TEST(SpriteSheetTest, ZeroGridDimensionsAreRejected)
{
	const auto encoded = makePnm('5', 1, 1, std::vector<std::uint8_t>{0});
	EXPECT_THROW(spk::SpriteSheet(encoded, {0, 1}), std::invalid_argument);
	EXPECT_THROW(spk::SpriteSheet(encoded, {1, 0}), std::invalid_argument);
	EXPECT_THROW(spk::SpriteSheet(encoded, {0, 0}), std::invalid_argument);
}

TEST(SpriteSheetTest, CoordinatesAndLinearIDsAtEveryBoundaryAreRejected)
{
	const auto encoded = makePnm('5', 1, 1, std::vector<std::uint8_t>{0});
	spk::SpriteSheet sheet(encoded, {2, 3});
	EXPECT_THROW((void)sheet.spriteID({2, 0}), std::out_of_range);
	EXPECT_THROW((void)sheet.spriteID({0, 3}), std::out_of_range);
	EXPECT_THROW((void)sheet.sprite({2, 3}), std::out_of_range);
	EXPECT_THROW((void)sheet.sprite(6), std::out_of_range);
	EXPECT_THROW((void)sheet.sprite(static_cast<std::size_t>(-1)), std::out_of_range);
}
