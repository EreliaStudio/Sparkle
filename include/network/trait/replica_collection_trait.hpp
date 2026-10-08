#pragma once
#include "client_binding_trait.hpp"
#include "client_handshake_trait.hpp"
#include "network/replication/protocol.hpp"
#include "requested_replica_trait.hpp"
namespace spk::Network
{
	// Complete client replication channel. Application storage remains in replica hooks.
	class ReplicaCollectionTrait : public RequestedReplicaTrait, protected ClientBindingTrait, protected ClientHandshakeTrait
	{
		Protocol _protocol;
		void _closeHandshakeSession() override;
		void _resetHandshakeSession(SessionID session) override;
		bool _sendHello(SessionID token) override;
		void _requireClientIdle() const override;
		bool _sendObjectRequest(const Request &request) override;
		void _onClientConnectionChanged() override;
		void _onClientTreatment() override;
		void _onClientUnbinding() override;
		void _onClientMessage(const spk::Message &message) override;

	protected:
		[[nodiscard]] virtual bool _sendMessage(const spk::Message &message);

	public:
		explicit ReplicaCollectionTrait(spk::Message::Type type, std::size_t maximumTracked = 65536, std::size_t maximumBytes = 2 * 1024 * 1024);
		~ReplicaCollectionTrait() override = default;
		void bind(spk::Client &client);
		using ClientBindingTrait::isBound;
		using ClientBindingTrait::unbind;
		[[nodiscard]] bool isSynchronized() const noexcept;
		[[nodiscard]] bool receiveMessage(const spk::Message &message);
	};
}
