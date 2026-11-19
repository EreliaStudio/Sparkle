#pragma once

#include "voxel/volume.hpp"

#include <compare>
#include <cstdint>
#include <map>
#include <vector>

namespace spk::Voxel::Network
{
	class Server;
	class Client;
}

namespace spk::Voxel
{
	struct ChunkPosition
	{
		std::int32_t x = 0;
		std::int32_t y = 0;
		std::int32_t z = 0;

		[[nodiscard]] auto operator<=>(const ChunkPosition &) const = default;
	};

	struct CellEdit
	{
		spk::Vector3UInt position;
		Volume::PackedCell::Storage value = Volume::PackedCell::Air;
	};

	// A headless collection of immutable Volume snapshots. Mutations are confined
	// to the thread driving the network synchronizers.
	class World final
	{
	private:
		friend class Network::Server;
		friend class Network::Client;

		struct Chunk
		{
			Volume volume;
			std::uint64_t revision;
		};

		spk::Vector3UInt _dimensions;
		Definition::Registry _definitions;
		std::map<ChunkPosition, Chunk> _chunks;
		std::vector<ChunkPosition> _changed;

		void _markChanged(ChunkPosition position);
		void _replace(ChunkPosition position, Volume volume, std::uint64_t revision);
		[[nodiscard]] bool _apply(ChunkPosition position, const std::vector<CellEdit> &edits);
		void _clear();

	public:
		static constexpr std::uint32_t MaxCellsPerChunk = 65536;

		World(spk::Vector3UInt chunkDimensions, Definition::Registry definitions);

		[[nodiscard]] spk::Vector3UInt chunkDimensions() const noexcept;
		[[nodiscard]] const Definition::Registry &definitions() const noexcept;
		[[nodiscard]] const Volume *chunk(ChunkPosition position) const noexcept;
		[[nodiscard]] std::uint64_t revision(ChunkPosition position) const noexcept;
		[[nodiscard]] std::vector<ChunkPosition> chunks() const;
		// Includes newly created, replaced and removed chunks. Rebuild the
		// changed chunk and its neighbors when meshing across boundaries.
		[[nodiscard]] std::vector<ChunkPosition> takeChangedChunks();
	};
}
