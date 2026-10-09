#include "exception.hpp"
#include "network/replication/client_replication_system.hpp"
#include "network/replication/interest.hpp"
#include "network/replication/interest_evaluator.hpp"
#include "network/replication/server_replicated_component.hpp"
#include "network/replication/server_replication_system.hpp"

#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <utility>

namespace
{
	class ThresholdInterest final : public spk::Network::Interest
	{
	public:
		std::int32_t threshold = 0;

		explicit ThresholdInterest(std::int32_t value) :
			threshold(value)
		{
		}

		[[nodiscard]] spk::UUID type() const override
		{
			return spk::UUID::fromString("11111111-2222-4333-8444-555555555555");
		}

		void serialize(spk::Message::Writer &writer) const override
		{
			writer << threshold;
		}
	};

	class MeasuredComponent final : public spk::Network::ServerReplicatedComponent
	{
	public:
		std::int32_t value = 0;

		using ServerReplicatedComponent::ServerReplicatedComponent;

	protected:
		void _writeNetworkState(spk::Message::Writer &writer) const override
		{
			writer << value;
		}
	};

	class ThresholdEvaluator final : public spk::Network::InterestEvaluator
	{
	public:
		[[nodiscard]] bool matches(
			const spk::Network::Interest &interest,
			const spk::Network::ServerReplicatedComponent &component,
			spk::ConnectionID) const override
		{
			auto *threshold = dynamic_cast<const ThresholdInterest *>(&interest);
			auto *measured = dynamic_cast<const MeasuredComponent *>(&component);
			return threshold && measured && measured->value >= threshold->threshold;
		}
	};

	class GameServerReplicationSystem final : public spk::Network::ServerReplicationSystem
	{
	protected:
		[[nodiscard]] std::unique_ptr<spk::Network::Interest> _createInterest(
			const spk::Message::Reader &reader) override
		{
			spk::UUID::Storage typeBytes{};
			std::int32_t threshold = 0;
			reader >> typeBytes >> threshold;
			if (spk::UUID(typeBytes) != ThresholdInterest(0).type())
			{
				return nullptr;
			}
			return std::make_unique<ThresholdInterest>(threshold);
		}
	};
}

TEST(InterestReplication, ConcreteInterestSerializesWithoutServerInternals)
{
	ThresholdInterest interest(25);
	spk::Message::Writer writer(71);
	writer << interest.type().bytes();
	interest.serialize(writer);
	auto message = std::move(writer).build();
	auto reader = message.reader();
	spk::UUID::Storage type{};
	std::int32_t threshold = 0;
	reader >> type >> threshold;
	EXPECT_EQ(spk::UUID(type), interest.type());
	EXPECT_EQ(threshold, 25);
	EXPECT_EQ(reader.readOffset(), reader.size());
}

TEST(InterestReplication, ServerEvaluatorAppliesApplicationPredicate)
{
	ThresholdEvaluator evaluator;
	ThresholdInterest interest(25);
	MeasuredComponent component(spk::UUID::generate());
	component.value = 24;
	EXPECT_FALSE(evaluator.matches(interest, component, 1));
	component.value = 25;
	EXPECT_TRUE(evaluator.matches(interest, component, 1));
}

TEST(InterestReplication, ServerRefreshIntervalIsChronoDuration)
{
	using namespace std::chrono_literals;
	GameServerReplicationSystem system;
	EXPECT_EQ(system.refreshInterval(), 50ms);
	system.setRefreshInterval(100ms);
	EXPECT_EQ(system.refreshInterval(), 100ms);
	EXPECT_ANY_THROW(system.setRefreshInterval(0ms));
	EXPECT_EQ(system.refreshInterval(), 100ms);
}

TEST(InterestReplication, ClientRequiresConnectionBeforeSubscribing)
{
	spk::Client client;
	spk::Network::ClientReplicationSystem system;
	system.bind(client);
	ThresholdInterest interest(25);
	EXPECT_THROW((void)system.subscribe(interest), spk::Exception);
	system.unbind();
}
