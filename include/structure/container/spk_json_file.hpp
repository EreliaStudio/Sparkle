#pragma once

#include "structure/container/spk_JSON_object.hpp"
#include <filesystem>
#include <cfenv>
#include <cmath>

namespace spk
{
	namespace JSON
	{
		/**
		 * @class File
		 * @brief A utility class for handling JSON files and managing their data.
		 * 
		 * This class provides functionality to load, parse, and save JSON data. It supports accessing
		 * JSON objects using keys or indices and converting JSON data to various types.
		 * 
		 * Example usage:
		 * @code
		 * spk::JSON::File jsonFile(L"example.json");
		 * if (jsonFile.contains(L"key"))
		 * {
		 *	 auto value = jsonFile[L"key"].as<std::wstring>();
		 *	 std::wcout << L"Value: " << value << std::endl;
		 * }
		 * @endcode
		 */
		class File
		{
		private:
			spk::JSON::Object _root; /**< @brief The root JSON object. */

		public:
			/**
			 * @brief Default constructor. Initializes an empty JSON file.
			 */
			File();

			/**
			 * @brief Constructs a JSON file by loading data from a specified file path.
			 * 
			 * @param p_filePath The file path to load JSON data from.
			 */
			File(const std::filesystem::path& p_filePath);

			/**
			 * @brief Loads JSON data from a specified file path.
			 * 
			 * @param p_filePath The file path to load JSON data from.
			 * @throws std::runtime_error If the file cannot be loaded or the JSON is invalid.
			 */
			void load(const std::filesystem::path& p_filePath);

			/**
			 * @brief Creates a JSON file object by parsing a JSON string.
			 * 
			 * @param p_content The JSON string to parse.
			 * @return A JSON file object containing the parsed data.
			 */
			static File loadFromString(const std::wstring& p_content);

			/**
			 * @brief Saves the JSON data to a specified file path.
			 * 
			 * @param p_filePath The file path to save JSON data to.
			 * @throws std::runtime_error If the file cannot be saved.
			 */
			void save(const std::filesystem::path& p_filePath) const;

			/**
			 * @brief Checks if the JSON file contains a specific key.
			 * 
			 * @param p_key The key to check for.
			 * @return `true` if the key exists, `false` otherwise.
			 */
			bool contains(const std::wstring& p_key) const;

			/**
			 * @brief Accesses a JSON object by key.
			 * 
			 * @param p_key The key of the JSON object to access.
			 * @return A constant reference to the JSON object.
			 * @throws std::runtime_error If the key does not exist.
			 */
			const spk::JSON::Object& operator[](const std::wstring& p_key) const;

			/**
			 * @brief Accesses a JSON object by index.
			 * 
			 * @param p_index The index of the JSON object to access.
			 * @return A constant reference to the JSON object.
			 * @throws std::runtime_error If the index is out of bounds.
			 */
			const spk::JSON::Object& operator[](size_t p_index) const;

			/**
			 * @brief Converts the root JSON object to a specified type.
			 * 
			 * @tparam TType The type to convert the JSON object to.
			 * @return A reference to the converted value.
			 */
			template <typename TType>
			const TType& as()
			{
				return (_root.as<TType>());
			}

			/**
			 * @brief Retrieves the root JSON object.
			 * 
			 * @return A constant reference to the root JSON object.
			 */
			const spk::JSON::Object& root() const;

			/**
			 * @brief Outputs the JSON file content to a wide output stream.
			 * 
			 * @param p_os The output stream.
			 * @param p_file The JSON file to output.
			 * @return A reference to the output stream.
			 */
			friend std::wostream& operator<<(std::wostream& p_os, const File& p_file);
		};
	}
}
