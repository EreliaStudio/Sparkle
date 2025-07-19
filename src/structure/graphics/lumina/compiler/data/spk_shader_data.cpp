#include "structure/graphics/lumina/compiler/data/spk_shader_data.hpp"
#include <algorithm>

namespace spk::Lumina
{
namespace
{
    bool contains(const std::vector<std::wstring>& vec, const std::wstring& name)
    {
        return std::find(vec.begin(), vec.end(), name) != vec.end();
    }
}

void Variable::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
    os << ind << (type ? type->name : L"void") << L" " << name;
}

void Statement::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
    os << ind << to_wstring(kind);
    if (!text.empty())
        os << L": " << text;
    os << L"\n";
    for (const auto& child : children)
        child.print(os, indent + 1);
}

void FunctionSymbol::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
    os << ind << L"Function: " << name << L"(";
    for (size_t i = 0; i < parameters.size(); ++i)
    {
        parameters[i].print(os);
        if (i + 1 < parameters.size())
            os << L", ";
    }
    os << L") -> " << (returnType ? returnType->name : L"void") << L"\n";
    for (const auto& stmt : body)
        stmt.print(os, indent + 1);
}

void TypeSymbol::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
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
    os << ind << kind << L": " << name << L"\n";
    size_t nextIndent = indent + 1;
    for (const auto& member : members)
    {
        member.print(os, nextIndent);
        os << L"\n";
    }
}

void NamespaceSymbol::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
    os << ind << L"Namespace: " << name << L"\n";
    size_t nextIndent = indent + 1;

    for (const auto& [n, t] : types)
    {
        t.print(os, nextIndent);
    }

    for (const auto& func : functions)
        func.print(os, nextIndent);

    for (const auto& ns : namespaces)
        ns.print(os, nextIndent);
}

void PipelineStage::print(std::wostream& os, size_t indent) const
{
    std::wstring ind(indent, L'\t');
    os << ind << L"PipelineStage: " << stage << L"\n";
    for (const auto& stmt : body)
        stmt.print(os, indent + 1);
}

void ShaderData::print(std::wostream& os, size_t indent) const
{
    globalNamespace.print(os, indent);
    for (const auto& stage : pipelineStages)
        stage.print(os, indent);
}

std::wostream& operator<<(std::wostream& os, const ShaderData& data)
{
    data.print(os);
    return os;
}
}
