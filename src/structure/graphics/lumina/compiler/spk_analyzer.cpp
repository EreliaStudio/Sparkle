#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

#include "structure/graphics/lumina/compiler/spk_lexer.hpp"
#include "utils/spk_string_utils.hpp"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>

namespace spk::Lumina
{
	AnalyzerException::AnalyzerException(const std::wstring &p_msg, const Location &p_location, const SourceManager &p_sourceManager) :
		std::runtime_error(spk::StringUtils::wstringToString(compose(p_msg, p_location, p_sourceManager)))
	{
	}

	std::wstring AnalyzerException::compose(const std::wstring &p_msg, const Location &p_location, const SourceManager &p_sourceManager)
	{
		const std::wstring &srcLine = p_sourceManager.getSourceLine(p_location);

		std::wstring underline(p_location.column, L' ');
		underline.append(1, '^');

		std::wostringstream oss;
		oss << p_location.source.wstring() << L":" << p_location.line << L":" << p_location.column << L": error: " << p_msg << L"\n"
			<< srcLine << L"\n"
			<< underline;
		return oss.str();
	}

	Analyzer::Analyzer(SourceManager &p_sourceManager) :
		_sourceManager(p_sourceManager)
	{
		_dispatch = {{ASTNode::Kind::Token, &Analyzer::_analyzeToken},
					 {ASTNode::Kind::Body, &Analyzer::_analyzeBody},
					 {ASTNode::Kind::Namespace, &Analyzer::_analyzeNamespace},
					 {ASTNode::Kind::Structure, &Analyzer::_analyzeStructureLike},
					 {ASTNode::Kind::AttributeBlock, &Analyzer::_analyzeStructureLike},
					 {ASTNode::Kind::ConstantBlock, &Analyzer::_analyzeStructureLike},
					 {ASTNode::Kind::Texture, &Analyzer::_analyzeTexture},
					 {ASTNode::Kind::Include, &Analyzer::_analyzeInclude},
					 {ASTNode::Kind::Function, &Analyzer::_analyzeFunction},
					 {ASTNode::Kind::Method, &Analyzer::_analyzeFunction},
					 {ASTNode::Kind::Constructor, &Analyzer::_analyzeFunction},
					 {ASTNode::Kind::Operator, &Analyzer::_analyzeFunction},
					 {ASTNode::Kind::PipelineDefinition, &Analyzer::_analyzePipelineDef},
					 {ASTNode::Kind::PipelineBody, &Analyzer::_analyzePipelineBody},
					 {ASTNode::Kind::VariableDeclaration, &Analyzer::_analyzeVariableDeclaration},
					 {ASTNode::Kind::Assignment, &Analyzer::_analyzeAssignment},
					 {ASTNode::Kind::IfStatement, &Analyzer::_analyzeIfStatement},
					 {ASTNode::Kind::ForLoop, &Analyzer::_analyzeForLoop},
					 {ASTNode::Kind::WhileLoop, &Analyzer::_analyzeWhileLoop},
					 {ASTNode::Kind::ReturnStatement, &Analyzer::_analyzeReturn},
					 {ASTNode::Kind::DiscardStatement, &Analyzer::_analyzeDiscardStatement},
					 {ASTNode::Kind::BinaryExpression, &Analyzer::_analyzeBinaryExpression},
					 {ASTNode::Kind::UnaryExpression, &Analyzer::_analyzeUnaryExpression},
					 {ASTNode::Kind::CallExpression, &Analyzer::_analyzeCallExpression},
					 {ASTNode::Kind::MemberAccess, &Analyzer::_analyzeMemberAccess},
					 {ASTNode::Kind::VariableReference, &Analyzer::_analyzeVariableReference},
					 {ASTNode::Kind::Literal, &Analyzer::_analyzeLiteral},
					 {ASTNode::Kind::TernaryExpression, &Analyzer::_analyzeTernaryExpression},
					 {ASTNode::Kind::LValue, &Analyzer::_analyzeLValue},
					 {ASTNode::Kind::RValue, &Analyzer::_analyzeRValue}};
                _loadBuiltinTypes();
                _pushScope();
                _loadBuiltinVariables();
        }

