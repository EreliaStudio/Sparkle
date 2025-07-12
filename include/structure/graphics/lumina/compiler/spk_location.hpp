#pragma once

#include <filesystem>

namespace spk::Lumina
{
	struct Location
	{
		std::filesystem::path source;
		size_t line;
		size_t column;
	};
}