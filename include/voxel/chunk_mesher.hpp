#pragma once

#include <cstdint>

#include "voxel/chunk.hpp"
#include "voxel/mesher.hpp"
#include "world/chunk_collection.hpp"

namespace spk::Voxel
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class ChunkMesher : public Mesher
	{
	private:
		using ChunkType = Chunk<SX, SY, SZ>;
		const World::ChunkCollection<ChunkType> &_collection;
		World::ChunkCoordinate _coordinate;

	protected:
		[[nodiscard]] OutsideCell outsideCell(spk::Vector3UInt position, Shape::Face face) const override
		{
			World::ChunkCoordinate neighbor = _coordinate;
			switch (face)
			{
			case Shape::Face::NegativeX:
				--neighbor.x;
				position.x = SX - 1;
				break;
			case Shape::Face::PositiveX:
				++neighbor.x;
				position.x = 0;
				break;
			case Shape::Face::NegativeY:
				--neighbor.y;
				position.y = SY - 1;
				break;
			case Shape::Face::PositiveY:
				++neighbor.y;
				position.y = 0;
				break;
			case Shape::Face::NegativeZ:
				--neighbor.z;
				position.z = SZ - 1;
				break;
			case Shape::Face::PositiveZ:
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
