#include "design_pattern/trait/memento_trait.hpp"
#include "engine/engine.hpp"
#include "exception.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/replication/client_replication_system.hpp"
#include "network/replication/server_replicated_component.hpp"
#include "network/replication/server_replication_system.hpp"

#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>

namespace
{
	class ServerHealth final : public spk::Network::ServerReplicatedComponent
	{
	private:
		int _health = 100;

	protected:
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			writer << _health;
		}

	public:
		using ServerReplicatedComponent::ServerReplicatedComponent;

		void setHealth(int value)
		{
			_health = value;
			invalidate();
		}
	};

	class ClientHealth final : public spk::Network::ClientReplicatedComponent,
							   public spk::MementoTrait<ClientHealth>
	{
	public:
		class State
		{
		private:
			int _health = 0;

			void capture(const ClientHealth &component)
			{
				_health = component._health;
			}

			void restore(ClientHealth &component) const noexcept
			{
				component._health = _health;
			}

			friend class spk::MementoTrait<ClientHealth>;
		};

		class MinimalState
		{
		private:
			int _health = 0;

			void capture(const ClientHealth &component)
			{
				_health = component._health;
			}

			void restore(ClientHealth &component) const noexcept
			{
				component._health = _health;
			}

			friend class spk::MementoTrait<ClientHealth>;
		};

	private:
		int _health = 100;
		bool _throwAfterRead = false;

	protected:
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
		using ClientReplicatedComponent::ClientReplicatedComponent;

		void setHealth(int value)
		{
			_health = value;
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
	ClientHealth health(spk::UUID::generate());
	auto full = health.save<ClientHealth::State>();
	auto minimal = health.save<ClientHealth::MinimalState>();
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
	ServerHealth authority(id);
	ClientHealth replica(id);
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

TEST(EngineReplication, ClientAndServerSystemsBindIndependently)
{
	spk::Client firstClient;
	spk::Client secondClient;
	spk::Server server;
	spk::Engine clientEngine;
	spk::Engine serverEngine;
	auto &clientSystem = clientEngine.addSystem<spk::Network::ClientReplicationSystem>();
	auto &serverSystem = serverEngine.addSystem<spk::Network::ServerReplicationSystem>();
	EXPECT_FALSE(clientSystem.isBound());
	EXPECT_FALSE(serverSystem.isBound());
	EXPECT_THROW(clientSystem.request(spk::UUID::generate()), spk::Exception);
	clientSystem.bind(firstClient);
	serverSystem.bind(server);
	EXPECT_TRUE(clientSystem.isBound());
	EXPECT_TRUE(serverSystem.isBound());
	clientSystem.bind(secondClient);
	EXPECT_TRUE(clientSystem.isBound());
	clientSystem.unbind();
	serverSystem.unbind();
	EXPECT_FALSE(clientSystem.isBound());
	EXPECT_FALSE(serverSystem.isBound());
}

TEST(EngineReplication, RebindingToSameTransportKeepsSubscription)
{
	spk::Client client;
	spk::Network::ClientReplicationSystem system;
	system.bind(client);
	system.bind(client);
	EXPECT_TRUE(system.isBound());
	system.unbind();
	EXPECT_FALSE(system.isBound());
}
