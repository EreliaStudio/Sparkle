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

namespace sparkle_test
{
    namespace
    {
        std::filesystem::path resourceRoot;
        std::filesystem::path resultRoot;

        [[nodiscard]] std::filesystem::path normalized(const std::filesystem::path&path)
        {
            std::error_code error;
            const std::filesystem::path result = std::filesystem::weakly_canonical(path, error);
            return error ?path.lexically_normal() : result;
        }

        [[nodiscard]] bool isSameOrChildPath(
            const std::filesystem::path&path,
            const std::filesystem::path&parent)
        {
            auto pathIterator =path.begin();
            auto parentIterator =parent.begin();
            for (; parentIterator !=parent.end(); ++parentIterator, ++pathIterator)
            {
                if (pathIterator ==path.end() || *pathIterator != *parentIterator)
                {
                    return false;
                }
            }
            return true;
        }
    }

    void configurePaths(const std::filesystem::path& resources, const std::filesystem::path& results)
    {
        if (resources.empty() || results.empty())
        {
            throw std::invalid_argument("Test resource and result roots must not be empty");
        }
        auto newResources = normalized(std::filesystem::absolute(resources));
        auto newResults = normalized(std::filesystem::absolute(results));
        resourceRoot.swap(newResources);
        resultRoot.swap(newResults);
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
        return resourceRoot.empty() ? executableDirectory() / "resources" : resourceRoot;
    }

    std::filesystem::path expectedImagesDirectory()
    {
        return resourcesDirectory() / "expectedImages";
    }

    std::filesystem::path resultsDirectory()
    {
        const std::filesystem::path result = (resultRoot.empty() ? executableDirectory() / "results" : resultRoot);
        std::filesystem::create_directories(result);
        return result;
    }

    std::filesystem::path expectedImagePath(
        const std::filesystem::path&category,
        const std::string&name)
    {
        return expectedImagesDirectory() /category / (name + ".png");
    }

    std::filesystem::path resultImagePath(
        const std::filesystem::path&category,
        const std::string&name)
    {
        return resultsDirectory() /category / (name + ".png");
    }

    void removeEmptyResultDirectories(const std::filesystem::path&startingPath)
    {
        const std::filesystem::path resultRoot = normalized(resultsDirectory());
        std::filesystem::path cursor = normalized(startingPath);

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
