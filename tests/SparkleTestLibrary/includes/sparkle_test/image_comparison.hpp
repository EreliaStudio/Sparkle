#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <utility>

namespace sparkle_test
{
	struct ImageComparisonOptions
	{
		struct ChannelDelta
		{
			int minimum = -4;
			int maximum = 4;
		};
		struct ColorDelta
		{
			ChannelDelta red;
			ChannelDelta green;
			ChannelDelta blue;
			ChannelDelta alpha{-8, 8};
		};
		std::uint8_t rgbTolerance = 4;
		std::uint8_t alphaTolerance = 8;
		std::uint8_t transparentAlphaThreshold = 4;
		// Inclusive bounds on actual - expected, in 8-bit component units.
		// When provided, these replace rgbTolerance and alphaTolerance.
		std::optional<ColorDelta> channelDeltas;
	};

	struct ImageComparisonResult
	{
		using Color = std::array<std::uint8_t, 4>;
		// Key order: actual RGBA, reference RGBA. Only rejected pixels are counted.
		using ColorPair = std::pair<Color, Color>;
		bool matches = false;
		int actualWidth = 0;
		int actualHeight = 0;
		int expectedWidth = 0;
		int expectedHeight = 0;
		std::size_t differentPixelCount = 0;
		std::map<ColorPair, std::size_t> colorDifferences;
		std::size_t outOfBoundsPixelCount = 0;
	};

	[[nodiscard]] ImageComparisonResult compareImages(
		const std::filesystem::path &actualPath,
		const std::filesystem::path &expectedPath,
		const std::filesystem::path &differencePath,
		ImageComparisonOptions options = {},
		bool printDifferences = true);
}
