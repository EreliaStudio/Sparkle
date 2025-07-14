#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"
#include "structure/spk_iostream.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_file_utils.hpp"
#include "utils/spk_string_utils.hpp"

#include <cwctype>
#include <filesystem>
#include <optional>
#include <unordered_map>

#include "structure/graphics/lumina/compiler/spk_Tokenizer.hpp"

namespace spk::Lumina
{
	void SourceManager::addIncludePath(const std::filesystem::path &p_path)
	{
		_includeFolders.emplace_back(p_path);
	}
	
	std::filesystem::path SourceManager::getFilePath(const std::filesystem::path &p_path)
	{
		if (std::filesystem::exists(p_path) && std::filesystem::is_regular_file(p_path))
		{
			return std::filesystem::canonical(p_path);
		}

		for (const auto &dir : _includeFolders)
		{
			auto candidate = dir / p_path;
			if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
			{
				return std::filesystem::canonical(candidate);
			}
		}
		throw std::runtime_error{"SourceManager::getFilePath - file not found: " + p_path.string()};
	}

	std::vector<Token> SourceManager::readToken(const std::filesystem::path &p_path)
	{
		std::filesystem::path full;
		try
		{
			full = getFilePath(p_path);
		}
		catch (const std::exception &e)
		{
			GENERATE_ERROR("File [" + p_path.string() + "] not found: " + e.what());
		}
		return readToken(full.filename().wstring(), spk::FileUtils::readFileAsWString(full));
	}

	std::vector<Token> SourceManager::readToken(const std::wstring &p_fileName, const std::wstring &p_source)
	{
		_files[p_fileName] = spk::StringUtils::splitWString(p_source, L"\n");
		return Tokenizer{p_source, p_fileName}.run();
	}

	const std::wstring& SourceManager::getSourceLine(const Location& p_location) const
	{
		static const std::wstring empty = L"";
		auto it = _files.find(p_location.source);
		if (it == _files.end() || p_location.line > it->second.size())
		{
			return empty;
		}
		return it->second[p_location.line];
	}
}
