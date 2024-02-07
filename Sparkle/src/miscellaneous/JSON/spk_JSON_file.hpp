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
		 * @class JSON::File
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

		public:
			File();
			File(const std::filesystem::path& p_filePath);

			void load(const std::filesystem::path& p_filePath);
			void save(const std::filesystem::path& p_filePath) const;

			bool contains(const std::string& p_key) const;

			const spk::JSON::Object &operator[](const std::string &p_key) const;

			const spk::JSON::Object &operator[](size_t p_index) const;

			template <typename TType>
			const TType &as()
			{
				return (_root.as<TType>());
			}

			const spk::JSON::Object &root() const;

			friend std::ostream &operator<<(std::ostream &p_os, const File &p_file);
		};
	}
}
