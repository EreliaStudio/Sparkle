#include "structure/graphics/lumina/compiler/spk_shader_data.hpp"
#include <algorithm>

namespace spk::Lumina
{
	namespace
	{
		bool contains(const std::vector<std::wstring> &p_vec, const std::wstring &p_name)
		{
			return std::find(p_vec.begin(), p_vec.end(), p_name) != p_vec.end();
		}
	}

	void Variable::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		p_os << ind << (type ? type->name : L"void") << L" " << name;
	}

	void Statement::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		p_os << ind << to_wstring(kind);
		if (!text.empty())
		{
			p_os << L": " << text;
		}
		p_os << L"\n";
		for (const auto &child : children)
		{
			child.print(p_os, p_indent + 1);
		}
	}

	void FunctionSymbol::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		p_os << ind << L"Function: " << name << L"(";
		for (size_t i = 0; i < parameters.size(); ++i)
		{
			parameters[i].print(p_os);
			if (i + 1 < parameters.size())
			{
				p_os << L", ";
			}
		}
		p_os << L") -> " << (returnType ? returnType->name : L"void") << L"\n";
		for (const auto &stmt : body)
		{
			stmt.print(p_os, p_indent + 1);
		}
	}

	void TypeSymbol::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		std::wstring kind;
		switch (role)
		{
		case Role::Attribute:
			kind = L"AttributeBlock";
			break;
		case Role::Constant:
			kind = L"ConstantBlock";
			break;
		default:
			kind = L"Structure";
			break;
		}
		p_os << ind << kind << L": " << name << L"\n";
		size_t nextIndent = p_indent + 1;
		for (const auto &member : members)
		{
			member.print(p_os, nextIndent);
			p_os << L"\n";
		}
	}

	void NamespaceSymbol::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		p_os << ind << L"Namespace: " << name << L"\n";
		size_t nextIndent = p_indent + 1;

		for (const auto &[n, t] : types)
		{
			t.print(p_os, nextIndent);
		}

		for (const auto &func : functions)
		{
			func.print(p_os, nextIndent);
		}

		for (const auto &ns : namespaces)
		{
			ns.print(p_os, nextIndent);
		}
	}

	void PipelineStage::print(std::wostream &p_os, size_t p_indent) const
	{
		std::wstring ind(p_indent, L'\t');
		p_os << ind << L"PipelineStage: " << stage << L"\n";
		for (const auto &stmt : body)
		{
			stmt.print(p_os, p_indent + 1);
		}
	}

	void ShaderData::print(std::wostream &p_os, size_t p_indent) const
	{
		globalNamespace.print(p_os, p_indent);
		for (const auto &stage : pipelineStages)
		{
			stage.print(p_os, p_indent);
		}
	}

	std::wostream &operator<<(std::wostream &p_os, const ShaderData &p_data)
	{
		p_data.print(p_os);
		return p_os;
	}
}
