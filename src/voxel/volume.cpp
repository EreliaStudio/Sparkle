#include "voxel/volume.hpp"

#include <algorithm>
#include <array>
#include <limits>
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
		_definitions{Definition::air()},
		_cells(checkedCellCount(dimensions), PackedCell::encode(
			0,
			Cell::HorizontalOrientation::XPositive,
			Cell::VerticalOrientation::YPositive))
	{
	}

	Volume::Builder::Builder(const Volume &volume) :
		_dimensions(volume._data->dimensions),
		_definitions(volume._data->definitions),
		_cells(volume._data->cells)
	{
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

	void Volume::Builder::set(spk::Vector3UInt position, Cell cell)
	{
		_ensureActive();
		const auto offset = cellOffset(position, _dimensions);
		const auto index = _definitionIndex(cell.definition());
		const auto packed = PackedCell::encode(
			index,
			cell.horizontalOrientation(),
			cell.verticalOrientation());
		if (index == _definitions.size())
		{
			_definitions.push_back(cell.definition());
		}
		_cells[offset] = packed;
	}

	void Volume::Builder::erase(spk::Vector3UInt position)
	{
		_ensureActive();
		_cells[cellOffset(position, _dimensions)] = PackedCell::encode(
			0,
			Cell::HorizontalOrientation::XPositive,
			Cell::VerticalOrientation::YPositive);
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
