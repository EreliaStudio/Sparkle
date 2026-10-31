#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "exception.hpp"
#include "math/vector3.hpp"
#include "voxel/cell.hpp"
#include "voxel/definition_registry.hpp"

namespace spk::Voxel
{
	struct VolumeTestAccess;

	class Volume
	{
	private:
		struct Data;
		friend struct VolumeTestAccess;

	public:
	struct PackedCell
	{
		using Storage = std::uint32_t;
		static constexpr Storage Air = 0;
		static constexpr Storage MaxDefinitionIndex = (Storage{1} << 29) - 1;

		[[nodiscard]] static Storage encode(
			std::size_t index,
			Cell::HorizontalOrientation horizontal,
			Cell::VerticalOrientation vertical)
		{
			const auto degrees = static_cast<std::uint32_t>(horizontal);
			if (degrees > 270 || degrees % 90 != 0 ||
				(vertical != Cell::VerticalOrientation::YPositive && vertical != Cell::VerticalOrientation::YNegative))
			{
				throw spk::Exception("Invalid Voxel Volume cell orientation");
			}
			if (index > MaxDefinitionIndex)
			{
				throw spk::Exception("Voxel Volume Definition index exceeds 29-bit capacity");
			}
			return (static_cast<Storage>(index) << 3) |
				   (static_cast<Storage>(vertical == Cell::VerticalOrientation::YNegative) << 2) |
				   (degrees / 90);
		}

		[[nodiscard]] static constexpr Storage definitionIndex(Storage value) noexcept
		{
			return value >> 3;
		}

		[[nodiscard]] static constexpr Cell::HorizontalOrientation horizontal(Storage value) noexcept
		{
			return static_cast<Cell::HorizontalOrientation>((value & 3U) * 90);
		}

		[[nodiscard]] static constexpr Cell::VerticalOrientation vertical(Storage value) noexcept
		{
			return (value & 4U) ? Cell::VerticalOrientation::YNegative : Cell::VerticalOrientation::YPositive;
		}
	};

		class Builder
		{
		private:
			friend struct VolumeTestAccess;
			spk::Vector3UInt _dimensions;
			std::vector<std::uint32_t> _cells;
			std::vector<Definition> _definitions;
			std::shared_ptr<const DefinitionRegistry> _registry;
			bool _consumed = false;

			void _ensureActive() const;
			[[nodiscard]] std::size_t _definitionIndex(const Definition &definition) const noexcept;

		public:
			explicit Builder(spk::Vector3UInt dimensions);
			explicit Builder(const Volume &volume);
			Builder(
				spk::Vector3UInt dimensions,
				const std::filesystem::path &filename,
				const DefinitionRegistry &registry);

			void bind(const DefinitionRegistry &registry);
			void set(spk::Vector3UInt position, Cell cell);
			void set(
				spk::Vector3UInt position,
				std::uint32_t definitionId,
				Cell::HorizontalOrientation horizontal = Cell::HorizontalOrientation::XPositive,
				Cell::VerticalOrientation vertical = Cell::VerticalOrientation::YPositive);
			void erase(spk::Vector3UInt position);
			void loadPacked(const std::filesystem::path &filename);
			[[nodiscard]] Volume build();
		};

	private:
		std::shared_ptr<const Data> _data;

		explicit Volume(std::shared_ptr<const Data> data) noexcept;

	public:
		[[nodiscard]] spk::Vector3UInt dimensions() const noexcept;
		[[nodiscard]] Cell cell(spk::Vector3UInt position) const;
	};
}
