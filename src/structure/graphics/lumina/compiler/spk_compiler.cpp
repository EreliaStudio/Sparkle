#include "structure/graphics/lumina/compiler/spk_compiler.hpp"

#include "utils/spk_file_utils.hpp"
#include "structure/spk_iostream.hpp"
#include <regex>

namespace 
{
	void dumpTokens(const std::vector<spk::Lumina::Token>& p_tokens,
                           std::wostream& p_os = std::wcout)
    {
        if (p_tokens.empty())
        {
            p_os << L"(no tokens)\n";
            return;
        }

        std::size_t typeW   = std::wstring(L"Type").size();
        std::size_t lexW    = std::wstring(L"Lexeme").size();
        std::size_t lineW   = std::wstring(L"Line").size();
        std::size_t colW    = std::wstring(L"Col").size();

        for (const auto& t : p_tokens)
        {
            typeW = std::max(typeW, spk::Lumina::to_wstring(t.type).size());
            lexW  = std::max(lexW,  t.lexeme.size());
            lineW = std::max(lineW, std::to_wstring(t.location.line).size());
            colW  = std::max(colW,  std::to_wstring(t.location.column).size());
        }

        const wchar_t* sep = L" | ";

        p_os << std::left << std::setw(typeW) << L"Type"   << sep
           << std::setw(lexW)  << L"Lexeme" << sep
           << std::setw(lineW) << L"Line"   << sep
           << std::setw(colW)  << L"Col"    << L'\n';

        p_os << std::wstring(typeW + lexW + lineW + colW + 3 * wcslen(sep), L'-')
           << L'\n';

        for (const auto& t : p_tokens)
        {
            p_os << std::left
               << std::setw(typeW) << to_wstring(t.type)  << sep
               << std::setw(lexW)  << t.lexeme                  << sep
               << std::setw(lineW) << t.location.line           << sep
               << std::setw(colW)  << t.location.column         << L'\n';
        }
    }
}

namespace spk::Lumina
{
	void Compiler::addIncludePath(const std::filesystem::path& p_path)
	{
		_sourceManager.addIncludePath(p_path);
	}

	Shader Compiler::compile(const std::wstring &p_source)
	{
		if (p_source.empty())
		{
			GENERATE_ERROR("Shader source code is empty.");
		}

		std::vector<Token> tokens = _sourceManager.readToken(L"Unnamed shader", p_source);

		dumpTokens(tokens);

		std::wstring compiledSource = L"";

		return Shader::fromSource(compiledSource);
	}
}