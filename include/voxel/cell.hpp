#pragma once

#include <cstdint>
#include <vector>

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

		friend class VoxelMesher;
		friend struct CellTestAccess;

		[[nodiscard]] spk::Vector3 transformPosition(spk::Vector3 position) const noexcept;
		[[nodiscard]] spk::Vector3 transformNormal(spk::Vector3 normal) const noexcept;
		[[nodiscard]] Shape::Polygon orientedPolygon(const Shape::Polygon &polygon) const;
		[[nodiscard]] std::vector<Shape::Polygon> orientedPolygons() const;

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
