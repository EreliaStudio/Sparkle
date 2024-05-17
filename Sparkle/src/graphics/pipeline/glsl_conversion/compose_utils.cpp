#include "graphics/pipeline/spk_pipeline.hpp"

#include <sstream>

namespace spk
{
	std::string extractVersionLine(std::string& p_outputCode)
	{
		size_t versionLinePos = p_outputCode.find("#version");
		if (versionLinePos != std::string::npos) {
			size_t lineEnd = p_outputCode.find('\n', versionLinePos);
			std::string versionLine = p_outputCode.substr(versionLinePos, lineEnd - versionLinePos);
			p_outputCode.erase(versionLinePos, lineEnd - versionLinePos + 1);
			return versionLine + "\n";
		}
		return "";
	}

	std::vector<std::pair<std::string, std::string>> getArgumentList(const std::string& p_code, const std::string& p_firstStageName, const std::string& p_secondStageName)
	{
		std::vector<std::pair<std::string, std::string>> result;
		std::istringstream iss(p_code);
		std::string line;
		std::regex pattern(p_firstStageName + "\\s*->\\s*" + p_secondStageName + "\\s*:\\s*(\\w+)\\s+(\\w+);");

		while (std::getline(iss, line))
		{
			std::smatch matches;
			if (std::regex_search(line, matches, pattern) && matches.size() == 3)
			{
				result.emplace_back(matches[1], matches[2]);
			}
		}

		return (result);
	}

	void removeArgumentList(std::string& p_code)
	{
		std::regex pattern(R"(\w+\s*->\s*\w+\s*:\s*\w+\s+\w+;)");

		p_code = std::regex_replace(p_code, pattern, "");
	}

	std::string composeLayoutLines(const std::vector<std::pair<std::string, std::string>>& p_argumentList, const std::string& p_mode)
	{
		std::string result;
		size_t locationValue = 0;
		for (const auto& arg : p_argumentList)
		{
			result += "layout (location = " + std::to_string(locationValue) + ") " + p_mode + " " + arg.first + " " + arg.second + ";\n";
			locationValue++;
		}
		return result;
	}

	void removeFunction(std::string& p_code, const std::string& p_functionName)
	{
		size_t funcPos = p_code.find(p_functionName);
		if (funcPos != std::string::npos)
		{
			size_t start = p_code.rfind('\n', funcPos);
			if (start == std::string::npos)
			{
				start = 0;
			}
			
			size_t braceCount = 0;
			size_t pos = funcPos;

			while (pos < p_code.length())
			{
				if (p_code[pos] == '{')
				{
					++braceCount;
				}
				else if (p_code[pos] == '}')
				{
					--braceCount;
					if (braceCount == 0) {
						break;
					}
				}
				++pos;
			}

			if (braceCount == 0 && pos <= p_code.length())
			{
				p_code.erase(start, pos - start + 1);
			}
		}
	}

	void replaceFunctionDeclaration(std::string& p_code, const std::string& p_functionName, const std::string& p_newDeclaration)
	{
		size_t startPos = p_code.find(p_functionName);
		if (startPos != std::string::npos)
		{
			size_t lineStart = p_code.rfind('\n', startPos);
			size_t lineEnd = p_code.find('{', startPos);
			if (lineStart == std::string::npos)
			{
				lineStart = 0;
			}
			p_code.replace(lineStart, lineEnd - lineStart, p_newDeclaration);
		}
	}

	void swapCodeContent(std::string& p_code, const std::string& p_toFind, const std::string& p_toReplace)
	{
		std::regex wordRegex("\\b" + p_toFind + "\\b");

		p_code = std::regex_replace(p_code, wordRegex, p_toReplace);
	}

	void insertLinkToDepth(std::string& p_code, const std::string& p_insertionCode)
	{
		std::string targetFunction = "void main()";
		std::size_t funcPos = p_code.find(targetFunction);

		if (funcPos != std::string::npos)
		{
			// Find the opening brace of the function
			std::size_t braceOpenPos = p_code.find('{', funcPos + targetFunction.length());
			if (braceOpenPos != std::string::npos)
			{
				int braceCount = 1; // Start after finding the first opening brace
				std::size_t pos = braceOpenPos + 1;
				// Loop through the string until all braces are closed
				while (pos < p_code.length() && braceCount > 0)
				{
					if (p_code[pos] == '{')
					{
						braceCount++;
					}
					else if (p_code[pos] == '}')
					{
						braceCount--;
					}
					pos++;
				}

				// We found the closing brace of the main function, insert before this brace
				if (braceCount == 0 && pos <= p_code.length())
				{
					p_code.insert(pos - 1, p_insertionCode);
				}
			}
		}
	}
}