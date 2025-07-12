#include "structure/graphics/lumina/compiler/spk_lexer.hpp"

namespace spk::Lumina
{
	const std::unordered_map<std::wstring, Token::Type> Lexer::kKeyword = {
		{L"struct", Token::Type::Struct},
		{L"namespace", Token::Type::Namespace},
		{L"attribute", Token::Type::AttributeBlock},
		{L"constant", Token::Type::ConstantBlock},
		{L"texture", Token::Type::Texture},
		{L"Input", Token::Type::Input},
		{L"VertexPass", Token::Type::VertexPass},
		{L"FragmentPass", Token::Type::FragmentPass},
		{L"Output", Token::Type::Output},
		{L"raiseException", Token::Type::RaiseException},
		{L"if", Token::Type::If},
		{L"else", Token::Type::Else},
		{L"for", Token::Type::For},
		{L"while", Token::Type::While},
		{L"return", Token::Type::Return},
		{L"discard", Token::Type::Discard},
		{L"true", Token::Type::BoolLiteral},
		{L"false", Token::Type::BoolLiteral},
	};

	wchar_t Lexer::advance()
	{
		wchar_t c = peek();
		if (c)
		{
			++_idx;
			if (c == L'\n')
			{
				++_line;
				_col = 1;
			}
			else
			{
				++_col;
			}
		}
		return c;
	}

	void Lexer::skipTrivia()
	{
		while (!eof())
		{
			wchar_t c = peek();
			if (std::iswspace(c))
			{
				wchar_t skippedChar = advance();
				continue;
			}
			if (c == L'\n')
			{
				wchar_t skippedChar = advance();
				continue;
			}
			break;
		}
	}

	void Lexer::consumePreprocessor()
	{
		bool cont;
		do
		{
			while (peek() && peek() != L'\n')
			{
				wchar_t skippedChar = advance();
			}
			cont = (_idx && _src[_idx - 1] == L'\\');
			if (cont && peek())
			{
				wchar_t skippedChar = advance();
			}
		} while (cont && peek());
	}

	bool Lexer::consumeIf(wchar_t p_expected)
	{
		if (peek() == p_expected)
		{
			wchar_t skippedChar = advance();
			return true;
		}
		return false;
	}

	Token Lexer::makeToken(Token::Type p_type, size_t p_start) const
	{
		return {p_type, std::wstring{_src.substr(p_start, _idx - p_start)}, {_file, _line, _col - static_cast<int>(_idx - p_start)}};
	}

	std::optional<Token> Lexer::matchOperator(wchar_t p_first, size_t p_start)
	{
		const auto emit = [&](Token::Type p_type) { return makeToken(p_type, p_start); };
		switch (p_first)
		{
		case L'+':
			if (consumeIf(L'+'))
			{
				return emit(Token::Type::PlusPlus);
			}
			if (consumeIf(L'='))
			{
				return emit(Token::Type::PlusEqual);
			}
			return emit(Token::Type::Plus);
		case L'-':
			if (consumeIf(L'-'))
			{
				return emit(Token::Type::MinusMinus);
			}
			if (consumeIf(L'='))
			{
				return emit(Token::Type::MinusEqual);
			}
			if (consumeIf(L'>'))
			{
				return emit(Token::Type::Arrow);
			}
			return emit(Token::Type::Minus);
		case L'*':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::MultiplyEqual);
			}
			return emit(Token::Type::Multiply);
		case L'/':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::DivideEqual);
			}
			return emit(Token::Type::Divide);
		case L'%':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::PercentEqual);
			}
			return emit(Token::Type::Percent);
		case L'=':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::EqualEqual);
			}
			return emit(Token::Type::Equal);
		case L'!':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::Different);
			}
			return emit(Token::Type::Bang);
		case L'<':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::LessEqual);
			}
			return emit(Token::Type::Less);
		case L'>':
			if (consumeIf(L'='))
			{
				return emit(Token::Type::GreaterEqual);
			}
			return emit(Token::Type::Greater);
		case L'&':
			if (consumeIf(L'&'))
			{
				return emit(Token::Type::AndAnd);
			}
			break;
		case L'|':
			if (consumeIf(L'|'))
			{
				return emit(Token::Type::OrOr);
			}
			break;
		case L'.':
			return emit(Token::Type::Dot);
		case L',':
			return emit(Token::Type::Comma);
		case L';':
			return emit(Token::Type::Semicolon);
		case L':':
			if (consumeIf(L':'))
			{
				return emit(Token::Type::DoubleColon);
			}
			return emit(Token::Type::Colon);
		case L'?':
			return emit(Token::Type::Question);
		case L'(':
			return emit(Token::Type::LeftParen);
		case L')':
			return emit(Token::Type::RightParen);
		case L'{':
			return emit(Token::Type::LeftBrace);
		case L'}':
			return emit(Token::Type::RightBrace);
		case L'[':
			return emit(Token::Type::LeftBracket);
		case L']':
			return emit(Token::Type::RightBracket);
		}
		return std::nullopt;
	}

	Token Lexer::scan()
	{
		skipTrivia();
		const size_t start = _idx;
		if (eof())
		{
			return makeToken(Token::Type::EndOfFile, start);
		}

		const wchar_t c = advance();

		if (isIdentStart(c))
		{
			while (isIdentBody(peek()))
			{
				wchar_t skippedChar = advance();
			}
			std::wstring text{_src.substr(start, _idx - start)};
			auto kw = kKeyword.find(text);
			return makeToken(kw == kKeyword.end() ? Token::Type::Identifier : kw->second, start);
		}

		if (std::iswdigit(c))
		{
			bool dotSeen = false;
			while (std::iswdigit(peek()) || (!dotSeen && peek() == L'.'))
			{
				dotSeen |= (peek() == L'.');
				wchar_t skippedChar = advance();
			}
			return makeToken(Token::Type::NumberLiteral, start);
		}

		if (c == L'\"')
		{
			bool esc = false;
			while (!eof())
			{
				wchar_t n = advance();
				if (!n)
				{
					break;
				}
				if (esc)
				{
					esc = false;
					continue;
				}
				if (n == L'\\')
				{
					esc = true;
					continue;
				}
				if (n == L'\"')
				{
					break;
				}
			}
			return makeToken(Token::Type::StringLiteral, start);
		}

		if (c == L'#')
		{
			consumePreprocessor();
			return makeToken(Token::Type::Preprocessor, start);
		}

		if (auto op = matchOperator(c, start))
		{
			return *op;
		}

		return makeToken(Token::Type::Unknown, start);
	}

	Lexer::Lexer(std::wstring_view p_src, std::wstring p_filename) :
		_src(p_src),
		_file(std::move(p_filename))
	{

	}

	std::vector<Token> Lexer::run()
	{
		std::vector<Token> out;

		while (true)
		{
			Token tok = scan();

			out.push_back(tok);

			if (tok.type == Token::Type::EndOfFile)
			{
				break;
			}
		}

		return out;
	}
}