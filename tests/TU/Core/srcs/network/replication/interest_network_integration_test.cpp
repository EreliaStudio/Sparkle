#include "core/context/update_context.hpp"
#include "design_pattern/trait/memento_trait.hpp"
#include "engine/engine.hpp"
#include "exception.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/replication/client_replication_system.hpp"
#include "network/replication/interest_evaluator.hpp"
#include "network/replication/server_replicated_component.hpp"
#include "network/replication/server_replication_system.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>
#include <utility>

namespace
{
	using namespace std::chrono_literals;

	class ValueInterest final : public spk::Network::Interest
	{
	private:
		std::int32_t _minimum;

	public:
		explicit ValueInterest(std::int32_t minimum) : _minimum(minimum) {}

		[[nodiscard]] std::int32_t minimum() const noexcept { return _minimum; }

		[[nodiscard]] spk::UUID type() const override
		{
			return spk::UUID::fromString("d31ad114-2222-4333-8444-555555555555");
		}

		void serialize(spk::Message::Writer &writer) const override
		{
			writer << _minimum;
		}
	};

	class ServerValue final : public spk::Network::ServerReplicatedComponent
	{
	private:
		std::int32_t _value = 0;

	protected:
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			writer << _value;
		}

	public:
		using ServerReplicatedComponent::ServerReplicatedComponent;

		void change(std::int32_t value)
		{
			_value = value;
			invalidate();
		}

		[[nodiscard]] std::int32_t value() const noexcept { return _value; }
	};

	class ClientValue final : public spk::Network::ClientReplicatedComponent
	{
	private:
		std::int32_t _value = -1;
		std::size_t _applications = 0;
		std::size_t _removals = 0;

	protected:
		void _readNetworkState(const spk::Message::Reader &reader) override
		{
			reader >> _value;
			++_applications;
		}

		void _onInterestLost() override { ++_removals; }

	public:
		using ClientReplicatedComponent::ClientReplicatedComponent;

		[[nodiscard]] std::int32_t value() const noexcept { return _value; }
		[[nodiscard]] std::size_t applications() const noexcept { return _applications; }
		[[nodiscard]] std::size_t removals() const noexcept { return _removals; }
	};

	class ValueEvaluator final : public spk::Network::InterestEvaluator
	{
	public:
		[[nodiscard]] bool matches(const spk::Network::Interest &interest,
			const spk::Network::ServerReplicatedComponent &component,
			spk::ConnectionID) const override
		{
			const auto *filter = dynamic_cast<const ValueInterest *>(&interest);
			const auto *value = dynamic_cast<const ServerValue *>(&component);
			return filter && value && value->value() >= filter->minimum();
		}
	};

	class ValueReplicationServer final : public spk::Network::ServerReplicationSystem
	{
	protected:
		[[nodiscard]] std::unique_ptr<spk::Network::Interest> _createInterest(
			const spk::Message::Reader &reader) override
		{
			spk::UUID::Storage type{};
			std::int32_t minimum = 0;
			reader >> type >> minimum;
			if (spk::UUID(type) != ValueInterest(0).type())
				return nullptr;
			return std::make_unique<ValueInterest>(minimum);
		}
	};

	struct NetworkScenario
	{
		spk::Server server;
		spk::Client firstClient;
		spk::Client secondClient;
		spk::Engine serverEngine;
		spk::Engine firstEngine;
		spk::Engine secondEngine;
		ValueReplicationServer &serverSystem;
		spk::Network::ClientReplicationSystem &firstSystem;
		spk::Network::ClientReplicationSystem &secondSystem;
		ServerValue &source;
		ClientValue &firstReplica;
		ClientValue &secondReplica;
		spk::UpdateContext context{};
		spk::UUID componentID = spk::UUID::generate();

		NetworkScenario() :
			serverSystem(serverEngine.addSystem<ValueReplicationServer>()),
			firstSystem(firstEngine.addSystem<spk::Network::ClientReplicationSystem>()),
			secondSystem(secondEngine.addSystem<spk::Network::ClientReplicationSystem>()),
			source(serverEngine.root().addComponent<ServerValue>(componentID)),
			firstReplica(firstEngine.root().addComponent<ClientValue>(componentID)),
			secondReplica(secondEngine.root().addComponent<ClientValue>(componentID))
		{
			serverSystem.setInterestEvaluator(std::make_shared<ValueEvaluator>());
			serverSystem.setRequestAuthorizer([](spk::ConnectionID, const ServerValue &) {
				return true;
			});
			server.start(0);
			serverSystem.bind(server);
			firstSystem.bind(firstClient);
			secondSystem.bind(secondClient);
			firstClient.connect("127.0.0.1", server.port());
		}

		~NetworkScenario()
		{
			firstSystem.unbind();
			secondSystem.unbind();
			serverSystem.unbind();
			firstClient.disconnect();
			secondClient.disconnect();
			server.stop();
		}

		void connectSecond()
		{
			secondClient.connect("127.0.0.1", server.port());
		}

		void tick(std::chrono::steady_clock::duration delta = 50ms)
		{
			context.deltaTime = delta;
			serverEngine.updateState(context);
			firstEngine.updateState(context);
			secondEngine.updateState(context);
		}

		template <typename TPredicate>
		bool await(TPredicate predicate)
		{
			for (int index = 0; index < 300; ++index)
			{
				tick();
				if (predicate())
					return true;
				std::this_thread::sleep_for(2ms);
			}
			return false;
		}
	};
}

