#include "utils/spk_string_utils.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>

namespace
{
	template <typename StringT>
	StringT collapseStringGeneric(const StringT &p_text, typename StringT::value_type p_collapseValue, const StringT &p_separators)
	{
		StringT result;
		bool collapsed = false;

		for (auto currentCharacter : p_text)
		{
			if (p_separators.find(currentCharacter) != StringT::npos)
			{
				if (!collapsed)
				{
					result.push_back(p_collapseValue);
					collapsed = true;
				}
			}
			else
			{
				result.push_back(currentCharacter);
				collapsed = false;
			}
		}

		auto first = result.find_first_not_of(p_collapseValue);
		if (first == StringT::npos)
		{
			return ({});
		}

		auto last = result.find_last_not_of(p_collapseValue);
		return (result.substr(first, last - first + 1));
	}
}

namespace spk
{
	namespace StringUtils
	{
		std::wstring stringToWString(const std::string &p_string)
		{
			return (std::wstring(p_string.begin(), p_string.end()));
		}

		std::string wstringToString(const std::wstring &p_wstring)
		{
			if (p_wstring.empty())
			{
				return (std::string());
			}

			int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &p_wstring[0], (int)p_wstring.size(), NULL, 0, NULL, NULL);
			std::string strTo(sizeNeeded, 0);
			WideCharToMultiByte(CP_UTF8, 0, &p_wstring[0], (int)p_wstring.size(), &strTo[0], sizeNeeded, NULL, NULL);

			return (strTo);
		}

		std::vector<std::string> splitString(const std::string &p_string, const std::string &p_delimiter)
		{
			if (p_string == "")
			{
				return (std::vector<std::string>());
			}

			std::vector<std::string> result;
			size_t start = 0;
			size_t end = p_string.find(p_delimiter);

			while (end != std::string::npos)
			{
				result.push_back(p_string.substr(start, end - start));
				start = end + p_delimiter.length();
				end = p_string.find(p_delimiter, start);
			}

			result.push_back(p_string.substr(start, end));
			return (result);
		}

		std::vector<std::wstring> splitWString(const std::wstring &p_wstring, const std::wstring &p_delimiter)
		{
			std::vector<std::wstring> result;
			size_t start = 0;
			size_t end = p_wstring.find(p_delimiter);

			while (end != std::wstring::npos)
			{
				result.push_back(p_wstring.substr(start, end - start));
				start = end + p_delimiter.length();
				end = p_wstring.find(p_delimiter, start);
			}

			result.push_back(p_wstring.substr(start, end));
			return (result);
		}

		std::wstring collapseString(const std::wstring &p_text, const wchar_t &p_mergedChar, const std::wstring &p_separators)
		{
			return (::collapseStringGeneric(p_text, p_mergedChar, p_separators));
		}

		std::string collapseString(const std::string &p_text, const char &p_mergedChar, const std::string &p_separators)
		{
			return (::collapseStringGeneric(p_text, p_mergedChar, p_separators));
		}

		std::wstring collapseWhitespace(const std::wstring &p_text)
		{
			static const std::wstring defaultSeparator = L" \t\n";
			return (::collapseStringGeneric(p_text, L' ', defaultSeparator));
		}

		std::string collapseWhitespace(const std::string &p_text)
		{
			static const std::string defaultSeparator = " \t\n";
			return (::collapseStringGeneric(p_text, ' ', defaultSeparator));
		}

		std::string trimWhitespace(const std::string &p_string)
		{
			const char *whitespace = " \t\n\r\f\v";
			size_t start = p_string.find_first_not_of(whitespace);
			size_t end = p_string.find_last_not_of(whitespace);
			return (start == std::string::npos || end == std::string::npos) ? "" : p_string.substr(start, end - start + 1);
		}

		std::wstring trimWhitespace(const std::wstring &p_string)
		{
			const wchar_t *whitespace = L" \t\n\r\f\v";
			size_t start = p_string.find_first_not_of(whitespace);
			size_t end = p_string.find_last_not_of(whitespace);
			return (start == std::string::npos || end == std::string::npos) ? L"" : p_string.substr(start, end - start + 1);
		}
	}
}
