#include "structure/container/spk_JSON_file.hpp"

#include <algorithm>
#include <fstream>

#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk
{
	namespace JSON
	{
		std::wstring handleEscapeSequence(const std::wstring &p_fileContent)
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
						} catch (const std::exception &)
						{
							GENERATE_ERROR("Invalid Unicode escape : <" + spk::StringUtils::wstringToString(p_fileContent).substr(i, 6) +
													 '>');
						}
						i += 4;
						break;
					default:
						GENERATE_ERROR("Invalid escape sequence: <" + spk::StringUtils::wstringToString(p_fileContent).substr(i, 2) + '>');
					}
					++i;
				}
				else
				{
					result += p_fileContent[i];
				}
			}
			return (result);
		}

		std::wstring applyGrammar(const std::wstring &p_fileContent)
		{
			std::wstring result;
			bool isLiteral(false);

			for (size_t i(0); i < p_fileContent.size(); ++i)
			{
				wchar_t c = p_fileContent[i];
				wchar_t nextChar = p_fileContent[i + 1];

				switch (c)
				{
				case L'\\':
					if (std::wstring(L"\"\\/bfnrtu").find(nextChar) == std::wstring::npos)
					{
						GENERATE_ERROR("Invalid escape sequence at line " +
												 std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, L'\n')) +
												 " column " + std::to_string(1 + i - p_fileContent.rfind('\n', i)) + ".");
					}
					if (nextChar == L'\\')
					{
						result += L"\\";
						++i;
					}
					result += c;
					break;
				case L'\"':
					if (i == 0 || (i >= 1 && p_fileContent[i - 1] != L'\\') ||
						(i >= 2 && p_fileContent[i - 1] == L'\\' && p_fileContent[i - 2] == L'\\'))
					{
						isLiteral = !isLiteral;
					}
					result += c;
					break;
				case L' ':
				case L'\t':
				case L'\n':
				case L'\r':
					if (isLiteral == true && c != L'\n' && c != L'\r')
					{
						result += c;
					}
					else if (isLiteral == true && (c == L'\n' || c == L'\r'))
					{
						GENERATE_ERROR("Unexpected end of string " +
												 std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, '\n')) + " column " +
												 std::to_string(i - p_fileContent.rfind('\n', i - 1)) + ".");
					}
					break;
				default:
					result += c;
				}
			}
			return (result);
		}

		std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index)
		{
			if (p_content[p_index] != L'"')
			{
				GENERATE_ERROR("Invalid attribute name [" + spk::StringUtils::wstringToString(p_content).substr(p_index, 2) + "]");
			}

			++p_index;
			size_t start = p_index;

			for (; p_index < p_content.size() && p_content[p_index] != L'"'; p_index++)
			{
				if (p_content[p_index] == L'\\' && p_content[p_index + 1] != L'"')
				{
					++p_index;
				}
			}
			++p_index;
			if (p_content[p_index] != L':')
			{
				GENERATE_ERROR("Invalid attribute name [" + spk::StringUtils::wstringToString(p_content).substr(start, p_index - start) +
										 "]");
			}
			++p_index;
			return (handleEscapeSequence(p_content.substr(start, p_index - start - 2)));
		}

		/**
		 * @brief Extracts the unit substring from the content at the specified index.
		 * @param p_content The content string.
		 * @param p_index The current index in the content string. It will be updated to the index after the unit substring.
		 * @return The extracted unit substring.
		 */
		std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index)
		{
			size_t oldIndex = p_index;
			bool isAString = false;

			if (p_content[p_index] == L'"')
			{
				isAString = true;
				++p_index;
			}
			for (; p_index < p_content.size() &&
				   ((p_content[p_index] != L',' && p_content[p_index] != L'}' && p_content[p_index] != L']') || isAString == true);
				 p_index++)
			{
				if (isAString == true && (p_index >= 1 && p_content[p_index] == L'"' && p_content[p_index - 1] != L'\\') ||
					(p_index >= 2 && p_content[p_index] == L'"' && p_content[p_index - 1] == L'\\' && p_content[p_index - 2] == L'\\'))
				{
					break;
				}
				if (isAString == false &&
					(p_content[p_index] == L'"' || p_content[p_index] == L'[' || p_content[p_index] == L'{' || p_content[p_index] == L','))
				{
					GENERATE_ERROR("Invalid JSON unit [" + spk::StringUtils::wstringToString(p_content).substr(oldIndex, 5) + "]");
				}
			}
			if (isAString == true)
			{
				if (p_content[p_index] != L'"')
				{
					GENERATE_ERROR("Invalid JSON unit [" +
											 spk::StringUtils::wstringToString(p_content).substr(oldIndex, p_index - oldIndex) + "]");
				}
				else
				{
					++p_index;
				}
			}

			return (handleEscapeSequence(p_content.substr(oldIndex, p_index - oldIndex)));
		}
	}
}