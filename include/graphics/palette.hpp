#pragma once

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "graphics/material.hpp"

namespace spk
{
	class Palette
	{
	public:
		using Materials = std::map<std::string, Material, std::less<>>;

		class Builder
		{
		private:
			Materials _materials;

		public:
			Builder() = default;
			explicit Builder(const Palette &palette);

			void setMaterial(std::string name, Material material);
			[[nodiscard]] Palette build() const;
		};

	private:
		std::shared_ptr<const Materials> _materials;

		explicit Palette(Materials materials);

	public:
		Palette();

		[[nodiscard]] const Materials &materials() const noexcept;
		[[nodiscard]] const Material *find(std::string_view name) const noexcept;
	};
}
