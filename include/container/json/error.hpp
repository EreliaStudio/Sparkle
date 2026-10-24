#pragma once

#include <exception>
#include <filesystem>
#include <string>

#include <exception.hpp>

namespace spk::JSON
{
	[[noreturn]] inline void throwAt(
		const std::filesystem::path &file,
		const std::string &path,
		const std::string &message,
		std::exception_ptr cause = nullptr)
	{
		const std::string formatted = file.generic_string() + ":" + path + ": " + message;
		if (cause != nullptr)
		{
			throw spk::Exception(formatted, cause);
		}
		throw spk::Exception(formatted);
	}
}
