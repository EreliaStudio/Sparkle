#include "sparkle_test/gif_comparison.hpp"

#include <algorithm>
#include <array>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <stb_image.h>
#include <stb_image_write.h>

namespace
{
	using Byte = std::uint8_t;

	void append16(std::vector<Byte> &output, std::uint32_t value)
	{
		output.push_back(static_cast<Byte>(value & 255));
		output.push_back(static_cast<Byte>((value >> 8) & 255));
	}

	struct IndexedFrame
	{
		std::array<std::uint32_t, 256> palette{};
		std::vector<Byte> pixels;
	};

	[[nodiscard]] IndexedFrame quantize(const sparkle_test::FramebufferImage &frame)
	{
		// Use an independent 256-colour palette for each frame. Flat-shaded voxel
		// scenes retain their main colours exactly; only rare colours are mapped
		// to the nearest representative, deterministically.
		std::map<std::uint32_t, std::size_t> histogram;
		for (std::size_t index = 0; index < frame.pixels.size(); index += 4)
		{
			const std::uint32_t color = (static_cast<std::uint32_t>(frame.pixels[index]) << 16) |
										(static_cast<std::uint32_t>(frame.pixels[index + 1]) << 8) | frame.pixels[index + 2];
			++histogram[color];
		}
		std::vector<std::pair<std::uint32_t, std::size_t>> ranked(histogram.begin(), histogram.end());
		std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
			return a.second != b.second ? a.second > b.second : a.first < b.first;
		});
		IndexedFrame result;
		const std::size_t count = std::min<std::size_t>(256, ranked.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			result.palette[index] = ranked[index].first;
		}
		std::unordered_map<std::uint32_t, Byte> lookup;
		lookup.reserve(histogram.size());
		for (const auto &[rgb, frequency] : histogram)
		{
			(void)frequency;
			int bestDistance = std::numeric_limits<int>::max();
			std::size_t bestIndex = 0;
			for (std::size_t index = 0; index < count; ++index)
			{
				const std::uint32_t chosen = result.palette[index];
				const int dr = static_cast<int>((rgb >> 16) & 255) - static_cast<int>((chosen >> 16) & 255);
				const int dg = static_cast<int>((rgb >> 8) & 255) - static_cast<int>((chosen >> 8) & 255);
				const int db = static_cast<int>(rgb & 255) - static_cast<int>(chosen & 255);
				const int distance = dr * dr + dg * dg + db * db;
				if (distance < bestDistance)
				{
					bestDistance = distance;
					bestIndex = index;
				}
			}
			lookup.emplace(rgb, static_cast<Byte>(bestIndex));
		}
		result.pixels.reserve(frame.pixels.size() / 4);
		for (std::size_t index = 0; index < frame.pixels.size(); index += 4)
		{
			const std::uint32_t color = (static_cast<std::uint32_t>(frame.pixels[index]) << 16) |
										(static_cast<std::uint32_t>(frame.pixels[index + 1]) << 8) | frame.pixels[index + 2];
			result.pixels.push_back(lookup.at(color));
		}
		return result;
	}

	[[nodiscard]] std::vector<Byte> encodeLzw(const std::vector<Byte> &pixels)
	{
		// A valid GIF LZW stream can reset the dictionary before it grows past
		// 9-bit codes. This deliberately simple encoding is test-only.
		std::vector<Byte> stream;
		std::uint32_t bitBuffer = 0;
		int bitCount = 0;
		const auto emit = [&](int code) {
			bitBuffer |= static_cast<std::uint32_t>(code) << bitCount;
			bitCount += 9;
			while (bitCount >= 8)
			{
				stream.push_back(static_cast<Byte>(bitBuffer & 255));
				bitBuffer >>= 8;
				bitCount -= 8;
			}
		};
		emit(256);
		for (std::size_t index = 0; index < pixels.size(); ++index)
		{
			if (index != 0 && index % 200 == 0)
			{
				emit(256);
			}
			emit(pixels[index]);
		}
		emit(257);
		if (bitCount != 0)
		{
			stream.push_back(static_cast<Byte>(bitBuffer & 255));
		}
		return stream;
	}

	void writePng(const std::filesystem::path &path, const sparkle_test::FramebufferImage &frame)
	{
		if (stbi_write_png(path.string().c_str(), static_cast<int>(frame.size.x), static_cast<int>(frame.size.y), 4, frame.pixels.data(), static_cast<int>(frame.size.x * 4)) == 0)
		{
			throw std::runtime_error("Cannot write GIF comparison frame: " + path.string());
		}
	}

	struct DecodedGif
	{
		std::vector<sparkle_test::FramebufferImage> frames;
		std::vector<int> delays;
	};

	[[nodiscard]] DecodedGif decodeGif(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::binary);
		if (!file)
		{
			throw std::runtime_error("Missing golden GIF: " + path.string());
		}
		std::vector<Byte> bytes((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		if (bytes.empty() || bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
		{
			throw std::runtime_error("Invalid golden GIF: " + path.string());
		}
		int *rawDelays = nullptr;
		int width = 0;
		int height = 0;
		int count = 0;
		int components = 0;
		Byte *raw = stbi_load_gif_from_memory(
			bytes.data(), static_cast<int>(bytes.size()), &rawDelays, &width, &height, &count, &components, 4);
		if (raw == nullptr)
		{
			throw std::runtime_error("Cannot decode golden GIF: " + path.string());
		}
		DecodedGif result;
		try
		{
			const std::size_t frameBytes = static_cast<std::size_t>(width) * height * 4;
			for (int index = 0; index < count; ++index)
			{
				sparkle_test::FramebufferImage frame;
				frame.size = {static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height)};
				const Byte *begin = raw + static_cast<std::size_t>(index) * frameBytes;
				frame.pixels.assign(begin, begin + frameBytes);
				result.frames.push_back(std::move(frame));
				result.delays.push_back(rawDelays != nullptr ? rawDelays[index] : 0);
			}
		} catch (...)
		{
			stbi_image_free(raw);
			stbi_image_free(rawDelays);
			throw;
		}
		stbi_image_free(raw);
		stbi_image_free(rawDelays);
		return result;
	}
}

