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
		_dispatchGlobal = {{Token::Type::Namespace, &Lexer::parseNamespace},
						   {Token::Type::Struct, &Lexer::parseStruct},
						   {Token::Type::AttributeBlock, &Lexer::parseAttributeBlock},
						   {Token::Type::ConstantBlock, &Lexer::parseConstantBlock},
						   {Token::Type::Texture, &Lexer::parseTexture},
						   {Token::Type::Preprocessor, &Lexer::parseInclude},
						   {Token::Type::ShaderPass, &Lexer::parseShader}};

		_dispatchBody = {{Token::Type::OpenCurlyBracket, &Lexer::parseBody},
						 {Token::Type::If, &Lexer::parseIf},
						 {Token::Type::For, &Lexer::parseFor},
						 {Token::Type::While, &Lexer::parseWhile},
						 {Token::Type::Return, &Lexer::parseReturn},
						 {Token::Type::Discard, &Lexer::parseDiscard}};

		// Structure/attribute/constant dispatches are intentionally left empty
		// so that only generic parsing is allowed (methods, operators, variables)
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

	std::unordered_map<Token::Type, Lexer::ParseFn> &Lexer::_currentDispatch()
	{
		if (_bodyDepth > 0)
		{
			return _dispatchBody;
		}

		switch (_container)
		{
		case Container::Struct:
			return _dispatchStructure;
		case Container::AttributeBlock:
			return _dispatchAttribute;
		case Container::ConstantBlock:
			return _dispatchConstant;
		default:
			return _dispatchGlobal;
		}
	}

	static bool isFunctionPattern(const Token &p_tokenA, const Token &p_tokenB, const Token &p_tokenC)
	{
		return p_tokenA.type == Token::Type::Identifier && p_tokenB.type == Token::Type::Identifier && p_tokenC.type == Token::Type::OpenParenthesis;
	}

	static bool isConstructorPattern(Lexer::Container p_c, const std::wstring &p_name, const Token &p_tokenA, const Token &p_tokenB)
	{
		return p_c == Lexer::Container::Struct && p_tokenA.type == Token::Type::Identifier && p_tokenA.lexeme == p_name && p_tokenB.type == Token::Type::OpenParenthesis;
	}

	bool Lexer::isFunctionStart() const
	{
		return _bodyDepth == 0 && isFunctionPattern(peek(), peek(1), peek(2));
	}

	bool Lexer::isConstructorStart() const
	{
		return _bodyDepth == 0 && _container == Container::Struct && peek().type == Token::Type::Identifier && peek().lexeme == _currentStruct && peek(1).type == Token::Type::OpenParenthesis;
	}

	static bool isOperatorToken(Token::Type p_type)
	{
		return p_type >= Token::Type::Plus && p_type <= Token::Type::CloseBracket;
	}

	static bool isOperatorPattern(const Token &p_tokenA, const Token &p_tokenB, const Token &p_tokenC, const Token &p_tokenD)
	{
		return p_tokenA.type == Token::Type::Identifier && p_tokenB.type == Token::Type::Operator && isOperatorToken(p_tokenC.type) && p_tokenD.type == Token::Type::OpenParenthesis;
	}

	bool Lexer::isOperatorStart() const
	{
		return _bodyDepth == 0 && isOperatorPattern(peek(), peek(1), peek(2), peek(3));
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

		std::unique_ptr<BodyNode> body;
		if (peek().type == Token::Type::OpenCurlyBracket)
		{
			auto compound = parseStructureContent();
			body.reset(static_cast<BodyNode *>(compound.release()));
		}

		if (peek().type == Token::Type::Semicolon)
		{
			advance();
		}

		return std::make_unique<FunctionNode>(p_kind, std::move(header), std::move(body));
	}

	std::unique_ptr<ASTNode> Lexer::parseOperator()
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

		std::unique_ptr<BodyNode> body;
		if (peek().type == Token::Type::OpenCurlyBracket)
		{
			auto compound = parseStructureContent();
			body.reset(static_cast<BodyNode *>(compound.release()));
		}

		if (peek().type == Token::Type::Semicolon)
		{
			advance();
		}

		return std::make_unique<OperatorNode>(std::move(header), std::move(body));
	}

	std::unique_ptr<ASTNode> Lexer::parseNamespace()
	{
		Token nameTok = expect(Token::Type::Namespace, DEBUG_INFO());
		Token ident = expect(Token::Type::Identifier, DEBUG_INFO());
		auto bodyAst = parseBody();
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyAst.release()));
		return std::make_unique<NamespaceNode>(ident, std::move(body));
	}

	std::unique_ptr<ASTNode> Lexer::parseStruct()
	{
		Token keywordToken = expect(Token::Type::Struct, DEBUG_INFO());
		Token ident = expect(Token::Type::Identifier, DEBUG_INFO());
		Container prev = _container;
		std::wstring prevName = _currentStruct;
		_container = Container::Struct;
		_currentStruct = ident.lexeme;
		auto bodyAst = parseBody();
		_container = prev;
		_currentStruct = prevName;
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyAst.release()));

		std::vector<std::unique_ptr<ASTNode>> variables;
		std::vector<std::unique_ptr<ASTNode>> constructors;
		std::vector<std::unique_ptr<ASTNode>> methods;
		std::vector<std::unique_ptr<ASTNode>> ops;
		for (auto &child : body->children)
		{
			switch (child->kind)
			{
			case ASTNode::Kind::VariableDeclaration:
				variables.push_back(std::move(child));
				break;
			case ASTNode::Kind::Constructor:
				constructors.push_back(std::move(child));
				break;
			case ASTNode::Kind::Method:
				methods.push_back(std::move(child));
				break;
			case ASTNode::Kind::Operator:
				ops.push_back(std::move(child));
				break;
			default:
				variables.push_back(std::move(child));
				break;
			}
		}

		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<StructureNode>(ident, std::move(variables), std::move(constructors), std::move(methods), std::move(ops));
	}

	std::unique_ptr<ASTNode> Lexer::parseAttributeBlock()
	{
		Token keywordToken = expect(Token::Type::AttributeBlock, DEBUG_INFO());
		Token ident = expect(Token::Type::Identifier, DEBUG_INFO());
		Container prev = _container;
		_container = Container::AttributeBlock;
		auto bodyAst = parseBody();
		_container = prev;
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyAst.release()));

		std::vector<std::unique_ptr<ASTNode>> variables;
		std::vector<std::unique_ptr<ASTNode>> methods;
		std::vector<std::unique_ptr<ASTNode>> ops;
		for (auto &child : body->children)
		{
			switch (child->kind)
			{
			case ASTNode::Kind::VariableDeclaration:
				variables.push_back(std::move(child));
				break;
			case ASTNode::Kind::Method:
				methods.push_back(std::move(child));
				break;
			case ASTNode::Kind::Operator:
				ops.push_back(std::move(child));
				break;
			default:
				variables.push_back(std::move(child));
				break;
			}
		}

		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<AttributeBlockNode>(ident, std::move(variables), std::move(methods), std::move(ops));
	}

	std::unique_ptr<ASTNode> Lexer::parseConstantBlock()
	{
		Token keywordToken = expect(Token::Type::ConstantBlock, DEBUG_INFO());
		Token ident = expect(Token::Type::Identifier, DEBUG_INFO());
		Container prev = _container;
		_container = Container::ConstantBlock;
		auto bodyAst = parseBody();
		_container = prev;
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyAst.release()));

		std::vector<std::unique_ptr<ASTNode>> variables;
		std::vector<std::unique_ptr<ASTNode>> methods;
		std::vector<std::unique_ptr<ASTNode>> ops;
		for (auto &child : body->children)
		{
			switch (child->kind)
			{
			case ASTNode::Kind::VariableDeclaration:
				variables.push_back(std::move(child));
				break;
			case ASTNode::Kind::Method:
				methods.push_back(std::move(child));
				break;
			case ASTNode::Kind::Operator:
				ops.push_back(std::move(child));
				break;
			default:
				variables.push_back(std::move(child));
				break;
			}
		}

		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<ConstantBlockNode>(ident, std::move(variables), std::move(methods), std::move(ops));
	}

	std::unique_ptr<ASTNode> Lexer::parseTexture()
	{
		Token keywordToken = expect(Token::Type::Texture, DEBUG_INFO());
		Token ident = expect(Token::Type::Identifier, DEBUG_INFO());
		expect(Token::Type::Semicolon, DEBUG_INFO());
		(void)keywordToken;
		return std::make_unique<TextureNode>(ident);
	}

	std::unique_ptr<ASTNode> Lexer::parseInclude()
	{
		Token hash = expect(Token::Type::Preprocessor, DEBUG_INFO());
		Token keywordToken = expect(Token::Type::Include, DEBUG_INFO());

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
			expect(Token::Type::Greater, DEBUG_INFO());
		}
		else
		{
			path.emplace_back(expect({Token::Type::StringLiteral, Token::Type::Identifier}, DEBUG_INFO()));
		}

		(void)keywordToken;
		return std::make_unique<IncludeNode>(std::move(path), system, hash.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseShader()
	{
		Token first = expect(Token::Type::ShaderPass, DEBUG_INFO());
		if (peek().type == Token::Type::Arrow)
		{
			advance();
			Token second = expect(Token::Type::ShaderPass, DEBUG_INFO());
			expect(Token::Type::Colon, DEBUG_INFO());
			std::vector<Token> decl;
			while ((eof() == false) && (peek().type != Token::Type::Semicolon))
			{
				decl.emplace_back(advance());
			}
			expect(Token::Type::Semicolon, DEBUG_INFO());
			return std::make_unique<PipelineDefinitionNode>(first, second, std::move(decl));
		}

		expect(Token::Type::OpenParenthesis, DEBUG_INFO());
		expect(Token::Type::CloseParenthesis, DEBUG_INFO());
		auto bodyAst = parseStructureContent();
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyAst.release()));
		return std::make_unique<PipelineBodyNode>(first, std::move(body));
	}

	bool Lexer::isVariableDeclarationStart() const
	{
		return peek().type == Token::Type::Identifier && peek(1).type == Token::Type::Identifier && peek(2).type != Token::Type::OpenParenthesis;
	}

	std::unique_ptr<ASTNode> Lexer::parseIf()
	{
		Token ifToken = expect(Token::Type::If, DEBUG_INFO());
		expect(Token::Type::OpenParenthesis, DEBUG_INFO());
		auto condition = parseExpression();
		expect(Token::Type::CloseParenthesis, DEBUG_INFO());
		auto thenCompoundAst = parseStructureContent();
		auto thenBody = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(thenCompoundAst.release()));
		std::unique_ptr<BodyNode> elseBody;
		if (peek().type == Token::Type::Else)
		{
			advance();
			auto elseCompoundAst = parseStructureContent();
			elseBody.reset(static_cast<BodyNode *>(elseCompoundAst.release()));
		}
		return std::make_unique<IfStatementNode>(std::move(condition), std::move(thenBody), std::move(elseBody), ifToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseFor()
	{
		Token forToken = expect(Token::Type::For, DEBUG_INFO());
		expect(Token::Type::OpenParenthesis, DEBUG_INFO());
		std::unique_ptr<ASTNode> initialization;
		if (peek().type != Token::Type::Semicolon)
		{
			initialization = parseExpression();
		}
		expect(Token::Type::Semicolon, DEBUG_INFO());
		std::unique_ptr<ASTNode> condition;
		if (peek().type != Token::Type::Semicolon)
		{
			condition = parseExpression();
		}
		expect(Token::Type::Semicolon, DEBUG_INFO());
		std::unique_ptr<ASTNode> increment;
		if (peek().type != Token::Type::CloseParenthesis)
		{
			increment = parseExpression();
		}
		expect(Token::Type::CloseParenthesis, DEBUG_INFO());
		auto bodyCompoundAst = parseStructureContent();
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyCompoundAst.release()));
		return std::make_unique<ForLoopNode>(
			std::move(initialization), std::move(condition), std::move(increment), std::move(body), forToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseWhile()
	{
		Token whileToken = expect(Token::Type::While, DEBUG_INFO());
		expect(Token::Type::OpenParenthesis, DEBUG_INFO());
		auto condition = parseExpression();
		expect(Token::Type::CloseParenthesis, DEBUG_INFO());
		auto bodyCompoundAst = parseStructureContent();
		auto body = std::unique_ptr<BodyNode>(static_cast<BodyNode *>(bodyCompoundAst.release()));
		return std::make_unique<WhileLoopNode>(std::move(condition), std::move(body), whileToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseReturn()
	{
		Token returnToken = expect(Token::Type::Return, DEBUG_INFO());
		std::unique_ptr<ASTNode> returnValue;
		if (peek().type != Token::Type::Semicolon)
		{
			returnValue = parseExpression();
		}
		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<ReturnStatementNode>(std::move(returnValue), returnToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseDiscard()
	{
		Token discardToken = expect(Token::Type::Discard, DEBUG_INFO());
		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<DiscardStatementNode>(discardToken.location);
	}

	std::unique_ptr<ASTNode> Lexer::parseVariableDeclaration()
	{
		Token typeToken = expect(Token::Type::Identifier, DEBUG_INFO());
		Token nameToken = expect(Token::Type::Identifier, DEBUG_INFO());
		std::unique_ptr<ASTNode> initializer;
		if (peek().type == Token::Type::Equal)
		{
			advance();
			initializer = parseExpression();
		}
		expect(Token::Type::Semicolon, DEBUG_INFO());
		return std::make_unique<VariableDeclarationNode>(typeToken, nameToken, std::move(initializer));
	}

	static int getPrecedence(Token::Type p_tokenType)
	{
		switch (p_tokenType)
		{
		case Token::Type::OrOr:
			return 1;
		case Token::Type::AndAnd:
			return 2;
		case Token::Type::EqualEqual:
		case Token::Type::Different:
			return 3;
		case Token::Type::Less:
		case Token::Type::Greater:
		case Token::Type::LessEqual:
		case Token::Type::GreaterEqual:
			return 4;
		case Token::Type::Plus:
		case Token::Type::Minus:
			return 5;
		case Token::Type::Multiply:
		case Token::Type::Divide:
		case Token::Type::Percent:
			return 6;
		default:
			return 0;
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
		if (peek().type == Token::Type::NumberLiteral || peek().type == Token::Type::BoolLiteral || peek().type == Token::Type::StringLiteral)
		{
			return std::make_unique<LiteralNode>(advance());
		}

		if (peek().type == Token::Type::Identifier)
		{
			Token name = advance();
			std::unique_ptr<ASTNode> node = std::make_unique<VariableReferenceNode>(name);
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
							{
								break;
							}
							advance();
						}
					}
					expect(Token::Type::CloseParenthesis, DEBUG_INFO());
					node = std::make_unique<CallExpressionNode>(std::move(node), std::move(args), name.location);
					continue;
				}
				if (peek().type == Token::Type::Dot)
				{
					Token dot = advance();
					Token member = expect(Token::Type::Identifier, DEBUG_INFO());
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
			expect(Token::Type::CloseParenthesis, DEBUG_INFO());
			return expr;
		}

		return std::make_unique<TokenNode>(advance());
	}

	std::unique_ptr<ASTNode> Lexer::parseUnary()
	{
		if (peek().type == Token::Type::Plus || peek().type == Token::Type::Minus || peek().type == Token::Type::Bang ||
			peek().type == Token::Type::PlusPlus || peek().type == Token::Type::MinusMinus)
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
			{
				break;
			}
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
			expect(Token::Type::Colon, DEBUG_INFO());
			auto elseExpr = parseExpression();
			return std::make_unique<TernaryExpressionNode>(std::move(left), std::move(thenExpr), std::move(elseExpr), q.location);
		}

		return left;
	}

	std::unique_ptr<ASTNode> Lexer::parseGeneric()
	{
		if (_bodyDepth > 0)
		{
			if (isConstructorPattern(_container, _currentStruct, peek(), peek(1)))
			{
				throw TokenException(L"Constructor not allowed inside a body", peek(), _sourceManager);
			}
			if (isFunctionPattern(peek(), peek(1), peek(2)))
			{
				throw TokenException(L"Function definition not allowed inside a body", peek(), _sourceManager);
			}
			if (isOperatorPattern(peek(), peek(1), peek(2), peek(3)))
			{
				throw TokenException(L"Operator definition not allowed inside a body", peek(), _sourceManager);
			}
		}

		if (isConstructorStart())
		{
			return parseFunction(ASTNode::Kind::Constructor);
		}

		if (isFunctionStart())
		{
			ASTNode::Kind kind = _container == Container::None ? ASTNode::Kind::Function : ASTNode::Kind::Method;
			return parseFunction(kind);
		}

		if (isOperatorStart())
		{
			return parseOperator();
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

	std::unique_ptr<ASTNode> Lexer::parseBody()
	{
		Location loc = expect(Token::Type::OpenCurlyBracket, DEBUG_INFO()).location;
		auto node = std::make_unique<BodyNode>(loc);

		while (eof() == false)
		{
			skipComment();
			if (peek().type == Token::Type::CloseCurlyBracket)
			{
				break;
			}

			if (_bodyDepth > 0)
			{
				Token::Type t = peek().type;
				if (t == Token::Type::Namespace || t == Token::Type::Struct || t == Token::Type::AttributeBlock || t == Token::Type::ConstantBlock ||
					t == Token::Type::Texture || t == Token::Type::Preprocessor || t == Token::Type::ShaderPass)
				{
					throw TokenException(L"Invalid declaration inside a body", peek(), _sourceManager);
				}
			}
			else if (_container != Container::None)
			{
				if (peek().type != Token::Type::Identifier)
				{
					throw TokenException(L"Unexpected token inside container", peek(), _sourceManager);
				}
			}

			auto &dispatch = _currentDispatch();
			auto it = dispatch.find(peek().type);
			if (it == dispatch.end())
			{
				node->children.emplace_back(parseGeneric());
			}
			else
			{
				node->children.emplace_back((this->*it->second)());
			}
		}

		expect(Token::Type::CloseCurlyBracket, DEBUG_INFO());

		return node;
	}

	std::unique_ptr<ASTNode> Lexer::parseStructureContent()
	{
		Location loc = expect(Token::Type::OpenCurlyBracket, DEBUG_INFO()).location;
		auto node = std::make_unique<BodyNode>(loc);

		++_bodyDepth;

		while (eof() == false)
		{
			skipComment();
			if (peek().type == Token::Type::CloseCurlyBracket)
			{
				break;
			}

			if (_bodyDepth > 0)
			{
				Token::Type t = peek().type;
				if (t == Token::Type::Namespace || t == Token::Type::Struct || t == Token::Type::AttributeBlock || t == Token::Type::ConstantBlock ||
					t == Token::Type::Texture || t == Token::Type::Preprocessor || t == Token::Type::ShaderPass)
				{
					throw TokenException(L"Invalid declaration inside a body", peek(), _sourceManager);
				}
			}
			else if (_container != Container::None)
			{
				if (peek().type != Token::Type::Identifier)
				{
					throw TokenException(L"Unexpected token inside container", peek(), _sourceManager);
				}
			}

			auto &dispatch = _currentDispatch();
			auto it = dispatch.find(peek().type);
			if (it == dispatch.end())
			{
				node->children.emplace_back(parseGeneric());
			}
			else
			{
				node->children.emplace_back((this->*it->second)());
			}
		}

		expect(Token::Type::CloseCurlyBracket, DEBUG_INFO());

		--_bodyDepth;

		return node;
	}

	static std::wstring makeUnexpected(Token::Type p_type, const Token &p_tok)
	{
		return L"Unexpected token [" + to_wstring(p_tok.type) + L"] while expecting a " + to_wstring(p_type) + L" token";
	}

	static std::wstring makeUnexpected(const std::vector<Token::Type> &p_types, const Token &p_tok)
	{
		std::wstring list;
		for (std::size_t i = 0; i < p_types.size(); ++i)
		{
			list += to_wstring(p_types[i]);

			if (i + 2 == p_types.size())
			{
				list += L" or ";
			}
			else if (i + 1 < p_types.size())
			{
				list += L", ";
			}
		}

		return L"Unexpected token [" + to_wstring(p_tok.type) + L"] while expecting {" + list + L"} token";
	}

	const Token &Lexer::expect(Token::Type p_type, const std::wstring &p_debugInfo)
	{
		return expect(p_type, makeUnexpected(p_type, peek()), p_debugInfo);
	}
	const Token &Lexer::expect(const std::vector<Token::Type> &p_types, const std::wstring &p_debugInfo)
	{
		return expect(p_types, makeUnexpected(p_types, peek()), p_debugInfo);
	}

	const Token &Lexer::expect(Token::Type p_type, const std::wstring &p_msg, const std::wstring &p_debugInfo)
	{
		if (peek().type != p_type)
		{
			throw TokenException(p_msg + L" - " + p_debugInfo, peek(), _sourceManager);
		}
		return advance();
	}

	const Token &Lexer::expect(const std::vector<Token::Type> &p_types, const std::wstring &p_msg, const std::wstring &p_debugInfo)
	{
		for (auto type : p_types)
		{
			if (peek().type == type)
			{
				return advance();
			}
		}
		throw TokenException(p_msg + L" - " + p_debugInfo, peek(), _sourceManager);
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

			auto &dispatch = _currentDispatch();
			auto it = dispatch.find(peek().type);
			if (it == dispatch.end())
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
