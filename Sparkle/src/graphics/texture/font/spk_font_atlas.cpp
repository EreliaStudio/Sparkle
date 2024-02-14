#include "graphics/texture/font/spk_font.hpp"
#include <array>

namespace spk
{
	void _applyOutline(std::vector<uint8_t> &p_atlasData, const spk::Vector2Int &p_atlasSize, const spk::Font::Key &p_key);

	Font::Atlas::Atlas()
	{
	}

	/**
	 * @brief Normalizes the atlas data.
	 *
	 * This function iterates through all pixels in the atlas and normalizes 
	 * their values. Any pixel not representing a character (`Font::CHAR_PIXEL`) 
	 * is set to be empty (`Font::EMPTY_PIXEL`). Pixels representing characters 
	 * remain unchanged. This ensures the atlas data contains only distinct 
	 * values corresponding to character pixels and empty pixels.
	 *
	 * @param p_atlasData The atlas data where pixels are stored.
	 * @param p_atlasSize The dimensions of the atlas.
	 */
	void _normalizeAtlasData(std::vector<uint8_t> &p_atlasData, const spk::Vector2Int &p_atlasSize)
	{
		size_t maxIndex = static_cast<size_t>(p_atlasSize.x * p_atlasSize.y);
		for (size_t i = 0; i < maxIndex; i++)
		{
			if (p_atlasData[i] != Font::CHAR_PIXEL)
				p_atlasData[i] = Font::EMPTY_PIXEL;
			else
				p_atlasData[i] = Font::CHAR_PIXEL;
		}
	}
	
	Font::Atlas::Atlas(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key) :
		_glyphDatas(p_fontConfiguration.validGlyphs().back() + 1)
	{
		Font::Atlas::BuildData buildData = _computeBuildData(p_fontData, p_fontConfiguration, p_key);

		_normalizeAtlasData(buildData.buffer, buildData.size);

		if (p_key.outlineSize != 0)
		{		
			if (p_key.outlineStyle == Font::OutlineStyle::Standard)
			{
				(const_cast<Key*>(&p_key))->computeCircle(buildData.size);
			}
		
			_applyOutline(buildData.buffer, buildData.size, p_key);
		}

		_texture.uploadToGPU(
			buildData.buffer.data(), buildData.size,
			Texture::Format::GreyLevel, Texture::Filtering::Linear,
			Texture::Wrap::Repeat, Texture::Mipmap::Disable);
	}

	std::string wstringToString(const std::wstring& wstr)
	{
		const size_t BUFF_SIZE = 7;
		std::string result;

		for (const wchar_t wc : wstr)
		{
			std::array<char, BUFF_SIZE> buffer;
			int retLength = 0;
			errno_t err = wctomb_s(&retLength, buffer.data(), buffer.size(), wc);

			if (err != 0)
			{
				throw std::runtime_error("Failed to convert wchar_t to char using wctomb_s.");
			}

			result.append(buffer.data(), retLength - 1);
		}
		return result;
	}

	const Font::Atlas::GlyphData &Font::Atlas::operator [](const wchar_t &p_char) const
	{
		if (_glyphDatas.size() < static_cast<size_t>(p_char))
		{
			std::string charStr = wstringToString(std::wstring(1, p_char));
			std::string errorMessage = "Char [" + charStr + "](dec : " + std::to_string(static_cast<size_t>(p_char)) + ") cannot be rendered : it doesn't exist in Font [" + _fontConfiguration.fileName() + "]";
			throwException(errorMessage);
		}
		return (_glyphDatas[static_cast<size_t>(p_char)]);
	}

	const Texture &Font::Atlas::texture() const
	{
		return (_texture);
	}
}