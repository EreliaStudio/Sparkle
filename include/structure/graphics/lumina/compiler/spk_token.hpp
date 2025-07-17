#pragma once

#include <filesystem>
#include <string>

#include "structure/graphics/lumina/compiler/spk_location.hpp"

namespace spk::Lumina
{
	struct Token
	{
		enum class Type
		{
			Identifier,

			Struct,
			Namespace,
			AttributeBlock,
			ConstantBlock,
			Texture,
			ShaderPass,
			RaiseException,
			If,
			Else,
			For,
			While,
			Return,
                        Discard,
                        Operator,

			NumberLiteral,
			BoolLiteral,
			StringLiteral,

			Plus,
			Minus,
			Multiply,
			Divide,
			Percent,
			PlusEqual,
			MinusEqual,
			MultiplyEqual,
			DivideEqual,
			PercentEqual,
			PlusPlus,
			MinusMinus,

			EqualEqual,
			Different,
			Less,
			Greater,
			LessEqual,
			GreaterEqual,
			AndAnd,
			OrOr,
			Bang,

			Equal,
			Dot,
			Arrow,
			DoubleColon,

			Comma,
			Semicolon,
			Colon,
			Question,

			OpenParenthesis,
			CloseParenthesis,
			OpenCurlyBracket,
			CloseCurlyBracket,
			OpenBracket,
			CloseBracket,

			Preprocessor,
			Include,

			Comment,
			Whitespace,
			EndOfFile,
			Unknown
		};

		Type type;
		std::wstring lexeme;
		Location location;
	};

	std::wstring to_wstring(Token::Type type);
	std::string to_string(Token::Type type);
}