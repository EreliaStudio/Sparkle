#pragma once

#include <string>
#include <vector>

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class MaterialLayout final
	{
	private:
		std::vector<std::string> _names;

	public:
		MaterialLayout() = default;
		explicit MaterialLayout(std::vector<std::string> names);

		[[nodiscard]] const std::vector<std::string> &names() const noexcept;
		[[nodiscard]] bool isCompatible(const spk::Palette &palette) const noexcept;
	};
}
