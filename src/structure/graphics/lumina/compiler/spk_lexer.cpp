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
			{Token::Type::Preprocessor, &Lexer::parsePreprocessor},
			{Token::Type::ShaderPass, &Lexer::parseShaderPass},
			{Token::Type::Namespace, &Lexer::parseNamespace},
			{Token::Type::Struct, &Lexer::parseDataBlock},
			{Token::Type::AttributeBlock, &Lexer::parseDataBlock},
			{Token::Type::ConstantBlock, &Lexer::parseDataBlock},
			{Token::Type::Texture, &Lexer::parseTextureDecl},
			{Token::Type::Identifier, &Lexer::parseFunctionOrVariable},
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

	std::unique_ptr<ASTNode> Lexer::parseInclude()
	{
		const Token &preprocessorToken = peek(-2);
		std::wstring path;
		const std::size_t line = peek().location.line;

		if (peek().type == Token::Type::Less)
		{
			advance();
			const Token &identifierToken = expect(Token::Type::Identifier, "Expected identifier after '<'");
			path = identifierToken.lexeme;
			while (peek().type == Token::Type::Divide)
			{
				advance();
				path += L'/';
				const Token &newIdentifierToken = expect(Token::Type::Identifier, "Expected identifier after '/'");
				path += newIdentifierToken.lexeme;
			}
			skipComment();
			expect(Token::Type::Greater, "Missing '>' in include path");
		}
		else if (peek().type == Token::Type::StringLiteral)
		{
			const Token &token = advance();
			path = token.lexeme;
			if (path.size() >= 2 && path.front() == L'"' && path.back() == L'"')
			{
				path = path.substr(1, path.size() - 2);
			}
		}
		else
		{
			throw TokenException{"Expected <path> or \"path\" after include", peek(), _sourceManager};
		}

		while (!eof() && peek().location.line == line)
		{
			if (peek().type != Token::Type::Comment && peek().type != Token::Type::Whitespace)
			{
				throw TokenException{"Unexpected token after include", peek(), _sourceManager};
			}
			advance();
		}

		return std::make_unique<IncludeASTNode>(std::filesystem::path{path}, preprocessorToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parsePreprocessor()
	{
		advance();
		if (peek().lexeme == L"include")
		{
			advance();
			return parseInclude();
		}
		throw TokenException{"Invalid preprocessor directive", peek(), _sourceManager};
	}

	std::unique_ptr<ASTNode> Lexer::parseNamespace()
	{
		const Token &keywordToken = expect(Token::Type::Namespace);
		const Token &nameToken = expect(Token::Type::Identifier, "Namespace needs a name");
		expect(Token::Type::OpenCurlyBracket);

		std::vector<std::unique_ptr<ASTNode>> declarations;
		while (!eof() && peek().type != Token::Type::CloseCurlyBracket)
		{
			skipComment();
			auto it = _dispatch.find(peek().type);
			if (it == _dispatch.end())
			{
				throw TokenException{"Unexpected token in namespace", peek(), _sourceManager};
			}
			declarations.emplace_back((this->*it->second)());
		}
		expect(Token::Type::CloseCurlyBracket);
		return std::make_unique<NamespaceASTNode>(nameToken.lexeme, std::move(declarations), keywordToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseDataBlock()
	{
		const Token& keywordToken = advance();
		const Token &nameToken = expect(Token::Type::Identifier, "Missing name");
		expect(Token::Type::OpenCurlyBracket);

		std::vector<std::unique_ptr<ASTNode>> members;
		while (!eof() && peek().type != Token::Type::CloseCurlyBracket)
		{
			skipComment();
			auto it = _dispatch.find(peek().type);
			if (it == _dispatch.end())
			{
				throw TokenException{"Unexpected token in block", peek(), _sourceManager};
			}
			members.emplace_back((this->*it->second)());
		}
		expect(Token::Type::CloseCurlyBracket);
		if (peek().type == Token::Type::Semicolon)
		{
			advance();
		}

		switch (keywordToken.type)
		{
		case Token::Type::Struct:
			return std::make_unique<StructASTNode>(nameToken.lexeme, std::move(members), nameToken.location);
		case Token::Type::AttributeBlock:
			return std::make_unique<AttributeBlockASTNode>(nameToken.lexeme, std::move(members), nameToken.location);
		case Token::Type::ConstantBlock:
			return std::make_unique<ConstantBlockASTNode>(nameToken.lexeme, std::move(members), nameToken.location);
		default:
			throw TokenException{"Unexpected token in block definition", keywordToken, _sourceManager};
		}
	}

	std::unique_ptr<ASTNode> Lexer::parseTextureDecl()
	{
		const Token &keywordToken = expect(Token::Type::Texture);
		const Token &nameToken = expect(Token::Type::Identifier, "Texture needs a name");
		expect(Token::Type::Semicolon, "Missing ';'");
		return std::make_unique<TextureDeclASTNode>(nameToken.lexeme, keywordToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseBody()
	{
		const Token &openToken = expect(Token::Type::OpenCurlyBracket, "Expected '{'");
		std::size_t depth = 1;
		while (!eof() && depth != 0)
		{
			if (peek().type == Token::Type::OpenCurlyBracket)
			{
				++depth;
			}
			else if (peek().type == Token::Type::CloseCurlyBracket)
			{
				--depth;
			}
			advance();
		}
		if (depth != 0)
		{
			throw TokenException{"Unterminated body - missing '}'", peek(), _sourceManager};
		}
		return std::make_unique<BodyASTNode>(openToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parsePipelineFlow()
	{
		const Token &inputToken = expect(Token::Type::ShaderPass);
		expect(Token::Type::Arrow);
		const Token &outputToken = expect(Token::Type::ShaderPass);
		expect(Token::Type::Colon);
		const Token &typeToken = expect(Token::Type::Identifier);
		const Token &nameToken = expect(Token::Type::Identifier);
		expect(Token::Type::Semicolon, "Expected ';'");

		if (inputToken.lexeme == L"Output")
		{
			throw TokenException{"Invalid input pass", inputToken, _sourceManager};
		}
		if (outputToken.lexeme == L"Input")
		{
			throw TokenException{"Invalid output pass", outputToken, _sourceManager};
		}

		return std::make_unique<PipelineFlowASTNode>(inputToken.lexeme, outputToken.lexeme, typeToken.lexeme, nameToken.lexeme, inputToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parsePipelinePass()
	{
		const Token &nameToken = expect(Token::Type::ShaderPass);

		if (nameToken.lexeme != L"VertexPass" && nameToken.lexeme != L"FragmentPass")
		{
			throw TokenException{"Only VertexPass or FragmentPass are allowed", nameToken, _sourceManager};
		}

		expect(Token::Type::OpenParenthesis);
		expect(Token::Type::CloseParenthesis, "Pass cannot have parameters");

		auto body = parseBody();

		return std::make_unique<PipelinePassASTNode>(nameToken.lexeme, std::move(body), nameToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseShaderPass()
	{
		if (peek(1).type == Token::Type::Arrow)
		{
			return parsePipelineFlow();
		}
		return parsePipelinePass();
	}

	std::unique_ptr<ASTNode> Lexer::parseFunctionOrVariable()
	{
		const Token &typeTok = expect(Token::Type::Identifier);
		const Token &nameToken = expect(Token::Type::Identifier, "Missing identifier");

		if (peek().type == Token::Type::OpenParenthesis)
		{
			advance();
			std::vector<std::unique_ptr<VariableDeclASTNode>> parameters;

			if (peek().type != Token::Type::CloseParenthesis)
			{
				while (true)
				{
					const Token &parameterTypeToken = expect(Token::Type::Identifier, "Parameter type");
					const Token &parameterNameToken = expect(Token::Type::Identifier, "Parameter name");
					parameters.emplace_back(std::make_unique<VariableDeclASTNode>(parameterTypeToken.lexeme, parameterNameToken.lexeme, nullptr, parameterTypeToken.location));
					if (peek().type == Token::Type::Comma)
					{
						advance();
						continue;
					}
					break;
				}
			}
			expect(Token::Type::CloseParenthesis);

			auto body = parseBody();
			return std::make_unique<FunctionASTNode>(typeTok.lexeme,
													 nameToken.lexeme,
													 std::move(parameters),
													 std::unique_ptr<BodyASTNode>(static_cast<BodyASTNode *>(body.release())),
													 typeTok.location);
		}

		std::unique_ptr<ExpressionASTNode> inputTokenit;
		if (peek().type == Token::Type::Equal)
		{
			advance();
			const Token &literalToken = expect({Token::Type::NumberLiteral, Token::Type::StringLiteral, Token::Type::BoolLiteral}, "Expected literal");
			inputTokenit = std::make_unique<LiteralExprASTNode>(literalToken.lexeme, literalToken.location);
		}
		expect(Token::Type::Semicolon, "Missing ';' after variable declaration");

		return std::make_unique<VariableDeclASTNode>(typeTok.lexeme, nameToken.lexeme, std::move(inputTokenit), typeTok.location);
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
