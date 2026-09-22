#pragma once

#include "network/node.hpp"
#include "network/server.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace spk
{
	class NodeRouter
	{
	private:
		Server _server;
		std::unordered_map<std::string, Node *> _nodes;
		std::unordered_map<Message::Type, Node *> _redirections;
		std::vector<ReceivedMessage> _incomingBuffer;
		std::vector<OutgoingMessage> _outgoingBuffer;

		[[nodiscard]] Node &_node(std::string_view name) const;
		void _dispatchIncoming();
		void _dispatchOutgoing();
		void _dispatchOutgoing(Node &node);

	public:
		void start(std::uint16_t port);
		void stop();
		void addNode(std::string name, Node &node);
		void removeNode(std::string_view name);
		void redirect(Message::Type messageType, std::string_view nodeName);
		void removeRedirection(Message::Type messageType);
		void dispatch();

		[[nodiscard]] Server &server() noexcept;
		[[nodiscard]] const Server &server() const noexcept;
	};
}
