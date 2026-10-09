#pragma once

#include "network/replication/interest_evaluator.hpp"
#include "network/replication/replication_batch.hpp"
#include "network/replication/replication_system.hpp"
#include "network/replication/server_replicated_component.hpp"
#include "network/server.hpp"

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <cstdint>

namespace spk::Network
{
	class ServerReplicationSystem : public ReplicationSystem
	{
	public:
		using Authorizer = std::function<bool(spk::ConnectionID, const ServerReplicatedComponent &)>;
		using Interval = std::chrono::steady_clock::duration;

	private:
		struct ClientSubscription
		{
			std::unique_ptr<Interest> interest;
		};

		spk::Server *_server = nullptr;
		spk::Server::MessageDispatcher::Contract _interestContract;
		spk::Server::MessageDispatcher::Contract _removalContract;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;
		std::mutex _peerMutex;
		std::set<spk::ConnectionID> _peers;
		std::map<spk::ConnectionID, std::map<spk::UUID, ClientSubscription>> _interests;
		std::map<spk::ConnectionID, std::map<spk::UUID, std::uint64_t>> _sent;
		Authorizer _authorizer;
		std::shared_ptr<const InterestEvaluator> _evaluator;
		Interval _refreshInterval = std::chrono::milliseconds(50);
		Interval _elapsed{};
		std::uint32_t _componentsPerSection = 16;

		[[nodiscard]] spk::ByteStream stateMessage(const ServerReplicatedComponent &component) const;
		void publishUpdates();
		void onInterestUpdate(const spk::ReceivedMessage &incoming);
		void onInterestRemoval(const spk::ReceivedMessage &incoming);

	protected:
		[[nodiscard]] virtual std::unique_ptr<Interest> _createInterest(
			const spk::Message::Reader &reader);
		void _updateState(spk::UpdateContext &context) override;

	public:
		explicit ServerReplicationSystem(spk::Message::Type messageType = 0x53504B10);
		~ServerReplicationSystem() override;

		void bind(spk::Server &server);
		void unbind();
		[[nodiscard]] bool isBound() const noexcept;
		void setRequestAuthorizer(Authorizer authorizer);
		void setInterestEvaluator(std::shared_ptr<const InterestEvaluator> evaluator);
		void setRefreshInterval(Interval interval);
		[[nodiscard]] Interval refreshInterval() const noexcept;
		void setComponentsPerSection(std::uint32_t count);
		[[nodiscard]] std::uint32_t componentsPerSection() const noexcept;
	};
}
