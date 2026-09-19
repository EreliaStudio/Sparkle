#include <filesystem>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>
#include <stb_image.h>
#include <stb_image_write.h>

#include "sparkle_test/image_comparison.hpp"

namespace
{
	[[nodiscard]] std::filesystem::path imageComparisonTempDirectory()
	{
		std::filesystem::path result = std::filesystem::temp_directory_path() / "sparkle_image_comparison_tests";
		std::filesystem::create_directories(result);
		return result;
	}

	[[nodiscard]] std::filesystem::path imageComparisonExpectedDirectory()
	{
		return imageComparisonTempDirectory();
	}

	[[nodiscard]] std::filesystem::path imageComparisonResultDirectory()
	{
		return imageComparisonTempDirectory();
	}

	void writePng(const std::filesystem::path &path, int width, int height, const std::vector<unsigned char> &pixels)
	{
		ASSERT_NE(stbi_write_png(path.string().c_str(), width, height, 4, pixels.data(), width * 4), 0);
	}
}

TEST(ImageComparisonTest, MatchingImagesLeaveNoArtifacts)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "matching_actual.png";
	const std::filesystem::path expectedPath = imageComparisonExpectedDirectory() / "matching_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "matching_diff.png";

	const std::vector<unsigned char> pixels = {
		10, 20, 30, 255, 40, 50, 60, 255, 70, 80, 90, 255, 100, 110, 120, 255};

	ASSERT_NE(stbi_write_png(actualPath.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4), 0);
	ASSERT_NE(stbi_write_png(expectedPath.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4), 0);
	ASSERT_NE(stbi_write_png(diffPath.string().c_str(), 2, 2, 4, pixels.data(), 2 * 4), 0);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0);
	EXPECT_FALSE(std::filesystem::exists(actualPath));
	EXPECT_FALSE(std::filesystem::exists(diffPath));
}

TEST(ImageComparisonTest, RgbDifferenceAtToleranceMatchesAndAboveToleranceDiffers)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "tolerance_boundary_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "tolerance_boundary_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "tolerance_boundary_diff.png";

	const std::vector<unsigned char> actualPixels = {
		104, 100, 100, 255, 105, 100, 100, 255};
	const std::vector<unsigned char> expectedPixels = {
		100, 100, 100, 255, 100, 100, 100, 255};

	writePng(actualPath, 2, 1, actualPixels);
	writePng(expectedPath, 2, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 1);

	EXPECT_EQ(diffPixels[0], 0);
	EXPECT_EQ(diffPixels[1], 0);
	EXPECT_EQ(diffPixels[2], 0);
	EXPECT_EQ(diffPixels[3], 255);

	const int redPixelOffset = 4;
	EXPECT_EQ(diffPixels[redPixelOffset + 0], 255);
	EXPECT_EQ(diffPixels[redPixelOffset + 1], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 2], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 3], 255);

	stbi_image_free(diffPixels);
}

TEST(ImageComparisonTest, TransparentPixelsIgnoreRgbDifferences)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "transparent_rgb_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "transparent_rgb_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "transparent_rgb_diff.png";

	const std::vector<unsigned char> actualPixels = {255, 0, 255, 0};
	const std::vector<unsigned char> expectedPixels = {0, 255, 0, 0};

	writePng(actualPath, 1, 1, actualPixels);
	writePng(expectedPath, 1, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 0);
}

