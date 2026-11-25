#pragma once

#include <optional>

#include "threading/task.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	template <typename TChunk>
	class ChunkRequester
	{
	public:
		using Result = std::optional<TChunk>;
		using Answer = typename Task<Result>::Answer;

		virtual ~ChunkRequester() = default;

		[[nodiscard]] virtual Answer request(const ChunkCoordinate &coordinate) = 0;
	};
}
