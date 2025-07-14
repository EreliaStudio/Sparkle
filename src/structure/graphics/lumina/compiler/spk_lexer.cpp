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
                        {Token::Type::OpenCurlyBracket, &Lexer::parseCompound},
                        {Token::Type::Namespace, &Lexer::parseNamespace},
                        {Token::Type::Struct, &Lexer::parseStruct},
                        {Token::Type::AttributeBlock, &Lexer::parseAttributeBlock},
                        {Token::Type::ConstantBlock, &Lexer::parseConstantBlock},
                        {Token::Type::Texture, &Lexer::parseTexture},
                        {Token::Type::Preprocessor, &Lexer::parseInclude},
                        {Token::Type::ShaderPass, &Lexer::parseShader}
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

       bool Lexer::isFunctionStart() const
       {
               return peek().type == Token::Type::Identifier &&
                      peek(1).type == Token::Type::Identifier &&
                      peek(2).type == Token::Type::OpenParenthesis;
       }

       std::unique_ptr<ASTNode> Lexer::parseFunction(ASTNode::Kind p_kind)
       {
               std::vector<Token> header;
               while (!eof())
               {
                       if (peek().type == Token::Type::OpenCurlyBracket || peek().type == Token::Type::Semicolon)
                       {
                               break;
                       }
                       header.emplace_back(advance());
               }

               std::unique_ptr<CompoundNode> body;
               if (peek().type == Token::Type::OpenCurlyBracket)
               {
                       auto compound = parseCompound();
                       body.reset(static_cast<CompoundNode*>(compound.release()));
               }

               if (peek().type == Token::Type::Semicolon)
               {
                       advance();
               }

               return std::make_unique<FunctionNode>(p_kind, std::move(header), std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseNamespace()
       {
               Token nameTok = expect(Token::Type::Namespace);
               Token ident = expect(Token::Type::Identifier);
               auto bodyAst = parseCompound();
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyAst.release()));
               return std::make_unique<NamespaceNode>(ident, std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseStruct()
       {
               Token kw = expect(Token::Type::Struct);
               Token ident = expect(Token::Type::Identifier);
               bool prev = _inStruct;
               _inStruct = true;
               auto bodyAst = parseCompound();
               _inStruct = prev;
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyAst.release()));
               expect(Token::Type::Semicolon, "Expected ';'");
               return std::make_unique<StructureNode>(ident, std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseAttributeBlock()
       {
               Token kw = expect(Token::Type::AttributeBlock);
               Token ident = expect(Token::Type::Identifier);
               bool prev = _inStruct;
               _inStruct = true;
               auto bodyAst = parseCompound();
               _inStruct = prev;
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyAst.release()));
               expect(Token::Type::Semicolon, "Expected ';'");
               return std::make_unique<AttributeBlockNode>(ident, std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseConstantBlock()
       {
               Token kw = expect(Token::Type::ConstantBlock);
               Token ident = expect(Token::Type::Identifier);
               bool prev = _inStruct;
               _inStruct = true;
               auto bodyAst = parseCompound();
               _inStruct = prev;
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyAst.release()));
               expect(Token::Type::Semicolon, "Expected ';'");
               return std::make_unique<ConstantBlockNode>(ident, std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseTexture()
       {
               Token kw = expect(Token::Type::Texture);
               Token ident = expect(Token::Type::Identifier);
               expect(Token::Type::Semicolon, "Expected ';'");
               (void)kw;
               return std::make_unique<TextureNode>(ident);
       }

       std::unique_ptr<ASTNode> Lexer::parseInclude()
       {
               Token hash = expect(Token::Type::Preprocessor);
               Token kw = expect(Token::Type::Include);

               bool system = false;
               std::vector<Token> path;

               if (peek().type == Token::Type::Less)
               {
                       system = true;
                       advance();
                       while (!eof() && peek().type != Token::Type::Greater)
                       {
                               path.emplace_back(advance());
                       }
                       expect(Token::Type::Greater, "Expected '>'");
               }
               else
               {
                       path.emplace_back(expect({Token::Type::StringLiteral, Token::Type::Identifier}));
               }

               (void)kw;
               return std::make_unique<IncludeNode>(std::move(path), system, hash.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseShader()
       {
               Token first = expect(Token::Type::ShaderPass);
               if (peek().type == Token::Type::Arrow)
               {
                       advance();
                       Token second = expect(Token::Type::ShaderPass);
                       expect(Token::Type::Colon);
                       std::vector<Token> decl;
                       while (!eof() && peek().type != Token::Type::Semicolon)
                       {
                               decl.emplace_back(advance());
                       }
                       expect(Token::Type::Semicolon, "Expected ';'");
                       return std::make_unique<PipelineDefinitionNode>(first, second, std::move(decl));
               }

               expect(Token::Type::OpenParenthesis);
               expect(Token::Type::CloseParenthesis);
               auto bodyAst = parseCompound();
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyAst.release()));
               return std::make_unique<PipelineBodyNode>(first, std::move(body));
       }

       std::unique_ptr<ASTNode> Lexer::parseGeneric()
       {
               if (isFunctionStart())
               {
                       ASTNode::Kind kind = _inStruct ? ASTNode::Kind::Method : ASTNode::Kind::Function;
                       return parseFunction(kind);
               }
               return std::make_unique<TokenNode>(advance());
       }

       std::unique_ptr<ASTNode> Lexer::parseCompound()
       {
               Location loc = expect(Token::Type::OpenCurlyBracket).location;
               auto node = std::make_unique<CompoundNode>(loc);
               while (!eof())
               {
                       skipComment();
                       if (peek().type == Token::Type::CloseCurlyBracket)
                       {
                               break;
                       }

                       auto it = _dispatch.find(peek().type);
                       if (it == _dispatch.end())
                       {
                               node->children.emplace_back(parseGeneric());
                               continue;
                       }

                       node->children.emplace_back((this->*it->second)());
               }
               expect(Token::Type::CloseCurlyBracket, "Expected '}'");
               return node;
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
                               result.emplace_back(parseGeneric());
                       }
                       else
                       {
                               result.emplace_back((this->*it->second)());
                       }
               }
               return result;
       }
}
