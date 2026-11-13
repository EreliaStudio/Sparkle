#include "voxel/volume.hpp"

#include "exception.hpp"

namespace spk::Voxel
{
	Volume::PackedCell::Storage Volume::PackedCell::encode(
		std::size_t id,
		Cell::HorizontalOrientation horizontal,
		Cell::VerticalOrientation vertical)
	{
		const auto degrees = static_cast<std::uint32_t>(horizontal);
		if (degrees > 270 || degrees % 90 != 0 ||
			(vertical != Cell::VerticalOrientation::YPositive && vertical != Cell::VerticalOrientation::YNegative))
		{
			throw spk::Exception("Invalid Voxel Volume cell orientation");
		}
		if (id > MaxDefinitionId)
		{
			throw spk::Exception("Voxel Volume Definition ID exceeds 29-bit capacity");
		}
		return (static_cast<Storage>(id) << 3) |
			   (static_cast<Storage>(vertical == Cell::VerticalOrientation::YNegative) << 2) |
			   (degrees / 90);
	}

	Volume::PackedCell::Storage Volume::PackedCell::definitionId(Storage value) noexcept
	{
		return value >> 3;
	}

	Cell::HorizontalOrientation Volume::PackedCell::horizontal(Storage value) noexcept
	{
		return static_cast<Cell::HorizontalOrientation>((value & 3U) * 90);
	}

	Cell::VerticalOrientation Volume::PackedCell::vertical(Storage value) noexcept
	{
		return (value & 4U) ? Cell::VerticalOrientation::YNegative : Cell::VerticalOrientation::YPositive;
	}
}
