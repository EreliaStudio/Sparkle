#include "structure/graphics/lumina/compiler/spk_token.hpp"

#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	std::wstring to_wstring(Token::Type p_type)
    {
        switch (p_type)
        {
            case Token::Type::Identifier:       return L"Identifier";
            case Token::Type::Struct:           return L"Struct";
            case Token::Type::Namespace:        return L"Namespace";
            case Token::Type::AttributeBlock:   return L"AttributeBlock";
            case Token::Type::ConstantBlock:    return L"ConstantBlock";
            case Token::Type::Texture:          return L"Texture";
            case Token::Type::ShaderPass:       return L"ShaderPass";
            case Token::Type::RaiseException:   return L"RaiseException";
            case Token::Type::If:               return L"If";
            case Token::Type::Else:             return L"Else";
            case Token::Type::For:              return L"For";
            case Token::Type::While:            return L"While";
            case Token::Type::Return:           return L"Return";
            case Token::Type::Discard:          return L"Discard";

            case Token::Type::NumberLiteral:    return L"NumberLiteral";
            case Token::Type::BoolLiteral:      return L"BoolLiteral";
            case Token::Type::StringLiteral:    return L"StringLiteral";

            case Token::Type::Plus:             return L"Plus";
            case Token::Type::Minus:            return L"Minus";
            case Token::Type::Multiply:         return L"Multiply";
            case Token::Type::Divide:           return L"Divide";
            case Token::Type::Percent:          return L"Percent";
            case Token::Type::PlusEqual:        return L"PlusEqual";
            case Token::Type::MinusEqual:       return L"MinusEqual";
            case Token::Type::MultiplyEqual:    return L"MultiplyEqual";
            case Token::Type::DivideEqual:      return L"DivideEqual";
            case Token::Type::PercentEqual:     return L"PercentEqual";
            case Token::Type::PlusPlus:         return L"PlusPlus";
            case Token::Type::MinusMinus:       return L"MinusMinus";

            case Token::Type::EqualEqual:       return L"EqualEqual";
            case Token::Type::Different:        return L"Different";
            case Token::Type::Less:             return L"Less";
            case Token::Type::Greater:          return L"Greater";
            case Token::Type::LessEqual:        return L"LessEqual";
            case Token::Type::GreaterEqual:     return L"GreaterEqual";
            case Token::Type::AndAnd:           return L"AndAnd";
            case Token::Type::OrOr:             return L"OrOr";
            case Token::Type::Bang:             return L"Bang";

            case Token::Type::Equal:            return L"Equal";
            case Token::Type::Dot:              return L"Dot";
            case Token::Type::Arrow:            return L"Arrow";
            case Token::Type::DoubleColon:      return L"DoubleColon";

            case Token::Type::Comma:            return L"Comma";
            case Token::Type::Semicolon:        return L"Semicolon";
            case Token::Type::Colon:            return L"Colon";
            case Token::Type::Question:         return L"Question";

            case Token::Type::OpenParenthesis:  return L"OpenParenthesis";
            case Token::Type::CloseParenthesis: return L"CloseParenthesis";
            case Token::Type::OpenCurlyBracket:        return L"OpenCurlyBracket";
            case Token::Type::CloseCurlyBracket:       return L"CloseCurlyBracket";
            case Token::Type::OpenBracket:      return L"OpenBracket";
            case Token::Type::CloseBracket:     return L"CloseBracket";

            case Token::Type::Preprocessor:     return L"Preprocessor";
            case Token::Type::Include:     		return L"Include";

            case Token::Type::Comment:          return L"Comment";
            case Token::Type::Whitespace:       return L"Whitespace";
            case Token::Type::EndOfFile:        return L"EndOfFile";
            case Token::Type::Unknown:          return L"Unknown";
            default:                  return L"(Invalid)";
        }
    }
	std::string to_string(Token::Type p_type)
	{
		return (spk::StringUtils::wstringToString(to_wstring(p_type)));
	}
}