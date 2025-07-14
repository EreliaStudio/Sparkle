#include "structure/graphics/lumina/compiler/spk_lexer.hpp"

#include "spk_debug_macro.hpp"
#include "structure/graphics/lumina/compiler/spk_token_exception.hpp"

#include <utility>

namespace spk::Lumina
{
	Lexer::Lexer(SourceManager &p_sourceManager, const std::vector<Token> &p_tokens) :
		_sourceManager(p_sourceManager),
		_tokens(p_tokens)
	{
		_dispatch = {

		};
	}

	const Token &Lexer::peek(std::ptrdiff_t p_offset) const
	{
		std::ptrdiff_t targetIndex = static_cast<std::ptrdiff_t>(_idx) + p_offset;
		if (targetIndex < 0)
		{
			targetIndex = 0;
		}
		if (static_cast<std::size_t>(targetIndex) >= _tokens.size())
		{
			return _tokens.back();
		}
		return _tokens[static_cast<std::size_t>(targetIndex)];
	}

	bool Lexer::eof() const
	{
		return peek().type == Token::Type::EndOfFile;
	}

	const Token &Lexer::advance()
	{
		if (!eof())
		{
			++_idx;
		}
		return peek(-1);
	}

	void Lexer::skipComment()
	{
		while (peek().type == Token::Type::Comment || peek().type == Token::Type::Whitespace)
		{
			advance();
		}
	}

	static std::string makeUnexpected(const Token &p_tok)
	{
		return "Unexpected token [" + to_string(p_tok.type) + "]";
	}

	const Token &Lexer::expect(Token::Type p_type)
	{
		return expect(p_type, makeUnexpected(peek()));
	}
	const Token &Lexer::expect(const std::vector<Token::Type> &p_types)
	{
		return expect(p_types, makeUnexpected(peek()));
	}

	const Token &Lexer::expect(Token::Type p_type, const std::string &p_msg)
	{
		if (peek().type != p_type)
		{
			throw TokenException{p_msg, peek(), _sourceManager};
		}
		return advance();
	}

	const Token &Lexer::expect(const std::vector<Token::Type> &p_types, const std::string &p_msg)
	{
		for (auto type : p_types)
		{
			if (peek().type == type)
			{
				return advance();
			}
		}
		throw TokenException{p_msg, peek(), _sourceManager};
	}
	
	std::vector<std::unique_ptr<ASTNode>> Lexer::run()
	{
		std::vector<std::unique_ptr<ASTNode>> result;

		while (!eof())
		{
			skipComment();
			if (eof())
			{
				break;
			}

			auto it = _dispatch.find(peek().type);
			if (it == _dispatch.end())
			{
				throw TokenException{makeUnexpected(peek()), peek(), _sourceManager};
			}

			result.emplace_back((this->*it->second)());
		}
		return result;
	}
}
