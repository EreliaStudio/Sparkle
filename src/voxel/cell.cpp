#include "voxel/cell.hpp"

#include <utility>

namespace spk::Voxel
{
	Cell::Cell(
		Definition definition,
		HorizontalOrientation horizontal,
		VerticalOrientation vertical) :
		_definition(std::move(definition)),
		_horizontalOrientation(horizontal),
		_verticalOrientation(vertical)
	{
	}

	const Definition &Cell::definition() const noexcept
	{
		return _definition;
	}

	Cell::HorizontalOrientation Cell::horizontalOrientation() const noexcept
	{
		return _horizontalOrientation;
	}

	Cell::VerticalOrientation Cell::verticalOrientation() const noexcept
	{
		return _verticalOrientation;
	}

	bool Cell::isAir() const noexcept
	{
		return _definition.isAir();
	}
}
