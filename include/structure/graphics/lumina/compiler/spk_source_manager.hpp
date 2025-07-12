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
		void addIncludePath(const std::filesystem::path &path)
		{
			_includeFolders.emplace_back(path);
		}

		std::filesystem::path getFilePath(const std::filesystem::path &path);

		std::vector<Token> readToken(const std::filesystem::path &path);
		std::vector<Token> readToken(const std::wstring &virtualFile, const std::wstring &source);

	private:
		static std::wstring cleanSource(const std::wstring &source);

		std::vector<std::filesystem::path> _includeFolders;
	};

}