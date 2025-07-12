#pragma once

#include <string>
#include <filesystem>

#include "structure/graphics/lumina/compiler/spk_source_manager.hpp"
#include "structure/graphics/lumina/spk_shader.hpp"

namespace spk::Lumina
{
	class Compiler
	{
	private:
		SourceManager _sourceManager;

	public:
		void addIncludePath(const std::filesystem::path& p_path);

		Shader compile(const std::wstring &p_source);
	};
}