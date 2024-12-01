#pragma once

#include <vector>
#include <filesystem>
#include <string>

namespace spk
{
	namespace FileUtils
	{
		std::vector<std::filesystem::path> listFiles(const std::filesystem::path& folderPath);
		std::wstring readFileAsWString(const std::filesystem::path& p_path);
		std::string readFileAsString(const std::filesystem::path& p_path);
		std::vector<uint8_t> readFileAsBytes(const std::filesystem::path& p_path);
	}
}