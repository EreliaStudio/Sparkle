#include "voxel/volume.hpp"

#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "exception.hpp"

namespace
{
	using PackedCell = spk::Voxel::Volume::PackedCell;
	using Registry = spk::Voxel::Definition::Registry;

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

	Volume::Builder::Builder(spk::Vector3UInt dimensions) :
		_dimensions(dimensions),
		_cells(checkedCellCount(dimensions), PackedCell::Air)
	{
	}

	Volume::Builder::Builder(spk::Vector3UInt dimensions, const Definition::Registry &registry) :
		Builder(dimensions)
	{
		bind(registry);
	}

	Volume::Builder::Builder(const Volume &volume) :
		_dimensions(volume._data->dimensions),
		_cells(volume._data->cells),
		_registry(volume._data->definitions)
	{
	}

	Volume::Builder::Builder(
		spk::Vector3UInt dimensions,
		const std::filesystem::path &filename,
		const Definition::Registry &registry) :
		Builder(dimensions, registry)
	{
		loadPacked(filename);
	}

	void Volume::Builder::_ensureActive() const
	{
		if (_consumed)
		{
			throw spk::Exception("Voxel Volume Builder has already been consumed");
		}
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
}
