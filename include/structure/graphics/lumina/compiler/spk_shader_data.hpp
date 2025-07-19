#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <ostream>
#include "structure/graphics/lumina/compiler/spk_ast.hpp"

namespace spk::Lumina
{
    struct TypeSymbol;
    struct NamespaceSymbol;

struct Variable
    {
        std::wstring name;
        TypeSymbol* type = nullptr;
        void print(std::wostream& os, size_t indent = 0) const;
};

    struct Statement
    {
        ASTNode::Kind kind = ASTNode::Kind::Token;
        std::wstring text;
        std::vector<Statement> children;
        void print(std::wostream& os, size_t indent = 0) const;
    };

    struct FunctionSymbol
    {
        std::wstring name;
        const TypeSymbol* returnType = nullptr;
        std::vector<Variable> parameters;
        // Normalized representation of the function header
        std::wstring signature;
        std::vector<Statement> body;
        void print(std::wostream& os, size_t indent = 0) const;

        FunctionSymbol() = default;
        FunctionSymbol(const std::wstring& name,
                       const TypeSymbol* returnType,
                       const std::vector<Variable>& parameters,
                       const std::vector<Statement>& body = {});
    };

struct TypeSymbol
{
    enum class Role
    {
        Structure,
        Attribute,
        Constant
    };

    std::wstring name;
    NamespaceSymbol* parent = nullptr;
    Role role = Role::Structure;
    std::unordered_set<TypeSymbol*> convertible;
    std::vector<Variable> members;
    std::vector<FunctionSymbol> constructors;
    std::vector<FunctionSymbol> methods;
    std::unordered_map<std::wstring, std::vector<FunctionSymbol>> operators;
    void print(std::wostream& os, size_t indent = 0) const;

    TypeSymbol() = default;
    TypeSymbol(const std::wstring& name,
               Role role = Role::Structure,
               NamespaceSymbol* parent = nullptr);

    void addConstructor(const std::vector<Variable>& parameters);
    void addMethod(TypeSymbol* returnType,
                   const std::wstring& methodName,
                   const std::vector<Variable>& parameters);
    void addOperator(TypeSymbol* returnType,
                     const std::wstring& operatorType,
                     const std::vector<Variable>& parameters);
};

struct NamespaceSymbol
    {
        std::wstring name;
        NamespaceSymbol* parent = nullptr;
        std::vector<NamespaceSymbol> namespaces;
        std::vector<TypeSymbol*> structures;
        std::vector<std::wstring> attributeBlocks;
        std::vector<std::wstring> constantBlocks;
        std::vector<Variable> textures;
        std::unordered_map<std::wstring, TypeSymbol> types;
        std::unordered_map<std::wstring, std::vector<FunctionSymbol>> functionSignatures;
        std::vector<FunctionSymbol> functions;
        void print(std::wostream& os, size_t indent = 0) const;

        TypeSymbol* addType(const std::wstring& name,
                            TypeSymbol::Role role = TypeSymbol::Role::Structure);
        TypeSymbol* typeSymbol(const std::wstring& typeName) const;

        void addFunction(TypeSymbol* returnType,
                         const std::wstring& name,
                         const std::vector<Variable>& parameters);
    };

    struct PipelineStage
    {
        std::wstring stage;
        std::vector<Statement> body;
        void print(std::wostream& os, size_t indent = 0) const;
    };

struct ShaderData
    {
        NamespaceSymbol globalNamespace;
        std::vector<PipelineStage> pipelineStages;
        void print(std::wostream& os, size_t indent = 0) const;
    };

    std::wostream& operator<<(std::wostream& os, const ShaderData& data);

    std::wstring composeSignature(const std::wstring& name,
                                  const std::vector<Variable>& parameters);
}

