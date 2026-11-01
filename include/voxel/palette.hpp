#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class Palette final
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
		spk::Palette _materials;

		explicit Palette(spk::Palette materials);

	public:
		Palette() = default;

		[[nodiscard]] const spk::Palette &materials() const noexcept;
		[[nodiscard]] const spk::Material *find(std::string_view name) const noexcept;
	};
}
