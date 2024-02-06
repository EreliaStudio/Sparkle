#include "miscellaneous/JSON/spk_JSON_file.hpp"

#include <fstream>
#include <algorithm>

#include "spk_basic_functions.hpp"

namespace spk
{
	namespace JSON
	{
		/**
		 * @brief Convert escape sequence such as '\\r' to their corresponding character.
		 * Also handle universal character name such as '\\uXXXX'.
		 *
		 * @param p_fileContent The string to start from.
		 * @return std::string The resulting string after handling.
		 */
		std::string _handleEscapeSequence(const std::string& p_fileContent)
		{
			std::string result;

			for (size_t i = 0; i < p_fileContent.size(); ++i)
			{
				if (p_fileContent[i] == '\\')
				{
					switch (p_fileContent[i + 1])
					{
					case '"':
						result += '"';
						break;
					case '\\':
						result += "\\";
						break;
					case '/':
						result += '/';
						break;
					case 'b':
						result += '\b';
						break;
					case 'f':
						result += '\f';
						break;
					case 'n':
						result += '\n';
						break;
					case 'r':
						result += '\r';
						break;
					case 't':
						result += '\t';
						break;
					case 'u':
						try
						{
							result += std::stoi(p_fileContent.substr(i + 2, 4), nullptr, 16);
						}
						catch (const std::exception&)
						{
							throwException("Invalid Unicode escape : <" + p_fileContent.substr(i, 6) + '>');
						}
						i += 4;
						break;
					default:
						throwException("Invalid escape sequence: <" + p_fileContent.substr(i, 2) + '>');
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
		 * @return std::string The resulting string after applying grammar rules.
		 */
		std::string _applyGrammar(const std::string& p_fileContent)
		{
			std::string result;
			bool isLiteral(false);

			for (size_t i(0); i < p_fileContent.size(); ++i)
			{
				char c = p_fileContent[i];
				char next_c = p_fileContent[i + 1];

				switch (c)
				{
				case '\\':
					if (std::string("\"\\/bfnrtu").find(next_c) == std::string::npos)
						throwException("Invalid escape sequence at line " +
							std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, '\n')) +
							" column " + std::to_string(1 + i - p_fileContent.rfind('\n', i)) + ".");
					if (next_c == '\\')
					{
						result += "\\";
						++i;
					}
					result += c;
					break;
				case '\"':
					if (i == 0 || (i >= 1 && p_fileContent[i - 1] != '\\') ||
						(i >= 2 && p_fileContent[i - 1] == '\\' && p_fileContent[i - 2] == '\\'))
						isLiteral = !isLiteral;
					result += c;
					break;
				case ' ': case '\t': case '\n': case '\r':
					if (isLiteral == true && c != '\n' && c != '\r')
						result += c;
					else if (isLiteral == true && (c == '\n' || c == '\r'))
						throwException("Unexpected end of string " +
							std::to_string(1 + std::count(p_fileContent.begin(), p_fileContent.begin() + i, '\n')) +
							" column " + std::to_string(i - p_fileContent.rfind('\n', i - 1)) + ".");
					break;
				default:
					result += c;
				}
			}
			return (result);
		}

		/**
		 * @brief Extracts the attribute name from the content at the specified index.
		 * @param p_content The content string.
		 * @param p_index The current index in the content string. It will be updated to the index after the attribute name.
		 * @return The extracted attribute name.
		 */
		std::string _getAttributeName(const std::string& p_content, size_t& p_index)
		{
			if (p_content[p_index] != '"')
				throwException("Invalid attribute name [" + p_content.substr(p_index, 2) + "]");

			++p_index;
			size_t start = p_index;

			for (; p_index < p_content.size() && p_content[p_index] != '"'; p_index++)
			{
				if (p_content[p_index] == '\\' && p_content[p_index + 1] != '"')
					++p_index;
			}
			++p_index;
			if (p_content[p_index] != ':')
				throwException("Invalid attribute name [" + p_content.substr(start, p_index - start) + "]");
			++p_index;
			return (_handleEscapeSequence(p_content.substr(start, p_index - start - 2)));
		}

		/**
		 * @brief Extracts the unit substring from the content at the specified index.
		 * @param p_content The content string.
		 * @param p_index The current index in the content string. It will be updated to the index after the unit substring.
		 * @return The extracted unit substring.
		 */
		std::string _extractUnitSubstring(const std::string& p_content, size_t& p_index)
		{
			size_t oldIndex = p_index;
			bool isAString = false;

			if (p_content[p_index] == '"')
			{
				isAString = true;
				++p_index;
			}
			for (; p_index < p_content.size() && ((p_content[p_index] != ',' &&
				p_content[p_index] != '}' && p_content[p_index] != ']') ||
				isAString == true);
				p_index++)
			{
				if (isAString == true &&
					(p_index >= 1 && p_content[p_index] == '"' && p_content[p_index - 1] != '\\') ||
					(p_index >= 2 && p_content[p_index] == '"' && p_content[p_index - 1] == '\\' && p_content[p_index - 2] == '\\'))
				{
					break;
				}
				if (isAString == false &&
					(p_content[p_index] == '"' || p_content[p_index] == '[' ||
						p_content[p_index] == '{' || p_content[p_index] == ','))
					throwException("Invalid JSON unit [" + p_content.substr(oldIndex, 5) + "]");
			}
			if (isAString == true)
			{
				if (p_content[p_index] != '"')
					throwException("Invalid JSON unit [" + p_content.substr(oldIndex, p_index - oldIndex) + "]");
				else
					++p_index;
			}

			return (_handleEscapeSequence(p_content.substr(oldIndex, p_index - oldIndex)));
		}
	}
}