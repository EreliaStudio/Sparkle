#pragma once

#include "structure/network/spk_client.hpp"
#include "structure/network/spk_server.hpp"
#include "structure/spk_safe_pointer.hpp"
#include <unordered_map>

namespace spk
{
	class Node
	{
	public:
		virtual void treatMessage(spk::Server::MessageObject &&p_message) = 0;
		virtual spk::ThreadSafeQueue<spk::Server::MessageObject> &messages() = 0;
	};

	class RemoteNode : public Node
	{
	private:
		spk::Client _client;

	public:
		void connect(const std::string &p_address, size_t p_port);
		void disconnect();
		void treatMessage(spk::Server::MessageObject &&p_message) override;
		spk::ThreadSafeQueue<spk::Server::MessageObject> &messages() override;
	};

	class LocalNode : public Node
	{
	private:
		spk::Pool<spk::Message> _messagePool;
		spk::ThreadSafeQueue<spk::Server::MessageObject> _messagesReceived;
		spk::ThreadSafeQueue<spk::Server::MessageObject> _messagesToReturn;

	public:
		void treatMessage(spk::Server::MessageObject &&p_message) override;
		spk::ThreadSafeQueue<spk::Server::MessageObject> &messageReceived();
		spk::Server::MessageObject obtainAwnerMessage(const spk::Message::Header::ClientId &p_emitterID, spk::Message::Header::Type p_type = -1);
		spk::Server::MessageObject obtainAwnerMessage(spk::Server::MessageObject &&p_questionMessage, spk::Message::Header::Type p_type = -1);
		void insertMessageAwnser(spk::Server::MessageObject &&p_messageAwnser);
		spk::ThreadSafeQueue<spk::Server::MessageObject> &messages() override;
	};

	class CentralNode
	{
	private:
		spk::Server _server;
		std::unordered_map<std::string, spk::SafePointer<Node>> _nodes;
		std::unordered_map<int32_t, spk::SafePointer<Node>> _redirections;

	public:
		CentralNode();
		void start(size_t p_serverPort);
		void addNode(std::string p_nodeName, spk::SafePointer<Node> p_node);
		void setupRedirection(int32_t p_messageType, std::string p_nodeName);
		void redirectMessageToNode();
		void redirectMessageToClients();
	};
}
