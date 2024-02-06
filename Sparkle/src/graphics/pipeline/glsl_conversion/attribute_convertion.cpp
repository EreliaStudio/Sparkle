#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{	
	void convertAttributeBlock(std::string& p_source, size_t p_higherBindingPointUsed)
	{ 
		std::regex attributeBlockRegex("AttributeBlock\\s+([a-zA-Z0-9_]+)\\s*\\{([^}]*)\\}");
		std::smatch match;

		while (std::regex_search(p_source, match, attributeBlockRegex)) {
			std::string blockName = match[1].str();
			std::string blockContents = match[2].str();

			std::string convertedBlock = "layout (binding = " + std::to_string(p_higherBindingPointUsed) + ") uniform " + Pipeline::Object::Attribute::BlockKey + "_" + blockName + "\n{\n" + blockContents + "\n} " + blockName;
        	p_source.replace(match.position(), match.length(), convertedBlock);
			p_higherBindingPointUsed++;
		}
	}
}