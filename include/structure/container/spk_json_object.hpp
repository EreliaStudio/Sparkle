#pragma once

#include <iostream>
#include <variant>
#include <map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <any>
#include <cstdint>

#include "utils/spk_string_utils.hpp"

namespace spk
{
	namespace JSON
	{
		/**
		 * @class Object
		 * @brief Represents a JSON object capable of holding various types of data, including primitives, objects, and arrays.
		 * 
		 * The `Object` class provides flexible storage for JSON data and supports operations
		 * such as accessing attributes, resizing arrays, and serializing data.
		 * 
		 * Example usage:
		 * @code
		 * spk::JSON::Object jsonObj(L"MyObject");
		 * jsonObj.setAsObject();
		 * jsonObj.addAttribute(L"key") = 42;
		 * std::wcout << jsonObj << std::endl;
		 * @endcode
		 */
		class Object
		{
		public:
			using Unit = std::variant<bool, long, double, std::wstring, Object*, std::nullptr_t>; /**< @brief A single JSON value. */
			using ContentType = std::variant<Unit, std::map<std::wstring, Object*>, std::vector<Object*>>; /**< @brief The underlying data structure for the object. */

		private:
			bool _initialized; /**< @brief Indicates whether the object has been initialized. */
			std::wstring _name; /**< @brief The name of the JSON object. */
			ContentType _content; /**< @brief The content of the JSON object. */
			static size_t _indent; /**< @brief Current indentation level for pretty printing. */
			const static uint8_t _indentSize = 4; /**< @brief Number of spaces per indentation level. */

			void printUnit(std::wostream& p_os) const;
			void printObject(std::wostream& p_os) const;
			void printArray(std::wostream& p_os) const;

		public:
			/**
			 * @brief Constructs a JSON object with a given name.
			 * @param p_name The name of the JSON object (default is "Unnamed").
			 */
			Object(const std::wstring& p_name = L"Unnamed");

			/**
			 * @brief Resets the object, clearing all content and state.
			 */
			void reset();

			/**
			 * @brief Checks if the object is of type `Object`.
			 * @return `true` if the object is an object, otherwise `false`.
			 */
			bool isObject() const;

			/**
			 * @brief Checks if the object is of type `Array`.
			 * @return `true` if the object is an array, otherwise `false`.
			 */
			bool isArray() const;

			/**
			 * @brief Checks if the object is of type `Unit`.
			 * @return `true` if the object is a unit, otherwise `false`.
			 */
			bool isUnit() const;

			/**
			 * @brief Adds an attribute to the object.
			 * @param p_key The key of the attribute.
			 * @return A reference to the newly added attribute.
			 * @throws std::runtime_error If an attribute with the same key already exists.
			 */
			Object& addAttribute(const std::wstring& p_key);

			/**
			 * @brief Retrieves all members of the object.
			 * @return A constant reference to a map of members.
			 * @throws std::runtime_error If the object is not of type `Object`.
			 */
			const std::map<std::wstring, Object*>& members() const;

			/**
			 * @brief Checks if the object contains a specific key.
			 * @param p_key The key to search for.
			 * @return `true` if the key exists, otherwise `false`.
			 */
			bool contains(const std::wstring& p_key) const;

			/**
			 * @brief Accesses an attribute by key, creating it if it does not exist.
			 * @param p_key The key of the attribute.
			 * @return A reference to the attribute.
			 */
			Object& operator[](const std::wstring& p_key);

			/**
			 * @brief Accesses an attribute by key.
			 * @param p_key The key of the attribute.
			 * @return A constant reference to the attribute.
			 * @throws std::runtime_error If the key does not exist.
			 */
			const Object& operator[](const std::wstring& p_key) const;

			/**
			 * @brief Sets the object as a JSON object (dictionary).
			 * @throws std::runtime_error If the object is already initialized.
			 */
			void setAsObject();

