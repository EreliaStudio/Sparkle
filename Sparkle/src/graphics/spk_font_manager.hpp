#pragma once

#include <unordered_map>
#include <string>

#include "design_pattern/spk_singleton.hpp"
#include "graphics/texture/font/spk_font.hpp"

namespace spk
{
	class FontManager
	{
	private:
		std::unordered_map<std::string, Font*> _loadedFont;

	public:
		FontManager();
		~FontManager();
		
		Font* loadFont(const std::string& p_fontName,
				const std::filesystem::path& p_path);

		Font* font(const std::string& p_fontName) const;
	};	
}