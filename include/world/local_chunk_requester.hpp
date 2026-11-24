#pragma once

#include "threading/worker_pool.hpp"
#include "world/chunk_requester.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData, typename TBuilder = typename TChunk::Builder>
	class LocalChunkRequester : public ChunkRequester<TChunk>
	{
	private:
		WorkerPool &_workerPool;
		Generator<TChunk, TGenerationData, TBuilder> &_generator;

	public:
		LocalChunkRequester(WorkerPool &workerPool, Generator<TChunk, TGenerationData, TBuilder> &generator) :
			_workerPool(workerPool),
			_generator(generator)
		{
		}

		void retain(const ChunkCoordinate &coordinate) override { _generator.retain(coordinate); }
		void release(const ChunkCoordinate &coordinate) noexcept override { _generator.release(coordinate); }

		[[nodiscard]] typename ChunkRequester<TChunk>::Answer request(const ChunkCoordinate &coordinate) override
		{
			return _workerPool.submit([generator = &_generator, coordinate, reservation = _generator.reserve(coordinate)]() mutable {
				auto job = std::move(reservation);
				return generator->generate(coordinate);
			});
		}
	};
}
