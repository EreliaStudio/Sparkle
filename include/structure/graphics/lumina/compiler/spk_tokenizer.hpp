#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "structure/graphics/lumina/compiler/spk_token.hpp"

namespace spk::Lumina
{
	class Tokenizer
	{
	public:
		Tokenizer(std::wstring_view p_src, std::wstring p_file = L"<memory>") :
			_src(p_src),
			_file(std::move(p_file))
		{
		}

		[[nodiscard]] std::vector<Token> run();

	private:
		[[nodiscard]] bool eof() const noexcept
		{
			return _idx >= _src.size();
		}
		[[nodiscard]] wchar_t peek(std::size_t p_off = 0) const noexcept
		{
			return (_idx + p_off < _src.size()) ? _src[_idx + p_off] : 0;
		}
		wchar_t advance();
		void skipComment();

		Token make(Token::Type p_type, std::size_t p_start, size_t p_line, size_t p_col) const;
		Token scanIdentifierOrKeyword(std::size_t p_start, size_t p_line, size_t p_col);
		Token scanNumber(std::size_t p_start, size_t p_line, size_t p_col);
		Token scanString(std::size_t p_start, size_t p_line, size_t p_col);
		Token scanPreprocessor(std::size_t p_start, size_t p_line, size_t p_col);
		Token scanComment(std::size_t p_start, size_t p_line, size_t p_col);
		std::optional<Token> scanOperator(std::size_t p_start, size_t p_line, size_t p_col);

		Token scan();

		// members
		std::wstring_view _src;
		std::wstring _file;
		std::size_t _idx = 0;
		size_t _line = 0;
		size_t _col = 0;
	};
}