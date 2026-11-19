#pragma once

#include "graphics/palette.hpp"

namespace spk::Voxel::Palette
{
	class Builder final : public spk::Palette::Builder
	{
	public:
		Builder();
		explicit Builder(const spk::Palette &palette);
	};
}
