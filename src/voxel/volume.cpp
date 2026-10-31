#include "voxel/volume.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <limits>
#include <unordered_map>
#include <utility>

#include "exception.hpp"
#include "voxel/detail/packed_cell.hpp"

namespace
{
	using PackedCell = spk::Voxel::Detail::PackedCell;

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

	std::uint32_t localDefinitionIndex(
		std::vector<spk::Voxel::Definition> &definitions,
		const spk::Voxel::Definition &definition)
	{
		const auto found = std::find(definitions.begin(), definitions.end(), definition);
		const auto index = static_cast<std::size_t>(found - definitions.begin());
		if (index == definitions.size())
		{
			if (index > PackedCell::MaxDefinitionIndex)
			{
				throw spk::Exception("Voxel Volume Definition index exceeds 29-bit capacity");
			}
			definitions.push_back(definition);
		}
		return static_cast<std::uint32_t>(index);
	}

	struct DecodedCells
	{
		std::vector<std::uint32_t> cells;
		std::vector<spk::Voxel::Definition> definitions;
	};

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

	DecodedCells readPackedFile(
		const std::filesystem::path &filename,
		std::size_t count,
		const spk::Voxel::DefinitionRegistry &registry)
	{
		validatePackedFileSize(filename, count);
		std::ifstream input(filename, std::ios::binary);
		if (!input)
		{
			throw spk::Exception("Could not open Voxel Volume packed file");
		}
		DecodedCells decoded{std::vector<std::uint32_t>(count), {spk::Voxel::Definition::air()}};
		std::unordered_map<std::uint32_t, std::uint32_t> remapping;
		for (auto &entry : decoded.cells)
		{
			const auto packed = readPackedWord(input);
			const auto id = PackedCell::definitionIndex(packed);
			auto found = remapping.find(id);
			if (found == remapping.end())
			{
				const auto index = localDefinitionIndex(decoded.definitions, registry.at(id));
				found = remapping.emplace(id, index).first;
			}
			entry = PackedCell::encode(found->second, PackedCell::horizontal(packed), PackedCell::vertical(packed));
		}
		if (input.peek() != std::char_traits<char>::eof())
		{
			throw spk::Exception("Voxel Volume packed file contains trailing bytes");
		}
		return decoded;
	}

}

namespace spk::Voxel
{
	struct Volume::Data
	{
		spk::Vector3UInt dimensions;
		std::vector<Definition> definitions;
		std::vector<PackedCell::Storage> cells;
	};

	Volume::Volume(std::shared_ptr<const Data> data) noexcept :
		_data(std::move(data))
	{
	}

	spk::Vector3UInt Volume::dimensions() const noexcept
	{
		return _data->dimensions;
	}

	Cell Volume::cell(spk::Vector3UInt position) const
	{
		const auto packed = _data->cells[cellOffset(position, _data->dimensions)];
		return Cell(
			_data->definitions[PackedCell::definitionIndex(packed)],
			PackedCell::horizontal(packed),
			PackedCell::vertical(packed));
	}

	Volume::Builder::Builder(spk::Vector3UInt dimensions) :
		_dimensions(dimensions),
		_cells(checkedCellCount(dimensions), PackedCell::Air),
		_definitions{Definition::air()}
	{
	}

	Volume::Builder::Builder(const Volume &volume) :
		_dimensions(volume._data->dimensions),
		_cells(volume._data->cells),
		_definitions(volume._data->definitions)
	{
	}

	Volume::Builder::Builder(
		spk::Vector3UInt dimensions,
		const std::filesystem::path &filename,
		const DefinitionRegistry &registry) :
		Builder(dimensions)
	{
		bind(registry);
		loadPacked(filename);
	}

	void Volume::Builder::_ensureActive() const
	{
		if (_consumed)
		{
			throw spk::Exception("Voxel Volume Builder has already been consumed");
		}
	}

	std::size_t Volume::Builder::_definitionIndex(const Definition &definition) const noexcept
	{
		for (std::size_t index = 0; index < _definitions.size(); ++index)
		{
			if (_definitions[index] == definition)
			{
				return index;
			}
		}
		return _definitions.size();
	}

	void Volume::Builder::bind(const DefinitionRegistry &registry)
	{
		_ensureActive();
		_registry = std::make_shared<const DefinitionRegistry>(registry);
	}

	void Volume::Builder::set(spk::Vector3UInt position, Cell cell)
	{
		_ensureActive();
		const auto offset = cellOffset(position, _dimensions);
		const auto index = _definitionIndex(cell.definition());
		const auto packed = PackedCell::encode(index, cell.horizontalOrientation(), cell.verticalOrientation());
		if (index == _definitions.size())
		{
			_definitions.push_back(cell.definition());
		}
		_cells[offset] = packed;
	}

	void Volume::Builder::set(
		spk::Vector3UInt position,
		std::uint32_t definitionId,
		Cell::HorizontalOrientation horizontal,
		Cell::VerticalOrientation vertical)
	{
		_ensureActive();
		if (!_registry)
		{
			throw spk::Exception("Voxel Volume Builder requires a bound DefinitionRegistry");
		}
		set(position, Cell(_registry->at(definitionId), horizontal, vertical));
	}

	void Volume::Builder::erase(spk::Vector3UInt position)
	{
		_ensureActive();
		_cells[cellOffset(position, _dimensions)] = PackedCell::Air;
	}

	void Volume::Builder::loadPacked(const std::filesystem::path &filename)
	{
		_ensureActive();
		if (!_registry)
		{
			throw spk::Exception("Voxel Volume Builder requires a bound DefinitionRegistry");
		}
		auto decoded = readPackedFile(filename, _cells.size(), *_registry);
		_cells.swap(decoded.cells);
		_definitions.swap(decoded.definitions);
	}

	Volume Volume::Builder::build()
	{
		_ensureActive();
		auto data = std::make_shared<Data>();
		data->dimensions = _dimensions;
		data->definitions = std::move(_definitions);
		data->cells = std::move(_cells);
		_consumed = true;
		return Volume(std::move(data));
	}
}
