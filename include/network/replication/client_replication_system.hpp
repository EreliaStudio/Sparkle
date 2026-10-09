#pragma once

#include "network/replication/replication_system.hpp"
#include "network/replication/client_replicated_component.hpp"
#include "network/replication/interest.hpp"
#include "network/client.hpp"

#include <atomic>
#include <map>
#include <memory>
#include <mutex>

namespace spk::Network
{
	class ClientReplicationSystem final : public ReplicationSystem
	{
	private:
		struct SubscriptionEntry
		{
			spk::UUID identifier;
			bool valid = true;
		};

		struct SubscriptionState
		{
			std::mutex mutex;
			spk::Client *client = nullptr;
			spk::Message::Type updateType = 0;
			spk::Message::Type removalType = 0;
			std::map<spk::UUID, std::weak_ptr<SubscriptionEntry>> subscriptions;
			void invalidate() noexcept;
		};

	public:
		class Subscription final
		{
		private:
			std::weak_ptr<SubscriptionState> _owner;
			std::shared_ptr<SubscriptionEntry> _entry;
			friend class ClientReplicationSystem;
			Subscription(std::shared_ptr<SubscriptionState> owner,
				std::shared_ptr<SubscriptionEntry> entry);

		public:
			Subscription() = default;
			Subscription(const Subscription &) = delete;
			Subscription &operator=(const Subscription &) = delete;
			Subscription(Subscription &&) noexcept = default;
			Subscription &operator=(Subscription &&other) noexcept;
			~Subscription();

			[[nodiscard]] bool isValid() const noexcept;
			void update(const Interest &interest);
			void cancel() noexcept;
		};

	private:
		spk::Client *_client = nullptr;
		spk::Client::MessageDispatcher::Contract _stateContract;
		spk::Client::MessageDispatcher::Contract _componentRemovalContract;
		spk::Client::DisconnectionContract _disconnectionContract;
		spk::Client::ConnectionContract _connectionContract;
		std::atomic_bool _resetPending{true};
		std::shared_ptr<SubscriptionState> _subscriptions = std::make_shared<SubscriptionState>();

		void onState(const spk::Message &message);
		void onComponentRemoval(const spk::Message &message);
		void resetReceivedRevisions();

	protected:
		void _updateState(spk::UpdateContext &) override;

	public:
		explicit ClientReplicationSystem(spk::Message::Type messageType = 0x53504B10);
		~ClientReplicationSystem() override;

		void bind(spk::Client &client);
		void unbind();
		[[nodiscard]] bool isBound() const noexcept;
		[[nodiscard]] Subscription subscribe(const Interest &interest);
	};
}
