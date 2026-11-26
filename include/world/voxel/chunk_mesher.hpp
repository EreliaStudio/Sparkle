#pragma once

#include <cstdint>
#include <limits>

#include "world/voxel/chunk.hpp"
#include "voxel/mesher.hpp"
#include "world/chunk_collection.hpp"

namespace spk::World::Voxel
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class ChunkMesher : public spk::Voxel::Mesher
	{
	private:
		using ChunkType = Chunk<SX, SY, SZ>;
		const World::ChunkCollection<ChunkType> &_collection;
		World::ChunkCoordinate _coordinate;

	protected:
		[[nodiscard]] OutsideCell outsideCell(spk::Vector3UInt position, spk::Voxel::Shape::Face face) const override
		{
			World::ChunkCoordinate neighbor = _coordinate;
			switch (face)
			{
			case spk::Voxel::Shape::Face::NegativeX:
				if (neighbor.x == std::numeric_limits<std::int32_t>::min())
				{
					return {};
				}
				--neighbor.x;
				position.x = SX - 1;
				break;
			case spk::Voxel::Shape::Face::PositiveX:
				if (neighbor.x == std::numeric_limits<std::int32_t>::max())
				{
					return {};
				}
				++neighbor.x;
				position.x = 0;
				break;
			case spk::Voxel::Shape::Face::NegativeY:
				if (neighbor.y == std::numeric_limits<std::int32_t>::min())
				{
					return {};
				}
				--neighbor.y;
				position.y = SY - 1;
				break;
			case spk::Voxel::Shape::Face::PositiveY:
				if (neighbor.y == std::numeric_limits<std::int32_t>::max())
				{
					return {};
				}
				++neighbor.y;
				position.y = 0;
				break;
			case spk::Voxel::Shape::Face::NegativeZ:
				if (neighbor.z == std::numeric_limits<std::int32_t>::min())
				{
					return {};
				}
				--neighbor.z;
				position.z = SZ - 1;
				break;
			case spk::Voxel::Shape::Face::PositiveZ:
				if (neighbor.z == std::numeric_limits<std::int32_t>::max())
				{
					return {};
				}
				++neighbor.z;
				position.z = 0;
				break;
			}

			const ChunkType *adjacent = nullptr;
			if (!_collection.tryGet(neighbor, adjacent))
			{
				return {};
			}
			return {OutsideCell::State::Occupied, adjacent->volume().at(position)};
		}

	public:
		ChunkMesher(const World::ChunkCollection<ChunkType> &collection, World::ChunkCoordinate coordinate) :
			_collection(collection),
			_coordinate(coordinate)
		{
		}
	};
}
