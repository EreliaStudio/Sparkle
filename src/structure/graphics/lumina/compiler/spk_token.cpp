#include "structure/graphics/lumina/compiler/spk_token.hpp"

#include "utils/spk_string_utils.hpp"

namespace spk::Lumina
{
	std::wstring to_wstring(Token::Type p_type)
    {
        using T = Token::Type;
        switch (p_type)
        {
            case T::Identifier:       return L"Identifier";
            case T::Struct:           return L"Struct";
            case T::Namespace:        return L"Namespace";
            case T::AttributeBlock:   return L"AttributeBlock";
            case T::ConstantBlock:    return L"ConstantBlock";
            case T::Texture:          return L"Texture";
            case T::Input:            return L"Input";
            case T::VertexPass:       return L"VertexPass";
            case T::FragmentPass:     return L"FragmentPass";
            case T::Output:           return L"Output";
            case T::RaiseException:   return L"RaiseException";
            case T::If:               return L"If";
            case T::Else:             return L"Else";
            case T::For:              return L"For";
            case T::While:            return L"While";
            case T::Return:           return L"Return";
            case T::Discard:          return L"Discard";

            case T::NumberLiteral:    return L"NumberLiteral";
            case T::BoolLiteral:      return L"BoolLiteral";
            case T::StringLiteral:    return L"StringLiteral";

            case T::Plus:             return L"Plus";
            case T::Minus:            return L"Minus";
            case T::Multiply:         return L"Multiply";
            case T::Divide:           return L"Divide";
            case T::Percent:          return L"Percent";
            case T::PlusEqual:        return L"PlusEqual";
            case T::MinusEqual:       return L"MinusEqual";
            case T::MultiplyEqual:    return L"MultiplyEqual";
            case T::DivideEqual:      return L"DivideEqual";
            case T::PercentEqual:     return L"PercentEqual";
            case T::PlusPlus:         return L"PlusPlus";
            case T::MinusMinus:       return L"MinusMinus";

            case T::EqualEqual:       return L"EqualEqual";
            case T::Different:        return L"Different";
            case T::Less:             return L"Less";
            case T::Greater:          return L"Greater";
            case T::LessEqual:        return L"LessEqual";
            case T::GreaterEqual:     return L"GreaterEqual";
            case T::AndAnd:           return L"AndAnd";
            case T::OrOr:             return L"OrOr";
            case T::Bang:             return L"Bang";

            case T::Equal:            return L"Equal";
            case T::Dot:              return L"Dot";
            case T::Arrow:            return L"Arrow";
            case T::DoubleColon:      return L"DoubleColon";

            case T::Comma:            return L"Comma";
            case T::Semicolon:        return L"Semicolon";
            case T::Colon:            return L"Colon";
            case T::Question:         return L"Question";

            case T::LeftParen:        return L"LeftParen";
            case T::RightParen:       return L"RightParen";
            case T::LeftBrace:        return L"LeftBrace";
            case T::RightBrace:       return L"RightBrace";
            case T::LeftBracket:      return L"LeftBracket";
            case T::RightBracket:     return L"RightBracket";

            case T::Preprocessor:     return L"Preprocessor";

            case T::Comment:          return L"Comment";
            case T::Whitespace:       return L"Whitespace";
            case T::EndOfFile:        return L"EndOfFile";
            case T::Unknown:          return L"Unknown";
            default:                  return L"(Invalid)";
        }
    }
	std::string to_string(Token::Type p_type)
	{
		return (spk::StringUtils::wstringToString(to_wstring(p_type)));
	}
}