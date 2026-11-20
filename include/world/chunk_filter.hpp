#pragma once

#include "engine/component3d.hpp"
#include "exception.hpp"

namespace spk
{
	template <typename TChunk>
	class ChunkCollection;

	// Non-owning link to the chunk stored by a ChunkCollection entry.
	template <typename TChunk>
	class ChunkFilter : public Component3D
	{
		friend class ChunkCollection<TChunk>;

	private:
		TChunk *_chunk = nullptr;

		void _unbind() noexcept
		{
			_chunk = nullptr;
		}

	public:
		explicit ChunkFilter(TChunk &chunk) :
			Component3D("Chunk filter"),
			_chunk(&chunk)
		{
		}

		[[nodiscard]] bool tryGet(TChunk *&chunk) noexcept
		{
			chunk = _chunk;
			return chunk != nullptr;
		}

		[[nodiscard]] bool tryGet(const TChunk *&chunk) const noexcept
		{
			chunk = _chunk;
			return chunk != nullptr;
		}

		[[nodiscard]] TChunk &chunk()
		{
			if (_chunk == nullptr)
			{
				throw spk::Exception("Chunk filter is no longer bound to a chunk");
			}
			return *_chunk;
		}

		[[nodiscard]] const TChunk &chunk() const
		{
			if (_chunk == nullptr)
			{
				throw spk::Exception("Chunk filter is no longer bound to a chunk");
			}
			return *_chunk;
		}
	};
}
