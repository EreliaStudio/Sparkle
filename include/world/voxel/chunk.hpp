#pragma once

#include <cstdint>
#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"
#include "world/chunk.hpp"

namespace spk::World::Voxel
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class Chunk : public spk::World::Chunk<SX, SY, SZ>
	{
	private:
		spk::Voxel::Volume _volume;

	public:
		Chunk() :
			_volume(spk::Voxel::Volume::Builder({SX, SY, SZ}).build())
		{
		}

		Chunk(const Chunk &) = default;
		Chunk(Chunk &&) = default;
		Chunk &operator=(const Chunk &) = default;
		Chunk &operator=(Chunk &&) = default;

		[[nodiscard]] const spk::Voxel::Volume &volume() const noexcept
		{
			return _volume;
		}

		void setVolume(spk::Voxel::Volume volume)
		{
			if (volume.dimensions() != spk::Vector3UInt{SX, SY, SZ})
			{
				throw spk::Exception("Voxel Chunk Volume dimensions do not match the Chunk size");
			}
			_volume = std::move(volume);
			this->notifyChunkEdition();
		}
	};
}
