#pragma once

#include <regex>

#include "miscellaneous/JSON/spk_JSON_object.hpp"

namespace spk
{
	class Pipeline;
}

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
		friend class Pipeline;

	private:
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

		void _parseBody(spk::JSON::Object& p_objectToFill, const std::string& p_body);
		
	public:
		/**
		 * @enum Type
		 * @brief Enumerates the possible types of shader instructions, aiding in their categorization and processing.
		 */
		enum class Type : uint8_t
		{
			Unknow,		 ///< Represents an unidentified or unsupported instruction type.
			Version,		///< Corresponds to the GLSL version declaration.
			Structure,	  ///< Represents a GLSL structure definition.
			UniformBlock,   ///< Corresponds to a uniform block declaration.
			Sampler,		///< Represents a texture sampler declaration.
			Function,	   ///< Corresponds to a function definition.
			Input,		  ///< Represents an input variable declaration.
			Output		  ///< Corresponds to an output variable declaration.
		};

		
		Type type; //!< The type of the shader instruction, determining how it should be processed.
		std::string content; //!< The raw GLSL code snippet corresponding to this shader instruction.
		spk::JSON::Object informations; //!< A JSON object containing detailed information extracted from the shader instruction, useful for further processing or analysis.

		/**
		 * @brief Default constructor.
		 * 
		 * Initializes a new instance of ShaderInstruction, setting its type to Unknow and leaving content and informations empty.
		 */
		ShaderInstruction();

		/**
		 * @brief Constructs a ShaderInstruction with a specific type and extracted metadata.
		 * 
		 * Initializes a new instance of ShaderInstruction using the provided type and metadata extracted from a regex match
		 * against shader code. This constructor is used to directly create a structured representation of a shader instruction
		 * from parsed GLSL code.
		 * 
		 * @param p_type The type of the shader instruction.
		 * @param p_matches A std::smatch object containing the regex match results used to extract instruction metadata.
		 */
		ShaderInstruction(const Type& p_type, const std::smatch& p_matches);

		/**
		 * @brief Parses shader code into a list of ShaderInstructions.
		 * 
		 * Analyzes the input shader code, identifying and categorizing individual instructions or declarations. This method
		 * is key to the internal shader management system, enabling the dynamic handling of shader code for purposes such as
		 * optimization or conditional inclusion.
		 * 
		 * @param p_inputCode The GLSL shader code to be parsed.
		 * @return A vector of ShaderInstruction instances, each representing a categorized piece of the input shader code.
		 */
		static std::vector<ShaderInstruction> parseShaderInstruction(const std::string& p_inputCode);
	};
}