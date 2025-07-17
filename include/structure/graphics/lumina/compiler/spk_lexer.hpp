#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "structure/graphics/lumina/compiler/spk_ast.hpp"
#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"
#include "structure/graphics/lumina/compiler/spk_token.hpp"

namespace spk::Lumina
{
	class Lexer
	{
	public:
		Lexer(SourceManager &p_sourceManager, const std::vector<Token> &p_tokens);

		std::vector<std::unique_ptr<ASTNode>> run();

	private:
		using ParseFn = std::unique_ptr<ASTNode> (Lexer::*)();

		void skipComment();
		std::unique_ptr<ASTNode> parseGeneric();
		std::unique_ptr<ASTNode> parseCompound();
		std::unique_ptr<ASTNode> parseNamespace();
		std::unique_ptr<ASTNode> parseStruct();
		std::unique_ptr<ASTNode> parseAttributeBlock();
		std::unique_ptr<ASTNode> parseConstantBlock();
		std::unique_ptr<ASTNode> parseTexture();
		std::unique_ptr<ASTNode> parseInclude();
		std::unique_ptr<ASTNode> parseShader();
		std::unique_ptr<ASTNode> parseIf();
		std::unique_ptr<ASTNode> parseFor();
		std::unique_ptr<ASTNode> parseWhile();
		std::unique_ptr<ASTNode> parseReturn();
		std::unique_ptr<ASTNode> parseDiscard();
		std::unique_ptr<ASTNode> parseVariableDeclaration();
		std::unique_ptr<ASTNode> parseExpression();
		std::unique_ptr<ASTNode> parseUnary();
		std::unique_ptr<ASTNode> parsePrimary();
		std::unique_ptr<ASTNode> parseBinaryRHS(int p_precedence, std::unique_ptr<ASTNode> p_left);
		std::unique_ptr<ASTNode> parseFunction(ASTNode::Kind p_kind);
		bool isFunctionStart() const;
		bool isVariableDeclarationStart() const;
		const Token &expect(Token::Type p_type, const std::wstring &p_debugInfo);
		const Token &expect(Token::Type p_type, const std::wstring &p_msg, const std::wstring &p_debugInfo);
		const Token &expect(const std::vector<Token::Type> &p_types, const std::wstring &p_debugInfo);
		const Token &expect(const std::vector<Token::Type> &p_types, const std::wstring &p_msg, const std::wstring &p_debugInfo);
		const Token &advance();
		const Token &peek(std::ptrdiff_t p_offset = 0) const;
		bool eof() const;

		SourceManager &_sourceManager;
		std::vector<Token> _tokens;
		std::size_t _idx = 0;
		std::unordered_map<Token::Type, ParseFn> _dispatch;
		bool _inStruct = false;
	};
}
