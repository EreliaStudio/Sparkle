#include "sparkle_test/paths.hpp"

#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

#if !defined(SPARKLE_TEST_RESOURCES_DIR)
#error SPARKLE_TEST_RESOURCES_DIR must be provided by the build system
#endif

#if !defined(SPARKLE_TEST_RESULTS_DIR)
#error SPARKLE_TEST_RESULTS_DIR must be provided by the build system
#endif

namespace sparkle_test
{
    namespace
    {
        [[nodiscard]] std::filesystem::path normalized(const std::filesystem::path& p_path)
        {
            std::error_code error;
            const std::filesystem::path result = std::filesystem::weakly_canonical(p_path, error);
            return error ? p_path.lexically_normal() : result;
        }

        [[nodiscard]] bool isSameOrChildPath(
            const std::filesystem::path& p_path,
            const std::filesystem::path& p_parent)
        {
            auto pathIterator = p_path.begin();
            auto parentIterator = p_parent.begin();
            for (; parentIterator != p_parent.end(); ++parentIterator, ++pathIterator)
            {
                if (pathIterator == p_path.end() || *pathIterator != *parentIterator)
                {
                    return false;
                }
            }
            return true;
        }
    }

    std::filesystem::path executablePath()
    {
#if defined(_WIN32)
        std::vector<wchar_t> buffer(512);
        while (true)
        {
            const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
            if (length == 0)
            {
                throw std::runtime_error("Unable to resolve the test executable path");
            }
            if (static_cast<std::size_t>(length) < buffer.size())
            {
                return normalized(std::filesystem::path(std::wstring_view(buffer.data(), length)));
            }
            buffer.resize(buffer.size() * 2);
        }
#elif defined(__linux__)
        std::vector<char> buffer(512);
        while (true)
        {
            const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());
            if (length < 0)
            {
                throw std::runtime_error("Unable to resolve the test executable path");
            }
            if (static_cast<std::size_t>(length) < buffer.size())
            {
                return normalized(std::filesystem::path(std::string_view(buffer.data(), static_cast<std::size_t>(length))));
            }
            buffer.resize(buffer.size() * 2);
        }
#else
        return normalized(std::filesystem::current_path());
#endif
    }

    std::filesystem::path executableDirectory()
    {
        return executablePath().parent_path();
    }

    std::filesystem::path resourcesDirectory()
    {
        return normalized(SPARKLE_TEST_RESOURCES_DIR);
    }

    std::filesystem::path expectedImagesDirectory()
    {
        return resourcesDirectory() / "expectedImages";
    }

    std::filesystem::path resultsDirectory()
    {
        const std::filesystem::path result = normalized(SPARKLE_TEST_RESULTS_DIR);
        std::filesystem::create_directories(result);
        return result;
    }

    std::filesystem::path expectedImagePath(
        const std::filesystem::path& p_category,
        const std::string& p_name)
    {
        return expectedImagesDirectory() / p_category / (p_name + ".png");
    }

    std::filesystem::path resultImagePath(
        const std::filesystem::path& p_category,
        const std::string& p_name)
    {
        return resultsDirectory() / p_category / (p_name + ".png");
    }

    void removeEmptyResultDirectories(const std::filesystem::path& p_startingPath)
    {
        const std::filesystem::path resultRoot = normalized(resultsDirectory());
        std::filesystem::path cursor = normalized(p_startingPath);

        if (std::filesystem::is_regular_file(cursor))
        {
            cursor = cursor.parent_path();
        }

        if (!isSameOrChildPath(cursor, resultRoot))
        {
            return;
        }

        while (!cursor.empty() && isSameOrChildPath(cursor, resultRoot))
        {
            std::error_code error;
            if (!std::filesystem::is_empty(cursor, error) || error)
            {
                break;
            }
            std::filesystem::remove(cursor, error);
            if (error || cursor == resultRoot)
            {
                break;
            }
            cursor = cursor.parent_path();
        }
    }
}
