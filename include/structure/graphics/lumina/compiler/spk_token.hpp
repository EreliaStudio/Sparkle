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
			Input,
			VertexPass,
			FragmentPass,
			Output,
			RaiseException,
			If,
			Else,
			For,
			While,
			Return,
			Discard,

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

			LeftParen,
			RightParen,
			LeftBrace,
			RightBrace,
			LeftBracket,
			RightBracket,

			Preprocessor,

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