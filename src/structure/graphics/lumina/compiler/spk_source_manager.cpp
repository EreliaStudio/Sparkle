// SourceManager_refactored.hpp
// Lightweight re‑implementation of the shader SourceManager & lexer.
// Keeps the public interface intact while isolating lexing state in a dedicated class
// and drastically reducing nested conditionals.
//
// © 2025  — rewrite by ChatGPT

#pragma once

#include "structure/graphics/lumina/compiler/spk_source_manager.hpp" // token types / Location
#include "structure/spk_iostream.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_file_utils.hpp"
#include "utils/spk_string_utils.hpp"

#include <cwctype>
#include <filesystem>
#include <optional>
#include <regex>
#include <unordered_map>

#include "structure/graphics/lumina/compiler/spk_lexer.hpp"

namespace spk::Lumina
{
	std::filesystem::path SourceManager::getFilePath(const std::filesystem::path &path)
	{
		if (std::filesystem::exists(path) && std::filesystem::is_regular_file(path))
		{
			return std::filesystem::canonical(path);
		}

		for (const auto &dir : _includeFolders)
		{
			auto candidate = dir / path;
			if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
			{
				return std::filesystem::canonical(candidate);
			}
		}
		throw std::runtime_error{"SourceManager::getFilePath – file not found: " + path.string()};
	}

	std::wstring SourceManager::cleanSource(const std::wstring &source)
	{
		static const std::wregex comments{LR"((//[^\n]*|/\*[\s\S]*?\*/))", std::regex_constants::ECMAScript | std::regex_constants::optimize};
		return spk::StringUtils::collapseString(std::regex_replace(source, comments, L""), L' ', L" \t");
	}

	std::vector<Token> SourceManager::readToken(const std::filesystem::path &path)
	{
		std::filesystem::path full;
		try
		{
			full = getFilePath(path);
		}
		catch (const std::exception &e)
		{
			GENERATE_ERROR("File [" + path.string() + "] not found: " + e.what());
		}
		return readToken(full.filename().wstring(), spk::FileUtils::readFileAsWString(full));
	}

	std::vector<Token> SourceManager::readToken(const std::wstring &virtualFile, const std::wstring &source)
	{
		const std::wstring cleaned = cleanSource(source);

		return Lexer{cleaned, virtualFile}.run();
	}
}
