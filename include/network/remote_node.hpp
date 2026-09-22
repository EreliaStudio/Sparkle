#pragma once

#include "network/client.hpp"
#include "network/node.hpp"
#include "network/server.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace spk
{
	class RemoteNode final : public Node
	{
	public:
		class Endpoint
		{
		public:
			struct Request
			{
				ConnectionID proxyConnection = InvalidConnectionID;
				ConnectionID originConnection = InvalidConnectionID;
				Message message;
			};

			using RequestQueue = ThreadSafeFIFO<Request>;

		private:
			Server _server;
			RequestQueue _requests;
			std::vector<ReceivedMessage> _received;

		public:
			void start(std::uint16_t port);
			void stop();
			[[nodiscard]] std::uint16_t port() const noexcept;
			[[nodiscard]] bool isRunning() const noexcept;

			void dispatch();
			[[nodiscard]] RequestQueue &requests() noexcept;
			void reply(const Request &request, Message message);
		};

		using IncomingQueue = ThreadSafeFIFO<ReceivedMessage>;

	private:
		Client _client;
		IncomingQueue _incoming;
		OutgoingQueue _outgoing;
		std::vector<ReceivedMessage> _requests;
		std::vector<Message> _responses;

		void _forwardRequests();
		void _collectResponses();

	public:
		[[nodiscard]] bool isConnected() const noexcept;
		void connect(std::string_view address, std::uint16_t port);
		void disconnect();
		void dispatch();

		void receive(ReceivedMessage message) override;
		[[nodiscard]] OutgoingQueue &outgoing() override;
	};
}
