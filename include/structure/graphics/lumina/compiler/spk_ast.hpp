#pragma once

#include "structure/graphics/lumina/compiler/spk_location.hpp"

namespace spk::Lumina
{
	/**
	 * Base class for every syntactic-/semantic node produced by the front-end.
	 * All nodes carry their source Location for diagnostics.
	 */
	struct ASTNode
	{
		enum class Kind
		{
			Include
		};

		Location location;
		Kind kind;

		explicit ASTNode(Kind p_kind, Location p_location) :
			location(std::move(p_location)),
			kind(p_kind)
		{
		}
		virtual ~ASTNode() = default;
	};

	struct IncludeASTNode : public ASTNode
	{
		std::filesystem::path includeFile;

		IncludeASTNode(const std::wstring& p_path, Location p_location) :
			ASTNode(Kind::Include, p_location)
		{
			includeFile = p_path;
		}
	};
}