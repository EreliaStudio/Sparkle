#include "structure/container/spk_JSON_file.hpp"

#include <fstream>
#include <algorithm>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	namespace JSON
	{
		std::wstring _handleEscapeSequence(const std::wstring& p_fileContent)
		{
			std::wstring result;

			for (size_t i = 0; i < p_fileContent.size(); ++i)
			{
				if (p_fileContent[i] == L'\\')
				{
					switch (p_fileContent[i + 1])
					{
					case L'"':
						result += L'"';
						break;
					case L'\\':
						result += L"\\";
						break;
					case L'/':
						result += L'/';
						break;
					case L'b':
						result += L'\b';
						break;
					case L'f':
						result += L'\f';
						break;
					case L'n':
						result += L'\n';
						break;
					case L'r':
						result += L'\r';
						break;
					case L't':
						result += L'\t';
						break;
					case L'u':
						try
						{
							result += std::stoi(p_fileContent.substr(i + 2, 4), nullptr, 16);
						}
						catch (const std::exception&)
						{
							throw std::runtime_error("Invalid Unicode escape : <" + spk::StringUtils::wstringToString(p_fileContent).substr(i, 6) + '>');
						}
						i += 4;
						break;
					default:
						throw std::runtime_error("Invalid escape sequence: <" + spk::StringUtils::wstringToString(p_fileContent).substr(i, 2) + '>');
					}
					++i;
				}
				else
					result += p_fileContent[i];
			}
			return (result);
		}

		/**
		 * @brief Applies grammar rules to the file content to ensure valid JSON syntax.
		 * @param p_fileContent The content of the file that will be parsed.
		 * @return std::wstring The resulting string after applying grammar rules.
		 */
		std::wstring _applyGrammar(const std::wstring& p_fileContent)
		{
			std::wstring result;
			bool isLiteral(false);

			for (size_t i(0); i < p_fileContent.size(); ++i)
			{
				wchar_t c = p_fileContent[i];
				wchar_t next_c = p_fileContent[i + 1];

				switch (c)
				{
				case L'\\':
					if (std::wstring(L"\"\\/bfnrtu").find(next_c) == std::wstring::npos)
						throw std::runtime_error("Invalid escape sequence at line " +
							std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, L'\n')) +
							" column " + std::to_string(1 + i - p_fileContent.rfind('\n', i)) + ".");
					if (next_c == L'\\')
					{
						result += L"\\";
						++i;
					}
					result += c;
					break;
				case L'\"':
					if (i == 0 || (i >= 1 && p_fileContent[i - 1] != L'\\') ||
						(i >= 2 && p_fileContent[i - 1] == L'\\' && p_fileContent[i - 2] == L'\\'))
						isLiteral = !isLiteral;
					result += c;
					break;
				case L' ':
				case L'\t':
				case L'\n':
				case L'\r':
					if (isLiteral == true && c != L'\n' && c != L'\r')
						result += c;
					else if (isLiteral == true && (c == L'\n' || c == L'\r'))
						throw std::runtime_error("Unexpected end of string " +
							std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, '\n')) +
							" column " + std::to_string(i - p_fileContent.rfind('\n', i - 1)) + ".");
					break;
				default:
					result += c;
				}
			}
			return (result);
		}

		std::wstring _getAttributeName(const std::wstring& p_content, size_t& p_index)
		{
			if (p_content[p_index] != L'"')
				throw std::runtime_error("Invalid attribute name [" + spk::StringUtils::wstringToString(p_content).substr(p_index, 2) + "]");

			++p_index;
			size_t start = p_index;

			for (; p_index < p_content.size() && p_content[p_index] != L'"'; p_index++)
			{
				if (p_content[p_index] == L'\\' && p_content[p_index + 1] != L'"')
					++p_index;
			}
			++p_index;
			if (p_content[p_index] != L':')
				throw std::runtime_error("Invalid attribute name [" + spk::StringUtils::wstringToString(p_content).substr(start, p_index - start) + "]");
			++p_index;
			return (_handleEscapeSequence(p_content.substr(start, p_index - start - 2)));
		}

		/**
		 * @brief Extracts the unit substring from the content at the specified index.
		 * @param p_content The content string.
		 * @param p_index The current index in the content string. It will be updated to the index after the unit substring.
		 * @return The extracted unit substring.
		 */
		std::wstring _extractUnitSubstring(const std::wstring& p_content, size_t& p_index)
		{
			size_t oldIndex = p_index;
			bool isAString = false;

			if (p_content[p_index] == L'"')
			{
				isAString = true;
				++p_index;
			}
			for (; p_index < p_content.size() && ((p_content[p_index] != L',' &&
				p_content[p_index] != L'}' && p_content[p_index] != L']') ||
				isAString == true);
				p_index++)
			{
				if (isAString == true &&
					(p_index >= 1 && p_content[p_index] == L'"' && p_content[p_index - 1] != L'\\') ||
					(p_index >= 2 && p_content[p_index] == L'"' && p_content[p_index - 1] == L'\\' && p_content[p_index - 2] == L'\\'))
				{
					break;
				}
				if (isAString == false &&
					(p_content[p_index] == L'"' || p_content[p_index] == L'[' ||
						p_content[p_index] == L'{' || p_content[p_index] == L','))
					throw std::runtime_error("Invalid JSON unit [" + spk::StringUtils::wstringToString(p_content).substr(oldIndex, 5) + "]");
			}
			if (isAString == true)
			{
				if (p_content[p_index] != L'"')
					throw std::runtime_error("Invalid JSON unit [" + spk::StringUtils::wstringToString(p_content).substr(oldIndex, p_index - oldIndex) + "]");
				else
					++p_index;
			}

			return (_handleEscapeSequence(p_content.substr(oldIndex, p_index - oldIndex)));
		}
	}
}