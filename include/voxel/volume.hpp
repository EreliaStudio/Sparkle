#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <vector>

#include "body.hpp"
#include "math/vector3.hpp"
#include "voxel/cell.hpp"

namespace spk::Voxel
{
	class Volume : public spk::Body
	{
	public:
		struct PackedCell
		{
			using Storage = std::uint32_t;
			static constexpr Storage Air = 0;
			static constexpr Storage MaxDefinitionId = (Storage{1} << 29) - 1;

			[[nodiscard]] static Storage encode(
				std::size_t id,
				Cell::HorizontalOrientation horizontal,
				Cell::VerticalOrientation vertical);
			[[nodiscard]] static Storage definitionId(Storage value) noexcept;
			[[nodiscard]] static Cell::HorizontalOrientation horizontal(Storage value) noexcept;
			[[nodiscard]] static Cell::VerticalOrientation vertical(Storage value) noexcept;
		};

		class Builder
		{
		private:
			spk::Vector3UInt _dimensions;
			std::vector<std::uint32_t> _cells;
			Definition::Collection _collection;
			bool _consumed = false;

			void _ensureActive() const;

		public:
			explicit Builder(spk::Vector3UInt dimensions);
			void copyFrom(const Volume &volume);
			void bind(const Definition::Collection &collection);
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
		struct Data
		{
			spk::Vector3UInt dimensions;
			Definition::Collection definitions;
			std::vector<PackedCell::Storage> cells;
		};

		std::shared_ptr<const Data> _data;

		explicit Volume(std::shared_ptr<const Data> data) noexcept;

	public:
		[[nodiscard]] spk::Vector3UInt dimensions() const noexcept;
		[[nodiscard]] Cell at(spk::Vector3UInt position) const;
		[[nodiscard]] Cell operator[](spk::Vector3UInt position) const;
		[[nodiscard]] PackedCell::Storage atRaw(spk::Vector3UInt position) const;
	};
}
