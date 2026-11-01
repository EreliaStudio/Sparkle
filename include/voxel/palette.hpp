#pragma once

#include <optional>
#include <string>
#include <utility>

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class Palette final : public spk::Palette
	{
	public:
		class Builder final : public spk::Palette::Builder
		{
		private:
			std::optional<spk::GPUResource::Identifier> _atlas;

		public:
			Builder() = default;
			explicit Builder(const Palette &palette);

			void setMaterial(std::string name, spk::Material material);
			[[nodiscard]] Palette build() const;
		};

	private:
		explicit Palette(spk::Palette palette);

	public:
		Palette() = default;
	};
}
