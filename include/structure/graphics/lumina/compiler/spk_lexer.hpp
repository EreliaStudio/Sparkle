#pragma once

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
		explicit Lexer(SourceManager& p_sourceManager, const std::vector<Token> &tokens);

		std::vector<std::unique_ptr<ASTNode>> run();

	private:
		using ParseFn = std::unique_ptr<ASTNode> (Lexer::*)();

		const Token &peek(std::size_t p_offset = 0) const;
		const Token &advance();
		bool eof() const;

		void skipComment();
    	const Token &expect(Token::Type p_type, const char* p_msg);
    	const Token &expect(const std::vector<Token::Type>& p_types, const char* p_msg);

		std::unique_ptr<ASTNode> parsePreprocessor();
		std::unique_ptr<ASTNode> parseInclude();

		SourceManager& _sourceManager;
		std::unordered_map<Token::Type, ParseFn> _dispatch;
		std::vector<Token> _tokens;
		std::size_t _idx = 0;
	};
}
