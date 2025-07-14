#include "structure/graphics/lumina/compiler/spk_ast.hpp"
#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
       std::wstring to_wstring(ASTNode::Kind p_kind)
       {
               switch (p_kind)
               {
                       case ASTNode::Kind::Token:                return L"Token";
                       case ASTNode::Kind::Compound:             return L"Compound";
                       case ASTNode::Kind::Namespace:            return L"Namespace";
                       case ASTNode::Kind::Structure:            return L"Structure";
                       case ASTNode::Kind::AttributeBlock:       return L"AttributeBlock";
                       case ASTNode::Kind::ConstantBlock:        return L"ConstantBlock";
                       case ASTNode::Kind::Texture:              return L"Texture";
                       case ASTNode::Kind::Include:              return L"Include";
                       case ASTNode::Kind::Function:             return L"Function";
                       case ASTNode::Kind::Method:               return L"Method";
                       case ASTNode::Kind::PipelineDefinition:   return L"PipelineDefinition";
                       case ASTNode::Kind::PipelineBody:         return L"PipelineBody";
                       default:                                  return L"(Invalid)";
               }
       }

       std::string to_string(ASTNode::Kind p_kind)
       {
               return spk::StringUtils::wstringToString(to_wstring(p_kind));
       }
       void TokenNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Token("
                       << to_wstring(token.type) << L", \"" << token.lexeme << L"\")\n";
       }

       void CompoundNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Compound{" << std::endl;
               for (const auto &child : children)
               {
                       if (child)
                       {
                               child->print(p_os, p_indent + 2);
                       }
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void NamespaceNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Namespace " << name.lexeme << L"{" << std::endl;
               if (body)
               {
                       body->print(p_os, p_indent + 2);
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void StructureNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Structure " << name.lexeme << L"{" << std::endl;
               if (body)
               {
                       body->print(p_os, p_indent + 2);
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void AttributeBlockNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"AttributeBlock " << name.lexeme << L"{" << std::endl;
               if (body)
               {
                       body->print(p_os, p_indent + 2);
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void ConstantBlockNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"ConstantBlock " << name.lexeme << L"{" << std::endl;
               if (body)
               {
                       body->print(p_os, p_indent + 2);
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void TextureNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Texture " << name.lexeme << L";" << std::endl;
       }

       void FunctionNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << to_wstring(kind);
               for (const auto &tok : header)
               {
                       p_os << L" " << tok.lexeme;
               }
               if (body)
               {
                       p_os << L" {" << std::endl;
                       body->print(p_os, p_indent + 2);
                       p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
               }
               else
               {
                       p_os << L";" << std::endl;
               }
       }

       void PipelineDefinitionNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Pipeline " << fromStage.lexeme << L" -> " << toStage.lexeme << L":";
               for (const auto &tok : declaration)
               {
                       p_os << L" " << tok.lexeme;
               }
               p_os << L";" << std::endl;
       }

       void PipelineBodyNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"PipelineStage " << stage.lexeme << L"(){" << std::endl;
               if (body)
               {
                       body->print(p_os, p_indent + 2);
               }
               p_os << std::wstring(p_indent, L' ') << L"}" << std::endl;
       }

       void IncludeNode::print(std::wostream &p_os, size_t p_indent) const
       {
               p_os << std::wstring(p_indent, L' ') << L"Include ";
               if (system)
               {
                       p_os << L"<";
               }
               for (const auto &tok : path)
               {
                       p_os << tok.lexeme;
               }
               if (system)
               {
                       p_os << L">";
               }
               p_os << std::endl;
       }
}
