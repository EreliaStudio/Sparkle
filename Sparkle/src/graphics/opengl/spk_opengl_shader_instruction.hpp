#pragma once

#include <regex>

#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk::OpenGL
{
	struct ShaderInstruction
	{
		static inline const std::string ValueKey = "Value";
		static inline const std::string TypeKey = "Type";
		static inline const std::string NameKey = "Name";
		static inline const std::string BodyKey = "Body";
		static inline const std::string SizeKey = "Size";
		static inline const std::string LengthKey = "Length";
		static inline const std::string AttributesKey = "Attributes";
		static inline const std::string BindingKey = "Binding";
		static inline const std::string ParametersKey = "Parameters";
		static inline const std::string LocationKey = "Location";

		enum class Type : uint8_t
		{
			Unknow,
			Version,
			Structure,
			UniformBlock,
			Sampler,
			Function,
			Input,
			Output
		};

		Type type;
		std::string content;
		spk::JSON::Object informations;

		ShaderInstruction();
		ShaderInstruction(const Type& p_type, const std::smatch& p_matches);

		static std::vector<ShaderInstruction> parseShaderInstruction(const std::string& p_inputCode);
	};
}