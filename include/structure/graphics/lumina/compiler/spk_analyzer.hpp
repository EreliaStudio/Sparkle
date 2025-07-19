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
        std::unordered_set<std::wstring> _types;
        std::unordered_map<std::wstring, std::unordered_set<std::wstring>> _implicitConversions;
        std::unordered_set<std::wstring> _textures;
        std::unordered_map<std::wstring, std::unordered_set<std::wstring>> _functionSignatures;
        std::vector<std::wstring> _containerStack;
        std::unordered_set<std::wstring> _includedFiles;
        std::vector<std::wstring> _includeStack;
        struct Symbol { std::wstring type; };
        std::vector<std::unordered_map<std::wstring, Symbol>> _scopes;
        std::unordered_map<ASTNode::Kind, AnalyzeFn> _dispatch;

        void _pushScope();
        void _popScope();
        void _loadBuiltinTypes();
        void _loadBuiltinVariables();
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

