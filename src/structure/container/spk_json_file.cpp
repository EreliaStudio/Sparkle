#include "structure/container/spk_JSON_file.hpp"

#include <algorithm>
#include <fstream>

#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

namespace spk::JSON
{
	std::wstring applyGrammar(const std::wstring &p_fileContent);
	std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index);
	std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index);

	void loadUnitNumbers(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);

	static void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);

	static std::wstring loadFileContent(const std::filesystem::path &p_filePath)
	{
		std::wifstream wif;

		wif.open(p_filePath);
		if (!wif.is_open())
		{
			GENERATE_ERROR("Failed to open file [" + p_filePath.string() + "]");
		}

		std::wstring result((std::istreambuf_iterator<wchar_t>(wif)), std::istreambuf_iterator<wchar_t>());
		wif.close();

		return (applyGrammar(result));
	}

	static void loadUnitString(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
	{
		p_objectToFill.set(p_unitSubString.substr(1, p_unitSubString.size() - 2));
	}

	static void loadUnitBoolean(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
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

	static void loadUnitNull(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString)
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

	static void loadArray(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
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

	static void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index)
	{
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
			GENERATE_ERROR("Unexpected data type in JSON: " + spk::StringUtils::wstringToString(p_content).substr(p_index, 10));
		}
	}

	File::File() :
		_root(L"Root")
	{
	}

	File::File(const std::filesystem::path &p_filePath) :
		_root(L"Root")
	{
		load(p_filePath);
	}

	void File::load(const std::filesystem::path &p_filePath)
	{
		std::wstring fileContent = loadFileContent(p_filePath);
		size_t index = 0;

		if (fileContent.empty())
		{
			GENERATE_ERROR("Empty file: " + p_filePath.string());
		}
		_root.reset();
		loadContent(_root, fileContent, index);
		if (index != fileContent.size())
		{
			GENERATE_ERROR("Invalid JSON file: " + p_filePath.string());
		}
	}

	File File::loadFromString(const std::wstring &p_content)
	{
		File result;
		size_t index = 0;

		result._root.reset();
		loadContent(result._root, applyGrammar(p_content), index);

		return (result);
	}

	void File::save(const std::filesystem::path &p_filePath) const
	{
		std::wofstream file;

		file.open(p_filePath);
		if (file.is_open() == false)
		{
			GENERATE_ERROR("Unable to open file: " + p_filePath.string());
		}
		file << _root;
		file.close();
	}

	bool File::contains(const std::wstring &p_key) const
	{
		return (_root.contains(p_key));
	}

	const spk::JSON::Object &File::operator[](const std::wstring &p_key) const
	{
		return (_root[p_key]);
	}

	const spk::JSON::Object &File::operator[](size_t p_index) const
	{
		return (_root[p_index]);
	}

	spk::JSON::Object &File::root()
	{
		return (_root);
	}

	const spk::JSON::Object &File::root() const
	{
		return (_root);
	}

	std::wostream &operator<<(std::wostream &p_os, const File &p_file)
	{
		p_os << p_file._root;
		return (p_os);
	}
}