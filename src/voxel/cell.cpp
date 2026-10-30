#include "voxel/cell.hpp"

#include <algorithm>
#include <utility>


namespace
{
	spk::Vector3 rotateY(spk::Vector3 value, unsigned quarterTurns, bool isPosition) noexcept
	{
		for (unsigned turn = 0; turn < quarterTurns; ++turn)
		{
			const float x = value.x;
			value.x = value.z;
			value.z = isPosition ? 1.0f - x : -x;
		}
		return value;
	}

	spk::Vector3 faceDirection(spk::Voxel::Shape::Face face) noexcept
	{
		using Face = spk::Voxel::Shape::Face;
		switch (face)
		{
		case Face::NegativeX:
			return {-1.0f, 0.0f, 0.0f};
		case Face::PositiveX:
			return {1.0f, 0.0f, 0.0f};
		case Face::NegativeY:
			return {0.0f, -1.0f, 0.0f};
		case Face::PositiveY:
			return {0.0f, 1.0f, 0.0f};
		case Face::NegativeZ:
			return {0.0f, 0.0f, -1.0f};
		case Face::PositiveZ:
			return {0.0f, 0.0f, 1.0f};
		}
		return {};
	}

	spk::Voxel::Shape::Face directionFace(spk::Vector3 direction) noexcept
	{
		using Face = spk::Voxel::Shape::Face;
		if (direction.x < 0.0f)
		{
			return Face::NegativeX;
		}
		if (direction.x > 0.0f)
		{
			return Face::PositiveX;
		}
		if (direction.y < 0.0f)
		{
			return Face::NegativeY;
		}
		if (direction.y > 0.0f)
		{
			return Face::PositiveY;
		}
		if (direction.z < 0.0f)
		{
			return Face::NegativeZ;
		}
		return Face::PositiveZ;
	}
}

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

	spk::Vector3 Cell::transformPosition(spk::Vector3 position) const noexcept
	{
		if (_verticalOrientation == VerticalOrientation::YNegative)
		{
			position.y = 1.0f - position.y;
		}
		return rotateY(position, static_cast<unsigned>(_horizontalOrientation) / 90u, true);
	}

	spk::Vector3 Cell::transformNormal(spk::Vector3 normal) const noexcept
	{
		if (_verticalOrientation == VerticalOrientation::YNegative)
		{
			normal.y = -normal.y;
		}
		return rotateY(normal, static_cast<unsigned>(_horizontalOrientation) / 90u, false);
	}

	Shape::Polygon Cell::orientedPolygon(const Shape::Polygon &polygon) const
	{
		Shape::Polygon result = polygon;
		for (auto &vertex : result.vertices)
		{
			vertex.position = transformPosition(vertex.position);
			if (vertex.normal)
			{
				vertex.normal = transformNormal(*vertex.normal);
			}
		}
		result.geometricNormal = transformNormal(result.geometricNormal);
		if (result.boundary)
		{
			result.boundary = directionFace(transformNormal(faceDirection(*result.boundary)));
		}
		if (_verticalOrientation == VerticalOrientation::YNegative)
		{
			std::reverse(result.vertices.begin(), result.vertices.end());
		}
		return result;
	}

	std::vector<Shape::Polygon> Cell::orientedPolygons() const
	{
		std::vector<Shape::Polygon> result;
		result.reserve(_definition.shape().polygons().size());
		for (const auto &polygon : _definition.shape().polygons())
		{
			result.push_back(orientedPolygon(polygon));
		}
		return result;
	}
}
