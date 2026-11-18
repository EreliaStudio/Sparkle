#include <filesystem>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include "sparkle_test/gif_comparison.hpp"

namespace
{
	[[nodiscard]] sparkle_test::FramebufferImage solidFrame(std::uint8_t r, std::uint8_t g, std::uint8_t b)
	{
		sparkle_test::FramebufferImage result;
		result.size = {2, 2};
		for (int index = 0; index < 4; ++index)
		{
			result.pixels.insert(result.pixels.end(), {r, g, b, 255});
		}
		return result;
	}

	[[nodiscard]] std::filesystem::path temporaryDirectory()
	{
		const auto dir = std::filesystem::temp_directory_path() / "sparkle_gif_comparison_tests";
		std::filesystem::create_directories(dir);
		return dir;
	}
}

TEST(GifComparisonTest, OneExpectedGIFStoresAndChecksMultipleFrames)
{
	const auto dir = temporaryDirectory();
	const auto expected = dir / "animation_expected.gif";
	const auto actual = dir / "animation_actual.gif";
	const std::vector frames{solidFrame(255, 0, 0), solidFrame(0, 255, 0), solidFrame(0, 0, 255)};
	sparkle_test::saveAnimationGif(frames, expected, 100);
	auto result = sparkle_test::compareAnimationGif(frames, expected, actual, dir / "matching");
	EXPECT_TRUE(result.matches);
	EXPECT_EQ(result.expectedFrames, 3u);
	EXPECT_EQ(result.actualFrames, 3u);
	EXPECT_TRUE(std::filesystem::exists(actual));
	EXPECT_EQ(result.differentPixels, (std::vector<std::size_t>{0, 0, 0}));
}

TEST(GifComparisonTest, ChangedFrameFailsAndKeepsFrameDiagnostics)
{
	const auto dir = temporaryDirectory();
	const auto expected = dir / "changed_expected.gif";
	const auto actual = dir / "changed_actual.gif";
	const std::vector frames{solidFrame(255, 0, 0), solidFrame(0, 255, 0)};
	sparkle_test::saveAnimationGif(frames, expected);
	auto changed = frames;
	changed[1] = solidFrame(0, 0, 255);
	const auto result = sparkle_test::compareAnimationGif(changed, expected, actual, dir / "changed");
	EXPECT_FALSE(result.matches);
	ASSERT_EQ(result.differentPixels.size(), 2u);
	EXPECT_EQ(result.differentPixels[0], 0u);
	EXPECT_EQ(result.differentPixels[1], 4u);
	EXPECT_TRUE(std::filesystem::exists(dir / "changed/frame_1_difference.png"));
}

TEST(GifComparisonTest, MissingReferenceNeverSkipsAndStillWritesPreview)
{
	const auto dir = temporaryDirectory();
	const auto missing = dir / "does_not_exist.gif";
	const auto actual = dir / "missing_actual.gif";
	std::filesystem::remove(missing);
	EXPECT_THROW(sparkle_test::compareAnimationGif({solidFrame(255, 0, 0)}, missing, actual, dir / "missing"), std::runtime_error);
	EXPECT_TRUE(std::filesystem::exists(actual));
}

TEST(GifComparisonTest, FrameCountAndTimingMismatchesAreErrors)
{
	const auto dir = temporaryDirectory();
	const auto expected = dir / "timing_expected.gif";
	const std::vector frames{solidFrame(255, 0, 0), solidFrame(0, 255, 0)};
	sparkle_test::saveAnimationGif(frames, expected);
	EXPECT_FALSE(sparkle_test::compareAnimationGif({frames.front()}, expected, dir / "short_actual.gif", dir / "short").matches);
	sparkle_test::GifComparisonOptions wrongTiming;
	wrongTiming.frameDelayMs = 120;
	EXPECT_FALSE(sparkle_test::compareAnimationGif(frames, expected, dir / "timing_actual.gif", dir / "timing", wrongTiming).matches);
}

TEST(GifComparisonTest, RejectsInvalidFrameDimensionsAndDelays)
{
	const auto dir = temporaryDirectory();
	const auto frame = solidFrame(255, 0, 0);
	EXPECT_THROW(sparkle_test::saveAnimationGif({}, dir / "empty.gif"), std::invalid_argument);
	EXPECT_THROW(sparkle_test::saveAnimationGif({frame}, dir / "invalid_delay.gif", 7), std::invalid_argument);
	auto broken = frame;
	broken.pixels.pop_back();
	EXPECT_THROW(sparkle_test::saveAnimationGif({frame, broken}, dir / "different.gif"), std::invalid_argument);
}
