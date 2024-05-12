#include "graphics/texture/font/spk_font.hpp"

namespace spk
{
	Font::Key::Key(const size_t &p_fontSize, const size_t &p_outlineSize, const OutlineStyle &p_outlineStyle) :
		fontSize(p_fontSize),
		outlineSize(p_outlineSize),
		outlineStyle(p_outlineStyle)
	{
		if (p_outlineStyle == Font::OutlineStyle::None)
			outlineSize = 0;
	}

	bool Font::Key::operator<(const Key &p_other) const
	{
		if (fontSize < p_other.fontSize)
			return (true);
		if (fontSize == p_other.fontSize && outlineSize < p_other.outlineSize)
			return (true);
		if (fontSize == p_other.fontSize && outlineSize == p_other.outlineSize && static_cast<int>(outlineStyle) < static_cast<int>(p_other.outlineStyle))
			return (true);
		return (false);
	}
}