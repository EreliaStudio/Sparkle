#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>

#include "structure/graphics/lumina/compiler/spk_ast.hpp"
#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"

namespace spk::Lumina
{
    class AnalyzerException : public std::runtime_error
    {
    public:
        AnalyzerException(const std::wstring& p_msg, const Location& p_location, const SourceManager& p_sourceManager);
    private:
        static std::wstring compose(const std::wstring& p_msg, const Location& p_location, const SourceManager& p_sourceManager);
    };

    class Analyzer
    {
    public:
        Analyzer(SourceManager& p_sourceManager);

        void run(const std::vector<std::unique_ptr<ASTNode>>& p_nodes);

    private:
        using AnalyzeFn = void (Analyzer::*)(const ASTNode*);

        SourceManager& _sourceManager;
        std::unordered_set<std::wstring> _pipelineStages;
        struct TypeSymbol;

        struct Variable
        {
            std::wstring name;
            TypeSymbol* type = nullptr;
        };

        struct TypeSymbol
        {
            std::wstring name;
            std::unordered_set<TypeSymbol*> convertible;
            std::vector<Variable> members;
            std::vector<FunctionSymbol> constructors;
            std::unordered_map<std::wstring, std::vector<FunctionSymbol>> operators;
        };

        struct NamespaceSymbol
        {
            std::wstring name;
            std::vector<NamespaceSymbol> namespaces;
            std::vector<TypeSymbol*> structures;
            std::vector<std::wstring> attributeBlocks;
            std::vector<std::wstring> constantBlocks;
            std::vector<Variable> textures;
            std::unordered_map<std::wstring, TypeSymbol> types;
            std::unordered_map<std::wstring, std::vector<FunctionSymbol>> functionSignatures;
            std::vector<FunctionSymbol> functions;
        };

        struct FunctionSymbol
        {
            std::wstring name;
            std::wstring returnType;
            std::vector<std::wstring> parameters;
            std::wstring signature;
        };

        std::vector<std::wstring> _containerStack;
        std::unordered_set<std::wstring> _includedFiles;
        std::vector<std::wstring> _includeStack;
        std::vector<std::unordered_map<std::wstring, Variable>> _scopes;
        NamespaceSymbol _anonymousNamespace;
        std::vector<NamespaceSymbol*> _namespaceStack;
        std::unordered_set<std::wstring> _namespaceNames;
        std::unordered_map<ASTNode::Kind, AnalyzeFn> _dispatch;

        void _pushScope();
        void _popScope();
        void _loadBuiltinTypes();
        void _loadBuiltinVariables();
        void _loadBuiltinFunctions();
        TypeSymbol* _findType(const std::wstring& p_name) const;
        const NamespaceSymbol* _findNamespace(const std::wstring& p_name) const;
        std::wstring _conversionInfo(const std::wstring& p_from) const;
        std::wstring _extractCalleeName(const ASTNode* p_node) const;
        std::vector<std::wstring> _parseParameters(const std::vector<Token>& p_header) const;
        std::vector<FunctionSymbol> _findFunctions(const std::wstring& p_name) const;
        std::wstring _evaluate(const ASTNode* p_node);

        void _pushContainer(const std::wstring& p_name);
        void _popContainer();
        std::wstring _currentContext() const;
        std::wstring _makeSignature(const std::vector<Token>& p_header) const;

        void _analyze(const ASTNode* p_node);

        void _analyzeToken(const ASTNode* p_node);
        void _analyzeBody(const ASTNode* p_node);
        void _analyzeNamespace(const ASTNode* p_node);
        void _analyzeStructureLike(const ASTNode* p_node);
        void _analyzeTexture(const ASTNode* p_node);
        void _analyzeInclude(const ASTNode* p_node);
        void _analyzeFunction(const ASTNode* p_node);
        void _analyzePipelineDef(const ASTNode* p_node);
        void _analyzePipelineBody(const ASTNode* p_node);
        void _analyzeVariableDeclaration(const ASTNode* p_node);
        void _analyzeAssignment(const ASTNode* p_node);
        void _analyzeIfStatement(const ASTNode* p_node);
        void _analyzeForLoop(const ASTNode* p_node);
        void _analyzeWhileLoop(const ASTNode* p_node);
        void _analyzeReturn(const ASTNode* p_node);
        void _analyzeDiscardStatement(const ASTNode* p_node);
        void _analyzeBinaryExpression(const ASTNode* p_node);
        void _analyzeUnaryExpression(const ASTNode* p_node);
        void _analyzeCallExpression(const ASTNode* p_node);
        void _analyzeMemberAccess(const ASTNode* p_node);
        void _analyzeVariableReference(const ASTNode* p_node);
        void _analyzeLiteral(const ASTNode* p_node);
        void _analyzeTernaryExpression(const ASTNode* p_node);
        void _analyzeLValue(const ASTNode* p_node);
        void _analyzeRValue(const ASTNode* p_node);
    };
}

