#pragma once

#include <regex>

#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk::OpenGL
{
	/**
	 * @class ShaderInstruction
	 * @brief Represents and categorizes a single instruction or declaration within OpenGL shader code, used internally for shader management.
	 *
	 * ShaderInstruction facilitates the parsing and categorization of GLSL shader code, enabling the internal system to interpret and manage
	 * shader instructions more efficiently. Each instance of this class is associated with a specific type of shader instruction, such as
	 * uniform blocks, samplers, or functions, and contains both the raw GLSL code and extracted metadata in a structured format.
	 *
	 * Although primarily intended for internal use within the shader parsing and generation system, this documentation provides insight into
	 * how shader code is analyzed and processed. This can aid in understanding the capabilities and limitations of the system's shader management.
	 *
	 * Main Components:
	 * - type: Identifies the instruction type, aiding in its processing and management.
	 * - content: Stores the GLSL code snippet corresponding to the instruction.
	 * - informations: A JSON object encapsulating detailed information extracted from the instruction, useful for further processing or analysis.
	 *
	 * The `parseShaderInstruction` static method is a key part of this class, analyzing GLSL code to identify and organize its constituent
	 * instructions. This method is crucial for the dynamic handling of shaders, supporting operations such as modification, optimization,
	 * or conditional inclusion of shader code.
	 *
	 * Note: This class and its methods are designed for internal framework use and are not intended to be directly utilized by end-users.
	 */
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