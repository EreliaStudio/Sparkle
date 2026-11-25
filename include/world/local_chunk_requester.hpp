#pragma once

#include "threading/worker_pool.hpp"
#include "world/chunk_requester.hpp"
#include "world/generator.hpp"

namespace spk::World
{
	template <typename TChunk, typename TGenerationData>
	class LocalChunkRequester : public ChunkRequester<TChunk>
	{
	private:
		WorkerPool &_workerPool;
		Generator<TChunk, TGenerationData> &_generator;

	public:
		LocalChunkRequester(WorkerPool &workerPool, Generator<TChunk, TGenerationData> &generator) :
			_workerPool(workerPool),
			_generator(generator)
		{
		}


		[[nodiscard]] typename ChunkRequester<TChunk>::Answer request(const ChunkCoordinate &coordinate) override
		{
			return _workerPool.submit([generator = &_generator, coordinate]() {
				return generator->generate(coordinate);
			});
		}
	};
}
