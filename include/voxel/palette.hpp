#pragma once

#include <set>
#include <string>
#include <string_view>

#include "graphics/palette.hpp"

namespace spk::Voxel
{
	class Model;

	class Palette final
	{
	public:
		class Builder
		{
		private:
			std::set<std::string, std::less<>> _required;
			spk::Palette::Builder _materials;

		public:
			Builder() = default;
			explicit Builder(const Model &model);
			explicit Builder(const Palette &palette);

			void addModel(const Model &model);
			void setMaterial(std::string_view name, spk::Material material);
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
