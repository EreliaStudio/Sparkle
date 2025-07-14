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
                        PipelineDefinition,
                        PipelineBody
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
}
