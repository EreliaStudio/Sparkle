#pragma once

#include "structure/math/spk_vector3.hpp"

namespace spk
{
	template <typename TContentType, size_t X, size_t Y, size_t Z>
	class IChunk
	{
	public:
		static inline const spk::Vector3UInt Size = spk::Vector3UInt(X, Y, Z);
		using ContentType = TContentType;

	private:
		TContentType _content[X][Y][Z];

		bool _isCoordinateValid(size_t p_x, size_t p_y, size_t p_z) const
		{
			return (p_x < X && p_y < Y && p_z < Z);
		}

	public:
		IChunk() = default;

		void setContent(const spk::Vector3UInt& p_pos, const TContentType& p_value)
		{
			setContent(p_pos.x, p_pos.y, p_pos.z, p_value);
		}

		void setContent(const spk::Vector2UInt& p_pos, size_t p_z, const TContentType& p_value)
		{
			setContent(p_pos.x, p_pos.y, p_z, p_value);
		}

		void setContent(size_t p_x, size_t p_y, size_t p_z, const TContentType& p_value)
		{
			if (_isCoordinateValid(p_x, p_y, p_z) == false)
			{
				throw std::invalid_argument("Position exceeds chunk size");
			}

			_content[p_x][p_y][p_z] = p_value;
		}

		TContentType* content() noexcept                    { return &_content[0][0][0]; }
		const TContentType* content() const noexcept        { return &_content[0][0][0]; }

		static constexpr std::size_t elementCount() noexcept { return X * Y * Z; }

		static constexpr std::size_t contentByteSize() noexcept
		{
			return elementCount() * sizeof(TContentType);
		}

		const TContentType& content(const spk::Vector3UInt& p_pos) const
		{
			return content(p_pos.x, p_pos.y, p_pos.z);
		}

		const TContentType& content(const spk::Vector2UInt& p_pos, size_t p_z) const
		{
			return content(p_pos.x, p_pos.y, p_z);
		}

		const TContentType& content(size_t p_x, size_t p_y, size_t p_z) const
		{
			if (_isCoordinateValid(p_x, p_y, p_z) == false)
			{
				throw std::invalid_argument("Position exceeds chunk size");
			}

			return _content[p_x][p_y][p_z];
		}

		TContentType& content(const spk::Vector3UInt& p_pos)
		{
			return content(p_pos.x, p_pos.y, p_pos.z);
		}

		TContentType& content(const spk::Vector2UInt& p_pos, size_t p_z)
		{
			return content(p_pos.x, p_pos.y, p_z);
		}

		TContentType& content(size_t p_x, size_t p_y, size_t p_z)
		{
			if (_isCoordinateValid(p_x, p_y, p_z) == false)
			{
				throw std::invalid_argument("Position exceeds chunk size");
			}

			return _content[p_x][p_y][p_z];
		}
	};

	template <typename TChunk>
	class ITilemap
	{
	public:
		using ChunkCoordinate = spk::Vector2Int;
		using WorldCoordinate = spk::Vector3Int;
		using AbsoluteCoordinate = spk::Vector3Int;
		using RelativeCoordinate = spk::Vector3Int;

	private:
		std::unordered_map<ChunkCoordinate, std::unique_ptr<TChunk>> _chunks;

	public:
		ITilemap() = default;

		virtual void _onChunkGeneration(const ChunkCoordinate& p_coordinates, spk::SafePointer<TChunk> p_chunk)
		{

		}

		void clear()
		{
			_chunks.clear();
		}

		bool contains(const ChunkCoordinate& p_coordinates) const
		{
			return (_chunks.contains(p_coordinates));
		}

		spk::SafePointer<TChunk> requestChunk(const ChunkCoordinate& p_coordinates)
		{
			if (contains(p_coordinates) == false)
			{
				_chunks[p_coordinates] = std::make_unique<TChunk>();
				_onChunkGeneration(p_coordinates, _chunks.at(p_coordinates).get());
			}

			return _chunks.at(p_coordinates).get();
		}

		spk::SafePointer<TChunk> chunk(const ChunkCoordinate& p_coordinates) const
		{
			if (contains(p_coordinates) == false)
			{
				return nullptr;
			}

			return _chunks.at(p_coordinates).get();
		}

		void addChunk(const ChunkCoordinate& p_coordinates, TChunk&& p_newChunk)
		{
			if (contains(p_coordinates) == false)
			{
				_chunks[p_coordinates] = std::make_unique<TChunk>(std::move(p_newChunk));
			}
			else
			{
				*(_chunks.at(p_coordinates)) = std::move(p_newChunk);
			}
		}

		static ChunkCoordinate worldToChunk(const WorldCoordinate& p_worldCoordinate)
		{
			return ChunkCoordinate(
					std::floor(static_cast<float>(p_worldCoordinate.x) / static_cast<float>(TChunk::Size.x)),
					std::floor(static_cast<float>(p_worldCoordinate.y) / static_cast<float>(TChunk::Size.y))
				);
		}

		static WorldCoordinate chunkToWorld(const ChunkCoordinate& p_chunkCoordinate)
		{
			return WorldCoordinate(
					p_chunkCoordinate.x * static_cast<int>(TChunk::Size.x),
					p_chunkCoordinate.y * static_cast<int>(TChunk::Size.y),
					0
				);
		}

		static RelativeCoordinate absoluteToRelative(const AbsoluteCoordinate& p_absoluteCoordinate)
		{
			ChunkCoordinate chunkCoord = worldToChunk(p_absoluteCoordinate);
			WorldCoordinate worldOrigin = chunkToWorld(chunkCoord);

			return (p_absoluteCoordinate - worldOrigin);
		}

		static RelativeCoordinate absoluteToRelative(const AbsoluteCoordinate& p_absoluteCoordinate, ChunkCoordinate p_chunkCoord)
		{
			WorldCoordinate worldOrigin = chunkToWorld(p_chunkCoord);

			return (p_absoluteCoordinate - worldOrigin);
		}
	};
}