TEST(ImageComparisonTest, DimensionMismatchMarksOutOfOverlapPixels)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "dimension_mismatch_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "dimension_mismatch_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "dimension_mismatch_diff.png";

	const std::vector<unsigned char> actualPixels = {
		10, 20, 30, 255, 40, 50, 60, 255, 70, 80, 90, 255, 100, 110, 120, 255};
	const std::vector<unsigned char> expectedPixels = {
		10, 20, 30, 255, 40, 50, 60, 255};

	writePng(actualPath, 2, 2, actualPixels);
	writePng(expectedPath, 2, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.actualWidth, 2);
	EXPECT_EQ(result.actualHeight, 2);
	EXPECT_EQ(result.expectedWidth, 2);
	EXPECT_EQ(result.expectedHeight, 1);
	EXPECT_EQ(result.differentPixelCount, 2);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 2);

	for (int x = 0; x < 2; ++x)
	{
		const int matchingPixelOffset = x * 4;
		EXPECT_EQ(diffPixels[matchingPixelOffset + 0], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 1], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 2], 0);
		EXPECT_EQ(diffPixels[matchingPixelOffset + 3], 255);

		const int missingPixelOffset = (2 + x) * 4;
		EXPECT_EQ(diffPixels[missingPixelOffset + 0], 255);
		EXPECT_EQ(diffPixels[missingPixelOffset + 1], 0);
		EXPECT_EQ(diffPixels[missingPixelOffset + 2], 0);
		EXPECT_EQ(diffPixels[missingPixelOffset + 3], 255);
	}

	stbi_image_free(diffPixels);
}

TEST(ImageComparisonTest, AlphaDifferenceAboveToleranceDiffers)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "alpha_difference_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "alpha_difference_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "alpha_difference_diff.png";

	const std::vector<unsigned char> actualPixels = {10, 20, 30, 255};
	const std::vector<unsigned char> expectedPixels = {10, 20, 30, 246};

	writePng(actualPath, 1, 1, actualPixels);
	writePng(expectedPath, 1, 1, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);
}

TEST(ImageComparisonTest, MissingFileThrows)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "missing_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "missing_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "missing_diff.png";

	std::filesystem::remove(actualPath);
	const std::vector<unsigned char> expectedPixels = {0, 0, 0, 255};
	writePng(expectedPath, 1, 1, expectedPixels);

	EXPECT_THROW(static_cast<void>(sparkle_test::compareImages(actualPath, expectedPath, diffPath)), std::runtime_error);
}

TEST(ImageComparisonTest, AllPixelsDifferentAreCounted)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "all_different_actual.png";
	const std::filesystem::path expectedPath = imageComparisonResultDirectory() / "all_different_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "all_different_diff.png";

	const std::vector<unsigned char> actualPixels = {
		255, 0, 0, 255, 0, 255, 0, 255, 0, 0, 255, 255, 255, 255, 255, 255};
	const std::vector<unsigned char> expectedPixels = {
		0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255};

	writePng(actualPath, 2, 2, actualPixels);
	writePng(expectedPath, 2, 2, expectedPixels);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 4);
}

TEST(ImageComparisonTest, DifferentPixelsAreMarkedInRed)
{
	const std::filesystem::path actualPath = imageComparisonResultDirectory() / "different_actual.png";
	const std::filesystem::path expectedPath = imageComparisonExpectedDirectory() / "different_expected.png";
	const std::filesystem::path diffPath = imageComparisonResultDirectory() / "different_diff.png";

	const std::vector<unsigned char> actualPixels = {
		0, 0, 0, 255, 255, 255, 255, 255, 0, 0, 0, 255, 0, 0, 0, 255};
	const std::vector<unsigned char> expectedPixels = {
		0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255, 0, 0, 0, 255};

	ASSERT_NE(stbi_write_png(actualPath.string().c_str(), 2, 2, 4, actualPixels.data(), 2 * 4), 0);
	ASSERT_NE(stbi_write_png(expectedPath.string().c_str(), 2, 2, 4, expectedPixels.data(), 2 * 4), 0);

	const sparkle_test::ImageComparisonResult result = sparkle_test::compareImages(actualPath, expectedPath, diffPath);

	EXPECT_FALSE(result.matches);
	EXPECT_EQ(result.differentPixelCount, 1);

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *diffPixels = stbi_load(diffPath.string().c_str(), &width, &height, &channels, 4);
	ASSERT_NE(diffPixels, nullptr);
	ASSERT_EQ(width, 2);
	ASSERT_EQ(height, 2);

	const int redPixelOffset = 4;
	EXPECT_EQ(diffPixels[redPixelOffset + 0], 255);
	EXPECT_EQ(diffPixels[redPixelOffset + 1], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 2], 0);
	EXPECT_EQ(diffPixels[redPixelOffset + 3], 255);

	stbi_image_free(diffPixels);
}

