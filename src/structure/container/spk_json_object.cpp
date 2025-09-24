#include "structure/container/spk_json_object.hpp"

#include <string>

#include "utils/spk_string_utils.hpp"

#include "structure/system/spk_exception.hpp"

#include "structure/container/spk_json_file.hpp"

#include "spk_debug_macro.hpp"

namespace spk
{
	namespace JSON
	{
		std::wstring applyGrammar(const std::wstring &p_fileContent);
		std::wstring extractUnitSubstring(const std::wstring &p_content, size_t &p_index);
		std::wstring getAttributeName(const std::wstring &p_content, size_t &p_index);

		void loadUnitNumbers(spk::JSON::Object &p_objectToFill, const std::wstring &p_unitSubString);

		static void loadContent(spk::JSON::Object &p_objectToFill, const std::wstring &p_content, size_t &p_index);

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

				p_objectToFill.pushBack(std::move(newObject));

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
				GENERATE_ERROR(
					"Unexpected data type in JSON: [" + spk::StringUtils::wstringToString(p_content).substr(p_index, 1) +
					"] (Char value: " + std::to_string(static_cast<int>(p_content[p_index])) + ")");
			}
		}

		size_t Object::_indent = 0;

		Object::Object(const std::wstring &p_name) :
			_name(p_name)
		{
			_initialized = false;
		}

		Object Object::fromString(const std::wstring &p_content)
		{
			spk::JSON::Object result;

			std::wstring cleanedContent = applyGrammar(p_content);

			size_t index = 0;
			loadContent(result, cleanedContent, index);

			return (result);
		}

		void Object::reset()
		{
			_initialized = false;
			_content = ContentType();
		}

		bool Object::isObject() const
		{
			return (_initialized && std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content));
		}

		bool Object::isArray() const
		{
			return (_initialized && std::holds_alternative<std::vector<std::shared_ptr<Object>>>(_content));
		}

		bool Object::isUnit() const
		{
			return (_initialized && std::holds_alternative<Unit>(_content));
		}

		Object &Object::addAttribute(const std::wstring &p_key)
		{
			if (_initialized == false)
			{
				_content = std::map<std::wstring, std::shared_ptr<Object>>();
				_initialized = true;
			}

			std::shared_ptr<Object> result = std::make_shared<Object>(p_key);
			Object *tmp = result.get();

			if (std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content).count(p_key) != 0)
			{
				GENERATE_ERROR("Can't add attribute named [" + spk::StringUtils::wstringToString(p_key) + "] : it already exists");
			}
			std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content)[p_key] = std::move(result);
			return (*tmp);
		}

		bool Object::contains(const std::wstring &p_key) const
		{
			if (_initialized == false ||
				std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Object does not hold a map, cannot access by key");
			}

			auto &map = std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content);

			return (map.contains(p_key));
		}

		Object &Object::operator[](const std::wstring &p_key)
		{
			if (_initialized == false)
			{
				setAsObject();
			}

			if (std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Object does not hold a map, cannot access by key");
			}

			auto &map = std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content);

			if (map.count(p_key) == 0)
			{
				addAttribute(p_key);
			}

			std::shared_ptr<Object> &result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false)
			{
				return (*(result));
			}
			else
			{
				if (std::holds_alternative<std::shared_ptr<Object>>(std::get<Unit>(result->_content)) == false)
				{
					return (*(result));
				}
				else
				{
					return (*(result->as<std::shared_ptr<Object>>()));
				}
			}
		}

		const Object &Object::operator[](const std::wstring &p_key) const
		{
			if (std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Object does not hold a map, cannot access by key");
			}

			auto &map = std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content);

			if (map.contains(p_key) == false)
			{
				GENERATE_ERROR("Can't acces JSON object named [" + spk::StringUtils::wstringToString(p_key) + "] : it does not exist");
			}

			const std::shared_ptr<Object> &result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false)
			{
				return (*(result));
			}
			else
			{
				if (std::holds_alternative<std::shared_ptr<Object>>(std::get<Unit>(result->_content)) == false)
				{
					return (*(result));
				}
				else
				{
					return (*(result->as<std::shared_ptr<Object>>()));
				}
			}
		}

		const std::map<std::wstring, std::shared_ptr<Object>> &Object::members() const
		{
			if (_initialized == false || std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Can't get object members : object is not initialized or is not of type object");
			}
			return (std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content));
		}

		void Object::setAsObject()
		{
			if (_initialized == true)
			{
				GENERATE_ERROR("Can't set object as object : it is already initialized");
			}
			_content = std::map<std::wstring, std::shared_ptr<Object>>();
			_initialized = true;
		}

		const std::vector<std::shared_ptr<Object>> &Object::asArray() const
		{
			if (_initialized == false || std::holds_alternative<std::vector<std::shared_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Can't get object as array : object is not initialized or is not of type array");
			}
			return (std::get<std::vector<std::shared_ptr<Object>>>(_content));
		}

		void Object::resize(size_t p_size)
		{
			if (_initialized == false)
			{
				_content = std::vector<std::shared_ptr<Object>>();
				_initialized = true;
			}

			auto &array = std::get<std::vector<std::shared_ptr<Object>>>(_content);

			const size_t oldSize = array.size();
			array.resize(p_size);

			for (size_t i = oldSize; i < p_size; ++i)
			{
				array[i] = std::make_shared<Object>(L"[" + std::to_wstring(i) + L"]");
			}
		}

		Object &Object::append()
		{
			if (_initialized == false)
			{
				_content = std::vector<std::shared_ptr<Object>>();
				_initialized = true;
			}

			std::shared_ptr<Object> newObject =
				std::make_shared<Object>(L"[" + std::to_wstring(std::get<std::vector<std::shared_ptr<Object>>>(_content).size()) + L"]");
			Object *result = newObject.get();
			std::get<std::vector<std::shared_ptr<Object>>>(_content).push_back(std::move(newObject));
			return (*result);
		}

		void Object::pushBack(const Object &p_object)
		{
			if (_initialized == false)
			{
				_content = std::vector<std::shared_ptr<Object>>();
				_initialized = true;
			}

			std::get<std::vector<std::shared_ptr<Object>>>(_content).push_back(std::make_shared<Object>(p_object));
		}

		Object &Object::operator[](size_t p_index)
		{
			if (!std::holds_alternative<std::vector<std::shared_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold an array, cannot access by index");
			}

			auto &vec = std::get<std::vector<std::shared_ptr<Object>>>(_content);

			// Check if the index is within bounds
			if (p_index >= vec.size())
			{
				throw std::out_of_range("Index out of range");
			}

			return *(vec[p_index]);
		}

		const Object &Object::operator[](size_t p_index) const
		{
			if (!std::holds_alternative<std::vector<std::shared_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold an array, cannot access by index");
			}

			const std::vector<std::shared_ptr<Object>> &vec = std::get<std::vector<std::shared_ptr<Object>>>(_content);

			if (p_index >= vec.size())
			{
				throw std::out_of_range("Index out of range");
			}

			return *(vec[p_index]);
		}

		void Object::setAsArray()
		{
			if (_initialized == true)
			{
				GENERATE_ERROR("Can't set object as Array : it is already initialized");
			}
			_content = std::vector<std::shared_ptr<Object>>();
			_initialized = true;
		}

		size_t Object::size() const
		{
			if (_initialized == false)
			{
				GENERATE_ERROR("Can't get object size : it is uninitialized");
			}
			if (!std::holds_alternative<std::vector<std::shared_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a vector, cannot perform size operation");
			}
			return (std::get<std::vector<std::shared_ptr<Object>>>(_content).size());
		}

		size_t Object::count(const std::wstring &p_key) const
		{
			if (!std::holds_alternative<std::map<std::wstring, std::shared_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a map, cannot perform count operation");
			}
			return (std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content).count(p_key));
		}

		void Object::_printUnit(std::wostream &p_os) const
		{
			const Unit &tmp = std::get<Unit>(_content);

			switch (tmp.index())
			{
			case 0:
				p_os << std::boolalpha << as<bool>();
				break;
			case 1:
				p_os << std::fixed << as<long>();
				break;
			case 2:
				p_os << std::fixed << as<double>();
				break;
			case 3:
				p_os << '"' << as<std::wstring>() << '"';
				break;
			case 4:
				p_os << *(as<std::shared_ptr<Object>>());
				break;
			case 5:
				p_os << "null";
				break;
			}
		}

		void Object::_printObject(std::wostream &p_os) const
		{
			const std::map<std::wstring, std::shared_ptr<Object>> &map = std::get<std::map<std::wstring, std::shared_ptr<Object>>>(_content);
			std::wstring cleanedKey;

			p_os << std::setw(_indent * _indentSize) << "{" << std::endl;
			++_indent;
			for (auto &tmp : map)
			{
				cleanedKey = tmp.first;

				p_os << std::setw(_indent * _indentSize) << '"' << cleanedKey << "\": ";

				if (tmp.second->_content.index() == 1 || tmp.second->_content.index() == 2)
				{
					p_os << std::endl;
				}

				p_os << *(tmp.second);
				if (&tmp != &(*map.rbegin()))
				{
					p_os << ',';
				}
				p_os << std::endl;
			}
			--_indent;
			p_os << std::setw(_indent * _indentSize) << "}";
		}

		void Object::_printArray(std::wostream &p_os) const
		{
			const std::vector<std::shared_ptr<Object>> &vector = std::get<std::vector<std::shared_ptr<Object>>>(_content);

			p_os << std::setw(_indent * _indentSize) << '[' << std::endl;
			++_indent;
			for (size_t i = 0; i < vector.size(); i++)
			{
				p_os << std::setw(_indent * _indentSize) << *(vector[i]);
				if (i != vector.size() - 1)
				{
					p_os << ',';
				}
				p_os << std::endl;
			}
			--_indent;
			p_os << std::setw(_indent * _indentSize) << ']';
		}

		std::wostream &operator<<(std::wostream &p_os, const Object &p_object)
		{
			switch (p_object._content.index())
			{
			case 0:
				p_object._printUnit(p_os);
				break;
			case 1:
				p_object._printObject(p_os);
				break;
			case 2:
				p_object._printArray(p_os);
				break;
			}
			return (p_os);
		}

		std::ostream &operator<<(std::ostream &p_os, const Object &p_object)
		{
			std::wstringstream wos;
			wos << p_object;
			p_os << spk::StringUtils::wstringToString(wos.str());
			return (p_os);
		}
	}
}