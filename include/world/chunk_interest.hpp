#pragma once

#include <vector>

#include "world/chunk_coordinate.hpp"

namespace spk
{
	class ChunkInterest
	{
	private:
		Vector3Int _radius;

	public:
		explicit ChunkInterest(Vector3Int radius = {0, 0, 0});

		[[nodiscard]] const Vector3Int &radius() const noexcept;
		[[nodiscard]] std::vector<ChunkCoordinate> coordinates(const ChunkCoordinate &center) const;

		[[nodiscard]] bool operator==(const ChunkInterest &other) const = default;
	};
}
