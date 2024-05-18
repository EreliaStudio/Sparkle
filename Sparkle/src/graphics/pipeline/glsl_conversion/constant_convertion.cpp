#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{	

	size_t convertConstantBlock(std::string& p_source)
	{
		static std::unordered_map<std::string, size_t> bindingRegistry = {};
		static size_t bindingCounter = 0;

		std::regex constantBlockRegex("ConstantBlock\\s+([a-zA-Z0-9_]+)\\s*\\{([^}]*)\\}");
		std::smatch match;
		size_t higherBindingPointUsed = 0;

		while (std::regex_search(p_source, match, constantBlockRegex)) {
			std::string blockName = match[1].str();
			std::string blockContents = match[2].str();

			std::string mapKey = blockContents;

			mapKey.erase(std::remove_if(mapKey.begin(), mapKey.end(), 
									[](unsigned char x){ return std::isspace(x); }), mapKey.end());

			size_t bindingPoint;
			auto it = bindingRegistry.find(mapKey);
			if (it != bindingRegistry.end())
			{
				bindingPoint = it->second;
			}
			else
			{
				bindingPoint = bindingCounter++;
				bindingRegistry[mapKey] = bindingPoint;
			}

			if (higherBindingPointUsed < bindingPoint)
				higherBindingPointUsed = bindingPoint;

			std::string convertedBlock = "layout (binding = " + std::to_string(bindingPoint) + ") uniform " + Pipeline::Constant::BlockKey + "_" + blockName + "\n{\n" + blockContents + "\n} " + blockName;
			p_source.replace(match.position(), match.length(), convertedBlock);
		}
		return (higherBindingPointUsed + 1);
	}
}