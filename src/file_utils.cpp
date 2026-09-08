#include "file_utils.hpp"
#include "exception.hpp"

namespace spk
{
	[[nodiscard]] std::string readTextFile(const std::filesystem::path &path)
	{
		std::ifstream file(path, std::ios::in | std::ios::binary);

		if (!file.is_open())
			throw Exception("Failed to open file: " + path.string());

		std::ostringstream stream;
		stream << file.rdbuf();

		if (file.bad())
			throw Exception("Failed to read file: " + path.string());

		return stream.str();
	}
}
