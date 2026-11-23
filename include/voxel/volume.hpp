#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

#include "container/byte_stream.hpp"
#include "math/vector3.hpp"
#include "network/message.hpp"
#include "voxel/cell.hpp"

namespace spk::Voxel
{
	class Volume
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
			Definition::Registry _registry;
			bool _consumed = false;

			void _ensureActive() const;

		public:
			explicit Builder(spk::Vector3UInt dimensions);
			void load(const Volume &volume);
			void load(const spk::ByteStream::Reader &reader);
			void bind(const Definition::Registry &registry);
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
			Definition::Registry definitions;
			std::vector<PackedCell::Storage> cells;
		};

		std::shared_ptr<const Data> _data;

		explicit Volume(std::shared_ptr<const Data> data) noexcept;
		friend spk::ByteStream::Writer &operator<<(spk::ByteStream::Writer &writer, const Volume &volume);

	public:
		[[nodiscard]] spk::Vector3UInt dimensions() const noexcept;
		[[nodiscard]] Cell at(spk::Vector3UInt position) const;
		[[nodiscard]] Cell operator[](spk::Vector3UInt position) const;
		[[nodiscard]] PackedCell::Storage atRaw(spk::Vector3UInt position) const;
		[[nodiscard]] std::span<const PackedCell::Storage> packedCells() const noexcept;
	};

	// A self-contained voxel payload, independent of Message headers and transport.
	// Uses a versioned little-endian encoding of dimensions and packed cells.
	spk::ByteStream::Writer &operator<<(spk::ByteStream::Writer &writer, const Volume &volume);
	spk::Message::Writer &operator<<(spk::Message::Writer &writer, const Volume &volume);
	const spk::ByteStream::Reader &operator>>(const spk::ByteStream::Reader &reader, Volume::Builder &builder);
}
