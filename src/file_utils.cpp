#include "file_utils.hpp"

namespace spk
{
	[[nodiscard]] std::string readTextFile(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::in);

		if (!file.is_open())
			throw std::runtime_error("Failed to open file: " + path.string());

		std::ostringstream stream;
		stream << file.rdbuf();

		if (file.bad())
			throw std::runtime_error("Failed to read file: " + path.string());

		return stream.str();
	}
}