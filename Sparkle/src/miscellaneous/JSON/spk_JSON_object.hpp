#pragma once

#include <iostream>
#include <variant>
#include <map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <any>

#include "spk_basic_functions.hpp"

namespace spk
{
	namespace JSON
	{
		/**
		 * @class Object
		 * @brief Represents a JSON object for serialization and deserialization.
		 *
		 * The Object class provides a versatile structure to represent JSON data, supporting
		 * various types such as booleans, numbers, strings, nested objects, and arrays.
		 * It offers functionality to dynamically add, modify, and query the structure of the
		 * JSON object, making it suitable for complex data manipulation.
		 *
		 * This class facilitates the construction and manipulation of JSON data, allowing
		 * for the creation of structured JSON objects and arrays. It supports serialization
		 * to and deserialization from string representations of JSON, thereby enabling the
		 * integration with external systems or storage in human-readable format.
		 *
		 * Usage example:
		 * @code
		 * spk::JSON::Object myObject;
		 * myObject.addAttribute("key").set<std::string>("value"); // Adds an attribute "key" with string value "value"
		 * std::cout << myObject; // Serializes and outputs the JSON object
		 * @endcode
		 *
		 * @note This class uses dynamic memory allocation for nested objects and arrays to manage the JSON structure efficiently. It provides mechanisms to check the type of a contained value, resize arrays, and access nested objects or array elements through overloads of the subscript operator.
		 */
		class Object
		{
		public:
			using Unit = std::variant<bool, long, double, std::string, Object*, std::nullptr_t>; /**< The type for a single JSON value. */
			using ContentType = std::variant<Unit, std::map<std::string, Object*>, std::vector<Object*>>; /**< The type for the content of the JSON object. */

		private:
			bool _initialized;
			std::string _name;
			ContentType _content;
			static size_t _indent;
			const static uint8_t _indentSize = 4;

		public:
			Object(const std::string& p_name = "Unnamed");

			void reset();

			bool isObject() const;
			bool isArray() const;
			bool isUnit() const;

			Object& addAttribute(const std::string& p_key);

			const std::map<std::string, Object*>& members() const;
			bool contains(const std::string& p_key) const;
			Object& operator[](const std::string& p_key);
			const Object& operator[](const std::string& p_key) const;
			void setAsObject();

			const std::vector<Object*>& asArray() const;
			void resize(size_t p_size);
			Object& append();
			void push_back(Object& p_object);
			Object& operator[](size_t p_index);
			const Object& operator[](size_t p_index) const;
			void setAsArray();
			size_t size() const;
			size_t count(const std::string& p_key) const;

			template <typename TType>
			bool hold() const
			{
				if (isUnit() == false)
					throwException("Can't verify the holding type of an object who isn't a Unit");
				return (std::holds_alternative<TType>(std::get<Unit>(_content)));
			}
			template <typename TType,
				typename std::enable_if<!std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType& p_value)
			{
				if (_initialized == false)
				{
					_content = Unit();
					_initialized = true;
				}

				std::get<Unit>(_content) = p_value;
			}
			template <typename TType,
				typename std::enable_if<std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType& p_value)
			{
				Object* tmpObject = new Object(p_value);
				set<Object*>(tmpObject);
			}
			template <typename TType>
			const TType& as() const
			{
				const Unit& unit = std::get<Unit>(_content);
				const TType* value = std::get_if<TType>(&unit);

				if (value == nullptr)
				{
					Unit tmpUnit = TType();
					std::string types[] = {"bool", "long", "double", "std::string", "Object*", "std::nullptr_t"};
					throwException("Wrong type request for object [" + _name + "] as Unit : Request type [" + types[tmpUnit.index()] + "] but unit contain [" + types[unit.index()] + "]");
				}

				return (*value);
			}

			void printUnit(std::ostream& p_os) const;
			void printObject(std::ostream& p_os) const;
			void printArray(std::ostream& p_os) const;

			friend std::ostream& operator<<(std::ostream& p_os, const Object& p_object);
		};
	}
}
