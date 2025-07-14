#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "structure/graphics/lumina/compiler/spk_location.hpp"

#include "structure/graphics/lumina/compiler/spk_token.hpp"

namespace spk::Lumina
{
	struct ASTNode
	{
		enum class Kind
		{
			/* directives / misc. */
			Include,
			Namespace,
			PipelineFlow,
			PipelinePass,

			/* blocks / types */
			Struct,
			AttributeBlock,
			ConstantBlock,
			TextureDecl,

			/* declarations */
			Function,
			Variable,

			/* statements */
			Body,
			If,
			For,
			While,
			Return,
			Discard,
			RaiseException,

			/* expressions */
			BinaryExpr,
			UnaryExpr,
			Literal,
			Identifier,
			Call,
			MemberAccess,
		};

		Location location;
		Kind kind;

		explicit ASTNode(Kind k, Location loc) :
			location(std::move(loc)),
			kind(k)
		{
		}
		virtual ~ASTNode() = default;

		virtual void print(std::wostream& p_os, size_t p_indent = 0) const = 0;
	};

	struct StatementASTNode : ASTNode
	{
		explicit StatementASTNode(Kind k, Location loc) :
			ASTNode(k, std::move(loc))
		{
		}
	};
	
	struct BodyASTNode : StatementASTNode
	{
		std::vector<std::unique_ptr<StatementASTNode>> statements;

		BodyASTNode(Location loc) :
			StatementASTNode(Kind::Body, std::move(loc))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct ExpressionASTNode : ASTNode
	{
		explicit ExpressionASTNode(Kind k, Location loc) :
			ASTNode(k, std::move(loc))
		{
		}
	};

	struct VariableDeclASTNode : ASTNode
	{
		std::wstring typeName;
		std::wstring name;
		std::unique_ptr<ExpressionASTNode> initializer;

