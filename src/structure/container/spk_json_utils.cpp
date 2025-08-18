#include "structure/container/spk_json_file.hpp"
#include "structure/container/spk_json_object.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_string_utils.hpp"

#include <string>

namespace spk::JSON::utils
{
	std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index);
	void loadUnitNumbers(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index);
	std::wstring applyGrammar(const std::wstring &p_fileContent);

	void loadUnitString(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	void loadUnitBoolean(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);
	void loadUnitNull(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);

	void loadUnit(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadObject(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadArray(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);
	void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);

	void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
	{
		if (p_index >= p_content.size())
		{
			GENERATE_ERROR("Unexpected end of JSON content");
		}

		switch (p_content[p_index])
		{
		case '\"':
		case 'f':
		case 't':
		case 'n':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			loadUnit(p_objectToFill, p_content, p_index);
			break;
		case '{':
			loadObject(p_objectToFill, p_content, p_index);
			break;
		case '[':
			loadArray(p_objectToFill, p_content, p_index);
			break;
		default:
			GENERATE_ERROR("Unexpected data type in JSON: [" + spk::StringUtils::wstringToString(p_content).substr(p_index, 1) + "] (Char value: " + std::to_string(static_cast<int>(p_content[p_index])) + ")");
		}
	}

	static void loadUnit(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
	{
		std::wstring substring = extractUnitSubstring(p_content, p_index);

		switch (substring[0])
		{
		case '"':
			loadUnitString(p_objectToFill, substring);
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			loadUnitNumbers(p_objectToFill, substring);
			break;
		case 't':
		case 'f':
			loadUnitBoolean(p_objectToFill, substring);
			break;
		case 'n':
			loadUnitNull(p_objectToFill, substring);
			break;
		default:
			GENERATE_ERROR("Invalid JSON value: " + spk::StringUtils::wstringToString(substring));
			break;
		}
	}

	static void loadObject(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
	{
		p_objectToFill.setAsObject();

		if (p_content[p_index] != '{')
		{
			GENERATE_ERROR("Invalid JSON object (missing '{')");
		}
		p_index++;
		for (; p_index < p_content.size() && p_content[p_index] != '}';)
		{
			std::wstring attributeName = getAttributeName(p_content, p_index);

			spk::JSON::Object &newObject = p_objectToFill.addAttribute(attributeName);

			loadContent(newObject, p_content, p_index);

			if (p_content[p_index] == ',')
			{
				p_index++;
			}
			else if (p_content[p_index] != '}')
			{
				GENERATE_ERROR("Invalid JSON object (missing ',' or '}')");
			}
		}
		if (p_content[p_index] != '}')
		{
			GENERATE_ERROR("Invalid JSON object (missing '}')");
		}
		p_index++;
	}

	void loadArray(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
	{
		p_objectToFill.setAsArray();
		if (p_content[p_index] != '[')
		{
			GENERATE_ERROR("Invalid JSON array (missing '[')");
		}
		p_index++;
		for (; p_index < p_content.size() && p_content[p_index] != ']';)
		{
			spk::JSON::Object newObject(L"[" + std::to_wstring(p_objectToFill.size()) + L"]");

			loadContent(newObject, p_content, p_index);

			p_objectToFill.push_back(newObject);

			if (p_content[p_index] == ',')
			{
				p_index++;
			}
			else if (p_content[p_index] != ']')
			{
				GENERATE_ERROR("Invalid JSON array (missing ',' or ']')");
			}
		}
		if (p_content[p_index] != ']')
		{
			GENERATE_ERROR("Invalid JSON array (missing ']')");
		}
		p_index++;
	}

	void loadUnitString(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		p_objectToFill.set(p_unitSubString.substr(1, p_unitSubString.size() - 2));
	}

	void loadUnitBoolean(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		if (p_unitSubString == L"true")
		{
			p_objectToFill.set(true);
		}
		else if (p_unitSubString == L"false")
		{
			p_objectToFill.set(false);
		}
		else
		{
			GENERATE_ERROR("Invalid boolean JSON value: " + spk::StringUtils::wstringToString(p_unitSubString));
		}
	}

	void loadUnitNull(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		if (p_unitSubString == L"null")
		{
			p_objectToFill.set(nullptr);
		}
		else
		{
			GENERATE_ERROR("Invalid null JSON value: " + spk::StringUtils::wstringToString(p_unitSubString));
		}
	}

	void loadUnitString(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		p_objectToFill.set(p_unitSubString.substr(1, p_unitSubString.size() - 2));
	}

	void loadUnitBoolean(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		if (p_unitSubString == L"true")
		{
			p_objectToFill.set(true);
		}
		else if (p_unitSubString == L"false")
		{
			p_objectToFill.set(false);
		}
		else
		{
			GENERATE_ERROR("Invalid boolean JSON value: " + spk::StringUtils::wstringToString(p_unitSubString));
		}
	}

	void loadUnitNull(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		if (p_unitSubString == L"null")
		{
			p_objectToFill.set(nullptr);
		}
		else
		{
			GENERATE_ERROR("Invalid null JSON value: " + spk::StringUtils::wstringToString(p_unitSubString));
		}
	}
} // namespace spk::JSON::utils