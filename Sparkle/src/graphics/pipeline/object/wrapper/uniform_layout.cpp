#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	std::map<std::string, Pipeline::UniformObject::Layout> Pipeline::_parseUniformLayout(const std::vector<OpenGL::ShaderInstruction>& p_shaderinstruction, const std::string& p_prefixName)
	{
		std::map<std::string, Pipeline::UniformObject::Layout> result;

		for (const auto& instruction : p_shaderinstruction)
		{
			if (instruction.type == OpenGL::ShaderInstruction::Type::UniformBlock)
			{
				std::string type = instruction.informations[OpenGL::ShaderInstruction::TypeKey].as<std::string>();
				
				if (type.substr(0, p_prefixName.size()) == p_prefixName)
				{
					Pipeline::UniformObject::Layout newLayout;

					newLayout.structure = &(_structures.at(type));
					newLayout.binding = std::stoi(instruction.informations[OpenGL::ShaderInstruction::BindingKey].as<std::string>());
					newLayout.type = type;

					result[instruction.informations[OpenGL::ShaderInstruction::NameKey].as<std::string>()] = newLayout;
				}
			}
		}

		return (result);
	}
}