		VariableDeclASTNode(std::wstring t, std::wstring n, std::unique_ptr<ExpressionASTNode> &&init, Location loc) :
			ASTNode(Kind::Variable, std::move(loc)),
			typeName(std::move(t)),
			name(std::move(n)),
			initializer(std::move(init))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct IncludeASTNode : ASTNode
	{
		std::filesystem::path includeFile;

		IncludeASTNode(const std::wstring &p, Location loc) :
			ASTNode(Kind::Include, std::move(loc)),
			includeFile(p)
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct NamespaceASTNode : ASTNode
	{
		std::wstring name;
		std::vector<std::unique_ptr<ASTNode>> declarations;

		NamespaceASTNode(std::wstring n, std::vector<std::unique_ptr<ASTNode>> decls, Location loc) :
			ASTNode(Kind::Namespace, std::move(loc)),
			name(std::move(n)),
			declarations(std::move(decls))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct PipelineFlowASTNode : ASTNode
	{
		std::wstring inputStage;
		std::wstring outputStage;
		std::wstring typeName;
		std::wstring varName;

		PipelineFlowASTNode(std::wstring in, std::wstring out, std::wstring t, std::wstring n, Location loc) :
			ASTNode(Kind::PipelineFlow, std::move(loc)),
			inputStage(std::move(in)),
			outputStage(std::move(out)),
			typeName(std::move(t)),
			varName(std::move(n))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct PipelinePassASTNode : ASTNode
	{
		std::wstring name;
		std::unique_ptr<ASTNode> body;

		PipelinePassASTNode(std::wstring n, std::unique_ptr<ASTNode> &&b, Location loc) :
			ASTNode(Kind::PipelinePass, std::move(loc)),
			name(std::move(n)),
			body(std::move(b))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct StructASTNode : ASTNode
	{
		std::wstring name;
		std::vector<std::unique_ptr<ASTNode>> members;

		StructASTNode(std::wstring n, std::vector<std::unique_ptr<ASTNode>> m, Location loc) :
			ASTNode(Kind::Struct, std::move(loc)),
			name(std::move(n)),
			members(std::move(m))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct AttributeBlockASTNode : ASTNode
	{
		std::wstring name;
		std::vector<std::unique_ptr<ASTNode>> members;

		AttributeBlockASTNode(std::wstring n, std::vector<std::unique_ptr<ASTNode>> m, Location loc) :
			ASTNode(Kind::AttributeBlock, std::move(loc)),
			name(std::move(n)),
			members(std::move(m))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct ConstantBlockASTNode : ASTNode
	{
		std::wstring name;
		std::vector<std::unique_ptr<ASTNode>> members;

		ConstantBlockASTNode(std::wstring n, std::vector<std::unique_ptr<ASTNode>> m, Location loc) :
			ASTNode(Kind::ConstantBlock, std::move(loc)),
			name(std::move(n)),
			members(std::move(m))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct TextureDeclASTNode : ASTNode
	{
		std::wstring name;

		TextureDeclASTNode(std::wstring n, Location loc) :
			ASTNode(Kind::TextureDecl, std::move(loc)),
			name(std::move(n))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct FunctionASTNode : ASTNode
	{
		std::wstring returnType;
		std::wstring name;
		std::vector<std::unique_ptr<VariableDeclASTNode>> parameters;
		std::unique_ptr<BodyASTNode> body;

		FunctionASTNode(std::wstring ret, std::wstring n, std::vector<std::unique_ptr<VariableDeclASTNode>> params, std::unique_ptr<BodyASTNode> &&b,
						Location loc) :
			ASTNode(Kind::Function, std::move(loc)),
			returnType(std::move(ret)),
			name(std::move(n)),
			parameters(std::move(params)),
			body(std::move(b))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct IfASTNode : StatementASTNode
	{
		std::unique_ptr<ExpressionASTNode> condition;
		std::unique_ptr<BodyASTNode> thenBody;
		std::unique_ptr<BodyASTNode> elseBody;

		IfASTNode(std::unique_ptr<ExpressionASTNode> &&cond, std::unique_ptr<BodyASTNode> &&thenB, std::unique_ptr<BodyASTNode> &&elseB,
				  Location loc) :
			StatementASTNode(Kind::If, std::move(loc)),
			condition(std::move(cond)),
			thenBody(std::move(thenB)),
			elseBody(std::move(elseB))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct ForASTNode : StatementASTNode
	{
		std::unique_ptr<StatementASTNode> init;
		std::unique_ptr<ExpressionASTNode> cond;
		std::unique_ptr<ExpressionASTNode> iter;
		std::unique_ptr<BodyASTNode> body;

		ForASTNode(std::unique_ptr<StatementASTNode> &&i, std::unique_ptr<ExpressionASTNode> &&c, std::unique_ptr<ExpressionASTNode> &&it,
				   std::unique_ptr<BodyASTNode> &&b, Location loc) :
			StatementASTNode(Kind::For, std::move(loc)),
			init(std::move(i)),
			cond(std::move(c)),
			iter(std::move(it)),
			body(std::move(b))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct WhileASTNode : StatementASTNode
	{
		std::unique_ptr<ExpressionASTNode> condition;
		std::unique_ptr<BodyASTNode> body;

		WhileASTNode(std::unique_ptr<ExpressionASTNode> &&cond, std::unique_ptr<BodyASTNode> &&b, Location loc) :
			StatementASTNode(Kind::While, std::move(loc)),
			condition(std::move(cond)),
			body(std::move(b))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct ReturnASTNode : StatementASTNode
	{
		std::unique_ptr<ExpressionASTNode> value; // nullptr for “return;”

		ReturnASTNode(std::unique_ptr<ExpressionASTNode> &&v, Location loc) :
			StatementASTNode(Kind::Return, std::move(loc)),
			value(std::move(v))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct DiscardASTNode : StatementASTNode
	{
		explicit DiscardASTNode(Location loc) :
			StatementASTNode(Kind::Discard, std::move(loc))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct RaiseExceptionASTNode : StatementASTNode
	{
		std::unique_ptr<ExpressionASTNode> formatString;
		std::vector<std::unique_ptr<ExpressionASTNode>> arguments;

		RaiseExceptionASTNode(std::unique_ptr<ExpressionASTNode> &&fmt, std::vector<std::unique_ptr<ExpressionASTNode>> args, Location loc) :
			StatementASTNode(Kind::RaiseException, std::move(loc)),
			formatString(std::move(fmt)),
			arguments(std::move(args))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct IdentifierExprASTNode : ExpressionASTNode
	{
		std::wstring name;

		IdentifierExprASTNode(std::wstring n, Location loc) :
			ExpressionASTNode(Kind::Identifier, std::move(loc)),
			name(std::move(n))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct LiteralExprASTNode : ExpressionASTNode
	{
		std::wstring literal; // raw lexeme

		LiteralExprASTNode(std::wstring lit, Location loc) :
			ExpressionASTNode(Kind::Literal, std::move(loc)),
			literal(std::move(lit))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct UnaryExprASTNode : ExpressionASTNode
	{
		Token::Type op;
		std::unique_ptr<ExpressionASTNode> operand;

		UnaryExprASTNode(Token::Type o, std::unique_ptr<ExpressionASTNode> &&expr, Location loc) :
			ExpressionASTNode(Kind::UnaryExpr, std::move(loc)),
			op(o),
			operand(std::move(expr))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct BinaryExprASTNode : ExpressionASTNode
	{
		std::unique_ptr<ExpressionASTNode> lhs;
		Token::Type op;
		std::unique_ptr<ExpressionASTNode> rhs;

		BinaryExprASTNode(std::unique_ptr<ExpressionASTNode> &&L, Token::Type o, std::unique_ptr<ExpressionASTNode> &&R, Location loc) :
			ExpressionASTNode(Kind::BinaryExpr, std::move(loc)),
			lhs(std::move(L)),
			op(o),
			rhs(std::move(R))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct CallExprASTNode : ExpressionASTNode
	{
		std::unique_ptr<ExpressionASTNode> callee;
		std::vector<std::unique_ptr<ExpressionASTNode>> args;

		CallExprASTNode(std::unique_ptr<ExpressionASTNode> &&c, std::vector<std::unique_ptr<ExpressionASTNode>> a, Location loc) :
			ExpressionASTNode(Kind::Call, std::move(loc)),
			callee(std::move(c)),
			args(std::move(a))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};

	struct MemberAccessExprASTNode : ExpressionASTNode
	{
		std::unique_ptr<ExpressionASTNode> object;
		std::wstring memberName;

		MemberAccessExprASTNode(std::unique_ptr<ExpressionASTNode> &&obj, std::wstring m, Location loc) :
			ExpressionASTNode(Kind::MemberAccess, std::move(loc)),
			object(std::move(obj)),
			memberName(std::move(m))
		{
		}

		void print(std::wostream& p_os, size_t p_indent = 0) const override;
	};
}
