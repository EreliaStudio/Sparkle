#pragma once

#include <cstdint>

#include "voxel/definition.hpp"

namespace spk::Voxel
{
	class Cell
	{
	public:
		enum class HorizontalOrientation : std::uint16_t
		{
			XPositive = 0,
			ZPositive = 270,
			XNegative = 180,
			ZNegative = 90
		};

		enum class VerticalOrientation : std::uint8_t
		{
			YPositive,
			YNegative
		};

	private:
		Definition _definition;
		HorizontalOrientation _horizontalOrientation = HorizontalOrientation::XPositive;
		VerticalOrientation _verticalOrientation = VerticalOrientation::YPositive;

	public:
		Cell() = default;
		explicit Cell(
			Definition definition,
			HorizontalOrientation horizontal = HorizontalOrientation::XPositive,
			VerticalOrientation vertical = VerticalOrientation::YPositive);

		[[nodiscard]] const Definition &definition() const noexcept;
		[[nodiscard]] HorizontalOrientation horizontalOrientation() const noexcept;
		[[nodiscard]] VerticalOrientation verticalOrientation() const noexcept;
		[[nodiscard]] bool isAir() const noexcept;
	};
}
