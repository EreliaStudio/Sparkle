#include "core/context/update_context.hpp"
#include "engine/engine.hpp"
#include "exception.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/replication/client_replication_system.hpp"
#include "network/replication/replication_batch.hpp"
#include "threading/worker_pool.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	using Batch = spk::Network::ReplicationBatch;

	class ReplicaValue final : public spk::Network::ClientReplicatedComponent
	{
	private:
		int _value = -1;
		int _commits = 0;
		std::thread::id _commitThread;
		bool _reject = false;
		bool _throwOnCommit = false;

	protected:
		[[nodiscard]] spk::ByteStream _decodeByteStream(const spk::ByteStream::Slice &reader) const override
		{
			int value = 0;
			reader >> value;
			spk::ByteStream::Writer writer;
			writer << value;
			return std::move(writer).build();
		}

		[[nodiscard]] bool _validateByteStream(const spk::ByteStream &state) const override
		{
			int value = 0;
			state.reader() >> value;
			return !_reject && value >= 0;
		}

		void _commitByteStream(const spk::ByteStream &state) override
		{
			if (_throwOnCommit)
			{
				throw spk::Exception("Commit rejected.");
			}
			state.reader() >> _value;
			_commitThread = std::this_thread::get_id();
			++_commits;
		}

	public:
		using ClientReplicatedComponent::ClientReplicatedComponent;
		[[nodiscard]] int value() const noexcept
		{
			return _value;
		}
		[[nodiscard]] int commits() const noexcept
		{
			return _commits;
		}
		[[nodiscard]] std::thread::id commitThread() const noexcept
		{
			return _commitThread;
		}
		void reject(bool value) noexcept
		{
			_reject = value;
		}
		void throwOnCommit(bool value) noexcept
		{
			_throwOnCommit = value;
		}
	};

	[[nodiscard]] Batch::Component makeState(spk::UUID identifier, int value, std::uint64_t revision)
	{
		spk::ByteStream::Writer writer;
		writer << value;
		return {identifier, revision, std::move(writer).build()};
	}

	[[nodiscard]] spk::Message makeMessage(const std::vector<Batch::Component> &records, std::uint32_t perSection)
	{
		const auto batch = Batch::encode(records, perSection);
		spk::Message::Writer writer(0x53504B12);
		writer.append(batch.data().data(), batch.size());
		return std::move(writer).build();
	}

	struct Fixture
	{
		spk::Client client;
		spk::Engine engine;
		spk::UUID firstID = spk::UUID::generate();
		spk::UUID secondID = spk::UUID::generate();
		spk::Network::ClientReplicationSystem &system;
		ReplicaValue &first;
		ReplicaValue &second;
		spk::UpdateContext context{};

		Fixture() :
			system(engine.addSystem<spk::Network::ClientReplicationSystem>()),
			first(engine.root().addComponent<ReplicaValue>(firstID)),
			second(engine.root().addComponent<ReplicaValue>(secondID))
		{
			system.bind(client);
		}

		~Fixture()
		{
			system.unbind();
		}

		void publish(std::vector<Batch::Component> components, std::uint32_t perSection = 16)
		{
			client.messages().publish(makeMessage(components, perSection));
		}

		void tick()
		{
			engine.updateState(context);
		}

		template <typename TPredicate>
		bool until(TPredicate predicate)
		{
			for (int i = 0; i < 200; ++i)
			{
				tick();
				if (predicate())
				{
					return true;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			return false;
		}
	};
}

TEST(ReplicationSections, MultipleComponentsInSingleMessage)
{
	Fixture fixture;
	fixture.publish({makeState(fixture.firstID, 10, 2), makeState(fixture.secondID, 20, 3)}, 2);
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), 10);
	EXPECT_EQ(fixture.second.value(), 20);
	EXPECT_EQ(fixture.first.receivedRevision(), 2u);
	EXPECT_EQ(fixture.second.receivedRevision(), 3u);
}

TEST(ReplicationSections, UnknownComponentDoesNotPreventNextRecord)
{
	Fixture fixture;
	fixture.publish({makeState(spk::UUID::generate(), 90, 1), makeState(fixture.firstID, 12, 4)}, 2);
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), 12);
}

TEST(ReplicationSections, InvalidComponentDoesNotPreventOtherRecords)
{
	Fixture fixture;
	fixture.publish({makeState(fixture.firstID, -4, 10), makeState(fixture.secondID, 15, 8)}, 2);
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), -1);
	EXPECT_FALSE(fixture.first.receivedRevision().has_value());
	EXPECT_EQ(fixture.second.value(), 15);
	EXPECT_EQ(fixture.second.receivedRevision(), 8u);
}

TEST(ReplicationSections, StaleAndDuplicateRevisionsAreIgnored)
{
	Fixture fixture;
	fixture.publish({makeState(fixture.firstID, 10, 5)});
	fixture.publish({makeState(fixture.firstID, 20, 5)});
	fixture.publish({makeState(fixture.firstID, 30, 4)});
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), 10);
	EXPECT_EQ(fixture.first.commits(), 1);
}

