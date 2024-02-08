#include "miscellaneous/JSON/spk_JSON_file.hpp"

#include <fstream>
#include <algorithm>

#include "spk_basic_functions.hpp"

namespace spk
{
	namespace JSON
	{
		std::string _applyGrammar(const std::string& p_fileContent);
		std::string _extractUnitSubstring(const std::string& p_content, size_t& p_index);
		std::string _getAttributeName(const std::string& p_content, size_t& p_index);

		void _loadUnitNumbers(spk::JSON::Object& p_objectToFill, const std::string& p_unitSubString);

		static void _loadContent(spk::JSON::Object& p_objectToFill, const std::string& p_content, size_t& p_index);

		static std::string _loadFileContent(const std::filesystem::path& p_filePath)
		{
			std::ifstream wif;

			wif.open(p_filePath);
			if (!wif.is_open())
			{
				throwException("Failed to open file [" + p_filePath.string() + "]");
			}

			std::string result((std::istreambuf_iterator<char>(wif)), std::istreambuf_iterator<char>());
			wif.close();

			return (_applyGrammar(result));
		}

		static void _loadUnitString(spk::JSON::Object& p_objectToFill, const std::string& p_unitSubString)
		{
			p_objectToFill.set(p_unitSubString.substr(1, p_unitSubString.size() - 2));
		}

		static void _loadUnitBoolean(spk::JSON::Object& p_objectToFill, const std::string& p_unitSubString)
		{
			if (p_unitSubString == "true")
				p_objectToFill.set(true);
			else if (p_unitSubString == "false")
				p_objectToFill.set(false);
			else
				throwException("Invalid boolean JSON value: " + p_unitSubString);
		}

		static void _loadUnitNull(spk::JSON::Object& p_objectToFill, const std::string& p_unitSubString)
		{
			if (p_unitSubString == "null")
				p_objectToFill.set(nullptr);
			else
				throwException("Invalid null JSON value: " + p_unitSubString);
		}

		static void _loadUnit(spk::JSON::Object& p_objectToFill, const std::string& p_content, size_t& p_index)
		{
			std::string substring = _extractUnitSubstring(p_content, p_index);

			switch (substring[0])
			{
			case '"':
				_loadUnitString(p_objectToFill, substring);
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case '-':
				_loadUnitNumbers(p_objectToFill, substring);
				break;
			case 't': case 'f':
				_loadUnitBoolean(p_objectToFill, substring);
				break;
			case 'n':
				_loadUnitNull(p_objectToFill, substring);
				break;
			default:
				throwException("Invalid JSON value: " + substring);
				break;
			}
		}

		static void _loadObject(spk::JSON::Object& p_objectToFill, const std::string& p_content, size_t& p_index)
		{
			p_objectToFill.setAsObject();

			if (p_content[p_index] != '{')
				throwException("Invalid JSON object (missing '{')");
			p_index++;
			for (; p_index < p_content.size() && p_content[p_index] != '}';)
			{
				std::string attributeName = _getAttributeName(p_content, p_index);

				spk::JSON::Object& newObject = p_objectToFill.addAttribute(attributeName);

				_loadContent(newObject, p_content, p_index);

				if (p_content[p_index] == ',')
					p_index++;
				else if (p_content[p_index] != '}')
					throwException("Invalid JSON object (missing ',' or '}')");
			}
			if (p_content[p_index] != '}')
				throwException("Invalid JSON object (missing '}')");
			p_index++;
		}

		static void _loadArray(spk::JSON::Object& p_objectToFill, const std::string& p_content, size_t& p_index)
		{
			p_objectToFill.setAsArray();
			if (p_content[p_index] != '[')
				throwException("Invalid JSON array (missing '[')");
			p_index++;
			for (; p_index < p_content.size() && p_content[p_index] != ']';)
			{
				spk::JSON::Object newObject("[" + std::to_string(p_objectToFill.size()) + "]");

				_loadContent(newObject, p_content, p_index);

				p_objectToFill.push_back(newObject);

				if (p_content[p_index] == ',')
					p_index++;
				else if (p_content[p_index] != ']')
					throwException("Invalid JSON array (missing ',' or ']')");
			}
			if (p_content[p_index] != ']')
				throwException("Invalid JSON array (missing ']')");
			p_index++;
		}

		static void _loadContent(spk::JSON::Object& p_objectToFill, const std::string& p_content, size_t& p_index)
		{
			switch (p_content[p_index])
			{
			case '\"':
			case 'f': case 't': case 'n':
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
			case '-':
				_loadUnit(p_objectToFill, p_content, p_index);
				break;
			case '{':
				_loadObject(p_objectToFill, p_content, p_index);
				break;
			case '[':
				_loadArray(p_objectToFill, p_content, p_index);
				break;
			default:
				throwException("Unexpected data type in JSON: " + p_content.substr(p_index, 10));
			}
		}

		File::File() :
			_root("Root")
		{
		}

		File::File(const std::filesystem::path& p_filePath) :
			_root("Root")
		{
			load(p_filePath);
		}

		void File::load(const std::filesystem::path& p_filePath)
		{
			std::string fileContent = _loadFileContent(p_filePath);
			size_t index = 0;

			if (fileContent.empty())
				throwException("Empty file: " + p_filePath.string());
			_root.reset();
			_loadContent(_root, fileContent, index);
			if (index != fileContent.size())
				throwException("Invalid JSON file: " + p_filePath.string());
		}

		void File::save(const std::filesystem::path& p_filePath) const
		{
			std::ofstream file;

			file.open(p_filePath);
			if (file.is_open() == false)
				throwException("Unable to open file: " + p_filePath.string());
			file << _root;
			file.close();
		}
		
		bool File::contains(const std::string& p_key) const
		{
			return (_root.contains(p_key));
		}

		const spk::JSON::Object& File::operator[](const std::string& p_key) const
		{
			return (_root[p_key]);
		}

		const spk::JSON::Object& File::operator[](size_t p_index) const
		{
			return (_root[p_index]);
		}

		const spk::JSON::Object& File::root() const
		{
			return (_root);
		}

		std::ostream& operator<<(std::ostream& p_os, const File& p_file)
		{
			p_os << p_file._root;
			return (p_os);
		}
	}
}