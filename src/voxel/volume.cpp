#include "voxel/volume.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <limits>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace
{
	using PackedCell = spk::Voxel::Volume::PackedCell;
	using Registry = spk::Voxel::Definition::Registry;

	constexpr std::uint32_t VolumeStreamMagic = 0x564B5053U; // "SPKV" in little-endian
	constexpr std::uint32_t VolumeStreamVersion = 1;
	constexpr std::size_t VolumeStreamHeaderBytes = 20;

	void writeWord(std::span<std::byte> bytes, std::size_t offset, std::uint32_t value) noexcept
	{
		for (std::size_t i = 0; i < 4; ++i)
		{
			bytes[offset + i] = std::byte((value >> (8 * i)) & 0xFFU);
		}
	}

	[[nodiscard]] std::uint32_t readWord(std::span<const std::byte> bytes, std::size_t offset) noexcept
	{
		std::uint32_t result = 0;
		for (std::size_t i = 0; i < 4; ++i)
		{
			result |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes[offset + i])) << (8 * i);
		}
		return result;
	}

	std::size_t checkedCellCount(spk::Vector3UInt dimensions)
	{
		if (dimensions.x == 0 || dimensions.y == 0 || dimensions.z == 0)
		{
			throw spk::Exception("Voxel Volume dimensions must be positive");
		}
		std::size_t count = 1;
		for (const auto dimension : std::array{dimensions.x, dimensions.y, dimensions.z})
		{
			if (dimension > std::numeric_limits<std::size_t>::max() / count)
			{
				throw spk::Exception("Voxel Volume dimensions overflow the cell count");
			}
			count *= dimension;
		}
		if (count > std::vector<PackedCell::Storage>{}.max_size())
		{
			throw spk::Exception("Voxel Volume dimensions exceed the storage capacity");
		}
		return count;
	}

	std::size_t cellOffset(spk::Vector3UInt position, spk::Vector3UInt dimensions)
	{
		if (position.x >= dimensions.x || position.y >= dimensions.y || position.z >= dimensions.z)
		{
			throw spk::Exception("Voxel Volume cell position is outside its dimensions");
		}
		return (static_cast<std::size_t>(position.z) * dimensions.y + position.y) * dimensions.x + position.x;
	}

	std::uint32_t readPackedWord(std::ifstream &input)
	{
		std::array<unsigned char, 4> bytes{};
		if (!input.read(reinterpret_cast<char *>(bytes.data()), bytes.size()))
		{
			throw spk::Exception("Voxel Volume packed file ended unexpectedly");
		}
		return std::uint32_t(bytes[0]) |
			   (std::uint32_t(bytes[1]) << 8) |
			   (std::uint32_t(bytes[2]) << 16) |
			   (std::uint32_t(bytes[3]) << 24);
	}

	void validatePackedFileSize(const std::filesystem::path &filename, std::size_t cells)
	{
		if (cells > std::numeric_limits<std::uintmax_t>::max() / 4)
		{
			throw spk::Exception("Voxel Volume packed file size overflows");
		}
		std::error_code error;
		const auto size = std::filesystem::file_size(filename, error);
		if (error || size != cells * 4)
		{
			throw spk::Exception("Voxel Volume packed file size does not match dimensions");
		}
	}

	std::vector<PackedCell::Storage> readPackedFile(
		const std::filesystem::path &filename,
		std::size_t count,
		const Registry &registry)
	{
		validatePackedFileSize(filename, count);
		std::ifstream input(filename, std::ios::binary);
		if (!input)
		{
			throw spk::Exception("Could not open Voxel Volume packed file");
		}
		std::vector<PackedCell::Storage> cells(count);
		for (auto &packed : cells)
		{
			packed = readPackedWord(input);
			(void)registry.at(PackedCell::definitionId(packed));
		}
		if (input.peek() != std::char_traits<char>::eof())
		{
			throw spk::Exception("Voxel Volume packed file contains trailing bytes");
		}
		return cells;
	}
}

