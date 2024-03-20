#include "graphics/texture/font/spk_font.hpp"
#include <array>

namespace spk
{
	Font::Atlas::Atlas()
	{

	}

	std::map<spk::Vector2Int, unsigned char> populateFadeValueMap(const int& p_outlineSize)
	{
		std::map<spk::Vector2Int, unsigned char> result;

		for (int x = -p_outlineSize; x <= p_outlineSize; x++)
		{
			for (int y = -p_outlineSize; y <= p_outlineSize; y++)
			{
				int distance = std::sqrt(x * x + y * y);
				if (distance > p_outlineSize)
					continue;

				float powerFactor = 3.0f; // Example value; adjust based on desired effect
				float normalizedDistance = static_cast<float>(distance) / static_cast<float>(p_outlineSize);
				float fadeFactor = 1 - std::pow(normalizedDistance, powerFactor);

				unsigned char value = static_cast<unsigned char>(255 * fadeFactor);

				result[spk::Vector2Int(x, y)] = value;
			}
		}

		return (result);
	}
	
	void Font::Atlas::computeOutlineBuffer(BuildData& p_buildData, const Key &p_key)
	{
		auto fadeValueMap = populateFadeValueMap(p_key.outlineSize);
		p_buildData.outlineBuffer.resize(p_buildData.size.x * p_buildData.size.y, 0);

		std::vector<spk::Vector2Int> borderPixels;
		for (int i = 0; i < p_buildData.size.x; i++)
		{
			for (int j = 0; j < p_buildData.size.y; j++)
			{
				if (p_buildData.fontBuffer[i + j * p_buildData.size.x] != EMPTY_PIXEL)
				{
					bool isBorder = false;
					for (int x = -1; x <= 1 && !isBorder; x++)
					{
						for (int y = -1; y <= 1; y++)
						{
							if (x == 0 && y == 0) continue;

							int checkX = i + x;
							int checkY = j + y;

							if (checkX >= 0 && checkX < p_buildData.size.x && checkY >= 0 && checkY < p_buildData.size.y)
							{
								if (p_buildData.fontBuffer[checkX + checkY * p_buildData.size.x] == EMPTY_PIXEL)
								{
									isBorder = true;
									break;
								}
							}
						}
					}

					if (isBorder)
					{
						borderPixels.emplace_back(i, j);
					}
				}
			}
		}

		for (auto& borderPixel : borderPixels)
		{
			for (auto& [offset, value] : fadeValueMap)
			{
				int index = (borderPixel.x + offset.x) + (borderPixel.y + offset.y) * p_buildData.size.x;
				if (index >= 0 && index < p_buildData.outlineBuffer.size() && p_buildData.fontBuffer[index] != CHAR_PIXEL)
				{
					p_buildData.outlineBuffer[index] = std::max(p_buildData.outlineBuffer[index], value);
				}
			}
		}
	}
	
	void Font::Atlas::pushCombinedTexture(BuildData& p_buildData)
	{
		std::vector<uint8_t> textureBuffer;

		textureBuffer.resize((p_buildData.fontBuffer.size() + 1) * 2);
	
		for (size_t i = 0; i < p_buildData.fontBuffer.size(); i++)
		{
			textureBuffer[i * 2] = p_buildData.fontBuffer[i];
			textureBuffer[i * 2 + 1] = p_buildData.outlineBuffer[i];
		}
		
		_texture.uploadToGPU(
			textureBuffer.data(), p_buildData.size,
			Texture::Format::DualChannel, Texture::Filtering::Linear,
			Texture::Wrap::Repeat, Texture::Mipmap::Enable);
	}

	Font::Atlas::Atlas(const std::vector<uint8_t> &p_fontData, const Configuration &p_fontConfiguration, const Key &p_key) :
		_glyphDatas(p_fontConfiguration.validGlyphs().back() + 1)
	{
		Font::Atlas::BuildData buildData = _computeBuildData(p_fontData, p_fontConfiguration, p_key);

		computeOutlineBuffer(buildData, p_key);

		pushCombinedTexture(buildData);
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