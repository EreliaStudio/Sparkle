#include "graphics/texture/font/spk_font.hpp"
#include <array>

#include "unordered_map"

namespace spk
{
	Font::Atlas::Atlas()
	{

	}
	
	void Font::Atlas::_pushCombinedTexture(BuildData& p_buildData)
	{
		_textureSize = p_buildData.size;
		_texturePixelData.resize((p_buildData.fontBuffer.size() + 1) * 2);
	
		for (size_t i = 0; i < p_buildData.fontBuffer.size(); i++)
		{
			_texturePixelData[i * 2] = p_buildData.fontBuffer[i];
			_texturePixelData[i * 2 + 1] = p_buildData.outlineBuffer[i];
		}
		_needUploadToGPU = true;
	}

	bool Font::Atlas::needUploadToGPU() const
	{
		return (_needUploadToGPU);
	}

	void Font::Atlas::uploadToGPU()
	{
		if (_needUploadToGPU == true)
		{
			_texture.uploadToGPU(
				_texturePixelData.data(), _textureSize,
				Texture::Format::DualChannel, Texture::Filtering::Linear,
				Texture::Wrap::Repeat, Texture::Mipmap::Enable);
			_needUploadToGPU = false;
		}
	}

	Font::Atlas::Atlas(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key) :
		_glyphDatas(p_fontConfiguration.validGlyphs().back() + 1)
	{
		Font::Atlas::BuildData buildData = _computeBuildData(p_fontData, p_fontConfiguration, p_key);

		_computeOutlineBuffer(buildData, p_key);

		_pushCombinedTexture(buildData);
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