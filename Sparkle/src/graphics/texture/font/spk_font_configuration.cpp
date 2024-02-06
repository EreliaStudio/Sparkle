#include "graphics/texture/font/spk_font.hpp"

namespace spk
{

	Font::Configuration::Configuration() : _fileName("Uninitialized")
	{

	}

	Font::Configuration::Configuration(const std::string &p_fileName, const std::vector<uint8_t> &p_fontData) : _fileName(p_fileName)
	{
		_computeGlyphInformation(p_fontData);
	}

	const std::string &Font::Configuration::fileName() const
	{
		return (_fileName);
	}

	const std::vector<uint8_t>& Font::Configuration::validGlyphs() const
	{
		return (_validGlyphs);
	}
}