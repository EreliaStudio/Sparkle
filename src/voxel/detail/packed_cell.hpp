#pragma once

#include <cstddef>
#include <cstdint>

#include "exception.hpp"
#include "voxel/cell.hpp"

namespace spk::Voxel::Detail
{
	struct PackedCell
	{
		using Storage = std::uint32_t;
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
}
