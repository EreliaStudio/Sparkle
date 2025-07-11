#include "structure/graphics/lumina/spk_compiler.hpp"

#include "structure/spk_iostream.hpp"
#include <regex>

namespace spk::Lumina
{
	void Compiler::addIncludePath(const std::filesystem::path& p_path)
	{
		_includeFolderPaths.push_back(p_path);
	}

	std::wstring Compiler::cleanUpSource(const std::wstring& p_source)
	{
		static const std::wregex regexToApply(
        	LR"((//[^\n]*|/\*[\s\S]*?\*/))",
        	std::regex_constants::ECMAScript | std::regex_constants::optimize
		);

    	return std::regex_replace(p_source, regexToApply, L"");
	}

	Shader Compiler::compile(const std::wstring &p_source)
	{
		if (p_source.empty())
		{
			GENERATE_ERROR("Shader source code is empty.");
		}

		std::wstring cleanedSource = StringUtils::collapseString(Compiler::cleanUpSource(p_source), L' ', L" \t");

		spk::cout << "Cleaned up source : " << std::endl;
		spk::cout << cleanedSource << std::endl;

		std::wstring compiledSource = p_source;

		return Shader::fromSource(compiledSource);
	}
}