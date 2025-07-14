#include "structure/graphics/lumina/compiler/spk_lexer.hpp"

#include "structure/spk_iostream.hpp"

#include "structure/graphics/lumina/compiler/spk_token_exception.hpp"

#include "utils/spk_string_utils.hpp"

#include <regex>

#include "spk_debug_macro.hpp"

namespace spk::Lumina
{
	Lexer::Lexer(SourceManager &p_sourceManager, const std::vector<Token> &p_tokens) :
		_sourceManager(p_sourceManager),
		_tokens(p_tokens)
	{
		_dispatch = {
			{Token::Type::Preprocessor, &Lexer::parsePreprocessor}
		};
	}

	void Lexer::skipComment()
	{
		while (true)
		{
			if (peek().type == Token::Type::Comment || peek().type == Token::Type::Whitespace)
			{
				advance();
			}
			else
			{
				break;
			}
		}
	}

	const Token& Lexer::expect(Token::Type p_type, const char *p_msg)
	{
		if (peek().type != p_type)
		{
			throw TokenException{p_msg, peek(), _sourceManager};
		}
		return advance();
	}

	const Token& Lexer::expect(const std::vector<Token::Type>& p_types, const char* p_msg)
	{
		for (const auto& type : p_types)
		{
			if (type == peek().type)
			{
				return advance();
			}
		}
		throw TokenException{p_msg, peek(), _sourceManager};
	}

	std::unique_ptr<ASTNode> Lexer::parseInclude()
	{
		const Token&   startTok  = peek(-2);
		const Location startLoc  = startTok.location;
		
		std::wstring   path;
		const std::size_t includeLine = peek().location.line;

		if (peek().type == Token::Type::Less)
		{
			advance();
			const Token& first = expect(Token::Type::Identifier, "Expected identifier after '<' in include path");
			path = first.lexeme;

			while (true)
			{
				if (peek().type != Token::Type::Divide)
				{
					break;
				}

				advance();                               // consume '/'
				path += L'/';

				const Token& seg = expect(Token::Type::Identifier, "Expected identifier after '/' in include path");
				path += seg.lexeme;
			}

			skipComment();
			expect(Token::Type::Greater, "Missing '>' that closes include path");
		}
		else if (peek().type == Token::Type::StringLiteral)
		{
			const Token& tok = advance();
			path = tok.lexeme;
			if (path.size() >= 2 && path.front() == L'"' && path.back() == L'"')
			{
				path = path.substr(1, path.size() - 2);
			}
		}
		else
		{
			throw TokenException{ "Expected '<Path>' block or '\"Path\"' after 'include'", peek(), _sourceManager};
		}

		while (!eof() && peek().location.line == includeLine)
		{
			if (peek().type != Token::Type::Comment &&
				peek().type != Token::Type::Whitespace)
			{
				throw TokenException{
					"Unexpected token after include definition",
					peek(), _sourceManager};
			}
			advance();
		}

		Location incLoc = startLoc;
		return std::make_unique<IncludeASTNode>( std::filesystem::path{path}, startLoc);
	}

	std::unique_ptr<ASTNode> Lexer::parsePreprocessor()
	{
		advance();

		if (peek().lexeme == L"include")
		{
			advance();
			return (parseInclude());
		}
		else
		{
			throw TokenException{"Invalid preprocessor command", peek(), _sourceManager};
		}
	}

	std::vector<std::unique_ptr<ASTNode>> Lexer::run()
	{
		std::vector<std::unique_ptr<ASTNode>> nodes;
		while (!eof())
		{
			auto it = _dispatch.find(peek().type);

			if (it == _dispatch.end())
			{
				std::string msg = "Unexpected token " + to_string(peek().type) + " at " + std::to_string(peek().location.line) + ":" + std::to_string(peek().location.column);
				throw TokenException{msg, peek(), _sourceManager};
			}

			ParseFn fn = it->second;
			nodes.emplace_back((this->*fn)()); 
		}
		return nodes;
	}

	const Token &Lexer::peek(std::size_t p_offset) const
	{
		return _tokens[_idx + p_offset];
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
}