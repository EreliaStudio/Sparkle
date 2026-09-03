#pragma once

#include <filesystem>
#include <string>

namespace sparkle_test
{
    [[nodiscard]] std::filesystem::path executablePath();
    [[nodiscard]] std::filesystem::path executableDirectory();
    [[nodiscard]] std::filesystem::path resourcesDirectory();
    [[nodiscard]] std::filesystem::path expectedImagesDirectory();
    [[nodiscard]] std::filesystem::path resultsDirectory();
    [[nodiscard]] std::filesystem::path expectedImagePath(
        const std::filesystem::path& p_category,
        const std::string& p_name);
    [[nodiscard]] std::filesystem::path resultImagePath(
        const std::filesystem::path& p_category,
        const std::string& p_name);
    void removeEmptyResultDirectories(const std::filesystem::path& p_startingPath);
}
