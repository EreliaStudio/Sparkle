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
                       Token,
                       Compound,
                       Namespace,
                       Structure,
                       AttributeBlock,
                       ConstantBlock,
                       Texture,
                       Include,
                       Function,
                      Method,
                      Operator,
                      PipelineDefinition,
                       PipelineBody,
                       VariableDeclaration,
                       Assignment,
                       IfStatement,
                       ForLoop,
                       WhileLoop,
                       ReturnStatement,
                       DiscardStatement,
                       BinaryExpression,
                       UnaryExpression,
                       CallExpression,
                       MemberAccess,
                       VariableReference,
                       Literal,
                       TernaryExpression,
                       LValue,
                       RValue
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

       std::wstring to_wstring(ASTNode::Kind p_kind);
       std::string to_string(ASTNode::Kind p_kind);

       struct TokenNode : public ASTNode
       {
               Token token;

               explicit TokenNode(const Token &p_token) :
                       ASTNode(Kind::Token, p_token.location),
                       token(p_token)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct CompoundNode : public ASTNode
       {
               std::vector<std::unique_ptr<ASTNode>> children;

               explicit CompoundNode(Location p_location) :
                       ASTNode(Kind::Compound, std::move(p_location))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct NamespaceNode : public ASTNode
       {
               Token name;
               std::unique_ptr<CompoundNode> body;

               NamespaceNode(const Token &p_name, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::Namespace, p_name.location),
                       name(p_name),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct StructureNode : public ASTNode
       {
               Token name;
               std::unique_ptr<CompoundNode> body;

               StructureNode(const Token &p_name, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::Structure, p_name.location),
                       name(p_name),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct AttributeBlockNode : public ASTNode
       {
               Token name;
               std::unique_ptr<CompoundNode> body;

               AttributeBlockNode(const Token &p_name, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::AttributeBlock, p_name.location),
                       name(p_name),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct ConstantBlockNode : public ASTNode
       {
               Token name;
               std::unique_ptr<CompoundNode> body;

               ConstantBlockNode(const Token &p_name, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::ConstantBlock, p_name.location),
                       name(p_name),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct TextureNode : public ASTNode
       {
               Token name;

               explicit TextureNode(const Token &p_name) :
                       ASTNode(Kind::Texture, p_name.location),
                       name(p_name)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

struct FunctionNode : public ASTNode
{
        std::vector<Token> header;
        std::unique_ptr<CompoundNode> body;

               FunctionNode(Kind p_kind, std::vector<Token> p_header, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(p_kind, p_header.empty() ? Location{} : p_header.front().location),
                       header(std::move(p_header)),
                       body(std::move(p_body))
               {
               }

        void print(std::wostream &p_os, size_t p_indent = 0) const override;
};

       struct OperatorNode : public ASTNode
       {
               std::vector<Token> header;
               std::unique_ptr<CompoundNode> body;

               OperatorNode(std::vector<Token> p_header, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::Operator, p_header.empty() ? Location{} : p_header.front().location),
                       header(std::move(p_header)),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct PipelineDefinitionNode : public ASTNode
       {
               Token fromStage;
               Token toStage;
               std::vector<Token> declaration;

               PipelineDefinitionNode(const Token &p_from, const Token &p_to, std::vector<Token> p_decl) :
                       ASTNode(Kind::PipelineDefinition, p_from.location),
                       fromStage(p_from),
                       toStage(p_to),
                       declaration(std::move(p_decl))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct PipelineBodyNode : public ASTNode
       {
               Token stage;
               std::unique_ptr<CompoundNode> body;

               PipelineBodyNode(const Token &p_stage, std::unique_ptr<CompoundNode> p_body) :
                       ASTNode(Kind::PipelineBody, p_stage.location),
                       stage(p_stage),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct IncludeNode : public ASTNode
       {
               std::vector<Token> path;
               bool system;

               IncludeNode(std::vector<Token> p_path, bool p_system, Location p_loc) :
                       ASTNode(Kind::Include, std::move(p_loc)),
                       path(std::move(p_path)),
                       system(p_system)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct VariableDeclarationNode : public ASTNode
       {
               Token typeName;
               Token name;
               std::unique_ptr<ASTNode> initializer;

               VariableDeclarationNode(Token p_type, Token p_name, std::unique_ptr<ASTNode> p_init) :
                       ASTNode(Kind::VariableDeclaration, p_type.location),
                       typeName(p_type),
                       name(p_name),
                       initializer(std::move(p_init))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct AssignmentNode : public ASTNode
       {
               std::unique_ptr<ASTNode> target;
               Token op;
               std::unique_ptr<ASTNode> value;

               AssignmentNode(std::unique_ptr<ASTNode> p_target, Token p_op, std::unique_ptr<ASTNode> p_value) :
                       ASTNode(Kind::Assignment, p_op.location),
                       target(std::move(p_target)),
                       op(p_op),
                       value(std::move(p_value))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct IfStatementNode : public ASTNode
       {
               std::unique_ptr<ASTNode> condition;
               std::unique_ptr<CompoundNode> thenBody;
               std::unique_ptr<CompoundNode> elseBody;

               IfStatementNode(std::unique_ptr<ASTNode> p_cond, std::unique_ptr<CompoundNode> p_then, std::unique_ptr<CompoundNode> p_else, Location p_loc) :
                       ASTNode(Kind::IfStatement, std::move(p_loc)),
                       condition(std::move(p_cond)),
                       thenBody(std::move(p_then)),
                       elseBody(std::move(p_else))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct ForLoopNode : public ASTNode
       {
               std::unique_ptr<ASTNode> init;
               std::unique_ptr<ASTNode> condition;
               std::unique_ptr<ASTNode> increment;
               std::unique_ptr<CompoundNode> body;

               ForLoopNode(std::unique_ptr<ASTNode> p_init, std::unique_ptr<ASTNode> p_cond, std::unique_ptr<ASTNode> p_inc, std::unique_ptr<CompoundNode> p_body, Location p_loc) :
                       ASTNode(Kind::ForLoop, std::move(p_loc)),
                       init(std::move(p_init)),
                       condition(std::move(p_cond)),
                       increment(std::move(p_inc)),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct WhileLoopNode : public ASTNode
       {
               std::unique_ptr<ASTNode> condition;
               std::unique_ptr<CompoundNode> body;

               WhileLoopNode(std::unique_ptr<ASTNode> p_cond, std::unique_ptr<CompoundNode> p_body, Location p_loc) :
                       ASTNode(Kind::WhileLoop, std::move(p_loc)),
                       condition(std::move(p_cond)),
                       body(std::move(p_body))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct ReturnStatementNode : public ASTNode
       {
               std::unique_ptr<ASTNode> value;

               ReturnStatementNode(std::unique_ptr<ASTNode> p_value, Location p_loc) :
                       ASTNode(Kind::ReturnStatement, std::move(p_loc)),
                       value(std::move(p_value))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct DiscardStatementNode : public ASTNode
       {
               explicit DiscardStatementNode(Location p_loc) :
                       ASTNode(Kind::DiscardStatement, std::move(p_loc))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct BinaryExpressionNode : public ASTNode
       {
               std::unique_ptr<ASTNode> left;
               Token op;
               std::unique_ptr<ASTNode> right;

               BinaryExpressionNode(std::unique_ptr<ASTNode> p_left, Token p_op, std::unique_ptr<ASTNode> p_right, Location p_loc) :
                       ASTNode(Kind::BinaryExpression, std::move(p_loc)),
                       left(std::move(p_left)),
                       op(p_op),
                       right(std::move(p_right))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct UnaryExpressionNode : public ASTNode
       {
               Token op;
               std::unique_ptr<ASTNode> operand;

               UnaryExpressionNode(Token p_op, std::unique_ptr<ASTNode> p_operand, Location p_loc) :
                       ASTNode(Kind::UnaryExpression, std::move(p_loc)),
                       op(p_op),
                       operand(std::move(p_operand))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct CallExpressionNode : public ASTNode
       {
               std::unique_ptr<ASTNode> callee;
               std::vector<std::unique_ptr<ASTNode>> arguments;

               CallExpressionNode(std::unique_ptr<ASTNode> p_callee, std::vector<std::unique_ptr<ASTNode>> p_args, Location p_loc) :
                       ASTNode(Kind::CallExpression, std::move(p_loc)),
                       callee(std::move(p_callee)),
                       arguments(std::move(p_args))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct MemberAccessNode : public ASTNode
       {
               std::unique_ptr<ASTNode> object;
               Token member;

               MemberAccessNode(std::unique_ptr<ASTNode> p_obj, Token p_member, Location p_loc) :
                       ASTNode(Kind::MemberAccess, std::move(p_loc)),
                       object(std::move(p_obj)),
                       member(p_member)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct VariableReferenceNode : public ASTNode
       {
               Token name;

               explicit VariableReferenceNode(Token p_name) :
                       ASTNode(Kind::VariableReference, p_name.location),
                       name(p_name)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct LiteralNode : public ASTNode
       {
               Token value;

               explicit LiteralNode(Token p_val) :
                       ASTNode(Kind::Literal, p_val.location),
                       value(p_val)
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct TernaryExpressionNode : public ASTNode
       {
               std::unique_ptr<ASTNode> condition;
               std::unique_ptr<ASTNode> thenExpr;
               std::unique_ptr<ASTNode> elseExpr;

               TernaryExpressionNode(std::unique_ptr<ASTNode> p_cond, std::unique_ptr<ASTNode> p_then, std::unique_ptr<ASTNode> p_else, Location p_loc) :
                       ASTNode(Kind::TernaryExpression, std::move(p_loc)),
                       condition(std::move(p_cond)),
                       thenExpr(std::move(p_then)),
                       elseExpr(std::move(p_else))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct LValueNode : public ASTNode
       {
               std::unique_ptr<ASTNode> expression;

               explicit LValueNode(std::unique_ptr<ASTNode> p_expr, Location p_loc) :
                       ASTNode(Kind::LValue, std::move(p_loc)),
                       expression(std::move(p_expr))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };

       struct RValueNode : public ASTNode
       {
               std::unique_ptr<ASTNode> expression;

               explicit RValueNode(std::unique_ptr<ASTNode> p_expr, Location p_loc) :
                       ASTNode(Kind::RValue, std::move(p_loc)),
                       expression(std::move(p_expr))
               {
               }

               void print(std::wostream &p_os, size_t p_indent = 0) const override;
       };
}
