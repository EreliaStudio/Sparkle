#pragma once

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace sparkle_test
{
	class TemporaryDirectory
	{
	private:
		std::filesystem::path _path;

		[[nodiscard]] static std::filesystem::path _makePath()
		{
			static std::atomic_uint64_t counter = 0;
			const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
			return std::filesystem::temp_directory_path() /
				("sparkle_tests_" + std::to_string(stamp) + "_" + std::to_string(counter.fetch_add(1)));
		}

	public:
		TemporaryDirectory() :
			_path(_makePath())
		{
			std::filesystem::create_directories(_path);
		}

		~TemporaryDirectory()
		{
			std::error_code error;
			std::filesystem::remove_all(_path, error);
		}

		TemporaryDirectory(const TemporaryDirectory &) = delete;
		TemporaryDirectory &operator=(const TemporaryDirectory &) = delete;

		[[nodiscard]] const std::filesystem::path &path() const noexcept
		{
			return _path;
		}

		[[nodiscard]] std::filesystem::path file(std::string_view name) const
		{
			return _path / std::filesystem::path(name);
		}

		void write(std::string_view name, std::string_view content) const
		{
			std::ofstream stream(file(name), std::ios::binary);
			stream.write(content.data(), static_cast<std::streamsize>(content.size()));
		}
	};

	[[nodiscard]] inline bool containsText(std::string_view text, std::string_view expected)
	{
		return text.find(expected) != std::string_view::npos;
	}
}
