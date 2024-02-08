#include "graphics/opengl/spk_opengl_shader_instruction.hpp"

namespace spk::OpenGL
{
	ShaderInstruction::ShaderInstruction() :
		type(Type::Unknow),
		content("")
	{

	}

	void ShaderInstruction::_parseBody(spk::JSON::Object& p_objectToFill, const std::string& p_body)
	{
		std::regex attributeParsing(R"((\w+)\s+(\w+)(?:\[(\d+)\])?)");
		auto begin = std::sregex_iterator(p_body.begin(), p_body.end(), attributeParsing);
		auto end = std::sregex_iterator();
		for (std::sregex_iterator i = begin; i != end; ++i)
		{
			std::smatch matches = *i;
			spk::JSON::Object insertedAttribute;
			insertedAttribute[ShaderInstruction::TypeKey].set<std::string>(matches[1].str());
			insertedAttribute[ShaderInstruction::NameKey].set<std::string>(matches[2].str());
			if (matches[3].matched == true)
				insertedAttribute[ShaderInstruction::LengthKey].set<std::string>(matches[3].str());
			else
				insertedAttribute[ShaderInstruction::LengthKey].set<std::string>("1");

			p_objectToFill.push_back(insertedAttribute);
		}
	}

	ShaderInstruction::ShaderInstruction(const ShaderInstruction::Type& p_type, const std::smatch& p_matches) :
		type(p_type),
		content(p_matches[0].str())
	{
		switch (p_type)
		{
			case Type::Version :
			{
				informations[ValueKey].set<std::string>(p_matches[1].str());
				break;
			}


			case Type::UniformBlock :
			{
				std::string body = p_matches[3].str();

				informations[BindingKey].set<std::string>(p_matches[1].str());
				informations[TypeKey].set<std::string>(p_matches[2].str());
				informations[BodyKey].set<std::string>(body);
				informations[NameKey].set<std::string>(p_matches[4].str());
				informations[AttributesKey].setAsArray();

				_parseBody(informations[AttributesKey], body);
				break;
			}
			case Type::Structure :
			{
				std::string body = p_matches[2].str();

				informations[TypeKey].set<std::string>(p_matches[1].str());
				informations[BodyKey].set<std::string>(body);
				informations[AttributesKey].setAsArray();

				_parseBody(informations[AttributesKey], body);
				break;
			}


			case Type::Sampler :
			{
				informations[TypeKey].set<std::string>(p_matches[1].str());
				informations[NameKey].set<std::string>(p_matches[2].str());
				break;
			}
			case Type::Function :
			{
				informations[TypeKey].set<std::string>(p_matches[1].str());
				informations[NameKey].set<std::string>(p_matches[2].str());
				informations[ParametersKey].set<std::string>(p_matches[3].str());
				informations[BodyKey].set<std::string>(p_matches[4].str());
				break;
			}

			case Type::Input :
			case Type::Output :
			{
				informations[LocationKey].set<std::string>(p_matches[1].str());
				informations[TypeKey].set<std::string>(p_matches[2].str());
				informations[NameKey].set<std::string>(p_matches[3].str());
				break;
			}

			default :
			{

				break;
			}
		}
	}
}