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

		std::wstring underline(p_token.location.column, L' ');
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