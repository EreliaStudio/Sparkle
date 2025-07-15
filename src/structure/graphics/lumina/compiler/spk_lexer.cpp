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
                        {Token::Type::ShaderPass, &Lexer::parseShader},
                        {Token::Type::If, &Lexer::parseIf},
                        {Token::Type::For, &Lexer::parseFor},
                        {Token::Type::While, &Lexer::parseWhile},
                        {Token::Type::Return, &Lexer::parseReturn},
                        {Token::Type::Discard, &Lexer::parseDiscard}
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
                if (eof() == false)
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
               while (eof() == false)
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
               Token keywordToken = expect(Token::Type::Struct);
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
               Token keywordToken = expect(Token::Type::AttributeBlock);
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
               Token keywordToken = expect(Token::Type::ConstantBlock);
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
               Token keywordToken = expect(Token::Type::Texture);
               Token ident = expect(Token::Type::Identifier);
               expect(Token::Type::Semicolon, "Expected ';'");
               (void)keywordToken;
               return std::make_unique<TextureNode>(ident);
       }

       std::unique_ptr<ASTNode> Lexer::parseInclude()
       {
               Token hash = expect(Token::Type::Preprocessor);
               Token keywordToken = expect(Token::Type::Include);

               bool system = false;
               std::vector<Token> path;

               if (peek().type == Token::Type::Less)
               {
                       system = true;
                       advance();
                       while ((eof() == false) && (peek().type != Token::Type::Greater))
                       {
                               path.emplace_back(advance());
                       }
                       expect(Token::Type::Greater, "Expected '>'");
               }
               else
               {
                       path.emplace_back(expect({Token::Type::StringLiteral, Token::Type::Identifier}));
               }

               (void)keywordToken;
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
                       while ((eof() == false) && (peek().type != Token::Type::Semicolon))
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

       bool Lexer::isVariableDeclarationStart() const
       {
               return peek().type == Token::Type::Identifier &&
                      peek(1).type == Token::Type::Identifier &&
                      peek(2).type != Token::Type::OpenParenthesis;
       }

       std::unique_ptr<ASTNode> Lexer::parseIf()
       {
               Token ifToken = expect(Token::Type::If);
               expect(Token::Type::OpenParenthesis);
               auto condition = parseExpression();
               expect(Token::Type::CloseParenthesis);
               auto thenCompoundAst = parseCompound();
               auto thenBody = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(thenCompoundAst.release()));
               std::unique_ptr<CompoundNode> elseBody;
               if (peek().type == Token::Type::Else)
               {
                       advance();
                       auto elseCompoundAst = parseCompound();
                       elseBody.reset(static_cast<CompoundNode*>(elseCompoundAst.release()));
               }
               return std::make_unique<IfStatementNode>(std::move(condition), std::move(thenBody), std::move(elseBody), ifToken.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseFor()
       {
               Token forToken = expect(Token::Type::For);
               expect(Token::Type::OpenParenthesis);
               std::unique_ptr<ASTNode> initialization;
               if (peek().type != Token::Type::Semicolon)
               {
                       initialization = parseExpression();
               }
               expect(Token::Type::Semicolon);
               std::unique_ptr<ASTNode> condition;
               if (peek().type != Token::Type::Semicolon)
               {
                       condition = parseExpression();
               }
               expect(Token::Type::Semicolon);
               std::unique_ptr<ASTNode> increment;
               if (peek().type != Token::Type::CloseParenthesis)
               {
                       increment = parseExpression();
               }
               expect(Token::Type::CloseParenthesis);
               auto bodyCompoundAst = parseCompound();
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyCompoundAst.release()));
               return std::make_unique<ForLoopNode>(std::move(initialization), std::move(condition), std::move(increment), std::move(body), forToken.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseWhile()
       {
               Token whileToken = expect(Token::Type::While);
               expect(Token::Type::OpenParenthesis);
               auto condition = parseExpression();
               expect(Token::Type::CloseParenthesis);
               auto bodyCompoundAst = parseCompound();
               auto body = std::unique_ptr<CompoundNode>(static_cast<CompoundNode*>(bodyCompoundAst.release()));
               return std::make_unique<WhileLoopNode>(std::move(condition), std::move(body), whileToken.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseReturn()
       {
               Token returnToken = expect(Token::Type::Return);
               std::unique_ptr<ASTNode> returnValue;
               if (peek().type != Token::Type::Semicolon)
               {
                       returnValue = parseExpression();
               }
               expect(Token::Type::Semicolon);
               return std::make_unique<ReturnStatementNode>(std::move(returnValue), returnToken.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseDiscard()
       {
               Token discardToken = expect(Token::Type::Discard);
               expect(Token::Type::Semicolon);
               return std::make_unique<DiscardStatementNode>(discardToken.location);
       }

       std::unique_ptr<ASTNode> Lexer::parseVariableDeclaration()
       {
               Token typeToken = expect(Token::Type::Identifier);
               Token nameToken = expect(Token::Type::Identifier);
               std::unique_ptr<ASTNode> initializer;
               if (peek().type == Token::Type::Equal)
               {
                       advance();
                       initializer = parseExpression();
               }
               expect(Token::Type::Semicolon);
               return std::make_unique<VariableDeclarationNode>(typeToken, nameToken, std::move(initializer));
       }

       static int getPrecedence(Token::Type p_tokenType)
       {
               switch (p_tokenType)
               {
                       case Token::Type::OrOr:             return 1;
                       case Token::Type::AndAnd:           return 2;
                       case Token::Type::EqualEqual:
                       case Token::Type::Different:        return 3;
                       case Token::Type::Less:
                       case Token::Type::Greater:
                       case Token::Type::LessEqual:
                       case Token::Type::GreaterEqual:     return 4;
                       case Token::Type::Plus:
                       case Token::Type::Minus:            return 5;
                       case Token::Type::Multiply:
                       case Token::Type::Divide:
                       case Token::Type::Percent:          return 6;
                       default:                            return 0;
               }
       }

       static bool isAssignmentOperator(Token::Type p_tokenType)
       {
               switch (p_tokenType)
               {
                       case Token::Type::Equal:
                       case Token::Type::PlusEqual:
                       case Token::Type::MinusEqual:
                       case Token::Type::MultiplyEqual:
                       case Token::Type::DivideEqual:
                       case Token::Type::PercentEqual:
                               return true;
                       default:
                               return false;
               }
       }

       std::unique_ptr<ASTNode> Lexer::parsePrimary()
       {
               if (peek().type == Token::Type::NumberLiteral ||
                   peek().type == Token::Type::BoolLiteral ||
                   peek().type == Token::Type::StringLiteral)
               {
                       return std::make_unique<LiteralNode>(advance());
               }

               if (peek().type == Token::Type::Identifier)
               {
                       Token name = advance();
                       auto node = std::make_unique<VariableReferenceNode>(name);
                       while (true)
                       {
                               if (peek().type == Token::Type::OpenParenthesis)
                               {
                                       advance();
                                       std::vector<std::unique_ptr<ASTNode>> args;
                                       if (peek().type != Token::Type::CloseParenthesis)
                                       {
                                               while (true)
                                               {
                                                       args.emplace_back(parseExpression());
                                                       if (peek().type != Token::Type::Comma)
                                                               break;
                                                       advance();
                                               }
                                       }
                                       expect(Token::Type::CloseParenthesis);
                                       node = std::make_unique<CallExpressionNode>(std::move(node), std::move(args), name.location);
                                       continue;
                               }
                               if (peek().type == Token::Type::Dot)
                               {
                                       Token dot = advance();
                                       Token member = expect(Token::Type::Identifier);
                                       node = std::make_unique<MemberAccessNode>(std::move(node), member, dot.location);
                                       continue;
                               }
                               break;
                       }
                       return node;
               }

               if (peek().type == Token::Type::OpenParenthesis)
               {
                       advance();
                       auto expr = parseExpression();
                       expect(Token::Type::CloseParenthesis);
                       return expr;
               }

               return std::make_unique<TokenNode>(advance());
       }

       std::unique_ptr<ASTNode> Lexer::parseUnary()
       {
               if (peek().type == Token::Type::Plus || peek().type == Token::Type::Minus ||
                   peek().type == Token::Type::Bang || peek().type == Token::Type::PlusPlus ||
                   peek().type == Token::Type::MinusMinus)
               {
                       Token op = advance();
                       auto operand = parseUnary();
                       return std::make_unique<UnaryExpressionNode>(op, std::move(operand), op.location);
               }
               return parsePrimary();
       }

       std::unique_ptr<ASTNode> Lexer::parseBinaryRHS(int p_precedence, std::unique_ptr<ASTNode> p_left)
       {
               while (true)
               {
                       Token::Type opType = peek().type;
                       int opPrec = getPrecedence(opType);
                       if (opPrec < p_precedence)
                               break;
                       Token op = advance();
                       auto right = parseUnary();
                       int nextPrec = getPrecedence(peek().type);
                       if (opPrec < nextPrec)
                       {
                               right = parseBinaryRHS(opPrec + 1, std::move(right));
                       }
                       p_left = std::make_unique<BinaryExpressionNode>(std::move(p_left), op, std::move(right), op.location);
               }
               return p_left;
       }

       std::unique_ptr<ASTNode> Lexer::parseExpression()
       {
               auto left = parseUnary();
               left = parseBinaryRHS(1, std::move(left));

               if (isAssignmentOperator(peek().type))
               {
                       Token op = advance();
                       auto rhs = parseExpression();
                       return std::make_unique<AssignmentNode>(std::move(left), op, std::move(rhs));
               }

               if (peek().type == Token::Type::Question)
               {
                       Token q = advance();
                       auto thenExpr = parseExpression();
                       expect(Token::Type::Colon);
                       auto elseExpr = parseExpression();
                       return std::make_unique<TernaryExpressionNode>(std::move(left), std::move(thenExpr), std::move(elseExpr), q.location);
               }

               return left;
       }

       std::unique_ptr<ASTNode> Lexer::parseGeneric()
       {
               if (isFunctionStart() == true)
               {
                       ASTNode::Kind kind = _inStruct ? ASTNode::Kind::Method : ASTNode::Kind::Function;
                       return parseFunction(kind);
               }

               if (isVariableDeclarationStart() == true)
               {
                       return parseVariableDeclaration();
               }

               auto expr = parseExpression();
               if (peek().type == Token::Type::Semicolon)
               {
                       advance();
               }
               return expr;
       }

       std::unique_ptr<ASTNode> Lexer::parseCompound()
       {
               Location loc = expect(Token::Type::OpenCurlyBracket).location;
               auto node = std::make_unique<CompoundNode>(loc);
               while (eof() == false)
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

                while (eof() == false)
                {
                        skipComment();
                        if (eof() == true)
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
