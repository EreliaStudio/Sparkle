#include "sparkle_test/image_comparison.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

#include "sparkle_test/paths.hpp"

namespace
{
	struct LoadedImage
	{
		int width = 0;
		int height = 0;
		std::vector<std::uint8_t> pixels;
	};

	[[nodiscard]] LoadedImage loadImage(const std::filesystem::path &path)
	{
		int width = 0;
		int height = 0;
		int channels = 0;
		unsigned char *rawPixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
		if (rawPixels == nullptr)
		{
			throw std::runtime_error("Failed to load image [" + path.string() + "]: " + stbi_failure_reason());
		}

		LoadedImage result;
		result.width = width;
		result.height = height;
		result.pixels.assign(rawPixels, rawPixels + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4));
		stbi_image_free(rawPixels);
		return result;
	}

	using ChannelDelta = sparkle_test::ImageComparisonOptions::ChannelDelta;
	using ChannelDeltas = std::array<ChannelDelta, 4>;

	[[nodiscard]] ChannelDeltas resolveDeltas(const sparkle_test::ImageComparisonOptions &options)
	{
		if (options.channelDeltas)
		{
			const auto &deltas = *options.channelDeltas;
			return {deltas.red, deltas.green, deltas.blue, deltas.alpha};
		}
		const ChannelDelta rgb{-static_cast<int>(options.rgbTolerance), options.rgbTolerance};
		const ChannelDelta alpha{-static_cast<int>(options.alphaTolerance), options.alphaTolerance};
		return {rgb, rgb, rgb, alpha};
	}

	void validateDeltas(const ChannelDeltas &deltas)
	{
		for (const auto &delta : deltas)
		{
			if (delta.minimum < -255 || delta.minimum > 0 || delta.maximum < 0 || delta.maximum > 255)
			{
				throw std::invalid_argument("Image comparison deltas require -255 <= minimum <= 0 <= maximum <= 255");
			}
		}
	}

	[[nodiscard]] bool channelDiffers(std::uint8_t actual, std::uint8_t expected, ChannelDelta tolerance)
	{
		const int delta = static_cast<int>(actual) - static_cast<int>(expected);
		return delta < tolerance.minimum || delta > tolerance.maximum;
	}

	[[nodiscard]] bool pixelDiffers(
		const std::uint8_t *actual, const std::uint8_t *expected, const ChannelDeltas &deltas, std::uint8_t transparentAlphaThreshold)
	{
		if (channelDiffers(actual[3], expected[3], deltas[3]))
		{
			return true;
		}
		if (actual[3] <= transparentAlphaThreshold && expected[3] <= transparentAlphaThreshold)
		{
			return false;
		}
		for (std::size_t channel = 0; channel < 3; ++channel)
		{
			if (channelDiffers(actual[channel], expected[channel], deltas[channel]))
			{
				return true;
			}
		}
		return false;
	}

	using ComparisonResult = sparkle_test::ImageComparisonResult;

	void recordColorDifference(ComparisonResult &result, const std::uint8_t *actual, const std::uint8_t *expected)
	{
		ComparisonResult::Color actualColor;
		ComparisonResult::Color expectedColor;
		std::copy_n(actual, 4, actualColor.begin());
		std::copy_n(expected, 4, expectedColor.begin());
		++result.colorDifferences[{actualColor, expectedColor}];
	}

	void writeColor(std::ostream &output, const ComparisonResult::Color &color)
	{
		output << "RGBA(" << static_cast<int>(color[0]) << ", " << static_cast<int>(color[1])
			   << ", " << static_cast<int>(color[2]) << ", " << static_cast<int>(color[3]) << ")";
	}

	void writeColorDifference(std::ostream &output, const ComparisonResult::ColorPair &colors, std::size_t count)
	{
		writeColor(output, colors.first);
		output << " diff to ";
		writeColor(output, colors.second);
		output << " - " << count << " times; delta(actual-reference) = (";
		for (std::size_t channel = 0; channel < 4; ++channel)
		{
			const int delta = static_cast<int>(colors.first[channel]) - static_cast<int>(colors.second[channel]);
			output << (channel == 0 ? "" : ", ") << (delta > 0 ? "+" : "") << delta;
		}
		output << ")\n";
	}

	void logDifferences(const ComparisonResult &result, const std::filesystem::path &actualPath, const std::filesystem::path &expectedPath, const ChannelDeltas &deltas, std::uint8_t transparentAlphaThreshold)
	{
		std::ostringstream output;
		output << "Difference :\nActual: " << actualPath << "\nReference: " << expectedPath << "\n";
		output << "Allowed deltas (actual-reference), RGBA:";
		for (const auto &delta : deltas)
		{
			output << " [" << delta.minimum << ", " << delta.maximum << "]";
		}
		output << "\nTransparent alpha threshold: " << static_cast<int>(transparentAlphaThreshold) << "\n";
		for (const auto &[colors, count] : result.colorDifferences)
		{
			writeColorDifference(output, colors, count);
		}
		output << "Outside image overlap: " << result.outOfBoundsPixelCount << " pixels\n";
		output << "Total different pixels: " << result.differentPixelCount << "\n";
		std::cout << output.str();
	}

}

