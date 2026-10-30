#pragma once

#include <cstdint>
#include <vector>

#include "voxel/definition.hpp"

namespace spk::Voxel
{
	class Cell
	{
	public:
		enum class Orientation : std::uint8_t
		{
			Y0,
			Y90,
			Y180,
			Y270,
			FlippedY0,
			FlippedY90,
			FlippedY180,
			FlippedY270
		};

	private:
		Definition _definition;
		Orientation _orientation = Orientation::Y0;

	public:
		Cell() = default;
		explicit Cell(Definition definition, Orientation orientation = Orientation::Y0);

		[[nodiscard]] const Definition &definition() const noexcept;
		[[nodiscard]] Orientation orientation() const noexcept;
		[[nodiscard]] bool isAir() const noexcept;

		[[nodiscard]] spk::Vector3 transformPosition(spk::Vector3 position) const noexcept;
		[[nodiscard]] spk::Vector3 transformNormal(spk::Vector3 normal) const noexcept;
		[[nodiscard]] Shape::Polygon orientedPolygon(const Shape::Polygon &polygon) const;
		[[nodiscard]] std::vector<Shape::Polygon> orientedPolygons() const;
	};
}
