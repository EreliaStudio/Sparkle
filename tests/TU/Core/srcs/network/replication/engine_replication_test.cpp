#include "design_pattern/trait/memento_trait.hpp"
#include "engine/engine.hpp"
#include "core/context/update_context.hpp"
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

TEST(EngineReplication, ClientComponentTracksRevisionAndIgnoresStaleUpdates)
{
	ClientHealth component(spk::UUID::generate());
	spk::Message::Writer firstWriter(121);
	firstWriter << 30;
	auto first = std::move(firstWriter).build();
	component.apply(first.reader(), 5);
	EXPECT_EQ(component.health(), 30);
	ASSERT_TRUE(component.receivedRevision().has_value());
	EXPECT_EQ(*component.receivedRevision(), 5u);

	spk::Message::Writer staleWriter(121);
	staleWriter << 99;
	auto stale = std::move(staleWriter).build();
	component.apply(stale.reader(), 4);
	component.apply(stale.reader(), 5);
	EXPECT_EQ(component.health(), 30);
	EXPECT_EQ(*component.receivedRevision(), 5u);

	component.resetReceivedRevision();
	EXPECT_FALSE(component.receivedRevision().has_value());
	component.apply(stale.reader(), 0);
	EXPECT_EQ(component.health(), 99);
	EXPECT_EQ(*component.receivedRevision(), 0u);
}

TEST(EngineReplication, FailedApplicationDoesNotAdvanceReceivedRevision)
{
	ClientHealth component(spk::UUID::generate());
	spk::Message::Writer writer(121);
	writer << 25;
	auto message = std::move(writer).build();
	component.rejectNextRead(true);
	EXPECT_THROW(component.apply(message.reader(), 8), std::runtime_error);
	EXPECT_FALSE(component.receivedRevision().has_value());
	EXPECT_EQ(component.health(), 100);
	component.rejectNextRead(false);
	component.apply(message.reader(), 8);
	EXPECT_EQ(component.health(), 25);
	EXPECT_EQ(*component.receivedRevision(), 8u);
}

TEST(EngineReplication, RebindingResetsComponentRevisionOnUpdateThread)
{
	spk::Client firstClient;
	spk::Client secondClient;
	spk::Engine engine;
	auto &system = engine.addSystem<spk::Network::ClientReplicationSystem>();
	spk::Entity player("Player");
	engine.addEntity(&player);
	auto &component = player.addComponent<ClientHealth>(spk::UUID::generate());

	spk::Message::Writer writer(121);
	writer << 45;
	auto message = std::move(writer).build();
	component.apply(message.reader(), 40);
	ASSERT_EQ(component.receivedRevision(), 40u);

	system.bind(firstClient);
	EXPECT_EQ(component.receivedRevision(), 40u);
	spk::UpdateContext context{};
	engine.updateState(context);
	EXPECT_FALSE(component.receivedRevision().has_value());

	component.apply(message.reader(), 50);
	system.bind(secondClient);
	EXPECT_EQ(component.receivedRevision(), 50u);
	engine.updateState(context);
	EXPECT_FALSE(component.receivedRevision().has_value());

	component.apply(message.reader(), 60);
	system.unbind();
	EXPECT_EQ(component.receivedRevision(), 60u);
	engine.updateState(context);
	EXPECT_FALSE(component.receivedRevision().has_value());
}

TEST(EngineReplication, DISABLED_StaleMessageFromPreviousConnectionIsRejected)
{
	// Issue #26: a queued packet from session A must not mutate session B's replica.
	// The current wire format has no session identifier, so this test is disabled
	// until the protocol can distinguish the two connections.
	const spk::UUID id = spk::UUID::generate();
	spk::Client firstClient;
	spk::Client secondClient;
	spk::Engine engine;
	auto &system = engine.addSystem<spk::Network::ClientReplicationSystem>();
	spk::Entity player("Player");
	engine.addEntity(&player);
	auto &component = player.addComponent<ClientHealth>(id);
	spk::UpdateContext context{};

	system.bind(firstClient);
	engine.updateState(context);
	spk::Message::Writer oldWriter(0x53504B11);
	oldWriter << id.bytes() << std::uint64_t{80} << 25;
	const auto delayedFromOldConnection = std::move(oldWriter).build();

	system.bind(secondClient);
	engine.updateState(context);
	ASSERT_FALSE(component.receivedRevision().has_value());
	ASSERT_EQ(component.health(), 100);

	// Simulate late delivery of session A bytes through the currently bound
	// receive queue. The transport currently exposes no original-session tag.
	secondClient.messages().publish(delayedFromOldConnection);
	engine.updateState(context);

	EXPECT_EQ(component.health(), 100);
	EXPECT_FALSE(component.receivedRevision().has_value());

	spk::Message::Writer newWriter(0x53504B11);
	newWriter << id.bytes() << std::uint64_t{0} << 75;
	secondClient.messages().publish(std::move(newWriter).build());
	engine.updateState(context);
	EXPECT_EQ(component.health(), 75);
	ASSERT_TRUE(component.receivedRevision().has_value());
	EXPECT_EQ(*component.receivedRevision(), 0u);
}
