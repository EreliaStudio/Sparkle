#pragma once

#include "miscellaneous/JSON/spk_JSON_object.hpp"
#include "spk_basic_functions.hpp"
#include <filesystem>
#include <cfenv>
#include <cmath>

namespace spk
{
	namespace JSON
	{
		/**
		 * @class File
		 * @brief Represents a utility class for reading and writing JSON files.
		 *
		 * This class provides a convenient interface for loading and saving JSON data
		 * from/to files. It encapsulates a JSON object, allowing you to easily access
		 * and manipulate JSON data within your C++ application.
		 *
		 * Usage example:
		 * @code
		 * // Create a File instance
		 * spk::JSON::File jsonFile;
		 *
		 * // Load JSON data from a file
		 * jsonFile.load("example.json");
		 *
		 * // Check if a key exists in the JSON data
		 * if (jsonFile.contains("intValue"))
		 * {
		 *     // Access an integer value from the JSON data using operator[]
		 *     const spk::JSON::Object& intValueObject = jsonFile["intValue"];
		 *
		 *     // Use the as method to convert the value to an integer
		 *     int intValue = intValueObject.as<int>();
		 *     std::cout << "Integer Value: " << intValue << std::endl;
		 * }
		 *
		 * // Check if another key exists in the JSON data
		 * if (jsonFile.contains("stringValue"))
		 * {
		 *     // Access a string value from the JSON data using operator[]
		 *     const spk::JSON::Object& stringValueObject = jsonFile["stringValue"];
		 *
		 *     // Use the as method to convert the value to a string
		 *     std::string stringValue = stringValueObject.as<std::string>();
		 *     std::cout << "String Value: " << stringValue << std::endl;
		 * }
		 * @endcode
		 */
		class File
		{
		private:
			spk::JSON::Object _root;

		public: /**
				 * @brief Default constructor for the JSON File class.
				 *
				 * Initializes a new instance of the File class without specifying a file path. The JSON object
				 * contained within this instance is initially empty.
				 */
			File();

			/**
			 * @brief Constructor for the JSON File class with a file path.
			 *
			 * Initializes a new instance of the File class and automatically loads JSON data from the specified
			 * file path.
			 *
			 * @param p_filePath The path of the file from which to load JSON data.
			 */
			File(const std::filesystem::path &p_filePath);

			/**
			 * @brief Loads JSON data from a file.
			 *
			 * Reads JSON data from the specified file path and populates the internal JSON object with this data.
			 *
			 * @param p_filePath The path of the file from which to load JSON data.
			 */
			void load(const std::filesystem::path &p_filePath);

			/**
			 * @brief Saves the JSON data to a file.
			 *
			 * Serializes the internal JSON object and writes it to the specified file path.
			 *
			 * @param p_filePath The path of the file to which the JSON data will be saved.
			 */
			void save(const std::filesystem::path &p_filePath) const;

			/**
			 * @brief Checks if the JSON data contains a specific key.
			 *
			 * Determines whether the internal JSON object contains an attribute with the specified key.
			 *
			 * @param p_key The key to search for in the JSON data.
			 * @return True if the key exists, false otherwise.
			 */
			bool contains(const std::string &p_key) const;

			/**
			 * @brief Accesses a JSON object by key.
			 *
			 * Retrieves a reference to a JSON object associated with the specified key.
			 *
			 * @param p_key The key of the JSON object to retrieve.
			 * @return A const reference to the JSON object associated with the specified key.
			 */
			const spk::JSON::Object &operator[](const std::string &p_key) const;

			/**
			 * @brief Accesses a JSON object by index.
			 *
			 * Retrieves a reference to a JSON object at the specified index within an array.
			 *
			 * @param p_index The index of the JSON object within an array.
			 * @return A const reference to the JSON object at the specified index.
			 */
			const spk::JSON::Object &operator[](size_t p_index) const;

			/**
			 * @brief Retrieves the stored value as the specified type.
			 *
			 * Accesses the value stored in the root JSON object as the specified type. Throws an exception if the actual type of the stored
			 * value does not match the requested type.
			 *
			 * @tparam TType The type to cast the stored value to.
			 * @return A const reference to the stored value cast to the specified type.
			 */
			template <typename TType>
			const TType &as()
			{
				return (_root.as<TType>());
			}

			/**
			 * @brief Retrieves the root JSON object.
			 *
			 * Provides access to the root JSON object of the file, allowing for direct manipulation and query of the JSON structure.
			 *
			 * @return A const reference to the root JSON object.
			 */
			const spk::JSON::Object &root() const;

			/**
			 * @brief Outputs the JSON file to a standard output stream.
			 *
			 * Serializes the JSON file to a string and writes it to the provided output stream. Supports pretty printing based on
			 * indentation settings of the JSON object.
			 *
			 * @param p_os The output stream to write the serialized JSON file to.
			 * @param p_file The JSON file to serialize and write.
			 * @return A reference to the updated output stream.
			 */
			friend std::ostream &operator<<(std::ostream &p_os, const File &p_file);
		};
	}
}
