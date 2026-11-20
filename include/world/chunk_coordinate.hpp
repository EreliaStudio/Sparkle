#pragma once

#include <cstdint>

#include "math/vector3.hpp"

namespace spk
{
	// Signed chunk indices are independent of their position in world units.
	using ChunkCoordinate = Vector3Int;

	// Origins may exceed int32 even when the chunk indices and sizes do not.
	using ChunkWorldCoordinate = TVector3<std::int64_t>;
}
