#include "structure/container/spk_json_object.hpp"

#include "structure/container/spk_json_file.hpp"
#include "structure/system/spk_exception.hpp"
#include "utils/spk_string_utils.hpp"

#include <string>

#include "structure/container/spk_json_utils.hpp"

namespace spk
{
	namespace JSON
	{
		size_t Object::_indent = 0;

		Object::Object(const std::wstring &p_name) :
			_name(p_name),
			_initialized(false)
		{
		}

		Object Object::fromString(const std::wstring &p_content)
		{
			spk::JSON::Object result;

			std::wstring cleanedContent = spk::JSON::utils::applyGrammar(p_content);

			size_t index = 0;
			spk::JSON::utils::loadContent(result, cleanedContent, index);

			return (result);
		}

		void Object::reset()
		{
			_initialized = false;
			_content = ContentType();
		}

		bool Object::isObject() const
		{
			return (_initialized && std::holds_alternative<std::map<std::wstring, std::unique_ptr<Object>>>(_content));
		}

		bool Object::isArray() const
		{
			return (_initialized && std::holds_alternative<std::vector<std::unique_ptr<Object>>>(_content));
		}

		bool Object::isUnit() const
		{
			return (_initialized && std::holds_alternative<Unit>(_content));
		}

		Object &Object::addAttribute(const std::wstring &p_key)
		{
			if (_initialized == false)
			{
				_content = std::map<std::wstring, std::unique_ptr<Object>>();
				_initialized = true;
			}

			std::unique_ptr<Object> result = std::make_unique<Object>(p_key);

			if (std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content).contains(p_key) == false)
			{
				GENERATE_ERROR("Can't add attribute named [" + spk::StringUtils::wstringToString(p_key) + "] : it already exists");
			}
			Object *tmpResult = result.get();
			std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content)[p_key] = std::move(result);
			return (*tmpResult);
		}

		bool Object::contains(const std::wstring &p_key) const
		{
			auto &map = std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content);

			return (map.contains(p_key));
		}

		Object &Object::operator[](const std::wstring &p_key)
		{
			if (_initialized == false)
			{
				setAsObject();
			}

			if (!std::holds_alternative<std::map<std::wstring, std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a map, cannot access by key");
			}

			auto &map = std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content);

			if (map.contains(p_key) == false)
			{
				addAttribute(p_key);
			}

			std::unique_ptr<Object> result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false ||
				std::holds_alternative<std::unique_ptr<Object>>(std::get<Unit>(result->_content)) == false)
			{
				return (*(result));
			}

			return (*(result->as<std::unique_ptr<Object>>()));
		}

		const Object &Object::operator[](const std::wstring &p_key) const
		{
			if (!std::holds_alternative<std::map<std::wstring, std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a map, cannot access by key");
			}

			auto &map = std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content);

			if (map.contains(p_key) == false)
			{
				GENERATE_ERROR("Can't acces JSON object named [" + spk::StringUtils::wstringToString(p_key) + "] : it does not exist");
			}

			const std::unique_ptr<Object> &result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false)
			{
				return (*(result));
			}
			
			if (std::holds_alternative<std::unique_ptr<Object>>(std::get<Unit>(result->_content)) == false)
			{
				return (*(result));
			}
			else
			{
				return (*(result->as<std::unique_ptr<Object>>()));
			}
		}

		const std::map<std::wstring, std::unique_ptr<Object>> &Object::members() const
		{
			if (_initialized == false || std::holds_alternative<std::map<std::wstring, std::unique_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Can't get object members : object is not initialized or is not of type object");
			}
			return (std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content));
		}

		void Object::setAsObject()
		{
			if (_initialized == true)
			{
				GENERATE_ERROR("Can't set object as object : it is already initialized");
			}
			_content = std::map<std::wstring, std::unique_ptr<Object>>();
			_initialized = true;
		}

		const std::vector<std::unique_ptr<Object>> &Object::asArray() const
		{
			if (_initialized == false || std::holds_alternative<std::vector<std::unique_ptr<Object>>>(_content) == false)
			{
				GENERATE_ERROR("Can't get object as array : object is not initialized or is not of type array");
			}
			return (std::get<std::vector<std::unique_ptr<Object>>>(_content));
		}

		void Object::resize(size_t p_size)
		{
			if (_initialized == false)
			{
				_content = std::vector<std::unique_ptr<Object>>();
				_initialized = true;
			}
			std::vector<std::unique_ptr<Object>> &array = std::get<std::vector<std::unique_ptr<Object>>>(_content);

			array.resize(p_size);

			for (size_t i = array.size(); i < p_size; i++)
			{
				array.push_back(std::make_unique<Object>(L"[" + std::to_wstring(i) + L"]"));
			}
		}

		Object &Object::append()
		{
			if (_initialized == false)
			{
				_content = std::vector<std::unique_ptr<Object>>();
				_initialized = true;
			}

			std::unique_ptr<Object> result = std::make_unique<Object>(L"[" + std::to_wstring(std::get<std::vector<std::unique_ptr<Object>>>(_content).size()) + L"]");
			std::get<std::vector<std::unique_ptr<Object>>>(_content).push_back(result);
			return (*result);
		}

		void Object::push_back(Object &p_object)
		{
			if (_initialized == false)
			{
				_content = std::vector<std::unique_ptr<Object>>();
				_initialized = true;
			}
			std::unique_ptr<Object> result = std::make_unique<Object>(p_object);
			std::get<std::vector<std::unique_ptr<Object>>>(_content).push_back(result);
		}

		Object &Object::operator[](size_t p_index)
		{
			if (!std::holds_alternative<std::vector<std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold an array, cannot access by index");
			}

			auto &vec = std::get<std::vector<std::unique_ptr<Object>>>(_content);

			// Check if the index is within bounds
			if (p_index >= vec.size())
			{
				throw std::out_of_range("Index out of range");
			}

			return *(vec[p_index]);
		}

		const Object &Object::operator[](size_t p_index) const
		{
			if (!std::holds_alternative<std::vector<std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold an array, cannot access by index");
			}

			const std::vector<std::unique_ptr<Object>> &vec = std::get<std::vector<std::unique_ptr<Object>>>(_content);

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
			_content = std::vector<std::unique_ptr<Object>>();
			_initialized = true;
		}

		size_t Object::size() const
		{
			if (_initialized == false)
			{
				GENERATE_ERROR("Can't get object size : it is uninitialized");
			}
			if (!std::holds_alternative<std::vector<std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a vector, cannot perform size operation");
			}
			return (std::get<std::vector<std::unique_ptr<Object>>>(_content).size());
		}

		size_t Object::count(const std::wstring &p_key) const
		{
			if (!std::holds_alternative<std::map<std::wstring, std::unique_ptr<Object>>>(_content))
			{
				GENERATE_ERROR("Object does not hold a map, cannot perform count operation");
			}
			return (std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content).count(p_key));
		}

		void Object::printUnit(std::wostream &p_os) const
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
				p_os << *(as<std::unique_ptr<Object>>());
				break;
			case 5:
				p_os << "null";
				break;
			}
		}

		void Object::printObject(std::wostream &p_os) const
		{
			const std::map<std::wstring, std::unique_ptr<Object>> &map = std::get<std::map<std::wstring, std::unique_ptr<Object>>>(_content);
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

		void Object::printArray(std::wostream &p_os) const
		{
			const std::vector<std::unique_ptr<Object>> &vector = std::get<std::vector<std::unique_ptr<Object>>>(_content);

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
				p_object.printUnit(p_os);
				break;
			case 1:
				p_object.printObject(p_os);
				break;
			case 2:
				p_object.printArray(p_os);
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
	} // namespace JSON
} // namespace spk
