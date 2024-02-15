#include "graphics/pipeline/spk_pipeline.hpp"

namespace spk
{	
	void convertTexture(std::string& p_source)
    {
        std::regex textureRegex("Texture\\s+([a-zA-Z0-9_]+);");
        std::smatch matches;
        std::string sourceCopy = p_source;

        std::string tempSource = p_source;

        while (std::regex_search(sourceCopy, matches, textureRegex))
        {
            std::string textureName = matches[1];
            std::string replacement = "uniform sampler2D " + textureName + ";";

            std::regex textureDeclRegex("Texture\\s+" + textureName + ";");
            tempSource = std::regex_replace(tempSource, textureDeclRegex, replacement);

            sourceCopy = matches.suffix().str();
        }

        p_source = tempSource;
    }
}