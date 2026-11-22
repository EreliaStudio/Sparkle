#pragma once

#include <cstdint>
#include <utility>

#include "exception.hpp"
#include "voxel/volume.hpp"
#include "world/chunk.hpp"

namespace spk::Voxel
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class Chunk : public spk::Chunk<SX, SY, SZ>
	{
	public:
		class Builder;

	private:
		const Volume _volume;

	public:
		explicit Chunk(Volume volume) :
			_volume(std::move(volume))
		{
			if (_volume.dimensions() != spk::Vector3UInt{SX, SY, SZ})
			{
				throw spk::Exception("Voxel Chunk Volume dimensions do not match the Chunk size");
			}
		}

		Chunk(const Chunk &) = default;
		Chunk(Chunk &&) = default;
		Chunk &operator=(const Chunk &) = delete;
		Chunk &operator=(Chunk &&) = delete;

		[[nodiscard]] const Volume &volume() const noexcept
		{
			return _volume;
		}

	};

	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class Chunk<SX, SY, SZ>::Builder
	{
	private:
		Volume::Builder _volumeBuilder;

	public:
		Builder() :
			_volumeBuilder({static_cast<std::uint32_t>(SX), static_cast<std::uint32_t>(SY), static_cast<std::uint32_t>(SZ)})
		{
		}

		[[nodiscard]] Volume::Builder &volumeBuilder() noexcept
		{
			return _volumeBuilder;
		}

		void load(const Chunk &chunk)
		{
			_volumeBuilder.load(chunk.volume());
		}

		[[nodiscard]] Chunk build() &&
		{
			return Chunk(_volumeBuilder.build());
		}
	};
}
