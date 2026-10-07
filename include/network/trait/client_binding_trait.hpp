#pragma once
#include "network/client.hpp"
#include <atomic>
#include <memory>
namespace spk::Network
{
	// Connection callbacks only signal; all virtual hooks run on the treating thread.
	class ClientBindingTrait
	{
		spk::Client *_client = nullptr;
		std::shared_ptr<std::atomic<std::uint64_t>> _connectionEdition;
		std::uint64_t _observedConnection = 0;
		spk::Message::Type _type{};
		bool _handling = false;
		spk::Client::ConnectionContract _connectionContract;
		spk::Client::DisconnectionContract _disconnectionContract;
		spk::Client::MessageDispatcher::Contract _messageContract;
		spk::Client::MessageDispatcher::TreatmentContract _treatmentContract;
		void _releaseClientBinding() noexcept;
		void _requireBindingIdle() const;
		void _subscribe(spk::Client &client, spk::Message::Type type);
		void _receiveClientMessage(const spk::Message &message);

	protected:
		virtual void _requireClientIdle() const;
		virtual void _onClientConnectionChanged();
		virtual void _onClientTreatment();
		virtual void _onClientUnbinding();
		virtual void _onClientMessage(const spk::Message &) = 0;
		void _synchronizeClientBinding();
		[[nodiscard]] bool _clientConnected() const noexcept;
		[[nodiscard]] bool _connectionSynchronized() const noexcept;
		[[nodiscard]] bool _sendToServer(const spk::Message &message);

	public:
		ClientBindingTrait() = default;
		virtual ~ClientBindingTrait();
		ClientBindingTrait(const ClientBindingTrait &) = delete;
		ClientBindingTrait &operator=(const ClientBindingTrait &) = delete;
		void bind(spk::Client &client, spk::Message::Type type);
		void unbind();
		[[nodiscard]] bool isBound() const noexcept;
	};
}
