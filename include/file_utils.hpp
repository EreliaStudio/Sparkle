#pragma once

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace spk
{
	[[nodiscard]] std::string readTextFile(const std::filesystem::path &path);
}