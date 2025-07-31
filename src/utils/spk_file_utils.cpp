#include "utils/spk_file_utils.hpp"

#include <fstream>

#include "structure/system/spk_exception.hpp"

namespace spk
{
	namespace FileUtils
	{
		std::vector<std::filesystem::path> listFolders(const std::filesystem::path &p_folderPath)
		{
			std::vector<std::filesystem::path> result;

			if (std::filesystem::exists(p_folderPath) == false || std::filesystem::is_directory(p_folderPath) == false)
			{
				throw std::invalid_argument("The provided path is not a valid directory.");
			}

			for (const auto &entry : std::filesystem::directory_iterator(p_folderPath))
			{
				if (std::filesystem::is_directory(entry.path()) == true)
				{
					result.push_back(entry.path());
				}
			}

			return (result);
		}

		std::vector<std::filesystem::path> listFiles(const std::filesystem::path &p_folderPath)
		{
			std::vector<std::filesystem::path> result;

			if (std::filesystem::exists(p_folderPath) == false || std::filesystem::is_directory(p_folderPath) == false)
			{
				throw std::invalid_argument("The provided path is not a valid directory.");
			}

			for (const auto &entry : std::filesystem::directory_iterator(p_folderPath))
			{
				if (std::filesystem::is_regular_file(entry.path()))
				{
					result.push_back(entry.path());
				}
			}

			return (result);
		}

		std::wstring readFileAsWString(const std::filesystem::path &p_path)
		{
			std::wfstream inputFile;
			inputFile.open(p_path, std::ios_base::in);

			std::wstring line;
			std::wstring result = L"";
			while (std::getline(inputFile, line))
			{
				result += line + L"\n";
			}

			inputFile.close();

			return (result);
		}

		std::string readFileAsString(const std::filesystem::path &p_path)
		{
			std::fstream inputFile;
			inputFile.open(p_path, std::ios_base::in);

			if (inputFile.is_open() == false)
			{
				GENERATE_ERROR("Can't open file [" + p_path.string() + "]");
			}

			std::string line;
			std::string result = "";
			while (std::getline(inputFile, line))
			{
				result += line + "\n";
			}

			inputFile.close();

			return (result);
		}

		std::vector<uint8_t> readFileAsBytes(const std::filesystem::path &p_path)
		{
			std::ifstream inputFile(p_path, std::ios::binary | std::ios::ate);

			if (!inputFile.is_open())
			{
				GENERATE_ERROR("Can't open file [" + p_path.string() + "]");
			}

			std::streamsize fileSize = inputFile.tellg();
			inputFile.seekg(0, std::ios::beg);

			std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));

			if (!inputFile.read(reinterpret_cast<char *>(buffer.data()), fileSize))
			{
				GENERATE_ERROR("Error reading file [" + p_path.string() + "]");
			}

			return (buffer);
		}
	}
}