			/**
			 * @brief Retrieves the object as an array.
			 * @return A constant reference to the vector of elements.
			 * @throws std::runtime_error If the object is not an array.
			 */
			const std::vector<Object*>& asArray() const;

			/**
			 * @brief Resizes the array.
			 * @param p_size The new size of the array.
			 */
			void resize(size_t p_size);

			/**
			 * @brief Appends a new element to the array.
			 * @return A reference to the newly appended object.
			 */
			Object& append();

			/**
			 * @brief Pushes a copy of an object into the array.
			 * @param p_object The object to push into the array.
			 */
			void push_back(Object& p_object);

			/**
			 * @brief Accesses an element by index.
			 * @param p_index The index of the element.
			 * @return A reference to the element.
			 * @throws std::runtime_error If the object is not an array.
			 */
			Object& operator[](size_t p_index);

			/**
			 * @brief Accesses an element by index.
			 * @param p_index The index of the element.
			 * @return A constant reference to the element.
			 * @throws std::runtime_error If the object is not an array.
			 */
			const Object& operator[](size_t p_index) const;

			/**
			 * @brief Sets the object as a JSON array.
			 * @throws std::runtime_error If the object is already initialized.
			 */
			void setAsArray();

			/**
			 * @brief Retrieves the size of the array.
			 * @return The number of elements in the array.
			 * @throws std::runtime_error If the object is not an array.
			 */
			size_t size() const;

			/**
			 * @brief Counts the occurrences of a specific key in the object.
			 * @param p_key The key to count.
			 * @return The number of occurrences of the key.
			 * @throws std::runtime_error If the object is not a dictionary.
			 */
			size_t count(const std::wstring& p_key) const;

			/**
			 * @brief Checks if the object holds a value of a specific type.
			 * @tparam TType The type to check.
			 * @return `true` if the object holds the specified type, otherwise `false`.
			 */
			template <typename TType>
			bool hold() const
			{
				if (isUnit() == false)
					throw std::runtime_error("Can't verify the holding type of an object who isn't a Unit");
				return (std::holds_alternative<TType>(std::get<Unit>(_content)));
			}

			/**
			 * @brief Sets the value of the object to a specific type.
			 * @tparam TType The type of the value.
			 * @param p_value The value to set.
			 */
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

			/**
			 * @brief Sets the value of the object to another JSON object.
			 * @tparam TType The type of the value.
			 * @param p_value The JSON object to set.
			 */
			template <typename TType,
				typename std::enable_if<std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType& p_value)
			{
				Object* tmpObject = new Object(p_value);
				set<Object*>(tmpObject);
			}

			/**
			 * @brief Retrieves the value of the object as a specific type.
			 * @tparam TType The type to retrieve.
			 * @return A constant reference to the value.
			 * @throws std::runtime_error If the type does not match.
			 */
			template <typename TType>
			const TType& as() const
			{
				const Unit& unit = std::get<Unit>(_content);
				const TType* value = std::get_if<TType>(&unit);

				if (value == nullptr)
				{
					Unit tmpUnit = TType();
					std::string types[] = { "bool", "long", "double", "std::wstring", "Object*", "std::nullptr_t" };
					throw std::runtime_error("Wrong type request for object [" + spk::StringUtils::wstringToString(_name) + "] as Unit : Request type [" + types[tmpUnit.index()] + "] but unit contain [" + types[unit.index()] + "]");
				}

				return (*value);
			}

			/**
			 * @brief Assigns a value to the object.
			 * @tparam TType The type of the value.
			 * @param value The value to assign.
			 * @return A reference to the object.
			 */
			template<typename TType>
			Object& operator=(const TType& value)
			{
				this->set<TType>(value);
				return (*this);
			}

			/**
			 * @brief Outputs the JSON object to a wide stream.
			 * @param p_os The output stream.
			 * @param p_object The JSON object to output.
			 * @return A reference to the output stream.
			 */
			friend std::wostream& operator<<(std::wostream& p_os, const Object& p_object);
		};
	}
}
