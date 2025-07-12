#pragma once

#include <string>
#include <filesystem>
#include <vector>

#include "structure/graphics/lumina/compiler/spk_token.hpp"

namespace spk::Lumina
{
	class SourceManager
	{
	public:
		void addIncludePath(const std::filesystem::path &p_path);
		std::filesystem::path getFilePath(const std::filesystem::path &p_path);

		std::vector<Token> readToken(const std::filesystem::path &p_path);
		std::vector<Token> readToken(const std::wstring &p_fileName, const std::wstring &p_source);

	private:
		std::vector<std::filesystem::path> _includeFolders;
	};

}