        void Analyzer::run(const std::vector<std::unique_ptr<ASTNode>> &p_nodes)
        {
		_pipelineStages.clear();
		_textures.clear();
		_functionSignatures.clear();
		_containerStack.clear();
		_scopes.clear();
		_includedFiles.clear();
		_includeStack.clear();
                _pushScope();
                _loadBuiltinVariables();
		for (const auto &node : p_nodes)
		{
			if (!node)
			{
				continue;
			}

			auto it = _dispatch.find(node->kind);
			if (it == _dispatch.end())
			{
				continue;
			}

			(this->*it->second)(node.get());
		}
	}

	void Analyzer::_analyze(const ASTNode *p_node)
	{
		if (!p_node)
		{
			return;
		}

		auto it = _dispatch.find(p_node->kind);
		if (it == _dispatch.end())
		{
			return;
		}

		(this->*it->second)(p_node);
	}

	void Analyzer::_analyzeBody(const ASTNode *p_node)
	{
		const BodyNode *n = static_cast<const BodyNode *>(p_node);
		_pushScope();
		for (const auto &child : n->children)
		{
			if (child)
			{
				_analyze(child.get());
			}
		}
		_popScope();
	}

	void Analyzer::_analyzeInclude(const ASTNode *p_node)
	{
		const IncludeNode *n = static_cast<const IncludeNode *>(p_node);
		std::wstring path;
		for (const auto &tok : n->path)
		{
			path += tok.lexeme;
		}

		std::filesystem::path full;
		try
		{
			full = _sourceManager.getFilePath(std::filesystem::path(path));
		} catch (const std::exception &)
		{
			throw AnalyzerException(L"Unable to resolve include path: " + path, n->location, _sourceManager);
		}

		std::wstring fullStr = full.wstring();
		if (std::find(_includeStack.begin(), _includeStack.end(), fullStr) != _includeStack.end())
		{
			throw AnalyzerException(L"Circular include detected: " + fullStr, n->location, _sourceManager);
		}

		if (_includedFiles.count(fullStr) > 0)
		{
			return;
		}

		_includedFiles.insert(fullStr);
		_includeStack.push_back(fullStr);

		std::vector<Token> tokens = _sourceManager.readToken(full);
		Lexer lexer(_sourceManager, tokens);
		auto nodes = lexer.run();
		for (const auto &node : nodes)
		{
			if (node)
			{
				_analyze(node.get());
			}
		}

		_includeStack.pop_back();
	}

	static bool isValidStage(const std::wstring &p_name)
	{
		return p_name == L"Input" || p_name == L"VertexPass" || p_name == L"FragmentPass" || p_name == L"Output";
	}

	void Analyzer::_analyzePipelineDef(const ASTNode *p_node)
	{
		const PipelineDefinitionNode *n = static_cast<const PipelineDefinitionNode *>(p_node);
		if (!isValidStage(n->fromStage.lexeme) || !isValidStage(n->toStage.lexeme))
		{
			throw AnalyzerException(L"Invalid pipeline stage in definition", n->location, _sourceManager);
		}

                const std::wstring pair = n->fromStage.lexeme + L"->" + n->toStage.lexeme;
                static const std::unordered_set<std::wstring> allowed = {L"Input->VertexPass", L"VertexPass->FragmentPass", L"FragmentPass->Output"};
                if (allowed.find(pair) == allowed.end())
                {
                        throw AnalyzerException(L"Invalid pipeline flow: " + pair, n->location, _sourceManager);
                }

                if (n->declaration.size() >= 2)
                {
                        const Token &typeTok = n->declaration[0];
                        const Token &nameTok = n->declaration[1];

                        if (_types.find(typeTok.lexeme) == _types.end())
                        {
                                throw AnalyzerException(L"Unknown type " + typeTok.lexeme, typeTok.location, _sourceManager);
                        }

                        auto &scope = _scopes.back();
                        if (!scope.insert({nameTok.lexeme, Symbol{typeTok.lexeme}}).second)
                        {
                                throw AnalyzerException(L"Redefinition of variable " + nameTok.lexeme, nameTok.location, _sourceManager);
                        }
                }
        }

	void Analyzer::_analyzePipelineBody(const ASTNode *p_node)
	{
		const PipelineBodyNode *n = static_cast<const PipelineBodyNode *>(p_node);
		if (!isValidStage(n->stage.lexeme))
		{
			throw AnalyzerException(L"Invalid pipeline stage body", n->location, _sourceManager);
		}

		if (!_pipelineStages.insert(n->stage.lexeme).second)
		{
			throw AnalyzerException(L"Duplicate pipeline body for stage " + n->stage.lexeme, n->location, _sourceManager);
		}

		if (n->body)
		{
			_analyze(n->body.get());
		}
	}

