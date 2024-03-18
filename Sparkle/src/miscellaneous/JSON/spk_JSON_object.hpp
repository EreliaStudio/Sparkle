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
			using Unit = std::variant<bool, long, double, std::string, Object *, std::nullptr_t>;			/**< The type for a single JSON value. */
			using ContentType = std::variant<std::nullptr_t, Unit, std::map<std::string, Object *>, std::vector<Object *>>; /**< The type for the content of the JSON object. */

		private:
			bool _initialized;
			std::string _name;
			ContentType _content;
			static size_t _indent;
			const static uint8_t _indentSize = 4;

			void printUnit(std::ostream &p_os) const;
			void printObject(std::ostream &p_os) const;
			void printArray(std::ostream &p_os) const;

		public:
			/**
			 * @brief Constructor for the JSON Object.
			 *
			 * Initializes a JSON object, optionally with a name. The object starts uninitialized and can be
			 * configured to hold various types of JSON data (e.g., units, objects, arrays).
			 *
			 * @param p_name The name of the JSON object, defaulting to "Unnamed".
			 */
			Object(const std::string &p_name = "Unnamed");

			/**
			 * @brief Resets the JSON object to an uninitialized state.
			 *
			 * Clears the content of the JSON object and marks it as uninitialized. This is useful for reusing
			 * the object for new JSON data.
			 */
			void reset();

			/**
			 * @brief Checks if the JSON object is configured as an object.
			 *
			 * @return True if the JSON object is structured as an object, false otherwise.
			 */
			bool isObject() const;

			/**
			 * @brief Checks if the JSON object is configured as an array.
			 *
			 * @return True if the JSON object is structured as an array, false otherwise.
			 */
			bool isArray() const;

			/**
			 * @brief Checks if the JSON object is a basic unit (e.g., bool, long, double, string, null).
			 *
			 * @return True if the JSON object represents a basic JSON unit, false otherwise.
			 */
			bool isUnit() const;

			/**
			 * @brief Adds a new attribute to the JSON object.
			 *
			 * If the JSON object is not already an object type, this method will initialize it as an object.
			 * Adds a new key-value pair where the value is a new, uninitialized JSON object.
			 *
			 * @param p_key The key under which to add the new attribute.
			 * @return A reference to the newly added JSON object attribute.
			 */
			Object &addAttribute(const std::string &p_key);

			/**
			 * @brief Retrieves a map of all members in the JSON object.
			 *
			 * Only applicable if the JSON object is of object type. Provides read-only access to the underlying
			 * map representing member attributes.
			 *
			 * @return A const reference to the map containing the JSON object's members.
			 */
			const std::map<std::string, Object *> &members() const;

			/**
			 * @brief Checks if the JSON object contains a specific key.
			 *
			 * Only applicable if the JSON object is of object type. Checks whether the object has an attribute
			 * with the specified key.
			 *
			 * @param p_key The key to check for existence.
			 * @return True if the attribute exists, false otherwise.
			 */
			bool contains(const std::string &p_key) const;

			/**
			 * @brief Accesses a child object by key.
			 *
			 * Provides read-write access to a child object of this JSON object, identified by the given key.
			 * If the key does not exist, a new JSON object is created and associated with that key.
			 *
			 * @param p_key The key of the child object to access.
			 * @return A reference to the child JSON object.
			 */
			Object &operator[](const std::string &p_key);

			/**
			 * @brief Accesses a child object by key (const version).
			 *
			 * Provides read-only access to a child object of this JSON object, identified by the given key.
			 * Throws an exception if the key does not exist.
			 *
			 * @param p_key The key of the child object to access.
			 * @return A const reference to the child JSON object.
			 */
			const Object &operator[](const std::string &p_key) const;

			/**
			 * @brief Sets the JSON object to an object type.
			 *
			 * Initializes the JSON object as an empty object if it was not already. This method is useful
			 * for explicitly declaring an object as a JSON object type, even without adding attributes.
			 */
			void setAsObject();

			/**
			 * @brief Retrieves the JSON object as an array.
			 *
			 * Provides read-only access to the underlying vector representing the JSON array.
			 * Only applicable if the JSON object is of array type.
			 *
			 * @return A const reference to the vector containing the JSON array elements.
			 */
			const std::vector<Object *> &asArray() const;

			/**
			 * @brief Resizes the JSON array.
			 *
			 * Adjusts the size of the JSON array. If the new size is greater than the current size, new elements are added.
			 * If the JSON object is not already an array, this method initializes it as an empty array before resizing.
			 *
			 * @param p_size The new size of the JSON array.
			 */
			void resize(size_t p_size);

			/**
			 * @brief Appends a new, uninitialized JSON object to the array.
			 *
			 * Only applicable if the JSON object is of array type. Adds a new JSON object to the end of the array.
			 *
			 * @return A reference to the newly appended JSON object.
			 */
			Object &append();

			/**
			 * @brief Adds an existing JSON object to the end of the JSON array.
			 *
			 * Only applicable if the JSON object is of array type.
			 *
			 * @param p_object The JSON object to be added to the array.
			 */
			void push_back(Object &p_object);

			/**
			 * @brief Accesses an element in the JSON array by index.
			 *
			 * Provides read-write access to an element of the JSON array. If the index is out of bounds, throws an exception.
			 *
			 * @param p_index The index of the element to access.
			 * @return A reference to the JSON object at the specified index.
			 */
			Object &operator[](size_t p_index);

			/**
			 * @brief Accesses an element in the JSON array by index (const version).
			 *
			 * Provides read-only access to an element of the JSON array. If the index is out of bounds, throws an exception.
			 *
			 * @param p_index The index of the element to access.
			 * @return A const reference to the JSON object at the specified index.
			 */
			const Object &operator[](size_t p_index) const;

			/**
			 * @brief Sets the JSON object to an array type.
			 *
			 * Initializes the JSON object as an empty array if it was not already. Useful for explicitly declaring an object as a JSON array.
			 */
			void setAsArray();

			/**
			 * @brief Returns the size of the JSON array or object.
			 *
			 * If the JSON object is an array, returns the number of elements. If it is an object, returns the number of key-value pairs.
			 *
			 * @return The size of the JSON array or object.
			 */
			size_t size() const;

			/**
			 * @brief Counts occurrences of a key in the JSON object.
			 *
			 * Only applicable if the JSON object is of object type. Counts the number of times a specific key appears in the object.
			 *
			 * @param p_key The key to count occurrences of.
			 * @return The number of occurrences of the specified key.
			 */
			size_t count(const std::string &p_key) const;

			/**
			 * @brief Checks if the JSON unit holds a value of the specified type.
			 *
			 * Validates whether the current JSON object, if it is a unit (a single value), contains a value of the specified template type.
			 * Throws an exception if the JSON object is not a unit.
			 *
			 * @tparam TType The type to check against the stored value.
			 * @return True if the unit holds a value of type TType, false otherwise.
			 */
			template <typename TType>
			bool hold() const
			{
				if (isUnit() == false)
					throwException("Can't verify the holding type of an object who isn't a Unit");
				return (std::holds_alternative<TType>(std::get<Unit>(_content)));
			}

			/**
			 * @brief Sets the value of the JSON object to the specified value.
			 *
			 * Initializes or updates the JSON unit with the provided value. If the JSON object was not previously initialized as a unit,
			 * it is initialized accordingly.
			 *
			 * @tparam TType The type of the value to set, excluding Object type to prevent recursion.
			 * @param p_value The value to set in the JSON object.
			 */
			template <typename TType,
					  typename std::enable_if<!std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType &p_value)
			{
				if (_initialized == false)
				{
					_content = Unit();
					_initialized = true;
				}

				std::get<Unit>(_content) = p_value;
			}

			/**
			 * @brief Sets the JSON object to a copy of the provided Object value.
			 *
			 * Specialization for setting an Object type. Creates a new Object instance and assigns it to the JSON object.
			 *
			 * @tparam TType Must be Object type for this specialization to be invoked.
			 * @param p_value The Object value to set in the JSON object.
			 */
			template <typename TType,
					  typename std::enable_if<std::is_same<TType, Object>::value, int>::type = 0>
			void set(const TType &p_value)
			{
				Object *tmpObject = new Object(p_value);
				set<Object *>(tmpObject);
			}

			/**
			 * @brief Retrieves the stored value as the specified type.
			 *
			 * Accesses the value stored in the JSON unit as the specified type. Throws an exception if the actual type of the stored
			 * value does not match the requested type.
			 *
			 * @tparam TType The type to cast the stored value to.
			 * @return A const reference to the stored value cast to the specified type.
			 */
			template <typename TType>
			const TType &as() const
			{
				const Unit &unit = std::get<Unit>(_content);
				const TType *value = std::get_if<TType>(&unit);

				if (value == nullptr)
				{
					Unit tmpUnit = TType();
					std::string types[] = {"bool", "long", "double", "std::string", "Object*", "std::nullptr_t"};
					throwException("Wrong type request for object [" + _name + "] as Unit : Request type [" + types[tmpUnit.index()] + "] but unit contain [" + types[unit.index()] + "]");
				}

				return (*value);
			}

			/**
			 * @brief Outputs the JSON object to a standard output stream.
			 *
			 * Serializes the JSON object to a string and writes it to the provided output stream. Supports pretty printing based on
			 * indentation settings of the JSON object.
			 *
			 * @param p_os The output stream to write the serialized JSON object to.
			 * @param p_object The JSON object to serialize and write.
			 * @return A reference to the updated output stream.
			 */
			friend std::ostream &operator<<(std::ostream &p_os, const Object &p_object);
		};
	}
}
