#include "graphics/pipeline/spk_pipeline.hpp"

#include <sstream>

namespace spk
{
	std::string extractVersionLine(std::string& p_outputCode);
    std::vector<std::pair<std::string, std::string>> getArgumentList(const std::string& p_code, const std::string& p_firstStageName, const std::string& p_secondStageName);
    std::string composeLayoutLines(const std::vector<std::pair<std::string, std::string>>& p_argumentList, const std::string& p_mode);
    void removeFunction(std::string& p_code, const std::string& p_functionName);
    void replaceFunctionDeclaration(std::string& p_code, const std::string& p_functionName, const std::string& p_newDeclaration);
    void swapCodeContent(std::string& p_code, const std::string& p_toFind, const std::string& p_toReplace);
    void removeArgumentList(std::string& p_code);
	void convertTexture(std::string& p_source);
	void convertAttributeBlock(std::string& p_source, size_t p_higherBindingPointUsed);
	size_t convertConstantBlock(std::string& p_source);

    std::string composeVertexCode(const std::string& p_inputCode)
    {
        std::string result = p_inputCode;

        std::string version = extractVersionLine(result);

        convertTexture(result);

        auto inputArgument = getArgumentList(result, "Input", "Geometry");
        auto outputArgument = getArgumentList(result, "Geometry", "Render");
        removeArgumentList(result);
        removeFunction(result, "renderPass");
        
        std::string inputLines = composeLayoutLines(inputArgument, "in");
        std::string outputLines = composeLayoutLines(outputArgument, "out");

        replaceFunctionDeclaration(result, "void geometryPass()", "void main()");
        swapCodeContent(result, "pixelPosition", "gl_Position");

        size_t higherBindingPoint = convertConstantBlock(result);
        convertAttributeBlock(result, higherBindingPoint);

        return (version + "\n" + inputLines + "\n" + outputLines + "\n" + result);
    }

	std::string composeFragmentCode(const std::string& p_inputCode)
    {
        std::string result = p_inputCode;

        std::string version = extractVersionLine(result);
        
        convertTexture(result);

        auto inputArgument = getArgumentList(result, "Geometry", "Render");
        removeArgumentList(result);
        removeFunction(result, "geometryPass");

        std::string inputLines = composeLayoutLines(inputArgument, "in");
        std::string outputLines = composeLayoutLines({std::make_pair("vec4", "pixelColor")}, "out");

        replaceFunctionDeclaration(result, "void renderPass()", "void main()");

        size_t higherBindingPoint = convertConstantBlock(result);
        convertAttributeBlock(result, higherBindingPoint);
        convertTexture(result);

        return (version + "\n" + inputLines + "\n" + outputLines + "\n" + result);
    }
}