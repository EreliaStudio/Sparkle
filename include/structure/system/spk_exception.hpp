#pragma once

#include <stdexcept>
#include <string>

inline std::runtime_error composeErrorMessage(const std::string &msg, const std::exception &ex, const std::string &file, int line)
{
	std::string informationString = file + ':' + std::to_string(line) + '\n';
	std::string combined = "--> " + msg + "\n\n" + ex.what();

	std::string formatted;
	formatted.reserve(combined.size() + 8);

	for (char c : combined)
	{
		formatted.push_back(c);
		if (c == '\n')
		{
			formatted.append("    ");
		}
	}

	return std::runtime_error{informationString + formatted};
}

#define PROPAGATE_ERROR(msg, ex) throw composeErrorMessage(std::string(msg), (ex), __FILE__, __LINE__)
#define GENERATE_ERROR(msg) throw std::runtime_error(std::string(__FILE__) + ':' + std::to_string(__LINE__) + "\n--> " + msg)