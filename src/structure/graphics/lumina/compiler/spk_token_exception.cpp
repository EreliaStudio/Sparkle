#include "structure/graphics/lumina/compiler/spk_token_exception.hpp"

#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	TokenException::TokenException(const std::wstring& p_msg, const Token& p_token, const SourceManager& p_sourceManager)
            : std::runtime_error(spk::StringUtils::wstringToString(compose(p_msg, p_token, p_sourceManager)))
    {}

	std::wstring TokenException::compose(const std::wstring& p_msg, const Token& p_token, const SourceManager& p_sourceManager)
	{
		const std::wstring& srcLine = p_sourceManager.getSourceLine(p_token.location);

                const size_t tabWidth = 4;
                std::wstring underline;
                underline.reserve(p_token.location.column + p_token.lexeme.size());
                size_t visualPos = 0;
                for (size_t i = 0; i < p_token.location.column && i < srcLine.size(); ++i)
                {
                        if (srcLine[i] == L'\t')
                        {
                                size_t spaces = tabWidth - (visualPos % tabWidth);
                                underline.append(spaces, L' ');
                                visualPos += spaces;
                        }
                        else
                        {
                                underline.push_back(L' ');
                                ++visualPos;
                        }
                }
                underline.append(p_token.lexeme.size(), '-');

		std::wostringstream oss;
		oss << p_token.location.source.wstring() << L':'
			<< p_token.location.line << L':'
			<< p_token.location.column << L": error: "
			<< p_msg << L'\n'
			<< srcLine << L'\n'
			<< underline;

		return oss.str();
	}
}