	void Analyzer::_analyzeNamespace(const ASTNode *p_node)
	{
		const NamespaceNode *n = static_cast<const NamespaceNode *>(p_node);
		if (n->body)
		{
			_pushContainer(n->name.lexeme);
			_pushScope();
			_analyze(n->body.get());
			_popScope();
			_popContainer();
		}
	}

	void Analyzer::_analyzeStructureLike(const ASTNode *p_node)
	{
		if (p_node->kind == ASTNode::Kind::Structure)
		{
			const auto *n = static_cast<const StructureNode *>(p_node);
			_types.insert(n->name.lexeme);
			_pushContainer(n->name.lexeme);
			_pushScope();
			for (const auto &c : n->variables)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->constructors)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->methods)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->operators)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			_popScope();
			_popContainer();
		}
		else if (p_node->kind == ASTNode::Kind::AttributeBlock)
		{
			const auto *n = static_cast<const AttributeBlockNode *>(p_node);
			_pushContainer(n->name.lexeme);
			_pushScope();
			for (const auto &c : n->variables)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->methods)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->operators)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			_popScope();
			_popContainer();
		}
		else if (p_node->kind == ASTNode::Kind::ConstantBlock)
		{
			const auto *n = static_cast<const ConstantBlockNode *>(p_node);
			_pushContainer(n->name.lexeme);
			_pushScope();
			for (const auto &c : n->variables)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->methods)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			for (const auto &c : n->operators)
			{
				if (c)
				{
					_analyze(c.get());
				}
			}
			_popScope();
			_popContainer();
		}
	}

	void Analyzer::_analyzeTexture(const ASTNode *p_node)
	{
		const TextureNode *n = static_cast<const TextureNode *>(p_node);
		if (!_textures.insert(n->name.lexeme).second)
		{
			throw AnalyzerException(L"Redefinition of texture " + n->name.lexeme, n->location, _sourceManager);
		}
	}

	void Analyzer::_analyzeFunction(const ASTNode *p_node)
	{
		const FunctionNode *fn = static_cast<const FunctionNode *>(p_node);
		std::wstring name;
		if (p_node->kind == ASTNode::Kind::Constructor)
		{
			if (!_containerStack.empty())
			{
				name = _containerStack.back();
			}
		}
		else if (p_node->kind == ASTNode::Kind::Operator)
		{
			if (fn->header.size() >= 3)
			{
				name = L"operator" + fn->header[2].lexeme;
			}
			else
			{
				name = L"operator";
			}
		}
		else
		{
			for (size_t i = 0; i + 1 < fn->header.size(); ++i)
			{
				if (fn->header[i + 1].type == Token::Type::OpenParenthesis)
				{
					name = fn->header[i].lexeme;
					break;
				}
			}
		}

		std::wstring sig = _makeSignature(fn->header);
		std::wstring ctxKey = _currentContext() + L"::" + name;
		if (!_functionSignatures[ctxKey].insert(sig).second)
		{
			throw AnalyzerException(L"Redefinition of function " + name, p_node->location, _sourceManager);
		}
		if (fn->body)
		{
			_pushScope();
			_analyze(fn->body.get());
			_popScope();
		}
	}

	void Analyzer::_analyzeVariableDeclaration(const ASTNode *p_node)
	{
		const auto *n = static_cast<const VariableDeclarationNode *>(p_node);
		if (_types.find(n->typeName.lexeme) == _types.end())
		{
			throw AnalyzerException(L"Unknown type " + n->typeName.lexeme, n->location, _sourceManager);
		}
		auto &cur = _scopes.back();
		if (cur.find(n->name.lexeme) != cur.end())
		{
			throw AnalyzerException(L"Redefinition of variable " + n->name.lexeme, n->name.location, _sourceManager);
		}
		std::wstring initType = L"void";
		if (n->initializer)
		{
			initType = _evaluate(n->initializer.get());
		}
		if (n->initializer && initType != n->typeName.lexeme)
		{
			throw AnalyzerException(L"Type mismatch in initialization of " + n->name.lexeme, n->location, _sourceManager);
		}
		cur[n->name.lexeme] = Symbol{n->typeName.lexeme};
	}

	void Analyzer::_analyzeAssignment(const ASTNode *p_node)
	{
		const auto *n = static_cast<const AssignmentNode *>(p_node);
		if (!n->target || !n->value)
		{
			return;
		}

		std::wstring lhs = _evaluate(n->target.get());
		std::wstring rhs = _evaluate(n->value.get());
		if (lhs != rhs)
		{
			throw AnalyzerException(L"Type mismatch in assignment", n->op.location, _sourceManager);
		}
	}

	void Analyzer::_analyzeIfStatement(const ASTNode *p_node)
	{
		const auto *n = static_cast<const IfStatementNode *>(p_node);
		if (n->condition)
		{
			std::wstring t = _evaluate(n->condition.get());
			if (t != L"bool")
			{
				throw AnalyzerException(L"If condition must be boolean", n->condition->location, _sourceManager);
			}
		}
		if (n->thenBody)
		{
			_analyze(n->thenBody.get());
		}
		if (n->elseBody)
		{
			_analyze(n->elseBody.get());
		}
	}

	void Analyzer::_analyzeForLoop(const ASTNode *p_node)
	{
		const auto *n = static_cast<const ForLoopNode *>(p_node);
		if (n->init)
		{
			_analyze(n->init.get());
		}
		if (n->condition)
		{
			if (_evaluate(n->condition.get()) != L"bool")
			{
				throw AnalyzerException(L"For condition must be boolean", n->condition->location, _sourceManager);
			}
		}
		if (n->increment)
		{
			_analyze(n->increment.get());
		}
		if (n->body)
		{
			_analyze(n->body.get());
		}
	}

	void Analyzer::_analyzeWhileLoop(const ASTNode *p_node)
	{
		const auto *n = static_cast<const WhileLoopNode *>(p_node);
		if (n->condition)
		{
			if (_evaluate(n->condition.get()) != L"bool")
			{
				throw AnalyzerException(L"While condition must be boolean", n->condition->location, _sourceManager);
			}
		}
		if (n->body)
		{
			_analyze(n->body.get());
		}
	}

	void Analyzer::_analyzeReturn(const ASTNode *p_node)
	{
		const auto *n = static_cast<const ReturnStatementNode *>(p_node);
		if (n->value)
		{
			(void)_evaluate(n->value.get());
		}
	}

	void Analyzer::_analyzeDiscardStatement(const ASTNode *)
	{
	}

	void Analyzer::_analyzeBinaryExpression(const ASTNode *p_node)
	{
		const auto *n = static_cast<const BinaryExpressionNode *>(p_node);
		std::wstring l = _evaluate(n->left.get());
		std::wstring r = _evaluate(n->right.get());
		if (l != r)
		{
			throw AnalyzerException(L"Type mismatch in binary expression", n->op.location, _sourceManager);
		}
	}

	void Analyzer::_analyzeUnaryExpression(const ASTNode *p_node)
	{
		const auto *n = static_cast<const UnaryExpressionNode *>(p_node);
		(void)_evaluate(n->operand.get());
	}

	void Analyzer::_analyzeCallExpression(const ASTNode *p_node)
	{
		const auto *n = static_cast<const CallExpressionNode *>(p_node);
		if (n->callee)
		{
			(void)_evaluate(n->callee.get());
		}
		for (const auto &a : n->arguments)
		{
			(void)_evaluate(a.get());
		}
	}

	void Analyzer::_analyzeMemberAccess(const ASTNode *p_node)
	{
		const auto *n = static_cast<const MemberAccessNode *>(p_node);
		if (n->object)
		{
			(void)_evaluate(n->object.get());
		}
	}

	void Analyzer::_analyzeVariableReference(const ASTNode *)
	{
	}

	void Analyzer::_analyzeLiteral(const ASTNode *)
	{
	}

	void Analyzer::_analyzeTernaryExpression(const ASTNode *p_node)
	{
		const auto *n = static_cast<const TernaryExpressionNode *>(p_node);
		std::wstring cond = _evaluate(n->condition.get());
		if (cond != L"bool")
		{
			throw AnalyzerException(L"Ternary condition must be boolean", n->condition->location, _sourceManager);
		}
		std::wstring thenType = _evaluate(n->thenExpr.get());
		std::wstring elseType = _evaluate(n->elseExpr.get());
		if (thenType != elseType)
		{
			throw AnalyzerException(L"Type mismatch in ternary expression", n->location, _sourceManager);
		}
	}

	void Analyzer::_analyzeLValue(const ASTNode *p_node)
	{
		const auto *n = static_cast<const LValueNode *>(p_node);
		(void)_evaluate(n->expression.get());
	}

	void Analyzer::_analyzeRValue(const ASTNode *p_node)
	{
		const auto *n = static_cast<const RValueNode *>(p_node);
		(void)_evaluate(n->expression.get());
	}

	void Analyzer::_analyzeToken(const ASTNode *)
	{
	}

	void Analyzer::_pushScope()
	{
		_scopes.emplace_back();
	}

	void Analyzer::_popScope()
	{
		if (!_scopes.empty())
		{
			_scopes.pop_back();
		}
	}

	void Analyzer::_pushContainer(const std::wstring &p_name)
	{
		_containerStack.push_back(p_name);
	}

	void Analyzer::_popContainer()
	{
		if (!_containerStack.empty())
		{
			_containerStack.pop_back();
		}
	}

	std::wstring Analyzer::_currentContext() const
	{
		if (_containerStack.empty())
		{
			return L"<global>";
		}
		std::wstring ctx = _containerStack.front();
		for (std::size_t i = 1; i < _containerStack.size(); ++i)
		{
			ctx += L"::" + _containerStack[i];
		}
		return ctx;
	}

	std::wstring Analyzer::_makeSignature(const std::vector<Token> &p_header) const
	{
		std::wstring sig;
		for (const auto &t : p_header)
		{
			if (t.type == Token::Type::Whitespace || t.type == Token::Type::Comment)
			{
				continue;
			}
			sig += t.lexeme;
		}
		return sig;
	}

        void Analyzer::_loadBuiltinTypes()
        {
		std::filesystem::path path = std::filesystem::path("doc") / "lumina_build_in.md";
		std::ifstream file(path);
		if (!file.is_open())
		{
			return;
		}
		std::string line;
		std::regex header("^###\\s+([A-Za-z0-9_]+)");
                while (std::getline(file, line))
                {
                        std::smatch m;
                        if (std::regex_search(line, m, header))
                        {
                                _types.insert(spk::StringUtils::stringToWString(m[1].str()));
                        }
                }
        }

        void Analyzer::_loadBuiltinVariables()
        {
                if (_scopes.empty())
                {
                        _pushScope();
                }

                auto &global = _scopes.back();
                global[L"pixelColor"] = Symbol{L"Color"};
                global[L"pixelPosition"] = Symbol{L"Vector4"};
        }

	std::wstring Analyzer::_evaluate(const ASTNode *p_node)
	{
		if (!p_node)
		{
			return L"void";
		}

		switch (p_node->kind)
		{
		case ASTNode::Kind::Literal:
		{
			const LiteralNode *lit = static_cast<const LiteralNode *>(p_node);
			switch (lit->value.type)
			{
			case Token::Type::NumberLiteral:
				return L"float";
			case Token::Type::BoolLiteral:
				return L"bool";
			case Token::Type::StringLiteral:
				return L"string";
			default:
				return L"void";
			}
		}
		case ASTNode::Kind::VariableReference:
		{
			const VariableReferenceNode *ref = static_cast<const VariableReferenceNode *>(p_node);
			for (auto it = _scopes.rbegin(); it != _scopes.rend(); ++it)
			{
				auto iter = it->find(ref->name.lexeme);
				if (iter != it->end())
				{
					return iter->second.type;
				}
			}
			throw AnalyzerException(L"Undefined variable " + ref->name.lexeme, ref->location, _sourceManager);
		}
		case ASTNode::Kind::BinaryExpression:
		{
			const BinaryExpressionNode *bin = static_cast<const BinaryExpressionNode *>(p_node);
			std::wstring l = _evaluate(bin->left.get());
			std::wstring r = _evaluate(bin->right.get());
			if (l != r)
			{
				throw AnalyzerException(L"Type mismatch in binary expression", bin->op.location, _sourceManager);
			}
			return l;
		}
		case ASTNode::Kind::UnaryExpression:
		{
			const UnaryExpressionNode *un = static_cast<const UnaryExpressionNode *>(p_node);
			return _evaluate(un->operand.get());
		}
		case ASTNode::Kind::LValue:
		{
			const LValueNode *lv = static_cast<const LValueNode *>(p_node);
			return _evaluate(lv->expression.get());
		}
		case ASTNode::Kind::RValue:
		{
			const RValueNode *rv = static_cast<const RValueNode *>(p_node);
			return _evaluate(rv->expression.get());
		}
		default:
			return L"void";
		}
	}
}
