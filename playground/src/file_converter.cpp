#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iomanip>
#include <stdint.h>

namespace spk::helper
{
	std::vector<uint8_t> readFileToVector(const std::string& filename)
	{
		std::ifstream file(filename, std::ios::binary | std::ios::ate);
		if (!file)
		{
			throw std::runtime_error("Failed to open file: " + filename);
		}

		std::streamsize size = file.tellg();
		file.seekg(0, std::ios::beg);

		std::vector<uint8_t> buffer(size);
		if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
		{
			throw std::runtime_error("Failed to read file: " + filename);
		}

		return buffer;
	}

	void saveVectorToCpp(const std::vector<uint8_t>& data, const std::string& outputFile)
	{
		std::ofstream cppFile(outputFile);
		if (!cppFile)
		{
			throw std::runtime_error("Failed to open output file: " + outputFile);
		}

		cppFile << "#include <vector>\n\n";
		cppFile << "const std::vector<uint8_t> embeddedFile = {\n    ";

		size_t count = 0;
		for (const auto& byte : data)
		{
			cppFile << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << ", ";
			if (++count % 16 == 0)
			{
				cppFile << "\n    ";
			}
		}

		cppFile << "\n};\n";
		cppFile.close();
	}

	void convertFile(const std::string& p_inputFile, const std::string& p_outputFile)
	{
		try
		{
			std::vector<uint8_t> fileData = readFileToVector(p_inputFile);
			saveVectorToCpp(fileData, p_outputFile);
			std::cout << "Successfully embedded file into " << p_outputFile << "\n";
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << "\n";
		}
	}
}