TEST(InterestNetwork, ClientWithoutSubscriptionReceivesNoState)
{
	NetworkScenario scenario;
	scenario.source.change(30);
	for (int i = 0; i < 4; ++i)
		scenario.tick();
	EXPECT_FALSE(scenario.firstReplica.receivedRevision().has_value());
	EXPECT_EQ(scenario.firstReplica.applications(), 0u);
}

TEST(InterestNetwork, SubscriptionReceivesMatchingInitialState)
{
	NetworkScenario scenario;
	scenario.source.change(30);
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(20));
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 30; }));
	EXPECT_TRUE(subscription.isValid());
	EXPECT_EQ(scenario.firstReplica.applications(), 1u);
}

TEST(InterestNetwork, NonMatchingInterestDoesNotPublish)
{
	NetworkScenario scenario;
	scenario.source.change(12);
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(20));
	for (int i = 0; i < 15; ++i)
	{
		scenario.tick();
		std::this_thread::sleep_for(2ms);
	}
	EXPECT_EQ(scenario.firstReplica.applications(), 0u);
	EXPECT_TRUE(subscription.isValid());
}

TEST(InterestNetwork, MutationPublishesOnlyNewRevision)
{
	NetworkScenario scenario;
	scenario.source.change(25);
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(0));
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 25; }));
	const auto before = scenario.firstReplica.applications();
	for (int i = 0; i < 5; ++i)
		scenario.tick();
	EXPECT_EQ(scenario.firstReplica.applications(), before);
	scenario.source.change(26);
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 26; }));
	EXPECT_EQ(scenario.firstReplica.applications(), before + 1);
}

TEST(InterestNetwork, InterestUpdateChangesMembership)
{
	NetworkScenario scenario;
	scenario.source.change(25);
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(30));
	for (int i = 0; i < 5; ++i)
		scenario.tick();
	EXPECT_EQ(scenario.firstReplica.applications(), 0u);
	subscription.update(ValueInterest(20));
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 25; }));
	subscription.update(ValueInterest(50));
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.removals() == 1; }));
	EXPECT_FALSE(scenario.firstReplica.receivedRevision().has_value());
}

TEST(InterestNetwork, RAIIDestructionRemovesInterestAndNotifiesReplica)
{
	NetworkScenario scenario;
	scenario.source.change(25);
	{
		auto subscription = scenario.firstSystem.subscribe(ValueInterest(0));
		ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 25; }));
	}
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.removals() == 1; }));
	EXPECT_FALSE(scenario.firstReplica.receivedRevision().has_value());
}

TEST(InterestNetwork, TwoOverlappingSubscriptionsDoNotDuplicateComponent)
{
	NetworkScenario scenario;
	scenario.source.change(25);
	auto first = scenario.firstSystem.subscribe(ValueInterest(0));
	auto second = scenario.firstSystem.subscribe(ValueInterest(20));
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 25; }));
	EXPECT_EQ(scenario.firstReplica.applications(), 1u);
	first.cancel();
	for (int i = 0; i < 5; ++i)
		scenario.tick();
	EXPECT_EQ(scenario.firstReplica.removals(), 0u);
	second.cancel();
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.removals() == 1; }));
}

TEST(InterestNetwork, SameInterestAcrossClientsMaintainsIndependentRevisions)
{
	NetworkScenario scenario;
	scenario.connectSecond();
	scenario.source.change(30);
	auto first = scenario.firstSystem.subscribe(ValueInterest(20));
	auto second = scenario.secondSystem.subscribe(ValueInterest(20));
	ASSERT_TRUE(scenario.await([&] {
		return scenario.firstReplica.value() == 30 && scenario.secondReplica.value() == 30;
	}));
	scenario.source.change(40);
	ASSERT_TRUE(scenario.await([&] {
		return scenario.firstReplica.value() == 40 && scenario.secondReplica.value() == 40;
	}));
	EXPECT_EQ(scenario.firstReplica.applications(), 2u);
	EXPECT_EQ(scenario.secondReplica.applications(), 2u);
}

TEST(InterestNetwork, ConnectionLossInvalidatesRAIIHandle)
{
	NetworkScenario scenario;
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(0));
	ASSERT_TRUE(subscription.isValid());
	scenario.firstClient.disconnect();
	EXPECT_FALSE(subscription.isValid());
	EXPECT_THROW(subscription.update(ValueInterest(20)), spk::Exception);
	subscription.cancel();
	EXPECT_FALSE(subscription.isValid());
}

TEST(InterestNetwork, RebindingInvalidatesOldHandle)
{
	NetworkScenario scenario;
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(0));
	scenario.firstSystem.bind(scenario.secondClient);
	EXPECT_FALSE(subscription.isValid());
	EXPECT_THROW(subscription.update(ValueInterest(20)), spk::Exception);
}

TEST(InterestNetwork, RefreshIntervalDelaysPublication)
{
	NetworkScenario scenario;
	scenario.serverSystem.setRefreshInterval(300ms);
	scenario.source.change(30);
	auto subscription = scenario.firstSystem.subscribe(ValueInterest(0));
	for (int i = 0; i < 4; ++i)
	{
		scenario.tick(50ms);
		std::this_thread::sleep_for(2ms);
	}
	EXPECT_EQ(scenario.firstReplica.applications(), 0u);
	ASSERT_TRUE(scenario.await([&] { return scenario.firstReplica.value() == 30; }));
}
