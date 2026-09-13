#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace sparkle_test
{
    struct ImageComparisonOptions
    {
        std::uint8_t rgbTolerance = 4;
        std::uint8_t alphaTolerance = 8;
        std::uint8_t transparentAlphaThreshold = 4;
    };

    struct ImageComparisonResult
    {
        bool matches = false;
        int actualWidth = 0;
        int actualHeight = 0;
        int expectedWidth = 0;
        int expectedHeight = 0;
        std::size_t differentPixelCount = 0;
    };

    [[nodiscard]] ImageComparisonResult compareImages(
        const std::filesystem::path& actualPath,
        const std::filesystem::path& expectedPath,
        const std::filesystem::path& differencePath,
        ImageComparisonOptions options = {});
}
