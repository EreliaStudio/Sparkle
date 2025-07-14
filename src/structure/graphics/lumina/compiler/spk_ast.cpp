#include "structure/graphics/lumina/compiler/spk_ast.hpp"

#include <iomanip>
#include <iostream>
#include <string>

using namespace spk::Lumina;

namespace
{
	void indent(std::wostream &p_os, std::size_t p_indent)
	{
		for (std::size_t i = 0; i < p_indent; ++i)
		{
			p_os << L' ';
		}
	}

	void printVec(const std::vector<std::unique_ptr<ASTNode>> &p_nodes, std::wostream &p_os, std::size_t p_indent)
	{
		for (const auto &n : p_nodes)
		{
			if (n)
			{
				n->print(p_os, p_indent);
			}
		}
	}
}

void IncludeASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Include \"" << includeFile.wstring() << L"\"\n";
}

void NamespaceASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Namespace " << name << L" {\n";
	printVec(declarations, p_os, p_indent + 2);
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void PipelineFlowASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"PipelineFlow " << inputStage << L" -> " << outputStage << L" : " << typeName << L' ' << varName << L'\n';
}

void PipelinePassASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"PipelinePass " << name << L" {\n";
	if (body)
	{
		body->print(p_os, p_indent + 2);
	}
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void StructASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Struct " << name << L" {\n";
	printVec(members, p_os, p_indent + 2);
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void AttributeBlockASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"AttributeBlock " << name << L" {\n";
	printVec(members, p_os, p_indent + 2);
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void ConstantBlockASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"ConstantBlock " << name << L" {\n";
	printVec(members, p_os, p_indent + 2);
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void TextureDeclASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"TextureDecl " << name << L'\n';
}

void VariableDeclASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Var " << typeName << L' ' << name;
	if (initializer)
	{
		p_os << L" = ";
		initializer->print(p_os, 0);
	}
	p_os << L'\n';
}

void FunctionASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Function " << returnType << L' ' << name << L"(";
	for (std::size_t i = 0; i < parameters.size(); ++i)
	{
		const auto &p = parameters[i];
		p_os << p->typeName << L' ' << p->name;
		if (i + 1 < parameters.size())
		{
			p_os << L", ";
		}
	}
	p_os << L") {\n";
	if (body)
	{
		body->print(p_os, p_indent + 2);
	}
	indent(p_os, p_indent);
	p_os << L"}\n";
}

void BodyASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	for (const auto &s : statements)
	{
		if (s)
		{
			s->print(p_os, p_indent);
		}
	}
}

void IfASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"If (";
	if (condition)
	{
		condition->print(p_os, 0);
	}
	p_os << L")\n";
	if (thenBody)
	{
		thenBody->print(p_os, p_indent + 2);
	}
	if (elseBody)
	{
		indent(p_os, p_indent);
		p_os << L"Else\n";
		elseBody->print(p_os, p_indent + 2);
	}
}

void ForASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"For (...)\n";
	if (body)
	{
		body->print(p_os, p_indent + 2);
	}
}

void WhileASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"While (...)\n";
	if (body)
	{
		body->print(p_os, p_indent + 2);
	}
}

void ReturnASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Return";
	if (value)
	{
		p_os << L' ';
		value->print(p_os, 0);
	}
	p_os << L'\n';
}

void DiscardASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"Discard\n";
}

void RaiseExceptionASTNode::print(std::wostream &p_os, std::size_t p_indent) const
{
	indent(p_os, p_indent);
	p_os << L"RaiseException ";
	if (formatString)
	{
		formatString->print(p_os, 0);
	}
	p_os << L'\n';
}

void IdentifierExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	p_os << name;
}

void LiteralExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	p_os << literal;
}

void UnaryExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	p_os << to_wstring(op) << L' ';
	if (operand)
	{
		operand->print(p_os, 0);
	}
}

void BinaryExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	if (lhs)
	{
		lhs->print(p_os, 0);
	}
	p_os << L' ' << to_wstring(op) << L' ';
	if (rhs)
	{
		rhs->print(p_os, 0);
	}
}

void CallExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	if (callee)
	{
		callee->print(p_os, 0);
	}
	p_os << L'(';
	for (std::size_t i = 0; i < args.size(); ++i)
	{
		if (args[i])
		{
			args[i]->print(p_os, 0);
		}
		if (i + 1 < args.size())
		{
			p_os << L", ";
		}
	}
	p_os << L')';
}

void MemberAccessExprASTNode::print(std::wostream &p_os, std::size_t) const
{
	if (object)
	{
		object->print(p_os, 0);
	}
	p_os << L'.' << memberName;
}
