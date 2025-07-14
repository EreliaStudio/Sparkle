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
}
