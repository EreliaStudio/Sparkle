#include <chrono>
#include <iostream>
#include <thread>

#include "core/context/update_context.hpp"
#include "engine/engine_headless.hpp"
#include "engine/entity3d.hpp"
#include "engine/transform3d.hpp"
#include "threading/worker_pool.hpp"
#include "voxel_world.hpp"
#include "world/chunk_collection.hpp"
#include "world/chunk_interest.hpp"
#include "world/local_chunk_requester.hpp"
#include "world/world_observer.hpp"
#include "world/world_streaming_system.hpp"

int main()
{
	using namespace std::chrono_literals;
	using Chunk = sparkle_world_generation_example::Chunk;
	using Streaming = spk::WorldStreamingSystem<Chunk>;

	spk::EngineHeadless engine;
	sparkle_world_generation_example::WorldGenerator generator(17);
	spk::WorkerPool workers(2);
	spk::LocalChunkRequester<Chunk, sparkle_world_generation_example::GenerationData> requester(workers, generator);
	spk::ChunkCollection<Chunk> collection;
	collection.attach(engine);

	spk::Entity3D player("Headless world observer");
	player.transform().place({0.0f, 0.0f, 5.0f});
	player.addComponent<spk::WorldObserver>(spk::ChunkInterest({1, 0, 0}));
	engine.addEntity(&player);

	Streaming &streaming = engine.addSystem<Streaming>(collection, requester);
	for (int tick = 0; tick < 500 && collection.size() < 3; ++tick)
	{
		spk::UpdateContext context{.time = std::chrono::milliseconds(tick), .deltaTime = 1ms};
		engine.updateState(context);
		std::this_thread::sleep_for(1ms);
	}

	bool complete = collection.size() == 3;
	for (std::int64_t x : {-2, -1, 0, 7, 8, 10})
	{
		const spk::ChunkWorldCoordinate world{x, 4, 5};
		const auto coordinate = Chunk::worldToChunkCoordinate(world);
		const Chunk *chunk = nullptr;
		complete = collection.tryGet(coordinate, chunk) && !chunk->volume().at(spk::Vector3UInt(Chunk::worldToLocalCoordinate(world))).isAir() && complete;
	}

	std::cout << "Generated " << collection.size() << " voxel chunks headlessly" << std::endl;
	engine.removeSystem(streaming);
	engine.removeEntity(&player);
	collection.detach();
	return complete ? 0 : 1;
}
