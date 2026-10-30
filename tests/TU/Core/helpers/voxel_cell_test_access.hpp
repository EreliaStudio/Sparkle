#pragma once

#include <vector>

#include "voxel/cell.hpp"

namespace spk::Voxel
{
	struct CellTestAccess
	{
		[[nodiscard]] static spk::Vector3 transformPosition(const Cell &cell, spk::Vector3 value) noexcept
		{
			return cell.transformPosition(value);
		}

		[[nodiscard]] static spk::Vector3 transformNormal(const Cell &cell, spk::Vector3 value) noexcept
		{
			return cell.transformNormal(value);
		}

		[[nodiscard]] static std::vector<Shape::Polygon> orientedPolygons(const Cell &cell)
		{
			return cell.orientedPolygons();
		}
	};
}
