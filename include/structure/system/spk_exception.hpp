#pragma once

#include <string>
#include <stdexcept>

inline std::runtime_error composeErrorMessage(const std::string& msg,
                                              const std::exception& ex,
                                              const std::string& file,
                                              int line)
{
	std::string informationString = file + ':' + std::to_string(line) + '\n';
    std::string combined = "--> " + msg + "\n\n" + ex.what();

    std::string formatted;
    formatted.reserve(combined.size() + 8);

    for (char c : combined)
    {
        formatted.push_back(c);
        if (c == '\n')
            formatted.append("    ");
    }

    return std::runtime_error{informationString + formatted};
}

#define PROPAGATE_ERROR(msg, ex) throw composeErrorMessage((msg), (ex), __FILE__, __LINE__)