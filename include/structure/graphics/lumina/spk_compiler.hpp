#pragma once

#include <string>
#include <filesystem>

#include "structure/graphics/lumina/spk_shader.hpp"

namespace spk::Lumina
{
	class Compiler
	{
	private:
		std::vector<std::filesystem::path> _includeFolderPaths;

		static std::wstring cleanUpSource(const std::wstring& p_source);

	public:
		void addIncludePath(const std::filesystem::path& p_path);

		Shader compile(const std::wstring &p_source);
	};
}