#include "design_pattern/trait/memento_trait.hpp"
#include "engine/engine.hpp"
#include "network/replication/replicated_component.hpp"
#include "network/replication/replication_system.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>

namespace
{
	class Health final : public spk::Network::ReplicatedComponent,
						 public spk::MementoTrait<Health>
	{
	public:
		class State
		{
		private:
			int _health = 0;

			void saveFrom(const Health &component)
			{
				_health = component._health;
			}

			void loadInto(Health &component) const noexcept
			{
				component._health = _health;
			}

			template <typename>
			friend class spk::Memento::Snapshot;
		};

		class MinimalState
		{
		private:
			int _health = 0;

			void saveFrom(const Health &component)
			{
				_health = component._health;
			}

			void loadInto(Health &component) const noexcept
			{
				component._health = _health;
			}

			template <typename>
			friend class spk::Memento::Snapshot;
		};

	private:
		int _health = 100;
		bool _throwAfterRead = false;

	protected:
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			writer << _health;
		}

		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			transaction<State>([&] {
				reader >> _health;
				if (_throwAfterRead)
					throw std::runtime_error("invalid state");
				if (reader.readOffset() != reader.size())
					throw std::runtime_error("unexpected bytes");
			});
		}

	public:
		Health(spk::UUID id, Mode mode) :
			ReplicatedComponent(id, mode)
		{
		}

		void setHealth(int value)
		{
			_health = value;
			invalidate();
		}

		[[nodiscard]] int health() const noexcept
		{
			return _health;
		}

		void rejectNextRead(bool value)
		{
			_throwAfterRead = value;
		}
	};
}

TEST(EngineReplication, TypedMementoSupportsNestedMultipleSnapshots)
{
	Health health(spk::UUID::generate(), Health::Mode::Authoritative);
	auto full = health.save<Health::State>();
	auto minimal = health.save<Health::MinimalState>();
	health.setHealth(8);
	health.load(full);
	EXPECT_EQ(health.health(), 100);
	health.setHealth(25);
	health.load(minimal);
	EXPECT_EQ(health.health(), 100);
}

TEST(EngineReplication, FailedDecodeRestoresPriorState)
{
	const spk::UUID id = spk::UUID::generate();
	Health authority(id, Health::Mode::Authoritative);
	Health replica(id, Health::Mode::Replica);
	authority.setHealth(20);
	spk::Message::Writer writer(121);
	authority.capture(writer);
	auto message = std::move(writer).build();
	replica.rejectNextRead(true);
	EXPECT_THROW(replica.apply(message.reader()), std::runtime_error);
	EXPECT_EQ(replica.health(), 100);
	replica.rejectNextRead(false);
	EXPECT_NO_THROW(replica.apply(message.reader()));
	EXPECT_EQ(replica.health(), 20);
}

TEST(EngineReplication, AuthorityCannotAcceptRemoteState)
{
	const spk::UUID id = spk::UUID::generate();
	Health authority(id, Health::Mode::Authoritative);
	spk::Message::Writer writer(121);
	writer << 9;
	auto message = std::move(writer).build();
	EXPECT_THROW(authority.apply(message.reader()), spk::Exception);
	EXPECT_EQ(authority.health(), 100);
}

TEST(EngineReplication, ReplicationSystemCanBeAttachedToEngine)
{
	spk::Client client;
	spk::Engine engine;
	auto &system = engine.addSystem<spk::Network::ReplicationSystem>(client);
	EXPECT_EQ(system.engine(), &engine);
}
