#include "world/chunk_interest.hpp"

#include <cstdint>
#include <limits>

#include "exception.hpp"

namespace spk
{
	ChunkInterest::ChunkInterest(Vector3Int radius) :
		_radius(radius)
	{
		if (_radius.x < 0 || _radius.y < 0 || _radius.z < 0)
		{
			throw spk::Exception("Chunk interest radii must be non-negative");
		}
	}

	const Vector3Int &ChunkInterest::radius() const noexcept
	{
		return _radius;
	}

	std::vector<ChunkCoordinate> ChunkInterest::coordinates(const ChunkCoordinate &center) const
	{
		const std::int64_t minX = static_cast<std::int64_t>(center.x) - _radius.x;
		const std::int64_t minY = static_cast<std::int64_t>(center.y) - _radius.y;
		const std::int64_t minZ = static_cast<std::int64_t>(center.z) - _radius.z;
		const std::int64_t maxX = static_cast<std::int64_t>(center.x) + _radius.x;
		const std::int64_t maxY = static_cast<std::int64_t>(center.y) + _radius.y;
		const std::int64_t maxZ = static_cast<std::int64_t>(center.z) + _radius.z;

		if (minX < std::numeric_limits<std::int32_t>::min() ||
			minY < std::numeric_limits<std::int32_t>::min() ||
			minZ < std::numeric_limits<std::int32_t>::min() ||
			maxX > std::numeric_limits<std::int32_t>::max() ||
			maxY > std::numeric_limits<std::int32_t>::max() ||
			maxZ > std::numeric_limits<std::int32_t>::max())
		{
			throw spk::Exception("Chunk interest exceeds the coordinate range");
		}

		std::vector<ChunkCoordinate> result;
		const std::uint64_t width = 2 * static_cast<std::uint64_t>(_radius.x) + 1;
		const std::uint64_t height = 2 * static_cast<std::uint64_t>(_radius.y) + 1;
		const std::uint64_t depth = 2 * static_cast<std::uint64_t>(_radius.z) + 1;
		const std::uint64_t maxCount = result.max_size();
		if (width > maxCount / height || width * height > maxCount / depth)
		{
			throw spk::Exception("Chunk interest has too many coordinates");
		}
		result.reserve(static_cast<std::size_t>(width * height * depth));

		for (std::int64_t z = minZ; z <= maxZ; ++z)
		{
			for (std::int64_t y = minY; y <= maxY; ++y)
			{
				for (std::int64_t x = minX; x <= maxX; ++x)
				{
					result.push_back({static_cast<std::int32_t>(x), static_cast<std::int32_t>(y), static_cast<std::int32_t>(z)});
				}
			}
		}

		return result;
	}
}
