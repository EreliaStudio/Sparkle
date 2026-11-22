#pragma once

#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <utility>

#include "design_pattern/contract_provider.hpp"
#include "exception.hpp"
#include "world/chunk_coordinate.hpp"

namespace spk::World
{
	template <std::int32_t SX, std::int32_t SY, std::int32_t SZ>
	class Chunk
	{
		static_assert(SX > 0 && SY > 0 && SZ > 0, "Chunk dimensions must be strictly positive");

	public:
		using EditionProvider = ContractProvider<>;
		using EditionCallback = EditionProvider::callback_type;
		using EditionContract = EditionProvider::Contract;

		static constexpr Vector3Int Size{SX, SY, SZ};

	private:
		EditionProvider _editionProvider;

		[[nodiscard]] static constexpr std::int64_t _floorDivide(std::int64_t value, std::int32_t dimension) noexcept
		{
			const std::int64_t quotient = value / dimension;
			return quotient - (value % dimension < 0 ? 1 : 0);
		}

		[[nodiscard]] static constexpr std::int32_t _checkedIndex(std::int64_t index)
		{
			if (index < std::numeric_limits<std::int32_t>::min() || index > std::numeric_limits<std::int32_t>::max())
			{
				throw spk::Exception("World position exceeds the chunk coordinate range");
			}
			return static_cast<std::int32_t>(index);
		}

		[[nodiscard]] static std::int32_t _checkedIndex(long double position, std::int32_t dimension)
		{
			if (!std::isfinite(position))
			{
				throw spk::Exception("World position must be finite");
			}
			const long double index = std::floor(position / static_cast<long double>(dimension));
			if (index < static_cast<long double>(std::numeric_limits<std::int32_t>::min()) ||
				index > static_cast<long double>(std::numeric_limits<std::int32_t>::max()))
			{
				throw spk::Exception("World position exceeds the chunk coordinate range");
			}
			return static_cast<std::int32_t>(index);
		}

		[[nodiscard]] static constexpr std::int32_t _localCoordinate(std::int64_t position, std::int32_t dimension) noexcept
		{
			std::int64_t remainder = position % dimension;
			if (remainder < 0)
			{
				remainder += dimension;
			}
			return static_cast<std::int32_t>(remainder);
		}

		template <std::floating_point TPosition>
		[[nodiscard]] static TPosition _localCoordinate(TPosition position, std::int32_t dimension)
		{
			TPosition remainder = std::fmod(position, static_cast<TPosition>(dimension));
			if (remainder < TPosition{0})
			{
				remainder += static_cast<TPosition>(dimension);
			}
			return remainder;
		}

	protected:
		void notifyChunkEdition()
		{
			_editionProvider.trigger();
		}

	public:
		Chunk() = default;

		// Each new instance owns its own event provider, including move construction.
		Chunk(const Chunk &) :
			Chunk()
		{
		}

		Chunk(Chunk &&) :
			Chunk()
		{
		}

		// Assigning data must not transfer subscriptions or invalidate observers
		// already attached to the destination instance.
		Chunk &operator=(const Chunk &) noexcept
		{
			return *this;
		}

		Chunk &operator=(Chunk &&) noexcept
		{
			return *this;
		}

		virtual ~Chunk() = default;

		[[nodiscard]] EditionContract subscribeToChunkEdition(EditionCallback callback)
		{
			return _editionProvider.subscribe(std::move(callback));
		}

		template <std::signed_integral TPosition>
			requires(sizeof(TPosition) <= sizeof(std::int64_t))
		[[nodiscard]] static constexpr ChunkCoordinate worldToChunkCoordinate(const TVector3<TPosition> &world)
		{
			return {
				_checkedIndex(_floorDivide(static_cast<std::int64_t>(world.x), SX)),
				_checkedIndex(_floorDivide(static_cast<std::int64_t>(world.y), SY)),
				_checkedIndex(_floorDivide(static_cast<std::int64_t>(world.z), SZ))};
		}

		template <std::floating_point TPosition>
		[[nodiscard]] static ChunkCoordinate worldToChunkCoordinate(const TVector3<TPosition> &world)
		{
			return {
				_checkedIndex(static_cast<long double>(world.x), SX),
				_checkedIndex(static_cast<long double>(world.y), SY),
				_checkedIndex(static_cast<long double>(world.z), SZ)};
		}

		[[nodiscard]] static constexpr ChunkWorldCoordinate chunkCoordinateToWorld(const ChunkCoordinate &coordinate) noexcept
		{
			return {
				static_cast<std::int64_t>(coordinate.x) * SX,
				static_cast<std::int64_t>(coordinate.y) * SY,
				static_cast<std::int64_t>(coordinate.z) * SZ};
		}

		template <std::signed_integral TPosition>
			requires(sizeof(TPosition) <= sizeof(std::int64_t))
		[[nodiscard]] static constexpr Vector3Int worldToLocalCoordinate(const TVector3<TPosition> &world) noexcept
		{
			return {
				_localCoordinate(static_cast<std::int64_t>(world.x), SX),
				_localCoordinate(static_cast<std::int64_t>(world.y), SY),
				_localCoordinate(static_cast<std::int64_t>(world.z), SZ)};
		}

		template <std::floating_point TPosition>
		[[nodiscard]] static TVector3<TPosition> worldToLocalCoordinate(const TVector3<TPosition> &world)
		{
			(void)worldToChunkCoordinate(world);
			return {
				_localCoordinate(world.x, SX),
				_localCoordinate(world.y, SY),
				_localCoordinate(world.z, SZ)};
		}
	};
}
