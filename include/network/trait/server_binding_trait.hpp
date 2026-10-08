#pragma once
#include "server_connection_observation_trait.hpp"
namespace spk::Network
{
	// Bind before server.start(); hooks run on the message-treatment owner thread.
	class ServerBindingTrait : protected ServerConnectionObservationTrait
	{
		spk::Server *_server = nullptr;
		bool _handling = false;
		spk::Server::MessageDispatcher::Contract _messageContract;
		spk::Server::MessageDispatcher::TreatmentContract _treatmentContract;
		void _releaseServerBinding() noexcept;
		void _requireBindingIdle() const;
		void _receiveServerMessage(const spk::ReceivedMessage &message);

	protected:
		virtual void _requireServerIdle() const;
		virtual void _onServerUnbinding();
		virtual void _onServerMessage(const spk::ReceivedMessage &) = 0;
		void _synchronizeServerBinding();
		[[nodiscard]] bool _sendTo(spk::ConnectionID connection, const spk::Message &message);

	public:
		ServerBindingTrait() = default;
		virtual ~ServerBindingTrait();
		ServerBindingTrait(const ServerBindingTrait &) = delete;
		ServerBindingTrait &operator=(const ServerBindingTrait &) = delete;
		void bind(spk::Server &server, spk::Message::Type type);
		void unbind();
		[[nodiscard]] bool isBound() const noexcept;

	private:
		spk::Message::Type _type{};
		void _subscribe(spk::Server &server, spk::Message::Type type);
	};
}
