#include "miscellaneous/JSON/spk_JSON_object.hpp"

#include <string>
#include "spk_basic_functions.hpp"

namespace spk
{
	namespace JSON
	{
		size_t Object::_indent = 0;

		Object::Object(const std::string& p_name) :
			_name(p_name)
		{
			_initialized = false;
		}

		void Object::reset()
		{
			_initialized = false;
			_content = ContentType();
		}

		bool Object::isObject() const
		{
			return (_initialized && std::holds_alternative<std::map<std::string, Object*>>(_content));
		}

		bool Object::isArray() const
		{
			return (_initialized && std::holds_alternative<std::vector<Object*>>(_content));
		}

		bool Object::isUnit() const
		{
			return (_initialized && std::holds_alternative<Unit>(_content));
		}

		Object& Object::addAttribute(const std::string& p_key)
		{
			if (_initialized == false)
			{
				_content = std::map<std::string, Object*>();
				_initialized = true;
			}

			Object* result = new Object(p_key);

			if (std::get<std::map<std::string, Object*>>(_content).count(p_key) != 0)
				throwException("Can't add attribute named [" + p_key + "] : it already exists");
			std::get<std::map<std::string, Object*>>(_content)[p_key] = result;
			return (*result);
		}
		
		bool Object::contains(const std::string& p_key) const
		{
			auto& map = std::get<std::map<std::string, Object*>>(_content);

			return (map.contains(p_key));
		}

		/*
			Forced to do this to prevent user from writing as<Object*>() after getting an object from the map
			it's not mandatory, but it's easier to read for the user with this.

			If not keeped, we coult simply :
			return (std::get<std::map<std::string, Object*>>(_content).at(p_key));
		*/
		Object& Object::operator[](const std::string& p_key)
		{
			if (_initialized == false)
			{
				setAsObject();
			}
			
			auto& map = std::get<std::map<std::string, Object*>>(_content);

			if (map.count(p_key) == 0)
			{
				addAttribute(p_key);
			}

			Object* result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false)
				return (*(result));
			else
			{
				if (std::holds_alternative<Object*>(std::get<Unit>(result->_content)) == false)
					return (*(result));
				else
					return (*(result->as<Object*>()));
			}
		}

		const Object& Object::operator[](const std::string& p_key) const
		{
			auto& map = std::get<std::map<std::string, Object*>>(_content);

			if (map.count(p_key) == 0)
				throwException("Can't acces JSON object named [" + p_key + "] : it does not exist");

			Object* result = map.at(p_key);

			if (std::holds_alternative<Unit>(result->_content) == false)
				return (*(result));
			else
			{
				if (std::holds_alternative<Object*>(std::get<Unit>(result->_content)) == false)
					return (*(result));
				else
					return (*(result->as<Object*>()));
			}
		}

		const std::map<std::string, Object*>& Object::members() const
		{
			if (_initialized == false || std::holds_alternative<std::map<std::string, Object*>>(_content) == false)
				throwException("Can't get object members : object is not initialized or is not of type object");
			return (std::get<std::map<std::string, Object*>>(_content));
		}

		void	Object::setAsObject()
		{
			if (_initialized == true)
				throwException("Can't set object as object : it is already initialized");
			_content = std::map<std::string, Object*>();
			_initialized = true;
		}
		
		const std::vector<Object*>& Object::asArray() const
		{
			if (_initialized == false || std::holds_alternative<std::map<std::string, Object*>>(_content) == false)
				throwException("Can't get object as array : object is not initialized or is not of type array");
			return (std::get<std::vector<Object*>>(_content));
		}

		void Object::resize(size_t p_size)
		{
			if (_initialized == false)
			{
				_content = std::vector<Object*>();
				_initialized = true;
			}
			std::vector<Object*>& array = std::get<std::vector<Object*>>(_content);

			for (size_t i = p_size; i < array.size(); i++)
				delete array[i];

			for (size_t i = array.size(); i < p_size; i++)
				array.push_back(new Object("[" + std::to_string(i) + "]"));
		}

		Object& Object::append()
		{
			if (_initialized == false)
			{
				_content = std::vector<Object*>();
				_initialized = true;
			}

			Object* result = new Object("[" + std::to_string(std::get<std::vector<Object*>>(_content).size()) + "]");
			std::get<std::vector<Object*>>(_content).push_back(result);
			return (*result);
		}

		void Object::push_back(Object& p_object)
		{
			if (_initialized == false)
			{
				_content = std::vector<Object*>();
				_initialized = true;
			}
			Object* result = new Object(p_object);
			std::get<std::vector<Object*>>(_content).push_back(result);
		}

		Object& Object::operator[](size_t p_index)
		{
			return (*(std::get<std::vector<Object*>>(_content)[p_index]));
		}

		const Object& Object::operator[](size_t p_index) const
		{
			return (*(std::get<std::vector<Object*>>(_content)[p_index]));
		}

		void	Object::setAsArray()
		{
			if (_initialized == true)
				throwException("Can't set object as Array : it is already initialized");
			_content = std::vector<Object*>();
			_initialized = true;
		}

		size_t Object::size() const
		{
			return (std::get<std::vector<Object*>>(_content).size());
		}

		size_t Object::count(const std::string& p_key) const
		{
			return (std::get<std::map<std::string, Object*>>(_content).count(p_key));
		}

		void Object::printUnit(std::ostream& p_os) const
		{
			const Unit& tmp = std::get<Unit>(_content);

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
				p_os << '"' << as<std::string>() << '"';
				break;
			case 4:
				p_os << *(as<Object*>());
				break;
			case 5:
				p_os << "null";
				break;
			}
		}

		void Object::printObject(std::ostream& p_os) const
		{
			const std::map<std::string, Object*>& map = std::get<std::map<std::string, Object*>>(_content);
			std::string cleanedKey;

			p_os << std::setw(_indent * _indentSize) << "{" << std::endl;
			++_indent;
			for (auto& tmp : map)
			{
				cleanedKey = tmp.first;

				p_os << std::setw(_indent * _indentSize) << '"' << cleanedKey << "\": ";

				if (tmp.second->_content.index() == 1 ||
					tmp.second->_content.index() == 2)
					p_os << std::endl;

				p_os << *(tmp.second);
				if (&tmp != &(*map.rbegin()))
					p_os << ',';
				p_os << std::endl;
			}
			--_indent;
			p_os << std::setw(_indent * _indentSize) << "}";
		}

		void Object::printArray(std::ostream& p_os) const
		{
			const std::vector<Object*>& vector = std::get<std::vector<Object*>>(_content);

			p_os << std::setw(_indent * _indentSize) << '[' << std::endl;
			++_indent;
			for (size_t i = 0; i < vector.size(); i++)
			{
				p_os << std::setw(_indent * _indentSize) << *(vector[i]);
				if (i != vector.size() - 1)
					p_os << ',';
				p_os << std::endl;
			}
			--_indent;
			p_os << std::setw(_indent * _indentSize) << ']';
		}

		std::ostream& operator<<(std::ostream& p_os, const Object& p_object)
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
	}
}