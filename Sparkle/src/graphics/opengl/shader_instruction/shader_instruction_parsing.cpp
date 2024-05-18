#include "graphics/opengl/spk_opengl_shader_instruction.hpp"
#include <unordered_map>

namespace spk::OpenGL
{
	std::string _sanitizeInputCode(const std::string& p_inputCode)
	{
		size_t firstNewlinePos = p_inputCode.find('\n');

		std::string versionLine = p_inputCode.substr(0, firstNewlinePos + 1);
		std::string output = p_inputCode.substr(firstNewlinePos + 1);

		std::regex spaceTabPattern("[ \\t]+");
		output = std::regex_replace(output, spaceTabPattern, " ");

		std::regex newlinePattern("\\n+");
		output = std::regex_replace(output, newlinePattern, "");

		return (versionLine + output);
	}
	
	std::vector<ShaderInstruction> ShaderInstruction::parseShaderInstruction(const std::string& p_inputCode)
	{
		std::vector<ShaderInstruction> result;

		std::string cleanInput = _sanitizeInputCode(p_inputCode);

		std::unordered_map<std::string, ShaderInstruction::Type> regexPatterns = {
			{R"(#version\s+(\d+))", ShaderInstruction::Type::Version},
			{R"(struct\s+(\w+)\s*\{([^}]*)\};)", ShaderInstruction::Type::Structure},
			{R"(uniform\s+(\w+)\s+(\w+);)", ShaderInstruction::Type::Sampler},
			{R"(layout\s*\(binding\s*=\s*(\d+)\)\s*uniform\s+(\w+)\s*\{([^}]*)\}\s*(\w+);)", ShaderInstruction::Type::UniformBlock},
			{R"((\w+)\s+(\w+)\s*\(([^)]*)\)\s*\{([^}]*)\})", ShaderInstruction::Type::Function},
			{R"(layout\s*\(location\s*=\s*(\d+)\)\s*in\s+(\w+)\s+(\w+);)", ShaderInstruction::Type::Input},
			{R"(layout\s*\(location\s*=\s*(\d+)\)\s*out\s+(\w+)\s+(\w+);)", ShaderInstruction::Type::Output}
		};

		for (const auto& [pattern, type] : regexPatterns)
		{
			std::regex regex(pattern);
			auto begin = std::sregex_iterator(cleanInput.begin(), cleanInput.end(), regex);
			auto end = std::sregex_iterator();

			for (std::sregex_iterator i = begin; i != end; ++i)
			{
				std::smatch matches = *i;
				result.emplace_back(type, matches);
			}
		}

		return result;
	}
}