namespace sparkle_test
{
	ImageComparisonResult compareImages(
		const std::filesystem::path &actualPath,
		const std::filesystem::path &expectedPath,
		const std::filesystem::path &differencePath,
		ImageComparisonOptions options,
		bool printDifferences)
	{
		const auto deltas = resolveDeltas(options);
		validateDeltas(deltas);
		const LoadedImage actual = loadImage(actualPath);
		const LoadedImage expected = loadImage(expectedPath);

		ImageComparisonResult result;
		result.actualWidth = actual.width;
		result.actualHeight = actual.height;
		result.expectedWidth = expected.width;
		result.expectedHeight = expected.height;

		const int diffWidth = std::max(actual.width, expected.width);
		const int diffHeight = std::max(actual.height, expected.height);
		std::vector<std::uint8_t> difference(static_cast<std::size_t>(diffWidth) * static_cast<std::size_t>(diffHeight) * 4, 0);
		for (std::size_t index = 3; index < difference.size(); index += 4)
		{
			difference[index] = 255;
		}

		for (int y = 0; y < diffHeight; ++y)
		{
			for (int x = 0; x < diffWidth; ++x)
			{
				bool pixelMatches = x < actual.width &&
									y < actual.height &&
									x < expected.width &&
									y < expected.height;

				if (pixelMatches == true)
				{
					const std::size_t actualIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(actual.width) + static_cast<std::size_t>(x)) * 4;
					const std::size_t expectedIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(expected.width) + static_cast<std::size_t>(x)) * 4;
					pixelMatches = (pixelDiffers(actual.pixels.data() + actualIndex, expected.pixels.data() + expectedIndex, deltas, options.transparentAlphaThreshold) == false);
					if (!pixelMatches)
					{
						recordColorDifference(result, actual.pixels.data() + actualIndex, expected.pixels.data() + expectedIndex);
					}
				}
				else
				{
					++result.outOfBoundsPixelCount;
				}

				if (pixelMatches == false)
				{
					const std::size_t diffIndex = (static_cast<std::size_t>(y) * static_cast<std::size_t>(diffWidth) + static_cast<std::size_t>(x)) * 4;
					difference[diffIndex + 0] = 255;
					difference[diffIndex + 1] = 0;
					difference[diffIndex + 2] = 0;
					difference[diffIndex + 3] = 255;
					++result.differentPixelCount;
				}
			}
		}

		result.matches = result.differentPixelCount == 0;

		if (result.matches == false)
		{
			if (printDifferences)
			{
				logDifferences(result, actualPath, expectedPath, deltas, options.transparentAlphaThreshold);
			}
			if (differencePath.has_parent_path())
			{
				std::filesystem::create_directories(differencePath.parent_path());
			}
			if (stbi_write_png(differencePath.string().c_str(), diffWidth, diffHeight, 4, difference.data(), diffWidth * 4) == 0)
			{
				throw std::runtime_error("Failed to write image comparison diff [" + differencePath.string() + "]");
			}
		}
		else
		{
			std::filesystem::remove(actualPath);
			std::filesystem::remove(differencePath);
			removeEmptyResultDirectories(actualPath.parent_path());
		}

		return result;
	}
}
