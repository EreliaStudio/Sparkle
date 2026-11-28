#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>

#include "sparkle_test/image_comparison.hpp"
#include "sparkle_test/open_gl_test_context.hpp"

namespace sparkle_test
{
	struct GifComparisonOptions
	{
		ImageComparisonOptions image{};
		std::uint32_t frameDelayMs = 100;
		std::size_t maximumDifferentPixelsPerFrame = 150;
	};

	struct GifComparisonResult
	{
		bool matches = false;
		std::size_t expectedFrames = 0;
		std::size_t actualFrames = 0;
		std::vector<std::size_t> differentPixels;
	};

	// Visual preview encoder. Uses a fixed 3-3-2 palette; comparison always
	// evaluates the original RGBA frames against the decoded reference instead.
	void saveAnimationGif(
		const std::vector<FramebufferImage> &frames,
		const std::filesystem::path &destination,
		std::uint32_t frameDelayMs = 100);

	// Exactly one expected .gif is checked into the repository. Decoded frames
	// are fed into the existing PNG comparator; temporary PNGs are test outputs,
	// not references stored in Git. Actual GIF is always saved for visual review,
	// even if the approved reference is missing or frames differ.
	[[nodiscard]] GifComparisonResult compareAnimationGif(
		const std::vector<FramebufferImage> &actualFrames,
		const std::filesystem::path &expectedGif,
		const std::filesystem::path &actualGif,
		const std::filesystem::path &diagnosticDirectory,
		GifComparisonOptions options = {});
}
