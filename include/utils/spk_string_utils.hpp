#pragma once

#include <string>
#include <vector>

namespace spk
{
	namespace StringUtils
	{
		std::wstring stringToWString(const std::string &p_str);
		std::string wstringToString(const std::wstring &p_wstr);

		std::vector<std::string> splitString(const std::string &p_str, const std::string &p_delimiter);
		std::vector<std::wstring> splitWString(const std::wstring &p_wstr, const std::wstring &p_delimiter);

		std::wstring collapseString(const std::wstring& p_text, const wchar_t& p_mergedChar, const std::wstring& p_separators);
		std::string  collapseString(const std::string& p_text, const char& p_mergedChar, const std::string& p_separators);

		std::wstring collapseWhitespace(const std::wstring& p_text);
		std::string collapseWhitespace(const std::string& p_text);

		std::string trimWhitespace(const std::string &p_str);
		std::wstring trimWhitespace(const std::wstring &p_str);
	}
}