namespace sparkle_test
{
	void saveAnimationGif(
		const std::vector<FramebufferImage> &frames,
		const std::filesystem::path &destination,
		std::uint32_t frameDelayMs)
	{
		if (frames.empty())
		{
			throw std::invalid_argument("GIF requires at least one frame");
		}
		if (frameDelayMs < 10 || frameDelayMs > 655350 || frameDelayMs % 10 != 0)
		{
			throw std::invalid_argument("GIF frame delay must be a multiple of 10ms in [10, 655350]");
		}
		const auto size = frames.front().size;
		if (size.x == 0 || size.y == 0 || size.x > 65535 || size.y > 65535)
		{
			throw std::invalid_argument("GIF dimensions must be in [1, 65535]");
		}
		for (const auto &frame : frames)
		{
			if (frame.size != size || frame.pixels.size() != static_cast<std::size_t>(size.x) * size.y * 4)
			{
				throw std::invalid_argument("GIF frames must have identical dimensions and RGBA pixel counts");
			}
		}

		std::vector<Byte> output{'G', 'I', 'F', '8', '9', 'a'};
		append16(output, size.x);
		append16(output, size.y);
		output.insert(output.end(), {0x70, 0, 0});
		// Loop forever for convenient manual inspection.
		output.insert(output.end(), {0x21, 0xff, 0x0b, 'N', 'E', 'T', 'S', 'C', 'A', 'P', 'E', '2', '.', '0', 0x03, 0x01, 0, 0, 0});
		for (const auto &frame : frames)
		{
			const IndexedFrame indexed = quantize(frame);
			output.insert(output.end(), {0x21, 0xf9, 0x04, 0x00});
			append16(output, frameDelayMs / 10);
			output.insert(output.end(), {0x00, 0x00});
			output.push_back(0x2c);
			append16(output, 0);
			append16(output, 0);
			append16(output, size.x);
			append16(output, size.y);
			output.push_back(0x87); // Frame-local 256-colour table.
			for (std::uint32_t rgb : indexed.palette)
			{
				output.push_back(static_cast<Byte>((rgb >> 16) & 255));
				output.push_back(static_cast<Byte>((rgb >> 8) & 255));
				output.push_back(static_cast<Byte>(rgb & 255));
			}
			output.push_back(8);
			const auto stream = encodeLzw(indexed.pixels);
			for (std::size_t offset = 0; offset < stream.size();)
			{
				const std::size_t count = std::min<std::size_t>(255, stream.size() - offset);
				output.push_back(static_cast<Byte>(count));
				output.insert(output.end(), stream.begin() + static_cast<std::ptrdiff_t>(offset), stream.begin() + static_cast<std::ptrdiff_t>(offset + count));
				offset += count;
			}
			output.push_back(0);
		}
		output.push_back(0x3b);
		if (destination.has_parent_path())
		{
			std::filesystem::create_directories(destination.parent_path());
		}
		std::ofstream file(destination, std::ios::binary);
		if (!file.write(reinterpret_cast<const char *>(output.data()), static_cast<std::streamsize>(output.size())))
		{
			throw std::runtime_error("Cannot save animation GIF: " + destination.string());
		}
	}

	GifComparisonResult compareAnimationGif(
		const std::vector<FramebufferImage> &actualFrames,
		const std::filesystem::path &expectedGif,
		const std::filesystem::path &actualGif,
		const std::filesystem::path &diagnosticDirectory,
		GifComparisonOptions options)
	{
		saveAnimationGif(actualFrames, actualGif, options.frameDelayMs);
		const auto expected = decodeGif(expectedGif);
		GifComparisonResult result;
		result.expectedFrames = expected.frames.size();
		result.actualFrames = actualFrames.size();
		result.matches = result.expectedFrames == result.actualFrames;
		std::filesystem::create_directories(diagnosticDirectory);

		const auto count = std::min(expected.frames.size(), actualFrames.size());
		for (std::size_t index = 0; index < count; ++index)
		{
			const std::string name = "frame_" + std::to_string(index);
			const auto referencePng = diagnosticDirectory / (name + "_expected.png");
			const auto actualPng = diagnosticDirectory / (name + "_actual.png");
			const auto differencePng = diagnosticDirectory / (name + "_difference.png");
			writePng(referencePng, expected.frames[index]);
			writePng(actualPng, actualFrames[index]);
			const auto comparison = compareImages(actualPng, referencePng, differencePng, options.image, false);
			result.differentPixels.push_back(comparison.differentPixelCount);
			const bool valid = comparison.differentPixelCount <= options.maximumDifferentPixelsPerFrame &&
							   comparison.actualWidth == comparison.expectedWidth &&
							   comparison.actualHeight == comparison.expectedHeight &&
							   expected.delays[index] == static_cast<int>(options.frameDelayMs);
			if (valid)
			{
				std::filesystem::remove(actualPng);
				std::filesystem::remove(differencePng);
				std::filesystem::remove(referencePng);
			}
			else
			{
				result.matches = false;
			}
		}
		return result;
	}
}