namespace
{
	using Options = sparkle_test::ImageComparisonOptions;
	using ColorDelta = Options::ColorDelta;
	using ChannelDelta = Options::ChannelDelta;
	[[nodiscard]] sparkle_test::ImageComparisonResult comparePixel(
		const std::vector<unsigned char> &actual, const std::vector<unsigned char> &expected, const Options &options)
	{
		const auto root = imageComparisonTempDirectory();
		writePng(root / "custom_actual.png", 1, 1, actual);
		writePng(root / "custom_expected.png", 1, 1, expected);
		return sparkle_test::compareImages(root / "custom_actual.png", root / "custom_expected.png", root / "custom_diff.png", options);
	}
	[[nodiscard]] ColorDelta uniformDeltas(ChannelDelta value)
	{
		return {value, value, value, value};
	}
}

TEST(ImageComparisonTest, SignedBoundsAreInclusiveForEveryComponent)
{
	Options options;
	options.channelDeltas = uniformDeltas({-3, 7});
	for (std::size_t channel = 0; channel < 4; ++channel)
	{
		SCOPED_TRACE(channel);
		for (const int delta : {-4, -3, 0, 7, 8})
		{
			SCOPED_TRACE(delta);
			std::vector<unsigned char> actual(4, 100);
			actual[channel] = static_cast<unsigned char>(100 + delta);
			const auto result = comparePixel(actual, {100, 100, 100, 100}, options);
			EXPECT_EQ(result.matches, delta >= -3 && delta <= 7);
			EXPECT_EQ(result.differentPixelCount, result.matches ? 0 : 1);
		}
	}
}

TEST(ImageComparisonTest, ComponentsHaveIndependentRangesAndOverrideLegacyTolerance)
{
	Options options;
	options.rgbTolerance = 255;
	options.alphaTolerance = 255;
	options.channelDeltas = ColorDelta{{-1, 2}, {-3, 4}, {-5, 6}, {-7, 8}};
	EXPECT_TRUE(comparePixel({99, 97, 95, 93}, {100, 100, 100, 100}, options).matches);
	EXPECT_TRUE(comparePixel({102, 104, 106, 108}, {100, 100, 100, 100}, options).matches);
	const std::vector<int> upper{2, 4, 6, 8};
	for (std::size_t channel = 0; channel < 4; ++channel)
	{
		SCOPED_TRACE(channel);
		std::vector<unsigned char> actual(4, 100);
		actual[channel] = static_cast<unsigned char>(101 + upper[channel]);
		EXPECT_FALSE(comparePixel(actual, {100, 100, 100, 100}, options).matches);
	}
}

TEST(ImageComparisonTest, SignedDeltaDoesNotWrapAtBlackOrWhite)
{
	Options options;
	options.channelDeltas = uniformDeltas({-2, 3});
	EXPECT_TRUE(comparePixel({0, 0, 0, 255}, {2, 2, 2, 255}, options).matches);
	EXPECT_TRUE(comparePixel({255, 255, 255, 255}, {252, 252, 252, 255}, options).matches);
	EXPECT_FALSE(comparePixel({0, 0, 0, 255}, {255, 255, 255, 255}, options).matches);
	EXPECT_FALSE(comparePixel({255, 255, 255, 255}, {0, 0, 0, 255}, options).matches);
}

TEST(ImageComparisonTest, ZeroRangesRequireExactComponents)
{
	Options options;
	options.channelDeltas = uniformDeltas({0, 0});
	EXPECT_TRUE(comparePixel({10, 20, 30, 255}, {10, 20, 30, 255}, options).matches);
	EXPECT_FALSE(comparePixel({11, 20, 30, 255}, {10, 20, 30, 255}, options).matches);
}

TEST(ImageComparisonTest, CustomAlphaBoundsStillApplyToTransparentPixels)
{
	Options options;
	options.channelDeltas = uniformDeltas({0, 0});
	options.channelDeltas->alpha = {-1, 2};
	EXPECT_TRUE(comparePixel({255, 0, 255, 2}, {0, 255, 0, 0}, options).matches);
	EXPECT_FALSE(comparePixel({255, 0, 255, 3}, {0, 255, 0, 0}, options).matches);
	EXPECT_FALSE(comparePixel({255, 0, 255, 0}, {0, 255, 0, 2}, options).matches);
}

