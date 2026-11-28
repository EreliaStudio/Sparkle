#pragma once

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class Geometry;
}

namespace spk::Voxel::Palette
{
	[[nodiscard]] bool isCompatible(const Geometry &geometry, const spk::Palette &palette) noexcept;

	class Builder final : public spk::Palette::Builder
	{
	public:
		Builder();
		explicit Builder(const spk::Palette &palette);
	};
}
