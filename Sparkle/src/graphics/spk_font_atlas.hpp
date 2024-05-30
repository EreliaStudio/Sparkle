#pragma once

#include <unordered_map>
#include <string>
#include <filesystem>

#include "design_pattern/spk_singleton.hpp"
#include "graphics/texture/font/spk_font.hpp"

namespace spk
{
	/**
	 * @class FontAtlas
	 * @brief Manages the loading and accessing of font resources.
	 *
	 * FontAtlas is designed to handle the creation and storage of font resources, ensuring that
	 * fonts are loaded only once and reused throughout the application. It uses a hash map to store
	 * font instances associated with their names. The class provides functionality to load new fonts
	 * from files and retrieve existing font instances by name.
	 *
	 * This class is implemented as a part of the Singleton design pattern, ensuring that there is a single
	 * instance of FontAtlas throughout the application.
	 *
	 * Usage example:
	 * @code
	 * spk::Font* myFont = FontAtlas::instance().loadFont("OpenSans", "path/to/OpenSans.ttf");
	 * spk::Font* secondPoint = FontAtlas::instance().font("OpenSans");
	 * @endcode
	 */
	class FontAtlas
	{
	private:
		std::unordered_map<std::string, Font*> _loadedFonts; // Stores loaded font resources, mapped by their names.

	public:
		/**
		 * Constructor for the FontAtlas class. Initializes an empty font storage.
		 */
		FontAtlas();

		/**
		 * Destructor for the FontAtlas class. Ensures proper cleanup of all loaded fonts.
		 */
		~FontAtlas();

		/**
		 * Loads a font from a specified file path and registers it under a given name.
		 * If the font is already loaded, returns the existing instance instead of reloading it.
		 * @param p_fontName The name to associate with the font.
		 * @param p_path The filesystem path to the font file.
		 * @return Pointer to the loaded Font instance.
		 */
		Font* loadFont(const std::string& p_fontName, const std::filesystem::path& p_path);

		/**
		 * Retrieves a font instance by its name. If the font is not found, returns nullptr.
		 * @param p_fontName The name of the font to retrieve.
		 * @return Pointer to the Font instance if found, nullptr otherwise.
		 */
		Font* font(const std::string& p_fontName) const;
	};  
}
