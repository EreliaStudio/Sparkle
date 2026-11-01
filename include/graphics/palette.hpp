#pragma once

#include <functional>
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
		public:
			using Validator = std::function<void(std::string_view, const Material &, const Materials &)>;

		private:
			Materials _materials;
			Validator _validator;

		public:
			explicit Builder(Validator validator = {});
			explicit Builder(const Palette &palette, Validator validator = {});

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
