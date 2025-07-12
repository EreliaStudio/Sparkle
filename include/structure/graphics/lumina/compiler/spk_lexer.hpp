#pragma once

#include <string>
#include <vector>
#include <optional>
#include <unordered_map>

#include "structure/graphics/lumina/compiler/spk_token.hpp"

namespace spk::Lumina
{
	class Lexer
	{
	public:
		Lexer(std::wstring_view src, std::wstring filename);

		std::vector<Token> run();

	private:
		const std::wstring_view _src;
		const std::wstring _file;
		size_t _idx = 0;
		size_t _line = 1;
		size_t _col = 1;

		[[nodiscard]] bool eof() const noexcept
		{
			return _idx >= _src.size();
		}
		[[nodiscard]] wchar_t peek(size_t off = 0) const noexcept
		{
			return (_idx + off < _src.size()) ? _src[_idx + off] : 0;
		}
		[[nodiscard]] wchar_t advance();
		void skipTrivia();
		void consumePreprocessor();
		bool consumeIf(wchar_t expected);

		Token makeToken(Token::Type type, size_t start) const;
		std::optional<Token> matchOperator(wchar_t first, size_t start);

		Token scan();

		static bool isIdentStart(wchar_t c) noexcept
		{
			return std::iswalpha(c) || c == L'_';
		}
		static bool isIdentBody(wchar_t c) noexcept
		{
			return std::iswalnum(c) || c == L'_';
		}

		static const std::unordered_map<std::wstring, Token::Type> kKeyword;
	};
}