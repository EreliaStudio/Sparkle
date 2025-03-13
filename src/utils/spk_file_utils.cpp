#include "utils/spk_file_utils.hpp"

#include <fstream>

namespace spk
{
	namespace FileUtils
	{
		std::vector<std::filesystem::path> listFiles(const std::filesystem::path &folderPath)
		{
			std::vector<std::filesystem::path> fileList;

			if (!std::filesystem::exists(folderPath) || !std::filesystem::is_directory(folderPath))
			{
				throw std::invalid_argument("The provided path is not a valid directory.");
			}

			for (const auto &entry : std::filesystem::directory_iterator(folderPath))
			{
				if (std::filesystem::is_regular_file(entry.path()))
				{
					fileList.push_back(entry.path());
				}
			}

			return fileList;
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
				throw std::runtime_error("Can't open file [" + p_path.string() + "]");
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
				throw std::runtime_error("Can't open file [" + p_path.string() + "]");
			}

			std::streamsize fileSize = inputFile.tellg();
			inputFile.seekg(0, std::ios::beg);

			std::vector<uint8_t> buffer(static_cast<size_t>(fileSize));

			if (!inputFile.read(reinterpret_cast<char *>(buffer.data()), fileSize))
			{
				throw std::runtime_error("Error reading file [" + p_path.string() + "]");
			}

			return buffer;
		}
	}
}