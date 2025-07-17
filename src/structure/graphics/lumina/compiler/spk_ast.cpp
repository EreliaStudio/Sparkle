#include "structure/graphics/lumina/compiler/spk_ast.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	std::wstring to_wstring(ASTNode::Kind p_kind)
	{
		switch (p_kind)
		{
		case ASTNode::Kind::Token:
			return L"Token";
                case ASTNode::Kind::Body:
                        return L"Body";
		case ASTNode::Kind::Namespace:
			return L"Namespace";
		case ASTNode::Kind::Structure:
			return L"Structure";
		case ASTNode::Kind::AttributeBlock:
			return L"AttributeBlock";
		case ASTNode::Kind::ConstantBlock:
			return L"ConstantBlock";
		case ASTNode::Kind::Texture:
			return L"Texture";
		case ASTNode::Kind::Include:
			return L"Include";
		case ASTNode::Kind::Function:
			return L"Function";
                case ASTNode::Kind::Method:
                        return L"Method";
                case ASTNode::Kind::Constructor:
                        return L"Constructor";
                case ASTNode::Kind::Operator:
                        return L"Operator";
		case ASTNode::Kind::PipelineDefinition:
			return L"PipelineDefinition";
		case ASTNode::Kind::PipelineBody:
			return L"PipelineBody";
		case ASTNode::Kind::VariableDeclaration:
			return L"VariableDeclaration";
		case ASTNode::Kind::Assignment:
			return L"Assignment";
		case ASTNode::Kind::IfStatement:
			return L"IfStatement";
		case ASTNode::Kind::ForLoop:
			return L"ForLoop";
		case ASTNode::Kind::WhileLoop:
			return L"WhileLoop";
		case ASTNode::Kind::ReturnStatement:
			return L"ReturnStatement";
		case ASTNode::Kind::DiscardStatement:
			return L"DiscardStatement";
		case ASTNode::Kind::BinaryExpression:
			return L"BinaryExpression";
		case ASTNode::Kind::UnaryExpression:
			return L"UnaryExpression";
		case ASTNode::Kind::CallExpression:
			return L"CallExpression";
		case ASTNode::Kind::MemberAccess:
			return L"MemberAccess";
		case ASTNode::Kind::VariableReference:
			return L"VariableReference";
		case ASTNode::Kind::Literal:
			return L"Literal";
		case ASTNode::Kind::TernaryExpression:
			return L"TernaryExpression";
		case ASTNode::Kind::LValue:
			return L"LValue";
		case ASTNode::Kind::RValue:
			return L"RValue";
		default:
			return L"(Invalid)";
		}
	}

	std::string to_string(ASTNode::Kind p_kind)
	{
		return spk::StringUtils::wstringToString(to_wstring(p_kind));
	}
	void TokenNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Token(" << to_wstring(token.type) << L", \"" << token.lexeme << L"\")\n";
	}

        void BodyNode::print(std::wostream &p_os, size_t p_indent) const
       {
                p_os << std::wstring(p_indent, L' ') << L"Body{" << std::endl;
		for (const auto &child : children)
		{
			if (child)
			{
				child->print(p_os, p_indent + 2);
			}
		}
		p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
	}

	void NamespaceNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Namespace " << name.lexeme << L"{" << std::endl;
		if (body)
		{
			body->print(p_os, p_indent + 2);
		}
		p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
	}

        void StructureNode::print(std::wostream &p_os, size_t p_indent) const
        {
                p_os << std::wstring(p_indent, L' ') << L"Structure " << name.lexeme << L"{" << std::endl;
                for (const auto& child : variables)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : constructors)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : methods)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : operators)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
        }

        void AttributeBlockNode::print(std::wostream &p_os, size_t p_indent) const
        {
                p_os << std::wstring(p_indent, L' ') << L"AttributeBlock " << name.lexeme << L"{" << std::endl;
                for (const auto& child : variables)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : methods)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : operators)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
        }

        void ConstantBlockNode::print(std::wostream &p_os, size_t p_indent) const
        {
                p_os << std::wstring(p_indent, L' ') << L"ConstantBlock " << name.lexeme << L"{" << std::endl;
                for (const auto& child : variables)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : methods)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                for (const auto& child : operators)
                {
                        if (child)
                        {
                                child->print(p_os, p_indent + 2);
                        }
                }
                p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
        }

	void TextureNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Texture " << name.lexeme << L";" << std::endl;
	}

        void FunctionNode::print(std::wostream &p_os, size_t p_indent) const
        {
		p_os << std::wstring(p_indent, L' ') << to_wstring(kind);
		for (const auto &tok : header)
		{
			p_os << L" " << tok.lexeme;
		}
		if (body)
		{
			p_os << L" {" << std::endl;
			body->print(p_os, p_indent + 2);
			p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
		}
		else
		{
			p_os << L";" << std::endl;
                }
        }

        void OperatorNode::print(std::wostream &p_os, size_t p_indent) const
        {
                p_os << std::wstring(p_indent, L' ') << to_wstring(kind);
                for (const auto &tok : header)
                {
                        p_os << L" " << tok.lexeme;
                }
                if (body)
                {
                        p_os << L" {" << std::endl;
                        body->print(p_os, p_indent + 2);
                        p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
                }
                else
                {
                        p_os << L";" << std::endl;
                }
        }

	void PipelineDefinitionNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Pipeline " << fromStage.lexeme << L" -> " << toStage.lexeme << L":";
		for (const auto &tok : declaration)
		{
			p_os << L" " << tok.lexeme;
		}
		p_os << L";" << std::endl;
	}

	void PipelineBodyNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"PipelineStage " << stage.lexeme << L"(){" << std::endl;
		if (body)
		{
			body->print(p_os, p_indent + 2);
		}
		p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
	}

	void IncludeNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Include ";
		if (system)
		{
			p_os << L"<";
		}
		for (const auto &tok : path)
		{
			p_os << tok.lexeme;
		}
		if (system)
		{
			p_os << L">";
		}
		p_os << std::endl;
	}

	void VariableDeclarationNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"VariableDeclaration " << typeName.lexeme << L" " << name.lexeme;
		if (initializer)
		{
			p_os << L" = " << std::endl;
			initializer->print(p_os, p_indent + 2);
		}
		p_os << L";" << std::endl;
	}

	void AssignmentNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ');
		if (target)
		{
			target->print(p_os, 0);
		}
		p_os << L" " << op.lexeme << L" ";
		if (value)
		{
			value->print(p_os, 0);
		}
		p_os << std::endl;
	}

	void IfStatementNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"If(";
		if (condition)
		{
			condition->print(p_os, 0);
		}
		p_os << L")" << std::endl;
		if (thenBody)
		{
			thenBody->print(p_os, p_indent + 2);
		}
		if (elseBody)
		{
			p_os << std::wstring(p_indent, L' ') << L"else" << std::endl;
			elseBody->print(p_os, p_indent + 2);
		}
	}

	void ForLoopNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"For" << std::endl;
		if (body)
		{
			body->print(p_os, p_indent + 2);
		}
	}

	void WhileLoopNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"While" << std::endl;
		if (body)
		{
			body->print(p_os, p_indent + 2);
		}
	}

	void ReturnStatementNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Return";
		if (value)
		{
			p_os << L" ";
			value->print(p_os, 0);
		}
		p_os << std::endl;
	}

	void DiscardStatementNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << L"Discard" << std::endl;
	}

	void BinaryExpressionNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ');
		if (left)
		{
			left->print(p_os, 0);
		}
		p_os << L" " << op.lexeme << L" ";
		if (right)
		{
			right->print(p_os, 0);
		}
	}

	void UnaryExpressionNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << op.lexeme;
		if (operand)
		{
			operand->print(p_os, 0);
		}
	}

	void CallExpressionNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ');
		if (callee)
		{
			callee->print(p_os, 0);
		}
		p_os << L"(";
		if (arguments.empty())
		{
			p_os << L")";
			return;
		}
		p_os << std::endl;
		for (const auto &arg : arguments)
		{
			if (arg)
			{
				arg->print(p_os, p_indent + 2);
				p_os << std::endl;
			}
		}
		p_os << std::wstring(p_indent, L' ') << L")";
	}

	void MemberAccessNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ');
		if (object)
		{
			object->print(p_os, 0);
		}
		p_os << L"." << member.lexeme;
	}

	void VariableReferenceNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << name.lexeme;
	}

	void LiteralNode::print(std::wostream &p_os, size_t p_indent) const
	{
		p_os << std::wstring(p_indent, L' ') << value.lexeme;
	}

	void TernaryExpressionNode::print(std::wostream &p_os, size_t p_indent) const
	{
		if (condition)
		{
			condition->print(p_os, p_indent);
		}
		p_os << L" ? ";
		if (thenExpr)
		{
			thenExpr->print(p_os, 0);
		}
		p_os << L" : ";
		if (elseExpr)
		{
			elseExpr->print(p_os, 0);
		}
	}

	void LValueNode::print(std::wostream &p_os, size_t p_indent) const
	{
		if (expression)
		{
			expression->print(p_os, p_indent);
		}
	}

	void RValueNode::print(std::wostream &p_os, size_t p_indent) const
	{
		if (expression)
		{
			expression->print(p_os, p_indent);
		}
	}
}
