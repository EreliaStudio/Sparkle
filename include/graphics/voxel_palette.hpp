#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "graphics/material.hpp"
#include "voxel/model.hpp"

namespace spk
{
	class VoxelPalette final
	{
	public:
		struct Slot
		{
			float opacity = 1.0f;
			Material material;
		};

		using Slots = std::map<std::string, Slot, std::less<>>;

		class Builder
		{
		private:
			struct Entry
			{
				float opacity = 1.0f;
				std::optional<Material> material;
			};

			std::map<std::string, Entry, std::less<>> _slots;

		public:
			Builder() = default;
			explicit Builder(const Voxel::Model &model);
			explicit Builder(const VoxelPalette &palette);

			void declareSlot(std::string name, float opacity);
			void addModel(const Voxel::Model &model);
			void setMaterial(std::string_view name, Material material);
			[[nodiscard]] VoxelPalette build() const;
		};

	private:
		std::shared_ptr<const Slots> _slots;

		explicit VoxelPalette(Slots slots);

	public:
		VoxelPalette();

		[[nodiscard]] const Slots &slots() const noexcept;
		[[nodiscard]] const Slot *find(std::string_view name) const noexcept;
	};
}
