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
		const Token &expect(Token::Type p_type);
		const Token &expect(Token::Type p_type, const std::string &p_msg);
		const Token &expect(const std::vector<Token::Type> &p_types);
		const Token &expect(const std::vector<Token::Type> &p_types, const std::string &p_msg);
		const Token &advance();
		const Token &peek(std::ptrdiff_t p_offset = 0) const;
		bool eof() const;

		SourceManager &_sourceManager;
		std::vector<Token> _tokens;
		std::size_t _idx = 0;
		std::unordered_map<Token::Type, ParseFn> _dispatch;
	};
}
