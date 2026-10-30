#pragma once

#include <cstdint>

#include "voxel/definition.hpp"

namespace spk::Voxel
{
	class Cell
	{
	public:
		using HorizontalOrientation = Shape::HorizontalOrientation;
		using VerticalOrientation = Shape::VerticalOrientation;

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