TEST(ReplicationSections, CommitExceptionPreservesRevision)
{
	Fixture fixture;
	fixture.first.throwOnCommit(true);
	fixture.publish({makeState(fixture.firstID, 20, 6), makeState(fixture.secondID, 30, 4)}, 2);
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), -1);
	EXPECT_FALSE(fixture.first.receivedRevision().has_value());
	EXPECT_EQ(fixture.second.value(), 30);
	fixture.first.throwOnCommit(false);
	fixture.publish({makeState(fixture.firstID, 20, 6)});
	fixture.tick();
	EXPECT_EQ(fixture.first.value(), 20);
}

TEST(ReplicationSections, ParallelParsingNeverCommitsFromWorkerThreads)
{
	Fixture fixture;
	fixture.system.setWorkerPool(std::make_shared<spk::WorkerPool>(2));
	fixture.publish({makeState(fixture.firstID, 17, 1), makeState(fixture.secondID, 29, 1)}, 1);
	ASSERT_TRUE(fixture.until([&] {
		return fixture.first.commits() == 1 && fixture.second.commits() == 1;
	}));
	EXPECT_EQ(fixture.first.commitThread(), std::this_thread::get_id());
	EXPECT_EQ(fixture.second.commitThread(), std::this_thread::get_id());
}

TEST(ReplicationSections, ParallelAndSequentialDecodersProduceSameValues)
{
	Fixture sequential;
	Fixture parallel;
	parallel.system.setWorkerPool(std::make_shared<spk::WorkerPool>(2));
	sequential.publish({makeState(sequential.firstID, 7, 3), makeState(sequential.secondID, 9, 4)}, 1);
	parallel.publish({makeState(parallel.firstID, 7, 3), makeState(parallel.secondID, 9, 4)}, 1);
	sequential.tick();
	ASSERT_TRUE(parallel.until([&] {
		return parallel.second.value() == 9;
	}));
	EXPECT_EQ(sequential.first.value(), parallel.first.value());
	EXPECT_EQ(sequential.second.value(), parallel.second.value());
}

TEST(ReplicationSections, QueuedWorkersCannotMutateAfterUnbind)
{
	Fixture fixture;
	auto pool = std::make_shared<spk::WorkerPool>(1);
	std::atomic_bool release = false;
	auto blocker = pool->submit([&] {
		while (!release.load(std::memory_order_acquire))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return 0;
	});
	fixture.system.setWorkerPool(pool);
	fixture.publish({makeState(fixture.firstID, 55, 12)});
	fixture.tick();
	fixture.system.unbind();
	release.store(true, std::memory_order_release);
	blocker.wait();
	for (int i = 0; i < 5; ++i)
	{
		fixture.tick();
	}
	EXPECT_EQ(fixture.first.value(), -1);
	EXPECT_FALSE(fixture.first.receivedRevision().has_value());
}

TEST(ReplicationSections, RebindingInvalidatesOldWorkerResults)
{
	Fixture fixture;
	auto pool = std::make_shared<spk::WorkerPool>(1);
	std::atomic_bool release = false;
	auto blocker = pool->submit([&] {
		while (!release.load(std::memory_order_acquire))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return 0;
	});
	fixture.system.setWorkerPool(pool);
	fixture.publish({makeState(fixture.firstID, 55, 12)});
	fixture.tick();
	spk::Client nextClient;
	fixture.system.bind(nextClient);
	release.store(true, std::memory_order_release);
	blocker.wait();
	for (int i = 0; i < 10; ++i)
	{
		fixture.tick();
	}
	EXPECT_EQ(fixture.first.value(), -1);
	nextClient.messages().publish(makeMessage({makeState(fixture.firstID, 88, 0)}, 1));
	ASSERT_TRUE(fixture.until([&] {
		return fixture.first.value() == 88;
	}));
	fixture.system.unbind();
}

TEST(ReplicationSections, EmptyBatchDoesNotMutateComponents)
{
	Fixture fixture;
	fixture.publish({});
	fixture.tick();
	EXPECT_FALSE(fixture.first.receivedRevision().has_value());
	EXPECT_EQ(fixture.first.value(), -1);
}


TEST(ReplicationSections, InterestRemovalInvalidatesOnlyEarlierPendingUpdates)
{
	Fixture fixture;
	auto pool = std::make_shared<spk::WorkerPool>(1);
	std::atomic_bool release = false;
	auto blocker = pool->submit([&] {
		while (!release.load(std::memory_order_acquire))
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return 0;
	});
	fixture.system.setWorkerPool(pool);
	fixture.publish({makeState(fixture.firstID, 55, 12), makeState(fixture.secondID, 65, 12)}, 2);
	fixture.tick();
	spk::Message::Writer removal(0x53504B13);
	removal << fixture.firstID.bytes();
	fixture.client.messages().publish(std::move(removal).build());
	fixture.tick();
	release.store(true, std::memory_order_release);
	blocker.wait();
	ASSERT_TRUE(fixture.until([&] {
		return fixture.second.value() == 65;
	}));
	EXPECT_EQ(fixture.first.value(), -1);
	EXPECT_FALSE(fixture.first.receivedRevision().has_value());
	fixture.publish({makeState(fixture.firstID, 75, 0)});
	ASSERT_TRUE(fixture.until([&] {
		return fixture.first.value() == 75;
	}));
}