TEST(ImageComparisonTest, InvalidRangesThrowBeforeReadingOrChangingImages)
{
	Options options;
	for (const ChannelDelta invalid : {ChannelDelta{-256, 0}, {0, 256}, {1, 2}, {-2, -1}, {2, -2}})
	{
		options.channelDeltas = uniformDeltas(invalid);
		EXPECT_THROW(static_cast<void>(sparkle_test::compareImages({}, {}, {}, options)), std::invalid_argument);
	}
}

TEST(ImageComparisonTest, RepeatedRejectedColorPairsAreCountedAndLogged)
{
	const auto root = imageComparisonTempDirectory();
	writePng(root / "group_actual.png", 4, 1, {110, 90, 100, 255, 110, 90, 100, 255, 100, 100, 100, 240, 104, 100, 100, 255});
	writePng(root / "group_expected.png", 4, 1, {100, 100, 100, 255, 100, 100, 100, 255, 100, 100, 100, 255, 100, 100, 100, 255});
	::testing::internal::CaptureStdout();
	const auto result = sparkle_test::compareImages(root / "group_actual.png", root / "group_expected.png", root / "group_diff.png");
	const auto output = ::testing::internal::GetCapturedStdout();
	const sparkle_test::ImageComparisonResult::ColorPair pair{{110, 90, 100, 255}, {100, 100, 100, 255}};
	EXPECT_EQ(result.colorDifferences.at(pair), 2);
	EXPECT_EQ(result.colorDifferences.size(), 2);
	EXPECT_EQ(result.differentPixelCount, 3);
	EXPECT_EQ(result.outOfBoundsPixelCount, 0);
	EXPECT_NE(output.find("RGBA(110, 90, 100, 255) diff to RGBA(100, 100, 100, 255) - 2 times; delta(actual-reference) = (+10, -10, 0, 0)"), std::string::npos);
	EXPECT_NE(output.find("delta(actual-reference) = (0, 0, 0, -15)"), std::string::npos);
	EXPECT_NE(output.find("Allowed deltas (actual-reference), RGBA: [-4, 4] [-4, 4] [-4, 4] [-8, 8]"), std::string::npos);
	EXPECT_NE(output.find("Total different pixels: 3"), std::string::npos);
}

TEST(ImageComparisonTest, AcceptedDifferencesProduceNoDiagnosticEntriesOrOutput)
{
	Options options;
	options.channelDeltas = uniformDeltas({-20, 20});
	::testing::internal::CaptureStdout();
	const auto result = comparePixel({110, 90, 100, 255}, {100, 100, 100, 255}, options);
	const auto output = ::testing::internal::GetCapturedStdout();
	EXPECT_TRUE(result.matches);
	EXPECT_TRUE(result.colorDifferences.empty());
	EXPECT_EQ(result.outOfBoundsPixelCount, 0);
	EXPECT_TRUE(output.empty());
}

TEST(ImageComparisonTest, DimensionErrorsAreReportedWithoutInventingColorPairs)
{
	const auto root = imageComparisonTempDirectory();
	writePng(root / "size_actual.png", 2, 1, {10, 20, 30, 255, 10, 20, 30, 255});
	writePng(root / "size_expected.png", 1, 1, {10, 20, 30, 255});
	::testing::internal::CaptureStdout();
	const auto result = sparkle_test::compareImages(root / "size_actual.png", root / "size_expected.png", root / "size_diff.png");
	const auto output = ::testing::internal::GetCapturedStdout();
	EXPECT_FALSE(result.matches);
	EXPECT_TRUE(result.colorDifferences.empty());
	EXPECT_EQ(result.outOfBoundsPixelCount, 1);
	EXPECT_EQ(result.differentPixelCount, 1);
	EXPECT_NE(output.find("Outside image overlap: 1 pixels"), std::string::npos);
}
