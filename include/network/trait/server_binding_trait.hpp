#pragma once
#include "network/server.hpp"
#include <memory>
#include <mutex>
#include <set>
namespace spk::Network
{
	// Bind before server.start(); hooks run on the message-treatment owner thread.
	class ServerBindingTrait
	{
		struct Signals
		{
			std::mutex mutex;
			std::set<spk::ConnectionID> live;
		};
		spk::Server *_server = nullptr;
		std::shared_ptr<Signals> _signals;
		std::set<spk::ConnectionID> _observed;
		bool _handling = false;
		spk::Server::ConnectionContract _connectionContract;
		spk::Server::DisconnectionContract _disconnectionContract;
		spk::Server::MessageDispatcher::Contract _messageContract;
		spk::Server::MessageDispatcher::TreatmentContract _treatmentContract;
		void _releaseServerBinding() noexcept;
		void _requireBindingIdle() const;
		void _receiveServerMessage(const spk::ReceivedMessage &message);

	protected:
		virtual void _requireServerIdle() const;
		virtual void _onServerConnectionOpened(spk::ConnectionID);
		virtual void _onServerConnectionClosed(spk::ConnectionID);
		virtual void _onServerUnbinding();
		virtual void _onServerMessage(const spk::ReceivedMessage &) = 0;
		void _synchronizeServerBinding();
		[[nodiscard]] bool _connectionLive(spk::ConnectionID connection) const;
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