namespace spk::Voxel
{
	Volume::Volume(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	spk::Vector3UInt Volume::dimensions() const noexcept
	{
		return _data->dimensions;
	}

	Volume::PackedCell::Storage Volume::atRaw(spk::Vector3UInt position) const
	{
		return _data->cells[cellOffset(position, _data->dimensions)];
	}

	std::span<const Volume::PackedCell::Storage> Volume::packedCells() const noexcept
	{
		return _data->cells;
	}

	Cell Volume::at(spk::Vector3UInt position) const
	{
		const auto packed = atRaw(position);
		return Cell(
			_data->definitions[PackedCell::definitionId(packed)],
			PackedCell::horizontal(packed),
			PackedCell::vertical(packed));
	}

	Cell Volume::operator[](spk::Vector3UInt position) const
	{
		return at(position);
	}

	void Volume::Builder::_ensureActive() const
	{
		if (_consumed)
		{
			throw spk::Exception("Voxel Volume Builder has already been consumed");
		}
	}

	Volume::Builder::Builder(spk::Vector3UInt dimensions) :
		_dimensions(dimensions),
		_cells(checkedCellCount(dimensions), PackedCell::Air)
	{
	}

	void Volume::Builder::load(const Volume &volume)
	{
		_ensureActive();
		const auto dimensions = volume.dimensions();
		if (dimensions.x != _dimensions.x || dimensions.y != _dimensions.y || dimensions.z != _dimensions.z)
		{
			throw spk::Exception("Voxel Volume Builder cannot load a Volume with different dimensions");
		}
		auto cells = volume._data->cells;
		_registry = volume._data->definitions;
		_cells.swap(cells);
	}

	void Volume::Builder::load(const spk::ByteStream::Reader &reader)
	{
		_ensureActive();
		const auto data = reader.data().subspan(reader.readOffset());
		if (data.size() < VolumeStreamHeaderBytes)
		{
			throw spk::Exception("Voxel Volume stream header is truncated");
		}
		if (readWord(data, 0) != VolumeStreamMagic || readWord(data, 4) != VolumeStreamVersion)
		{
			throw spk::Exception("Voxel Volume stream magic or version is unsupported");
		}
		const spk::Vector3UInt dimensions{readWord(data, 8), readWord(data, 12), readWord(data, 16)};
		if (dimensions != _dimensions)
		{
			throw spk::Exception("Voxel Volume stream dimensions do not match Builder dimensions");
		}
		if (_cells.size() > (std::numeric_limits<std::size_t>::max() - VolumeStreamHeaderBytes) / 4)
		{
			throw spk::Exception("Voxel Volume stream size overflows");
		}
		const auto length = VolumeStreamHeaderBytes + _cells.size() * 4;
		if (data.size() < length)
		{
			throw spk::Exception("Voxel Volume stream cells are truncated");
		}
		std::vector<PackedCell::Storage> decoded(_cells.size());
		for (std::size_t i = 0; i < decoded.size(); ++i)
		{
			const auto packed = readWord(data, VolumeStreamHeaderBytes + i * 4);
			if (!_registry.contains(PackedCell::definitionId(packed)))
			{
				throw spk::Exception("Voxel Volume stream contains an unknown Definition ID");
			}
			decoded[i] = packed;
		}
		_cells.swap(decoded);
		reader.skip(length);
	}

	void Volume::Builder::bind(const Definition::Registry &registry)
	{
		_ensureActive();
		for (const auto packed : _cells)
		{
			const auto id = PackedCell::definitionId(packed);
			if (id == 0)
			{
				continue;
			}
			const auto *current = _registry.tryGet(id);
			const auto *replacement = registry.tryGet(id);
			if (!current || !replacement || !(*current == *replacement))
			{
				throw spk::Exception("Voxel Volume Registry rebind would change Definition ID " + std::to_string(id));
			}
		}
		_registry = registry;
	}

	void Volume::Builder::set(
		spk::Vector3UInt position,
		std::uint32_t definitionId,
		Cell::HorizontalOrientation horizontal,
		Cell::VerticalOrientation vertical)
	{
		_ensureActive();
		const auto offset = cellOffset(position, _dimensions);
		const auto packed = PackedCell::encode(definitionId, horizontal, vertical);
		(void)_registry.at(definitionId);
		_cells[offset] = packed;
	}

	void Volume::Builder::erase(spk::Vector3UInt position)
	{
		_ensureActive();
		_cells[cellOffset(position, _dimensions)] = PackedCell::Air;
	}

	void Volume::Builder::loadPacked(const std::filesystem::path &filename)
	{
		_ensureActive();
		auto decoded = readPackedFile(filename, _cells.size(), _registry);
		_cells.swap(decoded);
	}

	Volume Volume::Builder::build()
	{
		_ensureActive();
		auto data = std::make_shared<Data>();
		data->dimensions = _dimensions;
		data->definitions = _registry;
		data->cells = std::move(_cells);
		_consumed = true;
		return Volume(std::move(data));
	}

	spk::ByteStream::Writer &operator<<(spk::ByteStream::Writer &writer, const Volume &volume)
	{
		const auto &cells = volume._data->cells;
		if (cells.size() > (std::numeric_limits<std::size_t>::max() - VolumeStreamHeaderBytes) / 4)
		{
			throw spk::Exception("Voxel Volume stream size overflows");
		}
		const auto length = VolumeStreamHeaderBytes + cells.size() * 4;
		if (length > std::numeric_limits<std::size_t>::max() - writer.size())
		{
			throw spk::Exception("Voxel Volume stream append overflows");
		}
		const auto start = writer.size();
		writer.resize(start + length);
		const auto data = writer.writableData().subspan(start, length);
		writeWord(data, 0, VolumeStreamMagic);
		writeWord(data, 4, VolumeStreamVersion);
		writeWord(data, 8, volume._data->dimensions.x);
		writeWord(data, 12, volume._data->dimensions.y);
		writeWord(data, 16, volume._data->dimensions.z);
		for (std::size_t i = 0; i < cells.size(); ++i)
		{
			writeWord(data, VolumeStreamHeaderBytes + i * 4, cells[i]);
		}
		return writer;
	}

	spk::Message::Writer &operator<<(spk::Message::Writer &writer, const Volume &volume)
	{
		writer.payload() << volume;
		return writer;
	}

	const spk::ByteStream::Reader &operator>>(const spk::ByteStream::Reader &reader, Volume::Builder &builder)
	{
		builder.load(reader);
		return reader;
	}
}
