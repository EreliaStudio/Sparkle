#pragma once

#include "network/client.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string_view>

namespace spk::NetworkInternal
{
	class ClientSession final
	{
	public:
		struct Settings
		{
			std::chrono::milliseconds handshakeTimeout = std::chrono::seconds(5);
			std::chrono::milliseconds incomingTransferTimeout = std::chrono::seconds(60);
			std::chrono::milliseconds acknowledgmentTimeout = std::chrono::seconds(30);
			std::size_t maxMessageBytes = 128u * 1024u * 1024u;
			std::size_t maxIncomingBytes = 512u * 1024u * 1024u;
			std::size_t maxPageBytes = 64u * 1024u;
			PageFailureLimits pageFailureLimits;
		};

		struct Callbacks
		{
			std::function<void(Connection, Message)> publish;
			std::function<void(Connection)> retireIncoming;
			std::function<void(Connection, Connection::ID)> acceptConnection;
			std::function<Connection()> beginConnection;
			std::function<void(Connection)> announcePending;
			std::function<void(Connection, Connection::Status, Connection::Error)> setStatus;
		};

	private:
		class Impl;
		std::unique_ptr<Impl> _impl;

	public:
		ClientSession(Callbacks callbacks, const Settings &settings);
		~ClientSession();
		ClientSession(const ClientSession &) = delete;
		ClientSession(ClientSession &&) = delete;
		ClientSession &operator=(const ClientSession &) = delete;
		ClientSession &operator=(ClientSession &&) = delete;

		[[nodiscard]] bool active() const noexcept;
		[[nodiscard]] bool isWorkerThread() const noexcept;
		[[nodiscard]] bool isConnected() const noexcept;
		Connection connect(std::string_view address, std::uint16_t port);
		void disconnect();
		void setHandshakeTimeout(std::chrono::milliseconds timeout);
		void setIncomingTransferTimeout(std::chrono::milliseconds timeout);
		void setAcknowledgmentTimeout(std::chrono::milliseconds timeout);
		void setMaxMessageBytes(std::size_t bytes);
		void setMaxPageBytes(std::size_t bytes);
		void setMaxIncomingBytes(std::size_t bytes);
		void setPageFailureLimits(PageFailureLimits limits);
		Transfer send(const Message &message, Message::Priority priority);
	};
}
