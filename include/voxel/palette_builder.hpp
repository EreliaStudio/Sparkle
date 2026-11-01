#pragma once

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class PaletteBuilder final : public spk::Palette::Builder
	{
	public:
		PaletteBuilder();
		explicit PaletteBuilder(const spk::Palette &palette);
	};
}
