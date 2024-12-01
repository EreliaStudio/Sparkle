#include "utils/spk_string_utils.hpp"
#include <windows.h>

namespace spk
{
	namespace StringUtils
	{
		std::wstring stringToWString(const std::string& p_str)
		{
			return std::wstring(p_str.begin(), p_str.end());
		}

		std::string wstringToString(const std::wstring& p_wstr)
		{
			if (p_wstr.empty())
			{
				return std::string();
			}

			int size_needed = WideCharToMultiByte(CP_UTF8, 0, &p_wstr[0], (int)p_wstr.size(), NULL, 0, NULL, NULL);
			std::string strTo(size_needed, 0);
			WideCharToMultiByte(CP_UTF8, 0, &p_wstr[0], (int)p_wstr.size(), &strTo[0], size_needed, NULL, NULL);

			return strTo;
		}

		std::vector<std::string> splitString(const std::string& p_str, const std::string& p_delimiter)
		{
			if (p_str == "")
				return (std::vector<std::string>());

			std::vector<std::string> result;
			size_t start = 0;
			size_t end = p_str.find(p_delimiter);

			while (end != std::string::npos)
			{
				result.push_back(p_str.substr(start, end - start));
				start = end + p_delimiter.length();
				end = p_str.find(p_delimiter, start);
			}

			result.push_back(p_str.substr(start, end));
			return result;
		}

		std::vector<std::wstring> splitWString(const std::wstring& p_wstr, const std::wstring& p_delimiter)
		{
			std::vector<std::wstring> result;
			size_t start = 0;
			size_t end = p_wstr.find(p_delimiter);

			while (end != std::wstring::npos)
			{
				result.push_back(p_wstr.substr(start, end - start));
				start = end + p_delimiter.length();
				end = p_wstr.find(p_delimiter, start);
			}

			result.push_back(p_wstr.substr(start, end));
			return result;
		}

		std::wstring mergeWhitespace(const std::wstring& str)
		{
			std::wstring result;
			bool spaceInserted = false;

			for (wchar_t ch : str)
			{
				if (ch == L'\t' || ch == L'\n' || ch == L' ')
				{
					if (!spaceInserted)
					{
						result += L' ';
						spaceInserted = true;
					}
				}
				else
				{
					result += ch;
					spaceInserted = false;
				}
			}

			auto start = result.find_first_not_of(L' ');
			auto end = result.find_last_not_of(L' ');

			if (start == std::wstring::npos)
			{
				return L"";
			}

			return result.substr(start, end - start + 1);
		}

		std::string mergeWhitespace(const std::string& str)
		{
			std::string result;
			bool spaceInserted = false;

			for (char ch : str)
			{
				if (ch == '\t' || ch == '\n' || ch == ' ')
				{
					if (!spaceInserted)
					{
						result += ' ';
						spaceInserted = true;
					}
				}
				else
				{
					result += ch;
					spaceInserted = false;
				}
			}

			auto start = result.find_first_not_of(' ');
			auto end = result.find_last_not_of(' ');

			if (start == std::string::npos)
			{
				return "";
			}

			return result.substr(start, end - start + 1);
		}

		std::string trimWhitespace(const std::string& p_str)
		{
			const char* whitespace = " \t\n\r\f\v";
			size_t start = p_str.find_first_not_of(whitespace);
			size_t end = p_str.find_last_not_of(whitespace);
			return (start == std::string::npos || end == std::string::npos) ? "" : p_str.substr(start, end - start + 1);
		}
		
		std::wstring trimWhitespace(const std::wstring& p_str)
		{
			const wchar_t* whitespace = L" \t\n\r\f\v";
			size_t start = p_str.find_first_not_of(whitespace);
			size_t end = p_str.find_last_not_of(whitespace);
			return (start == std::string::npos || end == std::string::npos) ? L"" : p_str.substr(start, end - start + 1);
		}
	}
}