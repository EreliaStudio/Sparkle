#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{
	Pipeline::Structure::Structure(const std::map<std::string, Pipeline::Structure>& p_availibleStructures, const OpenGL::ShaderInstruction& p_inputInstruction) :
		type(OpenGL::Type::Byte),
		nbElement(1)
	{
		int currentOffsetWithoutPadding = 0;
		int currentOffsetWithPadding = 0;
		int bytesLeft = 16;

		const spk::JSON::Object& array = p_inputInstruction.informations[OpenGL::ShaderInstruction::AttributesKey];
		for (size_t i = 0; i < array.size(); i++)
		{
			const spk::JSON::Object attributeObject = array[i];
			std::string attributeName = attributeObject[OpenGL::ShaderInstruction::NameKey].as<std::string>();
			std::string attributeType = attributeObject[OpenGL::ShaderInstruction::TypeKey].as<std::string>();
			if (p_availibleStructures.contains(attributeType) == false)
				throwException("No structure [" + attributeType + "] found");
			const Structure& attributeStructure = p_availibleStructures.at(attributeType);
			size_t length = std::stoi(attributeObject[OpenGL::ShaderInstruction::LengthKey].as<std::string>());

			if (bytesLeft != 16 && static_cast<int>(attributeStructure.sizeWithPadding) > bytesLeft)
			{
				currentOffsetWithPadding += bytesLeft;
				bytesLeft = 16;
			}

			Element newElement;

			newElement.offsetWithPadding = currentOffsetWithPadding;
			newElement.offsetWithoutPadding = currentOffsetWithoutPadding;
			newElement.length = length;
			newElement.pointer = &attributeStructure;

			elements[attributeName] = newElement;

			for (size_t i = 0; i < length; i++)
			{
				if (bytesLeft != 16 && static_cast<int>(attributeStructure.sizeWithPadding) > bytesLeft)
				{
					currentOffsetWithPadding += bytesLeft;
					bytesLeft = 16;
				}

				currentOffsetWithPadding += attributeStructure.sizeWithPadding;
				currentOffsetWithoutPadding += attributeStructure.sizeWithoutPadding;
				bytesLeft -= attributeStructure.sizeWithPadding;
				while (bytesLeft < 0)
					bytesLeft += 16;
			}
		}

		sizeWithPadding = currentOffsetWithPadding;
		sizeWithoutPadding = currentOffsetWithoutPadding;
	}

    Pipeline::Structure::Structure() :
		type(OpenGL::Type::Byte),
		nbElement(0),
		sizeWithPadding(0),
		sizeWithoutPadding(0)
	{

	}

	Pipeline::Structure::Structure(const OpenGL::Type& p_type, size_t p_nbElement, size_t p_size) :
		type(p_type),
		nbElement(p_nbElement),
		sizeWithPadding(p_size),
		sizeWithoutPadding(p_size)
	{

	}

	void Pipeline::Structure::write(void *p_destination, const void* p_source, size_t p_size) const
	{
		if (elements.empty() == true)
		{
			std::memcpy(p_destination, p_source, p_size);
		}
		else
		{
			for (const auto& [name, element] : elements)
			{
				for (size_t i = 0; i < element.length; i++)
				{
					char *nextDestination = static_cast<char *>(p_destination) + element.offsetWithPadding + element.pointer->sizeWithPadding * i;
					const char *nextSource = static_cast<const char *>(p_source) + element.offsetWithoutPadding + element.pointer->sizeWithoutPadding * i;

					element.pointer->write(nextDestination, nextSource, element.pointer->sizeWithoutPadding);
				}
			}
		}
	}

	void Pipeline::_loadStructureFromInstructions(const std::vector<OpenGL::ShaderInstruction>& p_instructions)
	{
		_structures = {
			{  "int", Pipeline::Structure(OpenGL::Type::Integer,			1,  sizeof(float) * 1)},
			{ "uint", Pipeline::Structure(OpenGL::Type::UnsignedInteger,	1,  sizeof(float) * 1)},
			{"float", Pipeline::Structure(OpenGL::Type::Float,				1,  sizeof(float) * 1)},
			
			{ "vec2", Pipeline::Structure(OpenGL::Type::Float, 				2,  sizeof(float) * 2)},
			{"ivec2", Pipeline::Structure(OpenGL::Type::Integer, 			2,  sizeof(float) * 2)},
			{"uvec2", Pipeline::Structure(OpenGL::Type::UnsignedInteger, 	2,  sizeof(float) * 2)},
			
			{ "vec3", Pipeline::Structure(OpenGL::Type::Float, 				3,  sizeof(float) * 3)},
			{"ivec3", Pipeline::Structure(OpenGL::Type::Integer, 			3,  sizeof(float) * 3)},
			{"uvec3", Pipeline::Structure(OpenGL::Type::UnsignedInteger, 	3,  sizeof(float) * 3)},
			
			{ "vec4", Pipeline::Structure(OpenGL::Type::Float, 				4,  sizeof(float) * 4)},
			{"ivec4", Pipeline::Structure(OpenGL::Type::Integer, 			4,  sizeof(float) * 4)},
			{"uvec4", Pipeline::Structure(OpenGL::Type::UnsignedInteger, 	4,  sizeof(float) * 4)},
			
			{ "mat4", Pipeline::Structure(OpenGL::Type::Float, 				16, sizeof(float) * 16)},
		};

		for (const auto& instruction : p_instructions)
		{
			switch(instruction.type)
			{
				case OpenGL::ShaderInstruction::Type::Structure :
				{
					std::string name = instruction.informations[OpenGL::ShaderInstruction::TypeKey].as<std::string>();

					_structures[name] = Pipeline::Structure(_structures, instruction);
					break;
				}
			}
		}

		for (const auto& instruction : p_instructions)
		{
			switch(instruction.type)
			{
				case OpenGL::ShaderInstruction::Type::UniformBlock :
				{
					std::string name = instruction.informations[OpenGL::ShaderInstruction::TypeKey].as<std::string>();

					_structures[name] = Pipeline::Structure(_structures, instruction);
					break;
				}
			}
		}
	}
}