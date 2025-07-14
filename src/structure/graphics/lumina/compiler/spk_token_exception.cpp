#include "structure/graphics/lumina/compiler/spk_token_exception.hpp"

#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	TokenException::TokenException(const std::string& p_msg, const Token& p_token, const SourceManager& p_sourceManager)
            : std::runtime_error(compose(p_msg, p_token, p_sourceManager))
    {}

	std::string TokenException::compose(const std::string& p_msg, const Token& p_token, const SourceManager& p_sourceManager)
	{
		const std::wstring& wsrcLine = p_sourceManager.getSourceLine(p_token.location);
		std::string srcLine = spk::StringUtils::wstringToString(wsrcLine);

		std::string underline(p_token.location.column, ' ');
		underline.append(p_token.lexeme.size(), '-');

		std::ostringstream oss;
		oss << p_token.location.source.string() << ':'
			<< p_token.location.line << ':'
			<< p_token.location.column << ": error: "
			<< p_msg << '\n'
			<< srcLine << '\n'
			<< underline;
		return oss.str();
	}
}