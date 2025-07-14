#include "structure/graphics/lumina/compiler/spk_tokenizer.hpp"

#include "spk_debug_macro.hpp"

namespace spk::Lumina
{
	static const std::unordered_map<std::wstring_view, Token::Type> kKeyword = {
		{L"struct", Token::Type::Struct},
		{L"namespace", Token::Type::Namespace},
		{L"attribute", Token::Type::AttributeBlock},
		{L"constant", Token::Type::ConstantBlock},
		{L"texture", Token::Type::Texture},
		{L"Input", Token::Type::ShaderPass},
		{L"VertexPass", Token::Type::ShaderPass},
		{L"FragmentPass", Token::Type::ShaderPass},
		{L"Output", Token::Type::ShaderPass},
		{L"raiseException", Token::Type::RaiseException},
		{L"if", Token::Type::If},
		{L"else", Token::Type::Else},
		{L"for", Token::Type::For},
		{L"while", Token::Type::While},
		{L"return", Token::Type::Return},
		{L"discard", Token::Type::Discard},
		{L"true", Token::Type::BoolLiteral},
		{L"false", Token::Type::BoolLiteral},
		{L"include", Token::Type::Include}
	};

	static const std::vector<std::pair<std::wstring_view, Token::Type>> kOperator = {
		{L"++", Token::Type::PlusPlus},	  {L"+=", Token::Type::PlusEqual},	 {L"+", Token::Type::Plus},			{L"--", Token::Type::MinusMinus},
		{L"-=", Token::Type::MinusEqual}, {L"->", Token::Type::Arrow},		 {L"-", Token::Type::Minus},		{L"*=", Token::Type::MultiplyEqual},
		{L"*", Token::Type::Multiply},	  {L"/=", Token::Type::DivideEqual}, {L"/", Token::Type::Divide},		{L"%=", Token::Type::PercentEqual},
		{L"%", Token::Type::Percent},	  {L"==", Token::Type::EqualEqual},	 {L"=", Token::Type::Equal},		{L"!=", Token::Type::Different},
		{L"!", Token::Type::Bang},		  {L"<=", Token::Type::LessEqual},	 {L"<", Token::Type::Less},			{L">=", Token::Type::GreaterEqual},
		{L">", Token::Type::Greater},	  {L"&&", Token::Type::AndAnd},		 {L"||", Token::Type::OrOr},		{L"::", Token::Type::DoubleColon},
		{L".", Token::Type::Dot},		  {L",", Token::Type::Comma},		 {L";", Token::Type::Semicolon},	{L":", Token::Type::Colon},
		{L"?", Token::Type::Question},
		{L"(", Token::Type::OpenParenthesis},  {L")", Token::Type::CloseParenthesis},
		{L"{", Token::Type::OpenCurlyBracket}, {L"}", Token::Type::CloseCurlyBracket},
		{L"[", Token::Type::OpenBracket},	   {L"]", Token::Type::CloseBracket},
	};

	wchar_t Tokenizer::advance()
	{
		wchar_t c = peek();
		if (c)
		{
			++_idx;
			if (c == L'\n')
			{
				++_line;
				_col = 0;
			}
			else
			{
				++_col;
			}
		}
		return c;
	}

	void Tokenizer::skipComment()
	{
		while (!eof())
		{
			if (std::iswspace(peek()))
			{
				advance();
				continue;
			}
			break;
		}
	}

	Token Tokenizer::make(Token::Type p_type, std::size_t p_start, size_t p_line, size_t p_col) const
	{
		return {p_type, std::wstring{_src.substr(p_start, _idx - p_start)}, {_file, p_line, p_col}};
	}

	Token Tokenizer::scanIdentifierOrKeyword(std::size_t p_start, size_t p_line, size_t p_col)
	{
		while (std::iswalnum(peek()) || peek() == L'_')
		{
			advance();
		}
		std::wstring_view text = _src.substr(p_start, _idx - p_start);
		auto it = kKeyword.find(text);
		return make(it == kKeyword.end() ? Token::Type::Identifier : it->second, p_start, p_line, p_col);
	}

	Token Tokenizer::scanNumber(std::size_t p_start, size_t p_line, size_t p_col)
	{
		bool dotSeen = false;
		while (std::iswdigit(peek()) || (!dotSeen && peek() == L'.'))
		{
			dotSeen |= (peek() == L'.');
			advance();
		}
		return make(Token::Type::NumberLiteral, p_start, p_line, p_col);
	}

	Token Tokenizer::scanString(std::size_t p_start, size_t p_line, size_t p_col)
	{
		bool esc = false;
		while (!eof())
		{
			wchar_t ch = advance();
			if (ch == 0)
			{
				break;
			}
			if (esc)
			{
				esc = false;
				continue;
			}
			if (ch == L'\\')
			{
				esc = true;
				continue;
			}
			if (ch == L'"')
			{
				break;
			}
		}
		return make(Token::Type::StringLiteral, p_start, p_line, p_col);
	}

	Token Tokenizer::scanPreprocessor(std::size_t p_start, size_t p_line, size_t p_col)
	{
		return make(Token::Type::Preprocessor, p_start, p_line, p_col);
	}

	Token Tokenizer::scanComment(std::size_t p_start, size_t p_line, size_t p_col)
	{
		if (peek() == L'/')
		{
			advance();
			while (!eof() && peek() != L'\n')
			{
				advance();
			}
			return make(Token::Type::Comment, p_start, p_line, p_col);
		}

		if (peek() == L'*')
		{
			advance();
			while (!eof())
			{
				if (peek() == L'*' && peek(1) == L'/')
				{
					advance();
					advance();
					break;
				}
				advance();
			}
			return make(Token::Type::Comment, p_start, p_line, p_col);
		}

		_idx = p_start;
		_col = p_col;
		return make(Token::Type::Unknown, p_start, p_line, p_col);
	}

	std::optional<Token> Tokenizer::scanOperator(std::size_t p_start, size_t p_line, size_t p_col)
	{
		for (const auto &[text, type] : kOperator)
		{
			if (_src.substr(p_start, text.size()) == text)
			{
				_idx = p_start + text.size();
				_col = p_col + text.size();
				return make(type, p_start, p_line, p_col);
			}
		}
		return std::nullopt;
	}

	Token Tokenizer::scan()
	{
		skipComment();
		std::size_t start = _idx;
		size_t startLine = _line;
		size_t startCol = _col;

		if (eof())
		{
			return make(Token::Type::EndOfFile, start, startLine, startCol);
		}

		wchar_t c = advance();

		if (std::iswalpha(c) || c == L'_')
		{
			return scanIdentifierOrKeyword(start, startLine, startCol);
		}
		if (std::iswdigit(c))
		{
			return scanNumber(start, startLine, startCol);
		}
		if (c == L'"')
		{
			return scanString(start, startLine, startCol);
		}
		if (c == L'#')
		{
			return scanPreprocessor(start, startLine, startCol);
		}
		if (c == L'/' && (peek() == L'/' || peek() == L'*'))
		{
			return scanComment(start, startLine, startCol);
		}

		if (auto tok = scanOperator(start, startLine, startCol))
		{
			return *tok;
		}

		return make(Token::Type::Unknown, start, startLine, startCol);
	}

	std::vector<Token> Tokenizer::run()
	{
		std::vector<Token> out;
		do
		{
			out.emplace_back(scan());
		} while (out.back().type != Token::Type::EndOfFile);
		return out;
	}
}