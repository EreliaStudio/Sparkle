#pragma once

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <string>
#include <stack>

#include "structure/graphics/lumina/compiler/spk_ast.hpp"
#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"
#include "structure/graphics/lumina/compiler/spk_shader_data.hpp"

namespace spk::Lumina
{
    class AnalyzerException : public std::runtime_error
    {
    public:
        AnalyzerException(const std::wstring& p_msg,
                           const Location& p_location,
                           const SourceManager& p_sourceManager,
                           const std::wstring& p_details = L"",
                           size_t p_markerLength = 1);
    private:
        static std::wstring compose(const std::wstring& p_msg,
                                    const Location& p_location,
                                    const SourceManager& p_sourceManager,
                                    const std::wstring& p_details,
                                    size_t p_markerLength);
    };

    class Analyzer
    {
    public:
        Analyzer(SourceManager& p_sourceManager);

        void run(const std::vector<std::unique_ptr<ASTNode>>& p_nodes);

        const ShaderData& getData() const;

    private:
        using AnalyzeFn = void (Analyzer::*)(const ASTNode*);

        SourceManager& _sourceManager;
        std::unordered_set<std::wstring> _pipelineStages;
        using Variable = spk::Lumina::Variable;
        using FunctionSymbol = spk::Lumina::FunctionSymbol;
        using TypeSymbol = spk::Lumina::TypeSymbol;
        using NamespaceSymbol = spk::Lumina::NamespaceSymbol;
        using Statement = spk::Lumina::Statement;
        using PipelineStage = spk::Lumina::PipelineStage;

        std::vector<std::wstring> _containerStack;
        std::unordered_set<std::wstring> _includedFiles;
        std::vector<std::wstring> _includeStack;
        std::vector<std::unordered_map<std::wstring, Variable>> _scopes;
        ShaderData _shaderData;
        std::vector<NamespaceSymbol*> _namespaceStack;
        std::vector<const TypeSymbol*> _returnTypeStack;
        std::unordered_set<std::wstring> _namespaceNames;
        std::unordered_map<ASTNode::Kind, AnalyzeFn> _dispatch;

        void _pushScope();
        void _popScope();
        void _loadBuiltinTypes();
        void _loadBuiltinVariables();
        void _loadBuiltinFunctions();
        void _defineBuiltinTypes();
        void _defineBuiltinFunctions();
        TypeSymbol* _findType(const std::wstring& p_name) const;
        const NamespaceSymbol* _findNamespace(const std::wstring& p_name) const;
        std::wstring _conversionInfo(const std::wstring& p_from) const;
	std::wstring _availableSignatures(const std::vector<FunctionSymbol>& p_funcs) const;
        std::wstring _extractCalleeName(const ASTNode* p_node) const;
        std::vector<Variable> _parseParameters(const std::vector<Token>& p_header) const;
        std::vector<FunctionSymbol> _findFunctions(const std::wstring& p_name) const;
        bool _canConvert(const std::wstring& from, const std::wstring& to) const;
        std::wstring _resolveCall(const ASTNode* callee,
                                  const std::wstring& name,
                                  const std::vector<std::wstring>& argTypes,
                                  const Location& loc);
        std::wstring _evaluate(const ASTNode* p_node);

        void _pushContainer(const std::wstring& p_name);
        void _popContainer();
        std::wstring _currentContext() const;
        std::wstring _makeSignature(const std::vector<Token>& p_header) const;
        std::wstring _makeCallSignature(const std::wstring& name,
                                        const std::vector<std::wstring>& argTypes) const;
        Statement _convertAST(const ASTNode* p_node) const;

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

