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

       std::wstring composeSignature(const std::wstring &p_name,
                                     const std::vector<Variable> &p_parameters)
       {
               std::wstring result = p_name + L"(";
               std::wstring parameterString;
               for (const auto &variable : p_parameters)
               {
                       if (!parameterString.empty())
                       {
                               parameterString += L", ";
                       }
                       if (variable.type)
                       {
                               parameterString += variable.type->name;
                       }
               }
               result += parameterString + L")";
               return result;
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
		for (const auto &method : methods)
		{
			method.print(p_os, nextIndent);
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

       FunctionSymbol::FunctionSymbol(const std::wstring &p_name,
                                      const TypeSymbol *p_returnType,
                                      const std::vector<Variable> &p_parameters,
                                      const std::vector<Statement> &p_body)
               : name(p_name), returnType(p_returnType), parameters(p_parameters), body(p_body)
       {
               signature = composeSignature(name, parameters);
       }

       TypeSymbol::TypeSymbol(const std::wstring &p_name,
                              Role p_role,
                              NamespaceSymbol *p_parent)
               : name(p_name), parent(p_parent), role(p_role)
       {
       }

        void NamespaceSymbol::addFunction(TypeSymbol *p_returnType,
                                          const std::wstring &p_name,
                                          const std::vector<Variable> &p_parameters)
        {
                functions.emplace_back(p_name, p_returnType, p_parameters);
        }

        TypeSymbol *NamespaceSymbol::addType(const std::wstring &p_name,
                                             TypeSymbol::Role p_role)
        {
                auto [it, inserted] = types.emplace(p_name, TypeSymbol(p_name, p_role));
                it->second.parent = this;
                return &it->second;
        }

        TypeSymbol *NamespaceSymbol::typeSymbol(const std::wstring &p_typeName) const
        {
                auto pos = p_typeName.find(L"::");
                if (pos != std::wstring::npos)
                {
                        std::wstring nsName = p_typeName.substr(0, pos);
                        std::wstring rest = p_typeName.substr(pos + 2);
                        for (const auto &ns : namespaces)
                        {
                                if (ns.name == nsName)
                                {
                                        return ns.typeSymbol(rest);
                                }
                        }
                        return nullptr;
                }

                auto it = types.find(p_typeName);
                if (it != types.end())
                {
                        return const_cast<TypeSymbol *>(&it->second);
                }
                if (parent)
                {
                        return parent->typeSymbol(p_typeName);
                }
                return nullptr;
        }

        void TypeSymbol::addConstructor(const std::vector<Variable> &p_parameters)
        {
                FunctionSymbol constructor(name, this, p_parameters);

                constructors.push_back(constructor);
                if (parent)
                {
                        parent->functions.push_back(constructor);
                }
        }

        void TypeSymbol::addMethod(TypeSymbol *p_returnType,
                                   const std::wstring &p_methodName,
                                   const std::vector<Variable> &p_parameters)
        {
                FunctionSymbol newMethod(p_methodName, p_returnType, p_parameters);

                methods.push_back(newMethod);
                if (parent)
                {
                        parent->functions.push_back(newMethod);
                }
        }

        void TypeSymbol::addOperator(TypeSymbol *p_returnType,
                                     const std::wstring &p_operatorType,
                                     const std::vector<Variable> &p_parameters)
        {
                FunctionSymbol newMethod(L"Operator" + p_operatorType, p_returnType, p_parameters);

                methods.push_back(newMethod);
                if (parent)
                {
                        parent->functions.push_back(newMethod);
                }
        }
}
