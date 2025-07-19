#include "structure/graphics/lumina/compiler/spk_analyzer.hpp"

#include "spk_debug_macro.hpp"
#include "structure/graphics/lumina/compiler/spk_lexer.hpp"
#include "utils/spk_string_utils.hpp"

#include <algorithm>
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
		_namespaceNames.clear();
		_shaderData.globalNamespace.name = L"";
		_namespaceStack.push_back(&_shaderData.globalNamespace);
		_loadBuiltinFunctions();
	}

	void Analyzer::run(const std::vector<std::unique_ptr<ASTNode>> &p_nodes)
	{
		_pipelineStages.clear();
		_containerStack.clear();
		_scopes.clear();
		_includedFiles.clear();
		_includeStack.clear();
		_namespaceNames.clear();
		_namespaceStack.clear();
		_shaderData = ShaderData{};
		_shaderData.globalNamespace.name = L"";
		_loadBuiltinTypes();
		_namespaceStack.push_back(&_shaderData.globalNamespace);
		_pushScope();
		_loadBuiltinVariables();
		_loadBuiltinFunctions();
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
			throw AnalyzerException(L"Unable to resolve include path: " + path + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}

		std::wstring fullStr = full.wstring();
		if (std::find(_includeStack.begin(), _includeStack.end(), fullStr) != _includeStack.end())
		{
			throw AnalyzerException(L"Circular include detected: " + fullStr + L" - " + DEBUG_INFO(), n->location, _sourceManager);
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
			throw AnalyzerException(L"Invalid pipeline stage in definition - " + DEBUG_INFO(), n->location, _sourceManager);
		}

		const std::wstring pair = n->fromStage.lexeme + L"->" + n->toStage.lexeme;
		static const std::unordered_set<std::wstring> allowed = {L"Input->VertexPass", L"VertexPass->FragmentPass", L"FragmentPass->Output"};
		if (allowed.find(pair) == allowed.end())
		{
			throw AnalyzerException(L"Invalid pipeline flow: " + pair + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}

		if (n->declaration.size() >= 2)
		{
			const Token &typeTok = n->declaration[0];
			const Token &nameTok = n->declaration[1];

			TypeSymbol *t = _findType(typeTok.lexeme);
			if (!t)
			{
				throw AnalyzerException(L"Unknown type " + typeTok.lexeme + L" - " + DEBUG_INFO(), typeTok.location, _sourceManager);
			}

			auto &scope = _scopes.back();
			Variable var;
			var.name = nameTok.lexeme;
			var.type = t;
			if (!scope.insert({nameTok.lexeme, var}).second)
			{
				throw AnalyzerException(L"Redefinition of variable " + nameTok.lexeme + L" - " + DEBUG_INFO(), nameTok.location, _sourceManager);
			}
		}
	}

	void Analyzer::_analyzePipelineBody(const ASTNode *p_node)
	{
		const PipelineBodyNode *n = static_cast<const PipelineBodyNode *>(p_node);
		if (!isValidStage(n->stage.lexeme))
		{
			throw AnalyzerException(L"Invalid pipeline stage body - " + DEBUG_INFO(), n->location, _sourceManager);
		}

		if (!_pipelineStages.insert(n->stage.lexeme).second)
		{
			throw AnalyzerException(L"Duplicate pipeline body for stage " + n->stage.lexeme + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}

		if (n->body)
		{
			_analyze(n->body.get());
			PipelineStage stage;
			stage.stage = n->stage.lexeme;
			for (const auto &child : n->body->children)
			{
				if (child)
				{
					stage.body.push_back(_convertAST(child.get()));
				}
			}
			_shaderData.pipelineStages.push_back(stage);
		}
	}

	void Analyzer::_analyzeNamespace(const ASTNode *p_node)
	{
		const NamespaceNode *n = static_cast<const NamespaceNode *>(p_node);
		if (n->body)
		{
			_namespaceNames.insert(n->name.lexeme);
			_pushContainer(n->name.lexeme);
			NamespaceSymbol child;
			child.name = n->name.lexeme;
			_namespaceStack.back()->namespaces.push_back(child);
			NamespaceSymbol *ns = &_namespaceStack.back()->namespaces.back();
			_namespaceStack.push_back(ns);
			_pushScope();
			_analyze(n->body.get());
			_popScope();
			_namespaceStack.pop_back();
			_popContainer();
		}
	}

	void Analyzer::_analyzeStructureLike(const ASTNode *p_node)
	{
		if (p_node->kind == ASTNode::Kind::Structure)
		{
			const auto *n = static_cast<const StructureNode *>(p_node);
			auto [it, inserted] =
				_namespaceStack.back()->types.emplace(n->name.lexeme, TypeSymbol{n->name.lexeme, TypeSymbol::Role::Structure, {}, {}, {}, {}});
			_namespaceStack.back()->structures.push_back(&it->second);
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
			_namespaceStack.back()->types.emplace(n->name.lexeme, TypeSymbol{n->name.lexeme, TypeSymbol::Role::Attribute, {}, {}, {}, {}});
			_namespaceStack.back()->attributeBlocks.push_back(n->name.lexeme);
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
			_namespaceStack.back()->types.emplace(n->name.lexeme, TypeSymbol{n->name.lexeme, TypeSymbol::Role::Constant, {}, {}, {}, {}});
			_namespaceStack.back()->constantBlocks.push_back(n->name.lexeme);
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
		auto &texList = _namespaceStack.back()->textures;
		auto dup = std::find_if(texList.begin(), texList.end(), [&](const Variable &p_variable) { return p_variable.name == n->name.lexeme; });
		if (dup != texList.end())
		{
			throw AnalyzerException(L"Redefinition of texture " + n->name.lexeme + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}
		if (_scopes.empty())
		{
			_pushScope();
		}
		Variable var;
		var.name = n->name.lexeme;
		var.type = _findType(L"Texture");
		_scopes.front()[n->name.lexeme] = var;
		texList.push_back(var);
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
		auto &vec = _namespaceStack.back()->functionSignatures[name];
		for (const auto &f : vec)
		{
			if (f.signature == sig)
			{
				throw AnalyzerException(L"Redefinition of function " + name + L" - " + DEBUG_INFO(), p_node->location, _sourceManager);
			}
		}

		const TypeSymbol *returnType = nullptr;
		if (p_node->kind == ASTNode::Kind::Constructor)
		{
			returnType = _findType(name);
		}
		else if (!fn->header.empty())
		{
			returnType = _findType(fn->header.front().lexeme);
		}
		FunctionSymbol sym;
		sym.name = name;
		sym.returnType = returnType;
		sym.parameters = _parseParameters(fn->header);
		sym.signature = sig;
		if (fn->body)
		{
			_pushScope();
			_analyze(fn->body.get());
			_popScope();
			for (const auto &child : fn->body->children)
			{
				if (child)
				{
					sym.body.push_back(_convertAST(child.get()));
				}
			}
		}
		vec.push_back(sym);
		_namespaceStack.back()->functions.push_back(sym);
		if (!_containerStack.empty())
		{
			TypeSymbol *tSym = _findType(_containerStack.back());
			if (tSym)
			{
				if (p_node->kind == ASTNode::Kind::Constructor)
				{
					tSym->constructors.push_back(sym);
				}
				else if (p_node->kind == ASTNode::Kind::Operator)
				{
					tSym->operators[name].push_back(sym);
				}
			}
		}
	}

	void Analyzer::_analyzeVariableDeclaration(const ASTNode *p_node)
	{
		const auto *n = static_cast<const VariableDeclarationNode *>(p_node);
		TypeSymbol *declType = _findType(n->typeName.lexeme);
		if (!declType)
		{
			throw AnalyzerException(L"Unknown type " + n->typeName.lexeme + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}
		auto &cur = _scopes.back();
		if (cur.find(n->name.lexeme) != cur.end())
		{
			throw AnalyzerException(L"Redefinition of variable " + n->name.lexeme + L" - " + DEBUG_INFO(), n->name.location, _sourceManager);
		}
		std::wstring initType = L"void";
		if (n->initializer)
		{
			initType = _evaluate(n->initializer.get());
		}
		if (n->initializer && initType != n->typeName.lexeme)
		{
			std::wstring msg = L"Type mismatch in initialization of " + n->name.lexeme + L"; expected " + n->typeName.lexeme + L" but got " +
							   initType + _conversionInfo(initType);
			throw AnalyzerException(msg + L" - " + DEBUG_INFO(), n->location, _sourceManager);
		}
		Variable var;
		var.name = n->name.lexeme;
		var.type = declType;
		cur[n->name.lexeme] = var;

		if (!_containerStack.empty())
		{
			TypeSymbol *tit = _findType(_containerStack.back());
			if (tit)
			{
				auto &members = tit->members;
				auto dup =
					std::find_if(members.begin(), members.end(), [&](const Variable &p_variable) { return p_variable.name == n->name.lexeme; });
				if (dup == members.end())
				{
					members.push_back(var);
				}
				else
				{
					*dup = var;
				}
			}
		}
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
			std::wstring msg = L"Type mismatch in assignment: expected " + lhs + L" but got " + rhs + _conversionInfo(rhs);
			throw AnalyzerException(msg + L" - " + DEBUG_INFO(), n->op.location, _sourceManager);
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
				throw AnalyzerException(L"If condition must be boolean - " + DEBUG_INFO(), n->condition->location, _sourceManager);
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
				throw AnalyzerException(L"For condition must be boolean - " + DEBUG_INFO(), n->condition->location, _sourceManager);
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
				throw AnalyzerException(L"While condition must be boolean - " + DEBUG_INFO(), n->condition->location, _sourceManager);
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
			std::wstring msg = L"Type mismatch in binary expression: left operand is " + l + L", right operand is " + r + _conversionInfo(r);
			throw AnalyzerException(msg + L" - " + DEBUG_INFO(), n->op.location, _sourceManager);
		}
	}

	void Analyzer::_analyzeUnaryExpression(const ASTNode *p_node)
	{
		const auto *n = static_cast<const UnaryExpressionNode *>(p_node);
		(void)_evaluate(n->operand.get());
	}

	void Analyzer::_analyzeCallExpression(const ASTNode *p_node)
	{
		(void)_evaluate(p_node);
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
			throw AnalyzerException(L"Ternary condition must be boolean - " + DEBUG_INFO(), n->condition->location, _sourceManager);
		}
		std::wstring thenType = _evaluate(n->thenExpr.get());
		std::wstring elseType = _evaluate(n->elseExpr.get());
		if (thenType != elseType)
		{
			std::wstring msg =
				L"Type mismatch in ternary expression: then branch is " + thenType + L", else branch is " + elseType + _conversionInfo(elseType);
			throw AnalyzerException(msg + L" - " + DEBUG_INFO(), n->location, _sourceManager);
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

	Analyzer::Statement Analyzer::_convertAST(const ASTNode *p_node) const
	{
		Statement stmt;
		if (!p_node)
		{
			return stmt;
		}
		stmt.kind = p_node->kind;
		std::wostringstream oss;
		p_node->print(oss);
		stmt.text = oss.str();
		if (!stmt.text.empty() && stmt.text.back() == L'\n')
		{
			stmt.text.pop_back();
		}
		if (p_node->kind == ASTNode::Kind::Body)
		{
			const BodyNode *b = static_cast<const BodyNode *>(p_node);
			for (const auto &c : b->children)
			{
				if (c)
				{
					stmt.children.push_back(_convertAST(c.get()));
				}
			}
		}
		else if (p_node->kind == ASTNode::Kind::IfStatement)
		{
			const auto *n = static_cast<const IfStatementNode *>(p_node);
			if (n->thenBody)
			{
				stmt.children.push_back(_convertAST(n->thenBody.get()));
			}
			if (n->elseBody)
			{
				stmt.children.push_back(_convertAST(n->elseBody.get()));
			}
		}
		else if (p_node->kind == ASTNode::Kind::ForLoop)
		{
			const auto *n = static_cast<const ForLoopNode *>(p_node);
			if (n->body)
			{
				stmt.children.push_back(_convertAST(n->body.get()));
			}
		}
		else if (p_node->kind == ASTNode::Kind::WhileLoop)
		{
			const auto *n = static_cast<const WhileLoopNode *>(p_node);
			if (n->body)
			{
				stmt.children.push_back(_convertAST(n->body.get()));
			}
		}
		return stmt;
	}

	std::vector<Analyzer::Variable> Analyzer::_parseParameters(const std::vector<Token> &p_header) const
	{
		std::vector<Variable> params;
		bool inParams = false;
		for (size_t i = 0; i < p_header.size(); ++i)
		{
			const auto &tok = p_header[i];
			if (tok.type == Token::Type::OpenParenthesis)
			{
				inParams = true;
				continue;
			}
			if (!inParams)
			{
				continue;
			}
			if (tok.type == Token::Type::CloseParenthesis)
			{
				break;
			}
			if (tok.type == Token::Type::Identifier)
			{
				std::wstring typeName = tok.lexeme;
				std::wstring varName;
				if (i + 1 < p_header.size() && p_header[i + 1].type == Token::Type::Identifier)
				{
					varName = p_header[i + 1].lexeme;
					++i;
				}
				Variable var;
				var.name = varName;
				var.type = _findType(typeName);
				params.push_back(var);
			}
		}
		return params;
	}

	std::vector<Analyzer::FunctionSymbol> Analyzer::_findFunctions(const std::wstring &p_name) const
	{
		// search current namespace stack
		for (auto it = _namespaceStack.rbegin(); it != _namespaceStack.rend(); ++it)
		{
			auto fit = (*it)->functionSignatures.find(p_name);
			if (fit != (*it)->functionSignatures.end())
			{
				return fit->second;
			}
		}

		// search top level namespaces by name prefix
		for (const auto &nsName : _namespaceNames)
		{
			const NamespaceSymbol *ns = _findNamespace(nsName);
			if (!ns)
			{
				continue;
			}
			auto it2 = ns->functionSignatures.find(p_name);
			if (it2 != ns->functionSignatures.end())
			{
				return it2->second;
			}
		}

		// anonymous/global namespace
		auto globalIt = _shaderData.globalNamespace.functionSignatures.find(p_name);
		if (globalIt != _shaderData.globalNamespace.functionSignatures.end())
		{
			return globalIt->second;
		}

		return {};
	}

	void Analyzer::_loadBuiltinTypes()
	{
		std::vector<std::filesystem::path> docs = {std::filesystem::path("doc") / "lumina_build_in.md",
												   std::filesystem::path("doc") / "lumina_cheat_sheet.md"};

		std::regex header("^###\\s+([A-Za-z0-9_]+)");
		std::regex conv("-\\s+Implicitly\\s+convertible\\s+p_from\\s+`([A-Za-z0-9_]+)`");

		for (const auto &path : docs)
		{
			std::ifstream file(path);
			if (!file.is_open())
			{
				continue;
			}

			std::string line;
			std::string current;
			bool readingConv = false;
			while (std::getline(file, line))
			{
				std::smatch m;
				if (std::regex_search(line, m, header))
				{
					current = m[1].str();
					std::wstring wcur = spk::StringUtils::stringToWString(current);
					_shaderData.globalNamespace.types.emplace(wcur, TypeSymbol{wcur, TypeSymbol::Role::Structure, {}, {}, {}, {}});
					readingConv = false;
					continue;
				}

				if (line.rfind("#### Implicit Conversions:", 0) == 0)
				{
					readingConv = true;
					continue;
				}

				if (line.rfind("####", 0) == 0 && readingConv)
				{
					readingConv = false;
					continue;
				}

				if (readingConv && std::regex_search(line, m, conv))
				{
					std::wstring from = spk::StringUtils::stringToWString(m[1].str());
					std::wstring to = spk::StringUtils::stringToWString(current);
					auto fIt = _shaderData.globalNamespace.types.emplace(from, TypeSymbol{from, TypeSymbol::Role::Structure, {}, {}, {}, {}}).first;
					auto tIt = _shaderData.globalNamespace.types.emplace(to, TypeSymbol{to, TypeSymbol::Role::Structure, {}, {}, {}, {}}).first;
					fIt->second.convertible.insert(&tIt->second);
				}
			}
		}

		_shaderData.globalNamespace.types.emplace(L"void", TypeSymbol{L"void", TypeSymbol::Role::Structure, {}, {}, {}, {}});
	}

	void Analyzer::_loadBuiltinVariables()
	{
		if (_scopes.empty())
		{
			_pushScope();
		}

		auto &global = _scopes.back();
		global[L"pixelColor"] = Variable{L"pixelColor", &_shaderData.globalNamespace.types[L"Color"]};
		global[L"pixelPosition"] = Variable{L"pixelPosition", &_shaderData.globalNamespace.types[L"Vector4"]};
	}

	void Analyzer::_loadBuiltinFunctions()
	{
		FunctionSymbol vert;
		vert.name = L"VertexPass";
		vert.returnType = &_shaderData.globalNamespace.types[L"void"];
		vert.signature = L"VertexPass()";
		_shaderData.globalNamespace.functionSignatures[L"VertexPass"].push_back(vert);
		_shaderData.globalNamespace.functions.push_back(vert);

		FunctionSymbol frag;
		frag.name = L"FragmentPass";
		frag.returnType = &_shaderData.globalNamespace.types[L"void"];
		frag.signature = L"FragmentPass()";
		_shaderData.globalNamespace.functionSignatures[L"FragmentPass"].push_back(frag);
		_shaderData.globalNamespace.functions.push_back(frag);
	}

	Analyzer::TypeSymbol *Analyzer::_findType(const std::wstring &p_name) const
	{
		for (auto it = _namespaceStack.rbegin(); it != _namespaceStack.rend(); ++it)
		{
			auto tit = (*it)->types.find(p_name);
			if (tit != (*it)->types.end())
			{
				return const_cast<TypeSymbol *>(&tit->second);
			}
		}
		return nullptr;
	}

	const Analyzer::NamespaceSymbol *Analyzer::_findNamespace(const std::wstring &p_name) const
	{
		for (const auto &ns : _shaderData.globalNamespace.namespaces)
		{
			if (ns.name == p_name)
			{
				return &ns;
			}
		}
		return nullptr;
	}

	std::wstring Analyzer::_extractCalleeName(const ASTNode *p_node) const
	{
		if (!p_node)
		{
			return {};
		}
		switch (p_node->kind)
		{
		case ASTNode::Kind::VariableReference:
		{
			const auto *n = static_cast<const VariableReferenceNode *>(p_node);
			return n->name.lexeme;
		}
		case ASTNode::Kind::MemberAccess:
		{
			const auto *m = static_cast<const MemberAccessNode *>(p_node);
			std::wstring lhs = _extractCalleeName(m->object.get());
			if (lhs.empty())
			{
				return {};
			}
			return lhs + L"::" + m->member.lexeme;
		}
		default:
			return {};
		}
	}

	std::wstring Analyzer::_conversionInfo(const std::wstring &p_from) const
	{
		TypeSymbol *symbol = _findType(p_from);
		if (!symbol || symbol->convertible.empty())
		{
			return L"";
		}
		std::wstring msg = L" (" + p_from + L" is convertible to: ";
		bool first = true;
		for (const auto *t : symbol->convertible)
		{
			if (!first)
			{
				msg += L", ";
			}
			if (t)
			{
				msg += t->name;
			}
			first = false;
		}
		msg += L")";
		return msg;
	}

	bool Analyzer::_canConvert(const std::wstring &p_from, const std::wstring &p_to) const
	{
		if (p_from == p_to)
		{
			return true;
		}
		TypeSymbol *f = _findType(p_from);
		TypeSymbol *t = _findType(p_to);
		if (!f || !t)
		{
			return false;
		}
		return f->convertible.find(t) != f->convertible.end();
	}

	std::wstring Analyzer::_resolveCall(const ASTNode *p_callee, const std::wstring &p_name, const std::vector<std::wstring> &p_argTypes, const Location &p_loc)
	{
		std::wstring target = p_name;
		std::vector<FunctionSymbol> funcs;
		bool ctor = false;
		TypeSymbol *typeCtx = nullptr;

		const ASTNode *objectNode = nullptr;
		if (p_callee && p_callee->kind == ASTNode::Kind::MemberAccess)
		{
			const auto *m = static_cast<const MemberAccessNode *>(p_callee);
			objectNode = m->object.get();
		}

		size_t pos = p_name.rfind(L"::");
		if (pos != std::wstring::npos)
		{
			std::wstring prefix = p_name.substr(0, pos);
			target = p_name.substr(pos + 2);

			const NamespaceSymbol *ns = _findNamespace(prefix);
			if (ns)
			{
				auto it = ns->functionSignatures.find(target);
				if (it != ns->functionSignatures.end())
				{
					funcs = it->second;
				}
			}

			if (funcs.empty())
			{
				typeCtx = _findType(prefix);
				if (typeCtx)
				{
					if (target == prefix)
					{
						funcs = typeCtx->constructors;
						ctor = true;
					}
					auto opIt = typeCtx->operators.find(target);
					if (opIt != typeCtx->operators.end())
					{
						funcs.insert(funcs.end(), opIt->second.begin(), opIt->second.end());
					}
				}
			}

			if (funcs.empty() && objectNode)
			{
				std::wstring objType = _evaluate(objectNode);
				typeCtx = _findType(objType);
				if (typeCtx)
				{
					auto opIt = typeCtx->operators.find(target);
					if (opIt != typeCtx->operators.end())
					{
						funcs = opIt->second;
					}
				}
			}
		}
		else
		{
			funcs = _findFunctions(target);
			if (funcs.empty())
			{
				typeCtx = _findType(target);
				if (typeCtx)
				{
					funcs = typeCtx->constructors;
					ctor = true;
				}
			}
		}

		if (funcs.empty())
		{
			throw AnalyzerException(L"Unknown function " + p_name + L" - " + DEBUG_INFO(), p_loc, _sourceManager);
		}

		for (const auto &f : funcs)
		{
			if (f.parameters.size() != p_argTypes.size())
			{
				continue;
			}
			bool match = true;
			for (size_t i = 0; i < p_argTypes.size(); ++i)
			{
				std::wstring paramType = f.parameters[i].type ? f.parameters[i].type->name : L"void";
				if (!_canConvert(p_argTypes[i], paramType))
				{
					match = false;
					break;
				}
			}
			if (match)
			{
				return ctor ? (typeCtx ? typeCtx->name : L"void") : (f.returnType ? f.returnType->name : L"void");
			}
		}

		if (ctor)
		{
			throw AnalyzerException(L"No matching constructor for " + p_name + L" - " + DEBUG_INFO(), p_loc, _sourceManager);
		}

		throw AnalyzerException(L"No matching overload for function " + p_name + L" - " + DEBUG_INFO(), p_loc, _sourceManager);
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
					return iter->second.type ? iter->second.type->name : L"void";
				}
			}
			throw AnalyzerException(L"Undefined variable " + ref->name.lexeme + L" - " + DEBUG_INFO(), ref->location, _sourceManager);
		}
		case ASTNode::Kind::MemberAccess:
		{
			const MemberAccessNode *mem = static_cast<const MemberAccessNode *>(p_node);
			std::wstring baseType = _evaluate(mem->object.get());
			TypeSymbol *ts = _findType(baseType);
			if (ts)
			{
				auto it = std::find_if(ts->members.begin(), ts->members.end(), [&](const Variable &p_variable) { return p_variable.name == mem->member.lexeme; });
				if (it != ts->members.end())
				{
					return it->type ? it->type->name : L"void";
				}
			}
			return L"void";
		}
		case ASTNode::Kind::BinaryExpression:
		{
			const BinaryExpressionNode *bin = static_cast<const BinaryExpressionNode *>(p_node);
			std::wstring l = _evaluate(bin->left.get());
			std::wstring r = _evaluate(bin->right.get());
			if (l != r)
			{
				std::wstring msg = L"Type mismatch in binary expression: left operand is " + l + L", right operand is " + r + _conversionInfo(r);
				throw AnalyzerException(msg + L" - " + DEBUG_INFO(), bin->op.location, _sourceManager);
			}
			return l;
		}
		case ASTNode::Kind::CallExpression:
		{
			const CallExpressionNode *call = static_cast<const CallExpressionNode *>(p_node);
			std::wstring name = _extractCalleeName(call->callee.get());
			if (name.empty())
			{
				throw AnalyzerException(L"Invalid function call - " + DEBUG_INFO(), p_node->location, _sourceManager);
			}
			std::vector<std::wstring> argTypes;
			for (const auto &arg : call->arguments)
			{
				argTypes.push_back(_evaluate(arg.get()));
			}

			return _resolveCall(call->callee.get(), name, argTypes, p_node->location);
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

	const ShaderData &Analyzer::getData() const
	{
		return _shaderData;
	}
}
